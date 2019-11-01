#include "srslte/rf_monitor/lbt.h"

//#define PROFILLING_SENSING
//#define MEASURE_POWER
//#define MEASURE_TIME_DIFFERENCE

// *********** Global variables ***********
// Global variables for FFT.
fftwf_complex *fft_out_samples, *fft_in_samples;
fftwf_plan lbt_fft_plan;
cf_t *freq_samples;

// Global variables for listen before talk.
uint32_t offset_in_fft_bins;
uint32_t bw_in_fft_bins;

// This is only valid for TX BW of 5 MHz and competition bandwidth of 20 MHz.
uint32_t channel_start_fft_bin_position[] = {580, 802, 0, 222};
// Variable used to set the events related to LBT procedure.
lbt_events_t lbt_procedure_event = LBT_IDLE_EVT;
// Mutex used to synchronize access to LBT event variable.
pthread_mutex_t lbt_procedure_evt_mutex;
// Condition variable used to synchronize reads to the LBT event variable.
pthread_cond_t lbt_procedure_evt_cv;
// Pointer to function used to calculate power.
float (*lbt_channel_power_measurement)(cf_t *data, uint32_t num_read_samples) = NULL;
// Structure used to keep stats on the channel occupancy.
lbt_stats_t phy_tx_lbt_stats;
// Flag used tell PHY TX to drop a packet.
volatile sig_atomic_t lbt_drop_packet_flag = false;

// *********** Functions **************
int lbt_initialize(rf_monitor_handle_t *rf_monitor_handle) {
  // Intializes random number generator.
  srand(time(NULL));
  // Initialize mutexes.
  if(pthread_mutex_init(&lbt_procedure_evt_mutex, NULL) != 0) {
    LBT_ERROR("LBT Procedure Event Mutex init failed.\n",0);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&lbt_procedure_evt_cv, NULL)) {
    LBT_ERROR("Conditional variable init failed.\n",0);
    return -1;
  }
  // Initialize FFT.
  lbt_initialize_fft();
  // Initialize parameters needed for the frequency domain LBT.
  lbt_initialize_freq_domain_parameters(rf_monitor_handle);
  // Initialize pointer to function measuring power with correct function.
  if(rf_monitor_handle->lbt_use_fft_based_pwr) {
    lbt_channel_power_measurement = &lbt_channel_fd_power_measurement;
    LBT_PRINT("Using frequency-domain based power measurements.\n",0);
  } else {
    lbt_channel_power_measurement = &lbt_channel_td_power_measurement;
    LBT_PRINT("Using time-domain based power measurements.\n",0);
  }
  return 0;
}

int lbt_uninitialize() {
  // Notify condition variable.
  pthread_cond_signal(&lbt_procedure_evt_cv);
  // Destroy mutexes.
  pthread_mutex_destroy(&lbt_procedure_evt_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&lbt_procedure_evt_cv) != 0) {
    LBT_ERROR("Conditional variable destruction failed.\n",0);
    return -1;
  }
  // Unnitialize FFT.
  lbt_uninitialize_fft();
  return 0;
}

