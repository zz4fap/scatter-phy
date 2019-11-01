#include "srslte/rf_monitor/iq_dumping.h"

// *********** Global variables ***********
static volatile sig_atomic_t run_procedure = false; // variable used to enable/disable actions.

static const char IQ_DUMPING_DATA_TYPE_STRING[6][30] = {"SRSLTE_FLOAT", "SRSLTE_COMPLEX_FLOAT", "SRSLTE_COMPLEX_SHORT", "SRSLTE_FLOAT_BIN", "SRSLTE_COMPLEX_FLOAT_BIN", "SRSLTE_COMPLEX_SHORT_BIN"};

// *********** Functions **************
void iq_dumping_alarm_handle(int sig) {
   if(sig == SIGALRM) {
      run_procedure = true;
   }
}

char* iq_dumping_concat_timestamp_string(char* filename) {
  char date_time_str[30];
  struct timeval tmnow;
  struct tm *tm;
  char usec_buf[20];
  gettimeofday(&tmnow, NULL);
  tm = localtime(&tmnow.tv_sec);
  strftime(date_time_str,30,"%Y%m%d_%H%M%S", tm);
  strcat(date_time_str,"_");
  sprintf(usec_buf,"%06ld",tmnow.tv_usec);
  strcat(date_time_str,usec_buf);
  return strcat(filename,date_time_str);
}

