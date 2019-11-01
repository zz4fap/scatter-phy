#include "check_phy_synchronization.h"

#define NOF_SNR_VALUES 26

static tput_context_t *tput_context;

int main(int argc, char *argv[]) {

  float SNR_VECTOR[NOF_SNR_VALUES] = {27.21,23.23,20.25,18.45,17.21,13.26,10.23,8.47,7.22,3.26,0.24,-1.54,-2.15,-2.49,-2.79,-3.52,-4.14,-4.69,-5.23,-6.78,-8.52,-9.76,-10.73,-11.54,-12.16,-12.74};
  float pss_prr[NOF_SNR_VALUES];
  bzero(pss_prr, sizeof(float)*NOF_SNR_VALUES);
  float pdsch_prr[NOF_SNR_VALUES];
  bzero(pdsch_prr, sizeof(float)*NOF_SNR_VALUES);

  change_process_priority(-20);

  // Allocate memory for a new Tput context object.
  tput_context = (tput_context_t*)srslte_vec_malloc(sizeof(tput_context_t));
  // Check if memory allocation was correctly done.
  if(tput_context == NULL) {
    printf("Error allocating memory for Tput context object.\n");
    exit(-1);
  }
  // Zero everything in the context.
  bzero(tput_context, sizeof(tput_context_t));

  tput_context->go_exit = false;

  // Initialize signal handler.
  initialize_signal_handler();

  // Retrieve input arguments.
  parse_args(&tput_context->args, argc, argv);

  if((MAX_DATA_COUNTER_VALUE % tput_context->args.nof_slots_to_tx) == 0) {
    tput_context->maximum_counter_value = MAX_DATA_COUNTER_VALUE;
  } else {
    tput_context->maximum_counter_value = (tput_context->args.nof_slots_to_tx)*((uint32_t)MAX_DATA_COUNTER_VALUE/tput_context->args.nof_slots_to_tx);
  }

  printf("maximum_counter_value: %d\n", tput_context->maximum_counter_value);

  // Create communicator to communicate with PHY.
  communicator_make("MODULE_MAC", "MODULE_PHY", "MODULE_CHANNEL_EMULATOR", &tput_context->handle);

  // Wait some time before starting anything.
  sleep(2);

  for(uint32_t snr_idx = 0; (snr_idx < NOF_SNR_VALUES && !tput_context->go_exit); snr_idx++) {
    // Enable Rx side to decode data.
    tput_context->run_rx_side = true;

    // Configure channel emulator.
    tput_context->chan_emu_conf.enable_simple_awgn_channel = true;
    tput_context->chan_emu_conf.snr = SNR_VECTOR[snr_idx];
    communicator_send_channel_emulator_config(tput_context->handle, &tput_context->chan_emu_conf);

    // Start Tx thread.
    printf("[Main] Starting Tx side thread...\n");
    start_tx_side_thread(tput_context);

    sleep(2);

    // Run Rx side loop.
    printf("[Main] Starting Rx side...\n");
    rx_side(tput_context);

    // Wait some time so that all packets are transmitted.
    printf("[Main] Leaving the application in 2 seconds...\n");
    sleep(2);

    // Stop Tx side thread.
    printf("[Main] Stopping Tx side thread...\n");
    stop_tx_side_thread(tput_context);

    pss_prr[snr_idx] = ((float)tput_context->total_nof_detected_slots)/tput_context->nof_transmitted_slot_counter;

    pdsch_prr[snr_idx] = ((float)tput_context->total_nof_decoded_slots)/tput_context->nof_transmitted_slot_counter;

    printf("[Main] -------> SNR: %1.4f [dB] - PSS PRR: %1.4f - PDSCH PRR: %1.4f\n", SNR_VECTOR[snr_idx], pss_prr[snr_idx], pdsch_prr[snr_idx]);
  }

  FILE *fid;
  char fileName[100];
  sprintf(fileName, "cqi_mcs_map_phy_bw_%d_mcs_%d.dat", tput_context->args.phy_bw_idx, tput_context->args.mcs);
  fid = fopen(fileName, "w");

  printf("\n\n--------------------------------------------------------------------\n");
  for(uint32_t snr_idx = 0; (snr_idx < NOF_SNR_VALUES && !tput_context->go_exit); snr_idx++) {
    printf("[Main] -------> SNR: %1.4f [dB] - PSS PRR: %1.4f - PDSCH PRR: %1.4f\n", SNR_VECTOR[snr_idx], pss_prr[snr_idx], pdsch_prr[snr_idx]);
    fprintf(fid, "%d\t%d\t%1.2f\t%1.4f\t%1.4f\n", tput_context->args.phy_bw_idx, tput_context->args.mcs, SNR_VECTOR[snr_idx], pss_prr[snr_idx], pdsch_prr[snr_idx]);
  }

  fclose(fid);

  // Free communicator related objects.
  communicator_free(&tput_context->handle);

  // Free memory used to store Tput context object.
  if(tput_context) {
    free(tput_context);
    tput_context = NULL;
  }

  return 0;
}