void *lbt_work(void *h) {

  LBT_INFO("Start: lbt_work()\n",0);

  rf_monitor_handle_t *rf_monitor_handle = (rf_monitor_handle_t *)h;
  srslte_timestamp_t first_sample_timestamp;
  uint32_t num_read_samples = 0;
  uint32_t nsamples = (rf_monitor_handle->rf)->rx_nof_samples; // Always read the maximum possible number of samples so that we avoid big gaps between first sample and current FPGA time.
  cf_t data[nsamples];
  lbt_states_t lbt_state = IDLE_ST;
  uint32_t backoff = rf_monitor_handle->maximum_backoff_period;
  uint32_t maximum_backoff_period = rf_monitor_handle->maximum_backoff_period;
  struct timespec lbt_procedure_start;
  float channel_energy = 0.0;
  bool is_channel_free = false;
  lbt_stats_t lbt_stats;
#ifdef PROFILLING_SENSING
  struct timespec read_samples_start_time, read_samples_stop_time, measure_power_start_time, measure_power_stop_time;
  double diff_read_samples = 0.0, diff_measure_power = 0.0;
  uint64_t read_samples_cnt = 0, measure_power_cnt = 0;
#endif

#ifdef MEASURE_POWER
  float channel_energy_sum = 0.0, max_energy = -1000.0, min_energy = 1000.0;
  uint64_t energy_cnt = 0;
#endif

#ifdef MEASURE_TIME_DIFFERENCE
  time_t full_secs;
  double frac_secs;
  double fpga_time_difference;
  uint64_t time_diff_cnt = 0;
  double avg_fpga_time_difference = 0.0;
#endif

  // Initalize LBT.
  lbt_initialize(rf_monitor_handle);

  // Initialize LBT stats struct.
  lbt_init_stats(&lbt_stats);

  LBT_PRINT("Number of samples to read every time: %d\n",nsamples);

  //******************************************************************************************************************************************
  // Thread loop, wait here until main thread says the contrary.
  while(run_rf_monitor_thread) {

#ifdef PROFILLING_SENSING
    // Mark the start time.
    clock_gettime(CLOCK_REALTIME, &read_samples_start_time);
#endif

    // Read IQ Samples.
    num_read_samples = srslte_rf_recv_channel_with_time(rf_monitor_handle->channel_handler, data, nsamples, true, &(first_sample_timestamp.full_secs), &(first_sample_timestamp.frac_secs));

    // Measure channel power.
    channel_energy = lbt_channel_power_measurement(data, num_read_samples);

    // Check if channel is FREE.
    if(channel_energy < rf_monitor_handle->lbt_threshold) {
      is_channel_free = true;
      lbt_stats.channel_free_cnt++;
      lbt_stats.free_energy += channel_energy;
    } else {
      is_channel_free = false;
      lbt_stats.channel_busy_cnt++;
      lbt_stats.busy_energy += channel_energy;
    }

#ifdef MEASURE_TIME_DIFFERENCE
    // Get FPGA time now.
    srslte_rf_get_time(rf_monitor_handle->rf, &full_secs, &frac_secs);
    fpga_time_difference = fpga_time_diff(first_sample_timestamp.full_secs, first_sample_timestamp.frac_secs, full_secs, frac_secs);
    if(fpga_time_difference > 0.7) {
      LBT_PRINT("Time now: %lld - %f - first sample time: %lld - %f - diff: %f\n",(long long)full_secs, frac_secs, (long long)first_sample_timestamp.full_secs, first_sample_timestamp.frac_secs, fpga_time_difference);
    }
    avg_fpga_time_difference += fpga_time_difference;
    time_diff_cnt++;
    if(time_diff_cnt%100000 == 0) {
      LBT_PRINT("Time diff: %f\n",avg_fpga_time_difference/time_diff_cnt);
      time_diff_cnt = 0;
      avg_fpga_time_difference = 0.0;
    }
#endif

#ifdef PROFILLING_SENSING
    // Get host PC time now.
    clock_gettime(CLOCK_REALTIME, &read_samples_stop_time);
    diff_read_samples += srslte_time_diff(read_samples_start_time, read_samples_stop_time);
    read_samples_cnt++;
    LBT_PRINT("Read samples elapsed average time = %f milliseconds for %d samples.\n", (diff_read_samples/read_samples_cnt), num_read_samples);
#endif

#ifdef MEASURE_POWER
    // Measure channel power.
    if(0){
      channel_energy_sum += channel_energy;
      energy_cnt++;
      if(energy_cnt%1000000==0) {
        LBT_PRINT("channel_energy_sum: %f - lbt_threshold: %f\n",channel_energy_sum/energy_cnt, rf_monitor_handle->lbt_threshold);
        channel_energy_sum = 0.0;
        energy_cnt = 0;
      }
    } else {
      if(channel_energy >= rf_monitor_handle->lbt_threshold) {
        LBT_PRINT("channel_energy: %f - num_read_samples: %d - lbt_threshold: %f\n",channel_energy, num_read_samples, rf_monitor_handle->lbt_threshold);
      }
      // Identify maximum and minimum values of power.
      energy_cnt++;
      if(channel_energy > max_energy) {
        max_energy = channel_energy;
      }
      if(channel_energy < min_energy) {
        min_energy = channel_energy;
      }
      if(energy_cnt%10000 == 0) {
        energy_cnt = 0;
        LBT_PRINT("Energy - MIN: %f - MAX: %f\n",min_energy,max_energy);
      }
    }
#endif

    // FSM to excute Listen Before Talk procedure with backoff.
    switch(lbt_state) {
      case IDLE_ST:
      {
        // If we have a packet to transmit we should go to check medium state with random backoff.
        if(get_lbt_procedure_event() == LBT_START_EVT) {
          // Set drop packet flag to false, indicating packet should not be dropped. It will be dropped only if the LBT procedure times-out.
          lbt_drop_packet_flag = false;
          // If both flags are true then send packet immediatly.
          if(is_channel_free && rf_monitor_handle->immediate_transmission) {
            // Channel is FREE then transmit packet.
            set_lbt_procedure_event(LBT_DONE_EVT);
            // After allowing transmission, go back to IDLE and wait for another packet.
            lbt_state = ALLOW_TX_ST;
            LBT_DEBUG("Channel is free, packet allowed to be transmitted immediatly.\n",0);
          } else { // In case one of them is false them wait for backoff period.
            // Set event to BUSY indicating LBT is running.
            set_lbt_procedure_event(LBT_BUSY_EVT);
            // Generate random number to be the backoff timer. Generate a backoff period between 0 and maximum_backoff_period-1.
            if(maximum_backoff_period > 0){
              backoff = (rand() % maximum_backoff_period);
            }
            // Print backoff time.
            LBT_DEBUG("Backoff: %d\n",backoff);
            // Change to check medium state.
            lbt_state = CHECK_MEDIUM_ST;
            // Timestamp start of LBT procedure.
            clock_gettime(CLOCK_REALTIME, &lbt_procedure_start);
          }
        }
        break;
      }
      case CHECK_MEDIUM_ST:
      {
#ifdef PROFILLING_SENSING
        // Mark the start time.
        clock_gettime(CLOCK_REALTIME, &measure_power_start_time);
#endif

#ifdef PROFILLING_SENSING
        clock_gettime(CLOCK_REALTIME, &measure_power_stop_time);
        diff_measure_power += srslte_time_diff(measure_power_start_time, measure_power_stop_time);
        measure_power_cnt++;
        LBT_PRINT("Measure power average time = %f milliseconds for %d samples\n", (diff_measure_power/measure_power_cnt), num_read_samples);
#endif

        // Check if channel is FREE.
        if(is_channel_free) {
          if(backoff == 0) {
            // Update statistics sent to PHY transmission.
            lbt_copy_stats(&lbt_stats, &phy_tx_lbt_stats);
            // Channel is FREE and backoff is equal to 0, then transmit packet.
            set_lbt_procedure_event(LBT_DONE_EVT);
            // After allowing transmission, go back to IDLE and wait for another packet.
            lbt_state = ALLOW_TX_ST;
            LBT_DEBUG("Backoff is zero, packet allowed to be transmitted.\n",0);
          } else {
            backoff--;
            LBT_DEBUG("Backoff is %d.\n",backoff);
          }
          LBT_DEBUG("Channel FREE: channel_energy: %f - lbt_threshold: %f\n",channel_energy, rf_monitor_handle->lbt_threshold);
        } else {
          LBT_DEBUG("Channel BUSY: channel_energy: %f - lbt_threshold: %f\n",channel_energy, rf_monitor_handle->lbt_threshold);
        }

        // Check also if procedure has timed-out, if so, drop packet.
        if(helpers_is_timeout(lbt_procedure_start, rf_monitor_handle->lbt_timeout)) {
          // Flag used tell PHY TX to drop packet.
          lbt_drop_packet_flag = true;
          // LBT procedure has timed-out, then we drop packet.
          set_lbt_procedure_event(LBT_DONE_EVT);
          // After allowing transmission, go back to IDLE and wait for another packet.
          lbt_state = ALLOW_TX_ST;
          LBT_DEBUG("LBT procedure has timed-out, drop packet.\n",0);
        }
        break;
      }
      case ALLOW_TX_ST:
      {
        // Set LBT procedure to IDLE again.
        set_lbt_procedure_event(LBT_IDLE_EVT);
        lbt_state = IDLE_ST;
        break;
      }
      default:
        LBT_ERROR("Invalid LBT state.\n",0);
    }

    // Print LBT statistics.
    if((lbt_stats.channel_free_cnt + lbt_stats.channel_busy_cnt)%50000 == 0) {
      LBT_INFO("[LBT STATS]: FREE: %lld - BUSY: %lld - FREE AVG. PWR: %1.4f - BUSY AVG. PWR: %1.4f\n",lbt_stats.channel_free_cnt,lbt_stats.channel_busy_cnt, (double)lbt_stats.free_energy/lbt_stats.channel_free_cnt, (double)lbt_stats.busy_energy/lbt_stats.channel_busy_cnt);
    }
  }
  //******************************************************************************************************************************************
  // Uninitalize LBT.
  lbt_uninitialize();
  // Print information that module is finishing.
  LBT_DEBUG("Leaving LBT module thread.\n",0);
  // Exit thread with result code.
  pthread_exit(0);
}