void *iq_dumping_work(void *h) {

  IQ_DUMPING_INFO("Start: iq_dumping_work()\n",0);

  rf_monitor_handle_t *rf_monitor_handle = (rf_monitor_handle_t *)h;
  srslte_timestamp_t first_sample_timestamp;
  uint32_t num_read_samples = 0, num_of_dumped_samples = 0, num_samp_to_write_into_file = 0;
  float rssi;
  iq_dumping_states_t iq_dumping_state = IQ_DUMPING_CHECK_FILE_EXIST_ST; // Sensing state variable. Start with SENSING CHECK FILE EXISTS
  uint32_t nsamples = (rf_monitor_handle->rf)->rx_nof_samples; // Number of samples to be read in every call to srslte_rf_recv_channel_with_time().
  cf_t data[nsamples];
  srslte_filesink_t dump_file_sink;
  char output_file_name[200];
  unsigned long number_of_dumps_counter = 0;
#ifdef PROFILLING_SENSING
  struct timespec start_time, end_time;
#endif

  // Calculate number of IQ samples to dump into file.
  uint32_t total_number_of_samples_to_dump = (rf_monitor_handle->single_log_duration/1000.0)*rf_monitor_handle->sample_rate;
  IQ_DUMPING_DEBUG("Will dump %d IQ samples into file.\n", total_number_of_samples_to_dump);

  // Install handler for alarm.
  signal(SIGALRM, iq_dumping_alarm_handle);

  // Set to true so that we can run file checking for the first time.
  run_procedure = true;

  // Thread loop, wait here until main thread says the contrary.
  while(run_rf_monitor_thread) {

#ifdef PROFILLING_SENSING
  // Mark the start time.
  clock_gettime(CLOCK_REALTIME, &start_time);
#endif

    // Read IQ Samples.
    num_read_samples = srslte_rf_recv_channel_with_time(rf_monitor_handle->channel_handler, data, nsamples, true, &(first_sample_timestamp.full_secs), &(first_sample_timestamp.frac_secs));

    // Check if number of read samples is OK.
    if(num_read_samples <= 0) {
      IQ_DUMPING_ERROR("Number of read samples: %d.\n",num_read_samples);
      continue;
    }

#ifdef PROFILLING_SENSING
  clock_gettime(CLOCK_REALTIME, &end_time);
  double diff = srslte_time_diff(start_time, end_time);
  IQ_DUMPING_DEBUG("Elapsed time = %f milliseconds for %d samples.\n", diff, num_read_samples);
#endif

    switch(iq_dumping_state) {
      case IQ_DUMPING_CHECK_FILE_EXIST_ST:
        if(run_procedure) {
          // Check if file exists every 1 minute.
          // Check if file exists
          if(access(rf_monitor_handle->path_to_start_file, F_OK) != -1 || rf_monitor_handle->iq_dumping) {
            // If file exists then go to WAIT state before dumping.
            iq_dumping_state = IQ_DUMPING_WAIT_BEFORE_DUMP_ST;
            IQ_DUMPING_DEBUG("File does exist.\n",0);
          } else {
            run_procedure = false;
            alarm(60);
            IQ_DUMPING_DEBUG("File does not exist, wait 1 minute before checking again.\n",0);
          }
        }
        break;
      case IQ_DUMPING_WAIT_BEFORE_DUMP_ST:
        if(run_procedure) {
          run_procedure = false;
          num_of_dumped_samples = 0; // Reset the counter of number of read samples.
          iq_dumping_state = IQ_DUMPING_DUMP_SAMPLES_ST;
          uint32_t seconds = rf_monitor_handle->logging_frequency/1000; // make sure it is an integer number of seconds.
          alarm(seconds);
          IQ_DUMPING_DEBUG("Wait %d seconds before dumping IQ samples.\n",seconds);
        }
        break;
      case IQ_DUMPING_DUMP_SAMPLES_ST:
        if(run_procedure) {
          // Open dump file.
          if(num_of_dumped_samples == 0) {
            int ret = sprintf(output_file_name,"%s%s%d_",rf_monitor_handle->path_to_log_files,DUMP_FILE_NAME,rf_monitor_handle->node_id);
            iq_dumping_concat_timestamp_string(&output_file_name[ret]);
            strcat(output_file_name,".dat");
            IQ_DUMPING_DEBUG("Opening file: %s with data type: %s.\n",output_file_name,IQ_DUMPING_DATA_TYPE_STRING[rf_monitor_handle->data_type]);
            filesink_init(&dump_file_sink, output_file_name, rf_monitor_handle->data_type);
          }

          // Dump IQ samples into file.
          // Calculate how many samples need to be written to the file.
          if(num_read_samples > total_number_of_samples_to_dump - num_of_dumped_samples) {
            num_samp_to_write_into_file = total_number_of_samples_to_dump - num_of_dumped_samples;
          } else {
            num_samp_to_write_into_file = num_read_samples;
          }
          // We write rx_nof_samples samples at each time.
          filesink_write(&dump_file_sink, data, num_samp_to_write_into_file);
          // Increment dump counter.
          num_of_dumped_samples += num_samp_to_write_into_file;

          // Finalize the dumping procedure and go back to waiting state.
          if(num_of_dumped_samples >= total_number_of_samples_to_dump) {
            // Close dump file.
            filesink_free(&dump_file_sink);
            IQ_DUMPING_DEBUG("%d IQ samples dumped into file, going back to wait state.\n",num_of_dumped_samples);
            // Increment the dump counter used to stop dumping samples into files.
            number_of_dumps_counter++;
            if(number_of_dumps_counter >= rf_monitor_handle->max_number_of_dumps) {
              iq_dumping_state = IQ_DUMPING_RSSI_ST;
              IQ_DUMPING_DEBUG("Going to RSSI mode.\n",0);
            } else {
              // Go back to wait state if the maximum number of dumps was not reached.
              iq_dumping_state = IQ_DUMPING_WAIT_BEFORE_DUMP_ST;
              IQ_DUMPING_DEBUG("Gaing to wait state before dumping IQ samples again.\n",0);
            }
          }
        }
        break;
      case IQ_DUMPING_RSSI_ST:
        // Print RSSI every 2 second.
        if(run_procedure) {
          run_procedure = false;
          rssi = 10.0*log10f(srslte_vec_avg_power_cf(data, num_read_samples));
          IQ_DUMPING_DEBUG("Frequency: %4.1f [MHz] - Sample rate: %4.2f MSps - RSSI: %3.2f dBm\r", rf_monitor_handle->central_frequency/1000000.0, rf_monitor_handle->sample_rate/1000000.0, rssi); fflush(stdout);
          alarm(2);
        }
        break;
    }
  }
  IQ_DUMPING_DEBUG("Leaving IQ Samples Dumping module thread.\n",0);
  // Exit thread with result code.
  pthread_exit(NULL);
}