void rx_side(tput_context_t *tput_context) {
  basic_ctrl_t basic_ctrl;
  phy_stat_t phy_rx_stat;
  uchar data[10000];
  bool ret;
  uint32_t errors[2] = {0, 0}, nof_errors = 0, nof_decoded[2] = {0, 0}, nof_detected[2] = {0, 0};
  uint32_t last_nof_decoded_counter[2] = {0, 0}, last_nof_detected_counter[2] = {0, 0};
  double prr[2] = {0.0, 0.0};
  uint32_t correct_cnt[MAX_NOF_PHYS] = {0, 0};
  uint32_t error_cnt[MAX_NOF_PHYS] = {0, 0};
#if(CHECK_RX_SEQUENCE==1)
  uint32_t sequence_error_cnt[MAX_NOF_PHYS] = {0, 0};
  uint32_t cnt[MAX_NOF_PHYS] = {0, 0};
  uint32_t loop_cnt[MAX_NOF_PHYS] = {0, 0};
  uint32_t data_cnt[MAX_NOF_PHYS];
  uint32_t last_tb_error_cnt[MAX_NOF_PHYS] = {0, 0};
  // Initialize expected data vector.
  for(uint32_t i = 0; i < MAX_NOF_PHYS; i++) {
    data_cnt[i] = i*tput_context->args.nof_slots_to_tx;
  }
#endif
#if(ENABLE_DECODING_TIME_PROFILLING==1)
  double decoding_time[MAX_NOF_PHYS] = {0.0, 0.0};
  double synch_plus_decoding_time[MAX_NOF_PHYS] = {0.0, 0.0};
  double max_decoding_time[MAX_NOF_PHYS] = {0.0, 0.0};
  uint32_t max_decoding_time_cnt[MAX_NOF_PHYS] = {0, 0};
#endif
  uint32_t send_to = 0, intf_id = 0;
  uint64_t timestamp, last_msg_timestamp = 0;
  int diff = -1;

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  tput_context->total_nof_detected_slots = 0;
  tput_context->total_nof_decoded_slots = 0;

  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);

  // Set timestamp.
  timestamp = get_host_time_now_us() + 2000;

  // Configure Rx of the radios.
  for(uint32_t phy_id = 0; phy_id < tput_context->args.nof_phys; phy_id++) {
    // Create basic control message to control Tx chain.
    createBasicControl(&basic_ctrl, PHY_RX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, (tput_context->args.rx_channel+phy_id), timestamp, 0, tput_context->args.rx_gain, tput_context->args.rf_boost, 1, NULL);
    // Send Rx control to PHY.
    communicator_send_basic_control(tput_context->handle, &basic_ctrl);
  }

  // Loop until otherwise said.
  printf("[Rx side] Starting Rx side thread loop...\n");
  while(!tput_context->go_exit && tput_context->run_rx_side) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

    // Keep timestamp for last decoded message.
    if(ret) {
      last_msg_timestamp = get_host_time_now_us();
    } else {
      diff = (int)(((int)get_host_time_now_us()) - last_msg_timestamp);
      if(diff >= 7000000) {
        // After 7 seconds without receiving anything we leave the Rx side function.
        break;
      }
    }

    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(!tput_context->go_exit && tput_context->run_rx_side && ret) {

      if(phy_rx_stat.status == PHY_SUCCESS) {
#if(ENABLE_DECODING_TIME_PROFILLING==1)
        if(phy_rx_stat.stat.rx_stat.decoding_time >= 0) {
          decoding_time[phy_rx_stat.phy_id] += phy_rx_stat.stat.rx_stat.decoding_time;
          // Get the maximum decoding time.
          if(phy_rx_stat.stat.rx_stat.decoding_time > max_decoding_time[phy_rx_stat.phy_id]) {
            max_decoding_time[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.decoding_time;
            max_decoding_time_cnt[phy_rx_stat.phy_id]++;
          }
        }
        if(phy_rx_stat.stat.rx_stat.synch_plus_decoding_time >= 0) {
          synch_plus_decoding_time[phy_rx_stat.phy_id] += phy_rx_stat.stat.rx_stat.synch_plus_decoding_time;
        }
#endif
        printf("[Rx side] PHY Rx Id: %d - TB #: %d - # Slots: %d - Slot #: %d - channel: %d - MCS: %d - last_noi: %d - peak: %1.2f - CQI: %d - RSSI: %1.2f [dBW] - SINR: %1.2f [dB] - Noise: %1.4f - CFO: %+2.2f [KHz] - nof received bytes: %d - decoding time: %f [ms] - data: %d\n", phy_rx_stat.phy_id, (correct_cnt[phy_rx_stat.phy_id]+1), phy_rx_stat.stat.rx_stat.nof_slots_in_frame, phy_rx_stat.stat.rx_stat.slot_counter, phy_rx_stat.ch, phy_rx_stat.mcs, phy_rx_stat.stat.rx_stat.last_noi, phy_rx_stat.stat.rx_stat.peak_value, phy_rx_stat.stat.rx_stat.cqi, phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.sinr, phy_rx_stat.stat.rx_stat.noise, phy_rx_stat.stat.rx_stat.cfo, phy_rx_stat.stat.rx_stat.length, phy_rx_stat.stat.rx_stat.decoding_time, phy_rx_stat.stat.rx_stat.data[0]);
        correct_cnt[phy_rx_stat.phy_id]++;
#if(CHECK_RX_SEQUENCE==1)
        // This piece of code was written to test with any number of vPHYs but with all of them transmitting 2 TBs each.
        if(phy_rx_stat.stat.rx_stat.data[0] != data_cnt[phy_rx_stat.phy_id]) {
          sequence_error_cnt[phy_rx_stat.phy_id]++;
          printf("[Rx side] PHY Rx Id: %d - Channel #: %d - Received data: %d != Expected data: %d - # Errors: %d\n", phy_rx_stat.phy_id, phy_rx_stat.ch, phy_rx_stat.stat.rx_stat.data[0], data_cnt[phy_rx_stat.phy_id], sequence_error_cnt[phy_rx_stat.phy_id]);
          if(tput_context->args.frame_type == 1) {
            loop_cnt[phy_rx_stat.phy_id] = (phy_rx_stat.stat.rx_stat.data[0] - (phy_rx_stat.phy_id*phy_rx_stat.stat.rx_stat.nof_slots_in_frame + (phy_rx_stat.stat.rx_stat.slot_counter-1)))/(tput_context->args.nof_phys*tput_context->args.nof_slots_to_tx);
            cnt[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.slot_counter - 1;
          }
          data_cnt[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.data[0];
          printf("[Rx side] PHY Rx Id: %d - Now data holds: %d\n", phy_rx_stat.phy_id, data_cnt[phy_rx_stat.phy_id]);
        }
        if(cnt[phy_rx_stat.phy_id] >= tput_context->args.nof_slots_to_tx-1) {
          loop_cnt[phy_rx_stat.phy_id]++;
          data_cnt[phy_rx_stat.phy_id] = phy_rx_stat.phy_id*tput_context->args.nof_slots_to_tx + loop_cnt[phy_rx_stat.phy_id]*tput_context->args.nof_phys*tput_context->args.nof_slots_to_tx;
          cnt[phy_rx_stat.phy_id] = 0;
        } else {
          data_cnt[phy_rx_stat.phy_id]++;
          cnt[phy_rx_stat.phy_id]++;
        }
        if(data_cnt[phy_rx_stat.phy_id] >= tput_context->maximum_counter_value) {
          data_cnt[phy_rx_stat.phy_id] = phy_rx_stat.phy_id*tput_context->args.nof_slots_to_tx;
          loop_cnt[phy_rx_stat.phy_id] = 0;
        }
        printf("PHY Rx Id: %d - next expected data: %d\n", phy_rx_stat.phy_id, data_cnt[phy_rx_stat.phy_id]);
        // Store PRR for each one of the PHYs.
        prr[phy_rx_stat.phy_id] = (double)phy_rx_stat.num_cb_total/phy_rx_stat.stat.rx_stat.total_packets_synchronized;
        // Store errors for each one of the PHYs.
        errors[phy_rx_stat.phy_id] = (phy_rx_stat.stat.rx_stat.detection_errors+phy_rx_stat.stat.rx_stat.decoding_errors);
        // Store number of correctly decoded packets.
        if(nof_decoded[phy_rx_stat.phy_id] == 0 && phy_rx_stat.num_cb_total > 1) {
          last_nof_decoded_counter[phy_rx_stat.phy_id] = phy_rx_stat.num_cb_total - 1;
        }
        nof_decoded[phy_rx_stat.phy_id] = phy_rx_stat.num_cb_total - last_nof_decoded_counter[phy_rx_stat.phy_id];
        // Store number of synchronized packets, i.e., whatever has correlation peak higher than the threashold.
        if(nof_detected[phy_rx_stat.phy_id] == 0 && phy_rx_stat.stat.rx_stat.total_packets_synchronized > 1) {
          last_nof_detected_counter[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - 1;
        }
        nof_detected[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - last_nof_detected_counter[phy_rx_stat.phy_id];
#endif
      } else if(phy_rx_stat.status == PHY_ERROR) {
        // Count the number of wrong sequence detection happening with the last subframe.
        if(phy_rx_stat.stat.rx_stat.slot_counter == tput_context->args.nof_slots_to_tx) {
          last_tb_error_cnt[phy_rx_stat.phy_id]++;
        }
        // Store number of correctly decoded packets.
        if(nof_decoded[phy_rx_stat.phy_id] == 0 && phy_rx_stat.num_cb_total > 1) {
          last_nof_decoded_counter[phy_rx_stat.phy_id] = phy_rx_stat.num_cb_total - 1;
        }
        nof_decoded[phy_rx_stat.phy_id] = phy_rx_stat.num_cb_total - last_nof_decoded_counter[phy_rx_stat.phy_id];
        // Store number of synchronized packets, i.e., whatever has correlation peak higher than the threashold.
        if(nof_detected[phy_rx_stat.phy_id] == 0 && phy_rx_stat.stat.rx_stat.total_packets_synchronized > 1) {
          last_nof_detected_counter[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - 1;
        }
        nof_detected[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - last_nof_detected_counter[phy_rx_stat.phy_id];
        // Print error statistics.
        printf("[Rx side] PHY Rx Id: %d - # detection errors: %d - Channel: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - RSSI: %3.2f [dBm] - Decoded CFI: %d - Found DCI: %d - Last NOI: %d - Noise: %1.4f - # decoding errors: %d - # last tb errors: %d\n", phy_rx_stat.phy_id, phy_rx_stat.stat.rx_stat.detection_errors, phy_rx_stat.ch, phy_rx_stat.stat.rx_stat.cfo, phy_rx_stat.stat.rx_stat.peak_value, phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.decoded_cfi, phy_rx_stat.stat.rx_stat.found_dci, phy_rx_stat.stat.rx_stat.last_noi, phy_rx_stat.stat.rx_stat.noise, phy_rx_stat.stat.rx_stat.decoding_errors, last_tb_error_cnt[phy_rx_stat.phy_id]);
        error_cnt[phy_rx_stat.phy_id]++;
      }

    }

  }

  uint32_t total_correct_cnt = 0, total_error_cnt = 0;
  for(uint32_t i = 0; i < MAX_NOF_PHYS; i++) {
    total_correct_cnt += correct_cnt[i];
    total_error_cnt += error_cnt[i];
    printf("[Rx side] PHY Rx Id: %d - Total # of Errors: %d\n", i, sequence_error_cnt[i]);
  }

  printf("\n\n[Rx side] # Correct pkts: %d - # Error pkts: %d\n", total_correct_cnt, total_error_cnt);
  nof_errors = (errors[0]+errors[1]);
  printf("[Rx side] MCS: %d - PRR[0]: %1.2f - PRR[1]: %1.2f - # Decoded[0]: %d - # Decoded[1]: %d - # Detected[0]: %d - # Detected[1]: %d - # errors: %d\n", phy_rx_stat.mcs, prr[0], prr[1], nof_decoded[0], nof_decoded[1], nof_detected[0], nof_detected[1], nof_errors);

#if(ENABLE_DECODING_TIME_PROFILLING==1)
  for(uint32_t i = 0; i < tput_context->args.nof_phys; i++) {
    printf("[Rx side] PHY Rx Id: %d - # of Measurements: %d - PHY # PRB: %d - MCS: %d - Avg. Synch + Decoding time: %1.4f [ms] - Avg. Decoding time: %1.4f [ms] - Max. Decoding time (%d): %1.4f [ms]\n", i, correct_cnt[i], helpers_get_prb_from_bw_index(tput_context->args.phy_bw_idx), tput_context->args.mcs, synch_plus_decoding_time[i]/correct_cnt[i], decoding_time[i]/correct_cnt[i], max_decoding_time_cnt[i], max_decoding_time[i]);
  }
#endif

  tput_context->total_nof_detected_slots = (nof_detected[0] + nof_detected[1]);

  tput_context->total_nof_decoded_slots = (nof_decoded[0] + nof_decoded[1]);

  printf("[Rx side] Leaving Rx side.\n");
}

void *tx_side_work(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  basic_ctrl_t basic_ctrl;
  uint64_t timestamp0, timestamp1, time_now, time_advance;
  uint32_t phy_id = 0, data_pos = 0, data_cnt = 0, send_to = 0, intf_id = 0;
  int64_t packet_cnt = 0;
  int numOfBytes = 0;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  uchar *data = NULL;

  // Set priority to TX thread.
  uhd_set_thread_priority(1.0, true);

  // Make all values randomly repeatable.
  srand(12041988);

  tput_context->nof_transmitted_slot_counter = 0;

  // Calculate number of bytes per slot.
  if(tput_context->args.phy_bw_idx == BW_IDX_OneDotFour && tput_context->args.mcs > 28) {
    // Number of bytes for 1.4 MHz and MCS 28.
    numOfBytes = srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, 28);
    // Number of bytes for 1.4 MHz and MCS > 28.
    numOfBytes += (tput_context->args.nof_slots_to_tx-1)*srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs);
  } else {
    numOfBytes = tput_context->args.nof_slots_to_tx*srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs);
  }
  // Allocate memory for data slots.
  data = (uchar*)srslte_vec_malloc(numOfBytes);
  // Generate some data.
  generateData(numOfBytes, data);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl, PHY_TX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, tput_context->args.rf_boost, numOfBytes, data);

  // Set the time in advance to send messages to PHY.
  time_advance = 2000;

  // Loop used to send messages to both PHYs.
  while(tput_context->run_tx_side_thread && packet_cnt != tput_context->args.nof_packets_to_tx && !tput_context->go_exit) {

    // Retrieve time now.
    time_now = get_host_time_now_us();
    // First frame will be always sent some ms in advance.
    timestamp0 = time_now + time_advance;
    // First frame will be always sent some ms in advance.
    timestamp1 = time_now + time_advance;

    for(uint32_t phy_id = 0; phy_id < tput_context->args.nof_phys; phy_id++) {
      // Set PHY ID.
      basic_ctrl.phy_id = phy_id;
      // Set Interface ID.
      basic_ctrl.intf_id = phy_id;
      // Set channel.
      basic_ctrl.ch = phy_id;
      // Set timestamp.
      basic_ctrl.timestamp = phy_id==0 ? timestamp0:timestamp1;
      //printf("timestamp: %" PRIu64 "\n", basic_ctrl.timestamp);
      // Set different data values for different TBs.
      data_pos = 0;
      for(uint32_t tb_cnt = 0; tb_cnt < tput_context->args.nof_slots_to_tx; tb_cnt++) {
        // Set data.
        basic_ctrl.data[data_pos] = data_cnt;
        // Increment data position.
        if(tput_context->args.phy_bw_idx == BW_IDX_OneDotFour && tput_context->args.mcs > 28 && tb_cnt == 0) {
          data_pos += srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, 28);
        } else {
          data_pos += srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs);
        }
        // Increment counter.
        data_cnt = (data_cnt + 1) % tput_context->maximum_counter_value;
      }

      if(packet_cnt+1 == tput_context->args.nof_packets_to_tx) {
        basic_ctrl.data[3] = 255;
      } else {
        basic_ctrl.data[3] = 0;
      }

      // Send data to specific PHY.
      communicator_send_basic_control(tput_context->handle, &basic_ctrl);
      // Increment transmitted slot counter.
      tput_context->nof_transmitted_slot_counter += tput_context->args.nof_slots_to_tx;
    }

    // Wait for the duration of the frame.
    usleep(usecs+time_advance);

    // Increment packet counter.
    packet_cnt++;
  }

  // Free alocated data.
  if(data) {
    free(data);
    data = NULL;
  } else {
    printf("Error: data should not be NULL here!!!!\n");
  }

  printf("[Tx side] Leaving Tx side thread.\n", 0);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    tput_context->go_exit = true;
    printf("SIGINT received. Exiting...\n",0);
  }
}