// Frequency domain power measurement.
float lbt_channel_fd_power_measurement(cf_t *data, uint32_t num_read_samples) {
  float channel_energy = -1000.0;
  // Calculate power only if we have enough samples to apply FFT to.
  if(num_read_samples >= NUM_OF_SENSING_SAMPLES) {
    // Retrieve current TX channel being used by the TX module.
    uint32_t channel_to_monitor = rf_monitor_get_current_channel_to_monitor();
    // Check if TX Channel is one of the allowed channels for the competition bandwidth.
    if(channel_to_monitor >= 0 && channel_to_monitor < MAX_NUMBER_OF_CHANNELS) {
      // Apply FFT to base band samples.
      lbt_apply_fft_on_bb_samples(data);
      // Calculate power on the specific bandwidth of the current TX channel.
      channel_energy = 10*log10(srslte_vec_avg_power_cf(&freq_samples[channel_start_fft_bin_position[channel_to_monitor]], bw_in_fft_bins));
    }
  }
  return channel_energy;
}

// Time domain power measurement.
float lbt_channel_td_power_measurement(cf_t *data, uint32_t num_read_samples) {
  // Calculate power on the specific time domain samples.
  return 10*log10(srslte_vec_avg_power_cf(data, num_read_samples));
}

lbt_events_t get_lbt_procedure_event() {
  lbt_events_t evt;
  // Lock a mutex prior to accessing the LBT procedure event.
  pthread_mutex_lock(&lbt_procedure_evt_mutex);
  evt = lbt_procedure_event;
  // Unlock mutex upon accessing the LBT procedure event.
  pthread_mutex_unlock(&lbt_procedure_evt_mutex);
  return evt;
}

