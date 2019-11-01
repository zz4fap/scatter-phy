#include "srslte/rf_monitor/iq_dump_plus_sensing.h"

// *********** Global variables ***********
static volatile sig_atomic_t run_procedure = false; // variable used to enable/disable actions.

static const char IQ_DUMP_PLUS_SENSING_DATA_TYPE_STRING[6][30] = {"SRSLTE_FLOAT", "SRSLTE_COMPLEX_FLOAT", "SRSLTE_COMPLEX_SHORT", "SRSLTE_FLOAT_BIN", "SRSLTE_COMPLEX_FLOAT_BIN", "SRSLTE_COMPLEX_SHORT_BIN"};

// ******************** Definitions and global varibales for Sensing *************************
static fftwf_complex *fft_out_samples, *fft_in_samples;
static fftwf_plan sensing_fft_plan;
static cf_t *freq_samples;
static float *bin_energy, *shifted_bin_energy;
//*******************************************************************************************

// *********** Functions **************
void iq_dump_plus_sensing_alarm_handle(int sig) {
   if(sig == SIGALRM) {
      run_procedure = true;
   }
}

char* iq_dump_plus_sensing_concat_timestamp_string(char* filename) {
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

void iq_dump_plus_sensing_init() {
  // Allocate Memory.
  fft_in_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*SW_RF_MON_FFT_SIZE);
  fft_out_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*SW_RF_MON_FFT_SIZE);
  // Allocate memory for subbands.
  freq_samples = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*SW_RF_MON_FFT_SIZE);
  if(!freq_samples) {
    IQ_DUMP_PLUS_SENSING_ERROR("Error allocating freq_samples memory.\n",0);
    exit(-1);
  }
  // Allocate memory for bins.
  bin_energy = (float*)srslte_vec_malloc(sizeof(float)*SW_RF_MON_FFT_SIZE);
  if(!bin_energy) {
    IQ_DUMP_PLUS_SENSING_ERROR("Error allocating bin_energy memory.\n",0);
    exit(-1);
  }
  shifted_bin_energy = (float*)srslte_vec_malloc(sizeof(float)*SW_RF_MON_FFT_SIZE);
  if(!shifted_bin_energy) {
    IQ_DUMP_PLUS_SENSING_ERROR("Error allocating shifted_bin_energy memory.\n",0);
    exit(-1);
  }
  // Apply FFT to the received base band samples.
  sensing_fft_plan = fftwf_plan_dft_1d(SW_RF_MON_FFT_SIZE, fft_in_samples, fft_out_samples, FFTW_FORWARD, FFTW_ESTIMATE);
}

void iq_dump_plus_sensing_free() {
  // Free allocated resources
  fftwf_destroy_plan(sensing_fft_plan);
  if(fft_in_samples) {
    fftwf_free(fft_in_samples);
  }
  if(fft_out_samples) {
    fftwf_free(fft_out_samples);
  }
  if(bin_energy) {
    free(bin_energy);
  }
  if(shifted_bin_energy) {
    free(shifted_bin_energy);
  }
  if(freq_samples){
    free(freq_samples);
  }
}

