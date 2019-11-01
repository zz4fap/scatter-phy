#include "srslte/rf_monitor/rf_monitor.h"

#define RF_MONITOR_RX_LO_OFFSET +42.0e6 // RX local offset.

// *********** Global variables ***********
pthread_attr_t rf_monitor_thread_attr;
pthread_t rf_monitor_thread_id;

// Variable used to stop rf monitor thread.
volatile sig_atomic_t run_rf_monitor_thread = true;

// Object used to hold rf monitor module configuration.
rf_monitor_handle_t rf_monitor_handle;

// Communicator handler dor RF Monitor.
LayerCommunicator_handle rf_monitor_comm_handle;

// This mutex is used to synchronize the access to the current TX channel
pthread_mutex_t current_channel_to_monitor_mutex;

// Pointer to functions with RF monitor modules.
void*(*rf_monitor_modules[5])(void*) = {iq_dumping_work, lbt_work, rssi_monitoring_work, spectrum_sensing_alg_work, iq_dump_plus_sensing_work};

// *********** Functions **************
void rf_monitor_set_handler(srslte_rf_t *rf, transceiver_args_t *prog_args) {
  rf_monitor_handle.channel_handler = rf->rf_monitor_channel_handler;
  rf_monitor_handle.single_log_duration = prog_args->single_log_duration;
  rf_monitor_handle.logging_frequency = prog_args->logging_frequency;
  rf_monitor_handle.max_number_of_dumps = prog_args->max_number_of_dumps;
  rf_monitor_handle.path_to_start_file = prog_args->path_to_start_file;
  rf_monitor_handle.path_to_log_files = prog_args->path_to_log_files;
  rf_monitor_handle.node_id = prog_args->node_id;
  rf_monitor_handle.rf = rf;
  rf_monitor_handle.central_frequency = prog_args->competition_center_frequency; // central frequency at which the sensing module should operate.
  rf_monitor_handle.sensing_channel = prog_args->rf_monitor_channel; // Specify which channel should be used for sensing the spectrum.
  rf_monitor_handle.nof_prb = prog_args->nof_prb; // Set the number of PRB.
  rf_monitor_handle.sample_rate = prog_args->rf_monitor_rx_sample_rate; // Sample rate. Value given in MHz.
  rf_monitor_handle.sensing_rx_gain = prog_args->sensing_rx_gain;
  rf_monitor_handle.data_type = prog_args->iq_dump_data_type; // Data type used to store IQ samples into file.
  rf_monitor_handle.competition_bw = prog_args->competition_bw; // Bandwidth specified during competion initialization.
  rf_monitor_handle.phy_bw = helpers_get_bw_from_nprb(prog_args->nof_prb); // Bandwidth used by the PHY.
  rf_monitor_handle.lbt_threshold = prog_args->lbt_threshold; // Threshold used to check if TX channel is BUSY or FREE.
  rf_monitor_handle.lbt_timeout = prog_args->lbt_timeout; // LBT Timeout in milliseconds.
  rf_monitor_handle.lbt_use_fft_based_pwr = prog_args->lbt_use_fft_based_pwr; // Enable/disable FFT based power measurements.
  rf_monitor_handle.lbt_channel_to_monitor = prog_args->default_tx_channel; // Set TX channel used by PHY to transmit data. This channel will be monitored by the RF Monitor.
  rf_monitor_handle.maximum_backoff_period = prog_args->max_backoff_period;
  rf_monitor_handle.iq_dumping = prog_args->iq_dumping;
  rf_monitor_handle.immediate_transmission = prog_args->immediate_transmission; // Enable/disable immediate transmissions, i.e., if the packet is sent immediately after channel is FREE.
  rf_monitor_handle.rf_monitor_option = prog_args->rf_monitor_option;
}