void set_lbt_procedure_event(lbt_events_t evt) {
  // Lock a mutex prior to accessing the LBT procedure event.
  pthread_mutex_lock(&lbt_procedure_evt_mutex);
  lbt_procedure_event = evt;
  // Unlock mutex upon accessing the LBT procedure event.
  pthread_mutex_unlock(&lbt_procedure_evt_mutex);
  // Notify other thread that LBT procedure status has changed.
  pthread_cond_signal(&lbt_procedure_evt_cv);
}

// Wait until LBT event is equal to DONE or IDLE, meaning the LBT procedure has finished up running.
void wait_lbt_to_be_done() {
  // Lock mutex so that we can wait for LBT procedure to be done.
  pthread_mutex_lock(&lbt_procedure_evt_mutex);
  // Wait for conditional variable to be true.
  while(lbt_procedure_event != LBT_DONE_EVT && lbt_procedure_event != LBT_IDLE_EVT) {
    pthread_cond_wait(&lbt_procedure_evt_cv, &lbt_procedure_evt_mutex);
    if(!run_rf_monitor_thread) {
      // We don not need to wait anymore as thread has been stopped.
      break;
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&lbt_procedure_evt_mutex);
}

// When this function is called, it means, there is a packet to be sent.
// When it returns true it means packet can be sent, otherwise, when it returns false packet must be dropped.
bool lbt_execute_procedure(lbt_stats_t *lbt_stats) {
  // Allow LBT procedure to start.
  set_lbt_procedure_event(LBT_START_EVT);
  // Wait until LBT procedure is done.
  wait_lbt_to_be_done();
  // Copy statistics.
  if(lbt_stats != NULL) {
    lbt_copy_stats(&phy_tx_lbt_stats, lbt_stats);
  }
  return !lbt_drop_packet_flag;
}

void lbt_apply_fft_on_bb_samples(cf_t *data) {
  // Execute FFT on samples read from USRP.
  // Copy base band samples into fft type for fftw execution.
  memcpy((uint8_t*)fft_in_samples,(uint8_t*)data,sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);
  // Apply FFT to the received base band samples.
  fftwf_execute(lbt_fft_plan);
  // Copy the result from the fft type into the array for sensing.
  memcpy((uint8_t*)freq_samples,(uint8_t*)fft_out_samples,sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);
}

void lbt_initialize_fft() {
  // Allocate Memory for FFT application.
   fft_in_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SENSING_SAMPLES);
   fft_out_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SENSING_SAMPLES);
   // Allocate memory for subbands.
   freq_samples = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * NUM_OF_SENSING_SAMPLES);
   if(!freq_samples) {
     LBT_ERROR("Error allocating freq_samples memory.\n",0);
     exit(-1);
   }
   // Create a one dimensional FFT plan.
   lbt_fft_plan = fftwf_plan_dft_1d(NUM_OF_SENSING_SAMPLES, fft_in_samples, fft_out_samples, FFTW_FORWARD, FFTW_ESTIMATE);
}

