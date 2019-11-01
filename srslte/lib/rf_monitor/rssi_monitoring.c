#include "srslte/rf_monitor/rssi_monitoring.h"

#include <signal.h>

// variable used to enable/disable actions.
volatile sig_atomic_t run_rssi_procedure = false;

bool read_hardware_rssi_enabled = false;

void rssi_monitoring_alarm_handle(int sig) {
   if(sig == SIGALRM) {
      run_rssi_procedure = true;
   }
}

void *rssi_monitoring_work(void *h) {

  RSSI_MONITORING_INFO("Start: rssi_monitoring_work()\n",0);

  rf_monitor_handle_t *sensing_handle = (rf_monitor_handle_t *)h;
  srslte_timestamp_t first_sample_timestamp;
  uint32_t num_read_samples = 0;
  float rssi;
  uint32_t nsamples = (sensing_handle->rf)->rx_nof_samples; // Number of samples to be read in every call to srslte_rf_recv_channel_with_time().
  cf_t data[nsamples];
#ifdef PROFILLING_RSSI_MONITORING
  struct timespec start_time, end_time;
#endif

  // Install handler for alarm.
  signal(SIGALRM, rssi_monitoring_alarm_handle);

  // Set to true so that we can run file checking for the first time.
  run_rssi_procedure = true;

  read_hardware_rssi_enabled = false;

  // Thread loop, wait here until main thread says the contrary.
  while(run_rf_monitor_thread) {

#ifdef PROFILLING_RSSI_MONITORING
    // Mark the start time.
    clock_gettime(CLOCK_REALTIME, &start_time);
#endif

    // Read IQ Samples.
    num_read_samples = srslte_rf_recv_channel_with_time(sensing_handle->channel_handler, data, nsamples, true, &(first_sample_timestamp.full_secs), &(first_sample_timestamp.frac_secs));

#ifdef PROFILLING_RSSI_MONITORING
    clock_gettime(CLOCK_REALTIME, &end_time);
    double diff = srslte_time_diff(start_time, end_time);
    RSSI_MONITORING_DEBUG("Elapsed time = %f milliseconds for %d samples.\n", diff, num_read_samples);
#endif

    // Print RSSI every 2 second.
    if(run_rssi_procedure) {
      run_rssi_procedure = false;
      rssi = 10.0*log10f(srslte_vec_avg_power_cf(data, num_read_samples));
      RSSI_MONITORING_DEBUG("Frequency: %4.1f MHz - Sample rate: %4.2f MSps - RSSI: %3.2f dBm\r", sensing_handle->central_frequency/1000000.0, sensing_handle->sample_rate/1000000.0, rssi); fflush(stdout);
      alarm(2);
    }

    if(read_hardware_rssi_enabled) {
      // Try to retrieve RSSI from USRP hardware.
      if(srslte_rf_has_rssi(sensing_handle->rf, sensing_handle->sensing_channel)) {
        rssi = srslte_rf_get_rssi(sensing_handle->rf, sensing_handle->sensing_channel);
      } else {
        rssi = srslte_rf_get_rx_gain(sensing_handle->rf, sensing_handle->sensing_channel);
      }
    }
  }
  RSSI_MONITORING_DEBUG("Leaving RSSI module thread.\n",0);
  // Exit thread with result code.
  pthread_exit(0);
}