void initialize_signal_handler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sig_int_handler);
}

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint32_t send_to, uint32_t intf_id, uint32_t phy_id, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, double rf_boost, uint32_t length, uchar *data) {
  basic_ctrl->trx_flag = trx_flag;
  basic_ctrl->phy_id = phy_id;
  basic_ctrl->seq_number = seq_num;
  basic_ctrl->bw_idx = bw_idx;
  basic_ctrl->send_to = send_to;
  basic_ctrl->intf_id = intf_id;
  basic_ctrl->ch = ch;
  basic_ctrl->timestamp = timestamp;
  basic_ctrl->mcs = mcs;
  basic_ctrl->gain = gain;
  basic_ctrl->rf_boost = rf_boost;
  basic_ctrl->length = length;
  if(data == NULL) {
    basic_ctrl->data = NULL;
  } else {
    basic_ctrl->data = data;
  }
}

void default_args(tput_test_args_t *args) {
  args->phy_bw_idx = BW_IDX_Five;
  args->mcs = 0;
  args->rx_channel = 0;
  args->rx_gain = 10;
  args->tx_channel = 0;
  args->tx_gain = 10;
  args->tx_side = true;
  args->nof_slots_to_tx = 1;
  args->interval = 10;
  args->nof_packets_to_tx = 10000;
  args->nof_phys = 2;
  args->frame_type = 1;
  args->rf_boost = 0.8;
  args->node_id = 0;
}