void lbt_uninitialize_fft() {
  // Free allocated FFT resources
  fftwf_destroy_plan(lbt_fft_plan);
  if(fft_in_samples) {
    fftwf_free(fft_in_samples);
  }
  if(fft_out_samples) {
    fftwf_free(fft_out_samples);
  }
  if(freq_samples) {
    free(freq_samples);
  }
}

void lbt_initialize_freq_domain_parameters(rf_monitor_handle_t *rf_monitor_handle) {
  double offset = rf_monitor_handle->sample_rate/2.0 - rf_monitor_handle->competition_bw/2.0;
  double delta_f = rf_monitor_handle->sample_rate/NUM_OF_SENSING_SAMPLES;
  offset_in_fft_bins = ceil(offset/delta_f);
  bw_in_fft_bins = floor(rf_monitor_handle->phy_bw/delta_f);
}

uint32_t lbt_get_initial_channel_fft_bin(uint32_t channel_to_monitor) {
  return (bw_in_fft_bins*channel_to_monitor + offset_in_fft_bins);
}

// Values return will result in 44.44 us.
uint32_t lbt_get_number_of_samples_to_read(uint32_t nof_prb) {
  uint32_t num_of_samples;
  switch(nof_prb) {
    case 6:
      num_of_samples = 86;
      break;
    case 15:
      num_of_samples = 171;
      break;
    case 25:
      num_of_samples = 256;
      break;
    case 50:
      num_of_samples = 512;
      break;
    case 75:
      num_of_samples = 683;
      break;
    case 100:
      num_of_samples = 1024;
      break;
    default:
      num_of_samples = 256;
  }
  return num_of_samples;
}

// Initialize LBT stats struct.
void lbt_init_stats(lbt_stats_t *lbt_stats) {
  lbt_stats->channel_free_cnt = 0;
  lbt_stats->channel_busy_cnt = 0;
  lbt_stats->free_energy = 0.0;
  lbt_stats->busy_energy = 0.0;
}

// Copy stats to structure used to sent stats to PHY TX .
void lbt_copy_stats(lbt_stats_t *lbt_stats, lbt_stats_t *phy_tx_lbt_stats) {
  phy_tx_lbt_stats->channel_free_cnt = lbt_stats->channel_free_cnt;
  phy_tx_lbt_stats->channel_busy_cnt = lbt_stats->channel_busy_cnt;
  phy_tx_lbt_stats->free_energy = lbt_stats->free_energy;
  phy_tx_lbt_stats->busy_energy = lbt_stats->busy_energy;
}
