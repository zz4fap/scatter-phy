#include "measure_throughput_fd_auto.h"

static tput_context_t *tput_context;

int main(int argc, char *argv[]) {

  char source_module[] = "MODULE_MAC";
  char target_module[] = "MODULE_PHY";

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

  // Create communicator.
  communicator_make(source_module, target_module, NULL, &tput_context->handle);

  // Wait some time before starting anything.
  sleep(2);

  // Start Tx thread.
  printf("[Main] Starting Tx side thread...\n");
  start_tx_side_thread(tput_context);

  // Run Rx side loop.
  printf("[Main] Starting Rx side...\n");
  rx_side(tput_context);

  // Wait some time so that all packets are transmitted.
  printf("[Main] Leaving the application in 2 seconds...\n");
  sleep(2);

  // Stop Tx side thread.
  printf("[Main] Stopping Tx side thread...\n");
  stop_tx_side_thread(tput_context);

  printf("[Main] Number of transmitted slots: %d\n", tput_context->nof_transmitted_slot_counter);

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
  bool ret, is_1st_packet = true, phy_stats[2] = {false, false};
  uint64_t nof_rec_bytes = 0, tput_avg_cnt = 0;
  uint32_t errors[2] = {0, 0}, nof_errors = 0, pkt_cnt = 0, nof_decoded[2] = {0, 0}, nof_detected[2] = {0, 0};
  double prr[2] = {0.0, 0.0}, rssi[2] = {0.0, 0.0};
  struct timespec time_1st_packet;
  double time_diff = 0.0;
  double tput = 0.0, tput_avg = 0.0;
  double min_tput = DBL_MAX, max_tput = 0.0;
  double tput_interval = ((double)tput_context->args.interval)*1000.0;
  uint32_t send_to = 0, intf_id = 0;

  // Open log file.
  char tput_file_name[200];
  sprintf(tput_file_name,"tput_prb_%d_mcs_%d_slots_%d.txt",helpers_get_prb_from_bw_index(tput_context->args.phy_bw_idx),tput_context->args.mcs,tput_context->args.nof_slots_to_tx);
  FILE *f = fopen(tput_file_name, "w");

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  // Set timestamp.
  uint64_t timestamp = get_host_time_now_us() + 2000;

  clock_gettime(CLOCK_REALTIME, &time_1st_packet);

  // Configure Rx of the radios.
  for(uint32_t phy_id = 0; phy_id < tput_context->args.nof_phys; phy_id++) {
    // Create basic control message to control Tx chain.
    createBasicControl(&basic_ctrl, PHY_RX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, (tput_context->args.rx_channel+phy_id), timestamp, 0, tput_context->args.rx_gain, tput_context->args.rf_boost, 1, NULL);
    // Send RX control to PHY.
    communicator_send_basic_control(tput_context->handle, &basic_ctrl);
  }

  // Loop until otherwise said.
  printf("[Rx side] Starting Rx side thread loop...\n");
  while(!tput_context->go_exit && tput_avg_cnt < 6) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(!tput_context->go_exit && ret) {

      if(phy_rx_stat.status == PHY_SUCCESS) {

        if(phy_rx_stat.seq_number == 66) {

          nof_rec_bytes += phy_rx_stat.stat.rx_stat.length;

          if(is_1st_packet) {
            is_1st_packet = false;
            clock_gettime(CLOCK_REALTIME, &time_1st_packet);
          } else {
            time_diff = profiling_diff_time(&time_1st_packet);

            if(time_diff >= tput_interval) {

              tput = (nof_rec_bytes*8)/(time_diff/1000.0);

              tput_avg += tput;
              tput_avg_cnt++;

              is_1st_packet = true;
              nof_rec_bytes = 0;

              if(tput < min_tput) {
                min_tput = tput;
              }

              if(tput > max_tput) {
                max_tput = tput;
              }

              nof_errors = (errors[0]+errors[1]);
              printf("[Rx side] MCS: %d - RSSI[0]: %1.2f - RSSI[1]: %1.2f - PRR[0]: %1.2f - PRR[1]: %1.2f - # Decoded[0]: %d - # Decoded[1]: %d - # Detected[0]: %d - # Detected[1]: %d - # errors: %d - Local tput: %f [bps]\n", phy_rx_stat.mcs, rssi[0], rssi[1], prr[0], prr[1], nof_decoded[0], nof_decoded[1], nof_detected[0], nof_detected[1], nof_errors, tput);

              fprintf(f, "%f\t%f\t%f\t%d\t%f\t%f\n", min_tput, max_tput, tput, nof_errors, prr[0], prr[1]);
            }
          }

          if(pkt_cnt >= 500) {
            // Store errors for each one of the PHYs.
            errors[phy_rx_stat.phy_id] = (phy_rx_stat.stat.rx_stat.detection_errors+phy_rx_stat.stat.rx_stat.decoding_errors);
            // Store PRR for each one of the PHYs.
            prr[phy_rx_stat.phy_id] = (double)phy_rx_stat.num_cb_total/phy_rx_stat.stat.rx_stat.total_packets_synchronized;
            // Store number of correctly decoded packets.
            nof_decoded[phy_rx_stat.phy_id] = phy_rx_stat.num_cb_total;
            // Store number of synchronized packets, i.e., whatever has correlation peak higher than the threashold.
            nof_detected[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized;
            // Store RSSI for each one of the PHYs.
            rssi[phy_rx_stat.phy_id] = phy_rx_stat.stat.rx_stat.rssi;
            // Set PHY stats flag.
            phy_stats[phy_rx_stat.phy_id] = true;
            // Reset flags and counters.
            if(phy_stats[0] && phy_stats[1]) {
              // Reset both phy stats flags.
              phy_stats[0] = false;
              phy_stats[1] = false;
              // Reset packet counter.
              pkt_cnt = 0;
            }
          } else {
            // Increment packet counter.
            pkt_cnt++;
          }
        }
      }
    }
  }
  printf("[Rx side] Min tput: %f - Max tput: %f - Average tput: %f [bps]\n",min_tput,max_tput,(tput_avg/tput_avg_cnt));

  fprintf(f, "%f\t%f\t%f\t%d\t%f\t%f\n", min_tput, max_tput, (tput_avg/tput_avg_cnt), nof_errors, prr[0], prr[1]);

  // Close log file.
  fclose(f);
}