int start_tx_side_thread(tput_context_t *tput_context) {
  // Enable Rx side thread.
  tput_context->run_tx_side_thread = true;
  // Create thread attr and Id.
  pthread_attr_init(&tput_context->tx_side_thread_attr);
  pthread_attr_setdetachstate(&tput_context->tx_side_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&tput_context->tx_side_thread_id, &tput_context->tx_side_thread_attr, tx_side_work, (void *)tput_context);
  if(rc) {
    printf("[Tx side] Return code from Tx side pthread_create() is %d\n", rc);
    return -1;
  }
  return 0;
}

int stop_tx_side_thread(tput_context_t *tput_context) {
  tput_context->run_tx_side_thread = false; // Stop Tx side thread.
  pthread_attr_destroy(&tput_context->tx_side_thread_attr);
  int rc = pthread_join(tput_context->tx_side_thread_id, NULL);
  if(rc) {
    printf("[Tx side] Return code from Tx side pthread_join() is %d\n", rc);
    return -1;
  }
  return 0;
}

// This function returns timestamp with microseconds precision.
inline uint64_t get_host_time_now_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
}

inline double profiling_diff_time(struct timespec *timestart) {
  struct timespec timeend;
  clock_gettime(CLOCK_REALTIME, &timeend);
  return time_diff(timestart, &timeend);
}