// This function is used to set everything needed for the sensing thread to run accordinly.
int rf_monitor_initialize(srslte_rf_t *rf, transceiver_args_t *prog_args) {

  // Create a communicator handle object for communication between rf monitor module and AI module.
  char source_module[] = "MODULE_RF_MON";
  char destination_module[] = "MODULE_AI";

  // Instantiate communicator module so that we can receive/transmit commands and data.
  communicator_make(source_module, destination_module, NULL, &rf_monitor_comm_handle);

  // Set RF Monitor handler.
  rf_monitor_set_handler(rf, prog_args);

  // Set the gain apllied to this RX channel cahin.
  double current_set_gain = srslte_rf_set_rx_gain(rf_monitor_handle.rf, rf_monitor_handle.sensing_rx_gain, rf_monitor_handle.sensing_channel);
  RF_MONITOR_INFO("RX gain set to: %.1f [dB]\n", current_set_gain);

  // Set receiver frequency for RF Monitor module.
  double lo_offset = (rf_monitor_handle.rf)->num_of_channels == 1 ? 0.0:(double)RF_MONITOR_RX_LO_OFFSET;
  float rx_freq = helpers_calculate_channel_center_frequency(rf_monitor_handle.central_frequency, rf_monitor_handle.competition_bw, rf_monitor_handle.phy_bw, rf_monitor_handle.lbt_channel_to_monitor);
  if(rf_monitor_handle.lbt_use_fft_based_pwr || rf_monitor_handle.rf_monitor_option == SPECTRUM_SENSING_MODULE || rf_monitor_handle.rf_monitor_option == IQ_DUMP_PLUS_SENSING_MODULE) {
     rx_freq = rf_monitor_handle.central_frequency;
  }
  rx_freq = srslte_rf_set_rx_freq2(rf_monitor_handle.rf, rx_freq, lo_offset, rf_monitor_handle.sensing_channel);
  srslte_rf_rx_wait_lo_locked(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel);
  RF_MONITOR_INFO("Center frequency to monitor set to: %.2f [MHz]\n", rx_freq/1000000.0);

  // Set the sample rate for RF Monitoring the channel in the specific channel.
  if(rf_monitor_handle.lbt_use_fft_based_pwr || rf_monitor_handle.rf_monitor_option == SPECTRUM_SENSING_MODULE || rf_monitor_handle.rf_monitor_option == IQ_DUMP_PLUS_SENSING_MODULE) {
    // The sample rate must be 23.04 MHz when FFT based power measurement is enabled, otherwise we use the value passed through command line.
    rf_monitor_handle.sample_rate = (double)srslte_sampling_freq_hz(100);
    RF_MONITOR_INFO("FFT power measurement is enabled then sample rate now is: %.2f [MHz]\n", rf_monitor_handle.sample_rate/1000000.0);
  }
  float srate_rf = srslte_rf_set_rx_srate(rf_monitor_handle.rf, rf_monitor_handle.sample_rate, rf_monitor_handle.sensing_channel);
  if(srate_rf != rf_monitor_handle.sample_rate) {
    RF_MONITOR_ERROR("Could not set the sensing sampling rate\n",0);
    return -1;
  }
  RF_MONITOR_INFO("Sampling rate set to: %.2f [MHz]\n", srate_rf/1000000.0);

  int error;
  // Stopping RX stream for specific channel.
  if((error = srslte_rf_stop_rx_stream(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel)) != 0) {
    RF_MONITOR_ERROR("Error stopping RX sensing stream: %d....\n",error);
    return -1;
  }

  // Flushing buffer so that it is clean and does not contain out samples.
  srslte_rf_flush_buffer(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel);

  // Opening RX stream for the specifc channel.
  if((error = srslte_rf_start_rx_stream(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel)) != 0) {
    RF_MONITOR_ERROR("Error starting RX sensing stream: %d....\n",error);
    return -1;
  }

  // Print RF Monitor handler contents.
  rf_monitor_print_handle();

  // Init mutex.
  if(pthread_mutex_init(&current_channel_to_monitor_mutex, NULL) != 0) {
    LBT_ERROR("Current channel to monitor Mutex init failed.\n",0);
    return -1;
  }

  // Create threads to perform spectrum sensing.
  pthread_attr_init(&rf_monitor_thread_attr);
  pthread_attr_setdetachstate(&rf_monitor_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread to sense channel.
  int rc = pthread_create(&rf_monitor_thread_id, &rf_monitor_thread_attr, (*rf_monitor_modules[rf_monitor_handle.rf_monitor_option]), (void *)&rf_monitor_handle);
  if(rc) {
    RF_MONITOR_ERROR("ERROR; return code from sensing pthread_create() is %d\n", rc);
    return -1;
  }

  return 0;
}

// Free all the resources used by the sesning module.
int rf_monitor_uninitialize() {
  int status;
  int rc;
  run_rf_monitor_thread = false; // Stop sensing thread.
  pthread_attr_destroy(&rf_monitor_thread_attr);
  rc = pthread_join(rf_monitor_thread_id, (void *)&status);
  if(rc) {
    RF_MONITOR_ERROR("ERROR; return code from sensing pthread_join() is %d\n", rc);
    return -1;
  }
  // Destroy mutexes.
  pthread_mutex_destroy(&current_channel_to_monitor_mutex);
  // Stop RX sensing stream.
  int error;
  if((error = srslte_rf_stop_rx_stream(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel)) != 0 ) {
    RF_MONITOR_ERROR("Error stopping RX sensing stream: %d....\n",error);
    return -1;
  }
  // Free communicator handle object for communication between rf monitor module and AI module.
  communicator_free(&rf_monitor_comm_handle);
  // Return the status.
  return status;
}

void rf_monitor_change_central_frequency(double central_frequency) {
  // Checking if last configured central frequency is different from the requested one.
  if(rf_monitor_handle.central_frequency != central_frequency) {
    // Set the new RX frequency.
    srslte_rf_set_rx_freq(rf_monitor_handle.rf, central_frequency, rf_monitor_handle.sensing_channel);
    // Wait for oscillators to stabilize and lock.
    srslte_rf_rx_wait_lo_locked(rf_monitor_handle.rf, rf_monitor_handle.sensing_channel);
    // Updating last configured sensing RX central frequency field.
    rf_monitor_handle.central_frequency = central_frequency;
    // Print new new central frequency.
    RF_MONITOR_DEBUG("RX sensing frequency set to: %.2f MHz\n",rf_monitor_handle.central_frequency/1000000.0);
  }
}

void rf_monitor_change_rx_gain(double sensing_rx_gain) {
  // Checking if last configured gain is different from the current requested one.
  if(rf_monitor_handle.sensing_rx_gain != sensing_rx_gain) {
    // Set a new RX gain.
    srslte_rf_set_rx_gain(rf_monitor_handle.rf, sensing_rx_gain, rf_monitor_handle.sensing_channel);
    // Updating last configured sensing RX gain field.
    rf_monitor_handle.sensing_rx_gain = sensing_rx_gain;
    // Print new RX gain.
    RF_MONITOR_DEBUG("RX sensing gain set to: %d\n", rf_monitor_handle.sensing_rx_gain);
  }
}

void rf_monitor_change_rx_sample_rate(double sample_rate) {
  // Checking if last configured RX sample rate is different from the current requested one.
  if(rf_monitor_handle.sample_rate != sample_rate) {
    float srate_rf = srslte_rf_set_rx_srate(rf_monitor_handle.rf, sample_rate, rf_monitor_handle.sensing_channel);
    if (srate_rf != sample_rate) {
      RF_MONITOR_ERROR("Could not set the sensing sampling rate.\n",0);
    }
    // Updating last configured sensing sampling rate field.
    rf_monitor_handle.sample_rate = sample_rate;
    // Print new RX sensing sampling rate.
    RF_MONITOR_INFO("RX sensing sampling rate set to: %.2f MHz\n", rf_monitor_handle.sample_rate/1000000.0);
  }
}

void rf_monitor_print_handle() {
  RF_MONITOR_PRINT("single_log_duration: %d [ms]\n",rf_monitor_handle.single_log_duration);
  RF_MONITOR_PRINT("logging_frequency: %d [ms]\n",rf_monitor_handle.logging_frequency);
  RF_MONITOR_PRINT("path_to_start_file: %s\n",rf_monitor_handle.path_to_start_file);
  RF_MONITOR_PRINT("path_to_log_files: %s\n",rf_monitor_handle.path_to_log_files);
  RF_MONITOR_PRINT("max_number_of_dumps: %d\n",rf_monitor_handle.max_number_of_dumps);
  RF_MONITOR_PRINT("node_id: %d\n",rf_monitor_handle.node_id);
  RF_MONITOR_PRINT("central_frequency: %1.2f [MHz]\n",rf_monitor_handle.central_frequency/1000000.0);
  RF_MONITOR_PRINT("sensing_channel: %d\n",rf_monitor_handle.sensing_channel);
  RF_MONITOR_PRINT("PRB: %d\n",rf_monitor_handle.nof_prb);
  RF_MONITOR_PRINT("sample_rate: %1.2f [MHz]\n",(double)rf_monitor_handle.sample_rate/1000000.0);
  RF_MONITOR_PRINT("rf_monitor_rx_gain: %1.2f [dB]\n",rf_monitor_handle.sensing_rx_gain);
  RF_MONITOR_PRINT("data_type: %d\n",rf_monitor_handle.data_type);
  RF_MONITOR_PRINT("competition_bw: %1.2f [MHz]\n",rf_monitor_handle.competition_bw/1000000.0);
  RF_MONITOR_PRINT("phy_bw: %1.2f [MHz]\n",rf_monitor_handle.phy_bw/1000000.0);
  RF_MONITOR_PRINT("LBT checking is %s\n",rf_monitor_handle.lbt_threshold < 100.0?"Enabled":"Disabled");
  RF_MONITOR_PRINT("lbt_threshold: %1.2f [dBm]\n",rf_monitor_handle.lbt_threshold);
  RF_MONITOR_PRINT("lbt_timeout: %" PRIu64 " [ms]\n",rf_monitor_handle.lbt_timeout);
  RF_MONITOR_PRINT("FFT based power measurement: %s\n",rf_monitor_handle.lbt_use_fft_based_pwr?"Enabled":"Disabled");
  RF_MONITOR_PRINT("Initial channel to monitor: %d\n",rf_monitor_handle.lbt_channel_to_monitor);
  RF_MONITOR_PRINT("Number of samples to read: %d\n",rf_monitor_handle.rf->rx_nof_samples);
  RF_MONITOR_PRINT("Maximum backoff period: %d\n",rf_monitor_handle.maximum_backoff_period);
  RF_MONITOR_PRINT("iq_dumping: %s\n",rf_monitor_handle.iq_dumping?"on":"off");
  RF_MONITOR_PRINT("immediate_transmission: %s\n",rf_monitor_handle.immediate_transmission?"Enabled":"Disabled");
  RF_MONITOR_PRINT("rf_monitor_option: %d\n",rf_monitor_handle.rf_monitor_option);
}

void rf_monitor_send_sensing_statistics(uint64_t seq_number, uint32_t status, time_t full_secs, double frac_secs, float frequency, float sample_rate, float gain, float rssi, int32_t data_length, uchar *data) {

  phy_stat_t phy_sensing_stat;

  // Set values to the RX Stats Structure.
  phy_sensing_stat.seq_number = seq_number;                                           // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  phy_sensing_stat.status = status;                                                   // Set initially with error. If retuned with error all values below are invalid.
  phy_sensing_stat.host_timestamp = helpers_get_host_time_now();                              // Host PC time value when (ch,slot) PHY data are demodulated
  phy_sensing_stat.fpga_timestamp = helpers_convert_fpga_time_into_int(full_secs, frac_secs); // FPGA internal timer. This field carries the timestamp of the first received sample.
  phy_sensing_stat.frame = 0;
  phy_sensing_stat.slot = 0;
  phy_sensing_stat.ch = 0;
  phy_sensing_stat.mcs = 0;                              // Set MCS to unspecified number. If this number is receiber by upper layer it means nothing was received and status MUST be checked.
  phy_sensing_stat.num_cb_total = 0;	                   // Number of Code Blocks (CB) received in the (ch, slot)
  phy_sensing_stat.num_cb_err = 0;                       // How many CBs get CRC error in the (ch, slot)
  phy_sensing_stat.stat.sensing_stat.frequency = frequency;
  phy_sensing_stat.stat.sensing_stat.sample_rate = sample_rate;
  phy_sensing_stat.stat.sensing_stat.gain = gain;
  phy_sensing_stat.stat.sensing_stat.rssi = rssi;
  phy_sensing_stat.stat.sensing_stat.length = data_length;    // Set data length to zero.
  phy_sensing_stat.stat.sensing_stat.data = data;             // Set data pointer to data.

  // Send PHY RX statistics to upper layer.
  RF_MONITOR_DEBUG("Sending Sensing statistics information upwards...\n",0);
  communicator_send_phy_stat_message(rf_monitor_comm_handle, SENSING_STAT, &phy_sensing_stat, NULL);
  // Print sensing information.
  RF_MONITOR_PRINT_SENSING_STATS(&phy_sensing_stat);
}

void rf_monitor_print_sensing_statistics(phy_stat_t *phy_stat) {
  // Print PHY Sensing Stats Structure.
  printf("******************* PHY RX Statistics *******************\n"\
    "Seq. number: %d\n"\
    "Status: %d\n"\
    "Host Timestamp: %" PRIu64 "\n"\
    "FPGA Timestamp: %d\n"\
    "Frequency: %1.2f\n"\
    "Sample Rate: %1.2f\n"\
    "Gain: %1.2f\n"\
    "Length: %d\n"\
    "*********************************************************************\n"\
    ,phy_stat->seq_number,phy_stat->status,phy_stat->host_timestamp,phy_stat->fpga_timestamp,phy_stat->stat.sensing_stat.frequency,phy_stat->stat.sensing_stat.sample_rate,phy_stat->stat.sensing_stat.gain,phy_stat->stat.sensing_stat.length);
}

void rf_monitor_change_channel_to_monitor(uint32_t lbt_channel_to_monitor) {
  // Calculate cntral frequency for the channel.
  double tx_channel_center_freq = helpers_calculate_channel_center_frequency(rf_monitor_handle.central_frequency, rf_monitor_handle.competition_bw, rf_monitor_handle.phy_bw, lbt_channel_to_monitor);
  // Set the new RX Channel frequencyfor RF Monitor.
  double lo_offset = (rf_monitor_handle.rf)->num_of_channels == 1 ? 0.0:(double)RF_MONITOR_RX_LO_OFFSET;
  // Set central frequency for channel.
  tx_channel_center_freq = srslte_rf_set_rx_freq2(rf_monitor_handle.rf, tx_channel_center_freq, lo_offset, rf_monitor_handle.sensing_channel);
  // Print new new central frequency.
  RF_MONITOR_DEBUG("Channel frequency to monitor set to: %.2f [MHz]\n",tx_channel_center_freq/1000000.0);
}

void rf_monitor_set_current_channel_to_monitor(uint32_t channel_to_monitor) {
  // Lock a mutex prior to accessing the current tx channel.
  pthread_mutex_lock(&current_channel_to_monitor_mutex);
  if(rf_monitor_handle.lbt_channel_to_monitor != channel_to_monitor) {
    // If we don't use frequency domain (FD) power measurement, then we should change the central frequency of the monitor channel.
    if(!rf_monitor_handle.lbt_use_fft_based_pwr) {
      rf_monitor_change_channel_to_monitor(channel_to_monitor);
    }
    // Update channel used to monitor TX.
    rf_monitor_handle.lbt_channel_to_monitor = channel_to_monitor;
  }
  // Unlock mutex upon accessing the current tx channel.
  pthread_mutex_unlock(&current_channel_to_monitor_mutex);
}

uint32_t rf_monitor_get_current_channel_to_monitor() {
  // Lock a mutex prior to accessing the current tx channel.
  pthread_mutex_lock(&current_channel_to_monitor_mutex);
  uint32_t channel_to_monitor = rf_monitor_handle.lbt_channel_to_monitor;
  // Unlock mutex upon accessing the current tx channel.
  pthread_mutex_unlock(&current_channel_to_monitor_mutex);
  return channel_to_monitor;
}