void *tx_side_work(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  basic_ctrl_t basic_ctrl;
  uint64_t timestamp0, timestamp1, time_now, time_advance;
  uint32_t phy_id = 0, send_to = 0, intf_id = 0;
  int64_t packet_cnt = 0;
  int numOfBytes = 0;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  // Create some data.
  uchar *data = NULL;

  sleep(2);

  // Calculate number of bytes per slot.
  if(tput_context->args.phy_bw_idx == BW_IDX_OneDotFour && tput_context->args.mcs == 28) {
    // Number of bytes for 1.4 MHz and MCS 27.
    numOfBytes = srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs-1);
    // Number of bytes for 1.4 MHz and MCS 28.
    numOfBytes += (tput_context->args.nof_slots_to_tx-1)*srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs);
  } else {
    numOfBytes = tput_context->args.nof_slots_to_tx*srslte_ra_get_tb_size_scatter(tput_context->args.phy_bw_idx-1, tput_context->args.mcs);
  }
  // Allocate memory for data slots.
  data = (uchar*)srslte_vec_malloc(numOfBytes);
  // Generate some data.
  generateData(numOfBytes, data);

  // Set priority to TX thread.
  uhd_set_thread_priority(1.0, true);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl, PHY_TX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, tput_context->args.rf_boost, numOfBytes, data);

  // Set the time in advance to send messages to PHY.
  time_advance = 2000;

  while(tput_context->run_tx_side_thread && packet_cnt != tput_context->args.nof_packets_to_tx) {

    // Retrieve time now.
    time_now = get_host_time_now_us();
    // First frame will be always sent some ms in advance.
    timestamp0 = time_now + time_advance;
    // First frame will be always sent some ms in advance.
    timestamp1 = time_now + time_advance;

    // Send data to specific PHY.
    for(uint32_t phy_id = 0; phy_id < tput_context->args.nof_phys; phy_id++) {
      // Set PHY ID.
      basic_ctrl.phy_id = phy_id;
      // Set Interface ID.
      basic_ctrl.intf_id = phy_id;
      // Set channel.
      basic_ctrl.ch = phy_id;
      // Set timestamp.
      basic_ctrl.timestamp = phy_id==0 ? timestamp0:timestamp1;
      // Send Tx basic control to PHY.
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

  printf("[Tx side] Leaving Tx side thread.\n",0);
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
  args->nof_packets_to_tx = -1;
  args->nof_phys = 2;
  args->frame_type = 1;
  args->rf_boost = 0.8;//1.0/120.0; // 0.8;
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
      args->phy_bw_idx = helpers_get_bw_index_from_prb(atoi(argv[optind]));
      printf("[Input argument] PHY BW in PRB: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
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
  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);
}