inline double time_diff(struct timespec *start, struct timespec *stop) {
  struct timespec diff;
  if(stop->tv_nsec < start->tv_nsec) {
    diff.tv_sec = stop->tv_sec - start->tv_sec - 1;
    diff.tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  } else {
    diff.tv_sec = stop->tv_sec - start->tv_sec;
    diff.tv_nsec = stop->tv_nsec - start->tv_nsec;
  }
  return (double)(diff.tv_sec*1000) + (double)(diff.tv_nsec/1.0e6);
}

void parse_args(tput_test_args_t *args, int argc, char **argv) {
  int opt;
  default_args(args);
  while((opt = getopt(argc, argv, "bdgikmnoprstI0123456789")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      printf("[Input argument] Tx gain: %d\n", args->tx_gain);
      break;
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      printf("[Input argument] Rx gain: %d\n", args->rx_gain);
      break;
    case 'i':
      args->interval = atoi(argv[optind]);
      printf("[Input argument] Tput interval: %d\n", args->interval);
      break;
    case 'k':
      args->nof_packets_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of packets to transmit: %d\n", args->nof_packets_to_tx);
      break;
    case 'm':
      args->mcs = atoi(argv[optind]);
      printf("[Input argument] MCS: %d\n", args->mcs);
      break;
    case 'n':
      args->nof_slots_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of consecutive slots to be transmitted: %d\n", args->nof_slots_to_tx);
      break;
    case 'p':
      args->phy_bw_idx = helpers_get_bw_idx_from_freq_bw(atoi(argv[optind]));
      if(args->phy_bw_idx == 0) {
        printf("[Input argument] Invalid PHY BW: %d\n", atoi(argv[optind]));
        exit(-1);
      }
      printf("[Input argument] PHY Frequency BW: %d - Mapped index: %d - NPRB: %d\n", atoi(argv[optind]), args->phy_bw_idx, helpers_get_prb_from_bw_index(args->phy_bw_idx));
      break;
    case 'r':
      args->rx_channel = atoi(argv[optind]);
      printf("[Input argument] Rx channel: %d\n", args->rx_channel);
      break;
    case 's':
      args->tx_side = false;
      printf("[Input argument] Tx side: %d\n", args->tx_side);
      break;
    case 't':
      args->tx_channel = atoi(argv[optind]);
      printf("[Input argument] Tx channel: %d\n", args->tx_channel);
      break;
    case 'd':
      args->nof_phys = atoi(argv[optind]);
      printf("[Input argument] Number of PHYs: %d\n", args->nof_phys);
      break;
    case 'o':
      args->rf_boost = atof(argv[optind]);
      printf("[Input argument] RF boost: %f\n", args->rf_boost);
      break;
    case 'I':
      args->node_id = atoi(argv[optind]);
      if(args->node_id >= MAXIMUM_NUMBER_OF_RADIOS) {
        printf("Invalid Node ID: %d. It has to be less than %d.\n", args->node_id, MAXIMUM_NUMBER_OF_RADIOS);
        exit(-1);
      }
      printf("[Input argument] SRN ID: %d\n", args->node_id);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;
    default:
      printf("Error parsing arguments...\n");
      exit(-1);
    }
  }
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  printf("Creating %d data bytes\n",numOfBytes);
  for(int i = 0; i < numOfBytes; i++) {
    data[i] = (uchar)(rand() % 256);
  }
}

void change_process_priority(int inc) {
  errno = 0;
  if(nice(inc) == -1) {
    if(errno != 0) {
      printf("Something went wrong with nice(): %s - Perhaps you should run it as root.\n",strerror(errno));
    }
  }
  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);
}