void *iq_dump_plus_sensing_work(void *h) {

  IQ_DUMP_PLUS_SENSING_INFO("Start: iq_dump_plus_sensing_work()\n",0);

  rf_monitor_handle_t *rf_monitor_handle = (rf_monitor_handle_t *)h;
  srslte_timestamp_t first_sample_timestamp;
  uint32_t num_read_samples = 0, num_of_dumped_samples = 0, num_samp_to_write_into_file = 0;
  float rssi;
  iq_dump_plus_sensing_states_t iq_dump_plus_sensing_state = IQ_DUMP_PLUS_SENSING_CHECK_FILE_EXIST_ST; // Sensing state variable. Start with SENSING CHECK FILE EXISTS
  // NOTE: SW_RF_MON_FFT_SIZE MUST be always less than or equal to nsamples.
  uint32_t nsamples = (rf_monitor_handle->rf)->rx_nof_samples; // Always read the maxium number of samples per packet per buffer allowed by this specific USRP, i.e., it is hardware dependent.
  cf_t data[nsamples];
  srslte_filesink_t dump_file_sink;
  char output_file_name[200];
  char timestamp_file_name[200];
  unsigned long number_of_dumps_counter = 0;
  FILE *fid;
  uint64_t timestamp_first_sample;
  struct timespec start_time_rf_mon, end_time_rf_mon;
  double diff_rf_mon;
#ifdef PROFILLING_IQ_DUMP_PLUS_SENSING
  struct timespec start_time, end_time;
#endif

  // Set priority to spectrum sesing thread.
  uhd_set_thread_priority(1.0, true);

  // Initialize all the necessary resources.
  iq_dump_plus_sensing_init();

  // Calculate number of IQ samples to dump into file.
  uint32_t total_number_of_samples_to_dump = (rf_monitor_handle->single_log_duration/1000.0)*rf_monitor_handle->sample_rate;
  IQ_DUMP_PLUS_SENSING_DEBUG("Will dump %d IQ samples into file.\n", total_number_of_samples_to_dump);

  // Install handler for alarm.
  signal(SIGALRM, iq_dump_plus_sensing_alarm_handle);

  // Set to true so that we can run file checking for the first time.
  run_procedure = true;

  // Mark the start time.
  clock_gettime(CLOCK_REALTIME, &start_time_rf_mon);

  // Thread loop, wait here until main thread says the contrary.
  while(run_rf_monitor_thread) {

#ifdef PROFILLING_IQ_DUMP_PLUS_SENSING
    // Mark the start time.
    clock_gettime(CLOCK_REALTIME, &start_time);
#endif

    // Read IQ Samples.
    num_read_samples = srslte_rf_recv_channel_with_time(rf_monitor_handle->channel_handler, data, nsamples, true, &(first_sample_timestamp.full_secs), &(first_sample_timestamp.frac_secs));
    if(num_read_samples < 0) {
      IQ_DUMP_PLUS_SENSING_ERROR("Problem reading BB samples.\n",0);
    }

    clock_gettime(CLOCK_REALTIME, &end_time_rf_mon);
    diff_rf_mon = srslte_time_diff(start_time_rf_mon, end_time_rf_mon);
    if(diff_rf_mon >= RF_MON_SENSING_PERIODICITY) {
      // This function calculates the FFT.
      iq_dump_plus_sensing_get_energy(data);
      // Send sensed data to upper layer. In fact it is queued in a safe queue.
      rf_monitor_send_sensing_statistics(0, PHY_SUCCESS, first_sample_timestamp.full_secs, first_sample_timestamp.frac_secs, rf_monitor_handle->central_frequency, rf_monitor_handle->sample_rate, rf_monitor_handle->sensing_rx_gain, 0.0, (SW_RF_MON_FFT_SIZE*sizeof(float)), (uint8_t*)shifted_bin_energy);
      // Mark the start time.
      clock_gettime(CLOCK_REALTIME, &start_time_rf_mon);
    }

#ifdef PROFILLING_IQ_DUMP_PLUS_SENSING
  clock_gettime(CLOCK_REALTIME, &end_time);
  double diff = srslte_time_diff(start_time, end_time);
  IQ_DUMP_PLUS_SENSING_DEBUG("Elapsed time = %f milliseconds for %d samples.\n", diff, num_read_samples);
#endif

    // Get timestamp of 1st received sample and convert into a uint64_t timestamp that will be stored in a file.
    timestamp_first_sample = helpers_convert_fpga_time_into_uint64_nanoseconds(first_sample_timestamp.full_secs, first_sample_timestamp.frac_secs);

    switch(iq_dump_plus_sensing_state) {
      case IQ_DUMP_PLUS_SENSING_CHECK_FILE_EXIST_ST:
        if(run_procedure) {
          // Check if file exists every 1 minute.
          // Check if file exists
          if(access(rf_monitor_handle->path_to_start_file, F_OK) != -1 || rf_monitor_handle->iq_dumping) {
            // If file exists then go to WAIT state before dumping.
            iq_dump_plus_sensing_state = IQ_DUMP_PLUS_SENSING_WAIT_BEFORE_DUMP_ST;
            IQ_DUMP_PLUS_SENSING_DEBUG("File does exist.\n",0);
          } else {
            run_procedure = false;
            alarm(60);
            IQ_DUMP_PLUS_SENSING_DEBUG("File does not exist, wait 1 minute before checking again.\n",0);
          }
        }
        break;
      case IQ_DUMP_PLUS_SENSING_WAIT_BEFORE_DUMP_ST:
        if(run_procedure) {
          run_procedure = false;
          num_of_dumped_samples = 0; // Reset the counter of number of read samples.
          iq_dump_plus_sensing_state = IQ_DUMP_PLUS_SENSING_DUMP_SAMPLES_ST;
          uint32_t seconds = rf_monitor_handle->logging_frequency/1000; // make sure it is an integer number of seconds.
          alarm(seconds);
          IQ_DUMP_PLUS_SENSING_DEBUG("Wait %d seconds before dumping IQ samples.\n",seconds);
        }
        break;
      case IQ_DUMP_PLUS_SENSING_DUMP_SAMPLES_ST:
        if(run_procedure) {
          // Open dump file.
          if(num_of_dumped_samples == 0) {
            int ret = sprintf(output_file_name,"%s%s%d_",rf_monitor_handle->path_to_log_files,IQ_DUMP_PLUS_SENSING_FILE_NAME,rf_monitor_handle->node_id);
            iq_dump_plus_sensing_concat_timestamp_string(&output_file_name[ret]);
            strcat(output_file_name,".dat");
            IQ_DUMP_PLUS_SENSING_DEBUG("Opening file: %s with data type: %s.\n",output_file_name,IQ_DUMP_PLUS_SENSING_DATA_TYPE_STRING[rf_monitor_handle->data_type]);
            filesink_init(&dump_file_sink, output_file_name, rf_monitor_handle->data_type);

            // Open file to store timestamps related to the IQ samples stored in the dumping file.
            ret = sprintf(timestamp_file_name,"%s%s%d_",rf_monitor_handle->path_to_log_files,IQ_DUMP_PLUS_SENSING_TIMESTAMP_FILE_NAME,rf_monitor_handle->node_id);
            iq_dump_plus_sensing_concat_timestamp_string(&timestamp_file_name[ret]);
            strcat(timestamp_file_name,".txt");
            IQ_DUMP_PLUS_SENSING_DEBUG("Opening timestamp file: %s.\n",timestamp_file_name);
            iq_dump_plus_sensing_fopen(timestamp_file_name, &fid);
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

          // Write timestamp of 1st received sample to file.
          iq_dump_plus_sensing_fwrite(&fid, timestamp_first_sample);

          // Finalize the dumping procedure and go back to waiting state.
          if(num_of_dumped_samples >= total_number_of_samples_to_dump) {
            // Close dump file.
            filesink_free(&dump_file_sink);
            IQ_DUMP_PLUS_SENSING_DEBUG("%d IQ samples dumped into file, going back to wait state.\n",num_of_dumped_samples);
            // Close timestamp file.
            iq_dump_plus_sensing_fclose(&fid);
            // Increment the dump counter used to stop dumping samples into files.
            number_of_dumps_counter++;
            if(number_of_dumps_counter >= rf_monitor_handle->max_number_of_dumps) {
              iq_dump_plus_sensing_state = IQ_DUMP_PLUS_SENSING_RSSI_ST;
              IQ_DUMP_PLUS_SENSING_DEBUG("Going to RSSI mode.\n",0);
            } else {
              // Go back to wait state if the maximum number of dumps was not reached.
              iq_dump_plus_sensing_state = IQ_DUMP_PLUS_SENSING_WAIT_BEFORE_DUMP_ST;
              IQ_DUMP_PLUS_SENSING_DEBUG("Gaing to wait state before dumping IQ samples again.\n",0);
            }
          }
        }
        break;
      case IQ_DUMP_PLUS_SENSING_RSSI_ST:
        // Print RSSI every 2 second.
        if(run_procedure) {
          run_procedure = false;
          rssi = 10.0*log10f(srslte_vec_avg_power_cf(data, num_read_samples));
          IQ_DUMP_PLUS_SENSING_DEBUG("Frequency: %4.1f [MHz] - Sample rate: %4.2f MSps - RSSI: %3.2f dBm\r", rf_monitor_handle->central_frequency/1000000.0, rf_monitor_handle->sample_rate/1000000.0, rssi); fflush(stdout);
          alarm(2);
        }
        break;
    }
  }

  // uninitialize all the used resources.
  iq_dump_plus_sensing_free();

  IQ_DUMP_PLUS_SENSING_DEBUG("Leaving IQ dump + Sensing module thread.\n",0);
  // Exit thread with result code.
  pthread_exit(0);
}

void iq_dump_plus_sensing_get_energy(cf_t *base_band_samples) {
  // Copy base band samples into fft type for fftw execution.
  memcpy((uint8_t*)fft_in_samples,(uint8_t*)base_band_samples,sizeof(cf_t)*SW_RF_MON_FFT_SIZE);
  // Execute FFT.
  fftwf_execute(sensing_fft_plan);
  // Copy the result from the fft type into the array for sensing.
  memcpy((uint8_t*)freq_samples,(uint8_t*)fft_out_samples,sizeof(cf_t)*SW_RF_MON_FFT_SIZE);
  // Calculate bin energy.
  srslte_vec_power_spectrum_cf(freq_samples, bin_energy, 1.0, SW_RF_MON_FFT_SIZE);
  // Shift the FFT.
  memcpy((uint8_t*)shifted_bin_energy, (uint8_t*)(bin_energy+(SW_RF_MON_FFT_SIZE/2)), sizeof(float)*(SW_RF_MON_FFT_SIZE/2));
  memcpy((uint8_t*)(shifted_bin_energy+(SW_RF_MON_FFT_SIZE/2)), (uint8_t*)bin_energy, sizeof(float)*(SW_RF_MON_FFT_SIZE/2));
}

void iq_dump_plus_sensing_fopen(const char *filename, FILE **f) {
  *f = fopen(filename, "w");
  if(!*f) {
    perror("fopen");
  }
}

void iq_dump_plus_sensing_fwrite(FILE **f, uint64_t timestamp) {
  fprintf(*f, "%" PRIu64 "\n", timestamp);
}

void iq_dump_plus_sensing_fclose(FILE **f) {
  fclose(*f);
}
