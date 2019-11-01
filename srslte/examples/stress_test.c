#include "measure_throughput_fd_auto.h"

#include <stdio.h>
#include <stdlib.h>

#define NORMAL_TEST 1

static tput_context_t *tput_context;

void *tx_rx_side_work(void *h);
void *rx_stats_work(void *h);
int start_rx_stats_thread(tput_context_t *tput_context);
int stop_rx_stats_thread(tput_context_t *tput_context);
void rx_basic_control_side(tput_context_t *tput_context);

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

  tput_context->go_exit = false;

  // Initialize signal handler.
  initialize_signal_handler();

  // Retrieve input arguments.
  parse_args(&tput_context->args, argc, argv);

  // Create communicator.
  communicator_make(source_module, target_module, NULL, &tput_context->handle);

  // Wait some time before starting anything.
  sleep(2);

#if(NORMAL_TEST==1)
  // Start Rx thread.
  printf("[Main] Starting Tx side thread...\n");
  start_tx_side_thread(tput_context);

  // Start Rx stats thread.
  printf("[Main] Starting Rx stats thread...\n");
  start_rx_stats_thread(tput_context);

  // Run Rx side loop.
  printf("[Main] Starting Rx side...\n");
  rx_basic_control_side(tput_context);
#else
  // Run tests with Rx and Tx configuration.
  printf("[Main] Starting Tx/Rx side thread...\n");
  // Enable Rx side thread.
  tput_context->run_tx_side_thread = true;
  tx_rx_side_work(tput_context);
#endif

#if(NORMAL_TEST==1)
  // Wait some time so that all packets are transmitted.
  printf("[Main] Leaving the application in 2 seconds...\n");
  sleep(2);
  // Stop Rx side thread.
  printf("[Main] Stopping Tx side thread...\n");
  stop_tx_side_thread(tput_context);

  // Stop Rx stats thread.
  printf("[Main] Stopping Rx stats thread...\n");
  stop_rx_stats_thread(tput_context);
#endif

  // Free communicator related objects.
  communicator_free(&tput_context->handle);

  // Free memory used to store Tput context object.
  if(tput_context) {
    free(tput_context);
    tput_context = NULL;
  }

  return 0;
}

void rx_basic_control_side(tput_context_t *tput_context) {
  basic_ctrl_t basic_ctrl;
  int64_t packet_cnt = 0;
  uint32_t send_to = 0, intf_id = 0;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  unsigned int offset0 = 2000; // given in ms.
  unsigned int offset1 = 3000; // given in ms.

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  // Set timestamp.
  uint64_t timestamp = get_host_time_now_us() + 2000;

  // Create basic control message to control Rx chain.
  for(uint32_t phy_id = 0; phy_id < 2; phy_id++) {
    createBasicControl(&basic_ctrl, PHY_RX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, (tput_context->args.rx_channel+phy_id), timestamp, 0, tput_context->args.rx_gain, tput_context->args.rf_boost, 1, NULL);
  }

  sleep(1);

  // Loop until otherwise said.
  printf("[Rx side] Starting Rx side thread loop...\n");
  while(!tput_context->go_exit && packet_cnt != tput_context->args.nof_packets_to_tx) {

    // Retrieve time now.
    timestamp = get_host_time_now_us();

    // Send RX control to PHY.
    for(uint32_t phy_id = 0; phy_id < 2; phy_id++) {
      basic_ctrl.phy_id = phy_id; //phy_id==0?1:0;
      basic_ctrl.ch = rand() % 10;
      // Set timestamp.
      basic_ctrl.timestamp = timestamp;
      basic_ctrl.timestamp += basic_ctrl.phy_id==0?(uint64_t)offset0:(uint64_t)offset1;
      communicator_send_basic_control(tput_context->handle, &basic_ctrl);
    }

    // Wait for the duration of the frame.
    usleep(usecs+offset1);

    packet_cnt++;
  }
}

void *tx_basic_control_side(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  basic_ctrl_t basic_ctrl;
  uint64_t timestamp;
  int64_t packet_cnt = 0;
  uint32_t send_to = 0, intf_id = 0;
  unsigned int tb_size = srslte_ra_get_tb_size_scatter((tput_context->args.phy_bw_idx-1), tput_context->args.mcs);
  int numOfBytes = tput_context->args.nof_slots_to_tx*tb_size;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  unsigned int offset0 = 2000; // given in ms.
  unsigned int offset1 = 3000; // given in ms.
  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  // Create basic control message to control Tx chain.
  for(uint32_t phy_id = 0; phy_id < 2; phy_id++) {
    createBasicControl(&basic_ctrl, PHY_TX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, tput_context->args.rf_boost, numOfBytes, data);
  }

  while(tput_context->run_tx_side_thread && packet_cnt != tput_context->args.nof_packets_to_tx) {

    // Retrieve time now.
    timestamp = get_host_time_now_us();

    // Send data to specific PHY.
    for(uint32_t phy_id = 0; phy_id < 2; phy_id++) {
      basic_ctrl.phy_id = phy_id; //phy_id==0?1:0;
      basic_ctrl.ch = rand() % 10;
      basic_ctrl.gain = rand() % 31;
      // Set timestamp.
      basic_ctrl.timestamp = timestamp;
      basic_ctrl.timestamp += basic_ctrl.phy_id==0?(uint64_t)offset0:(uint64_t)offset1;
      communicator_send_basic_control(tput_context->handle, &basic_ctrl);
    }

    //printf("timestamp: %" PRIu64 " - timestamp + Xms: %" PRIu64 "\n", timestamp, basic_ctrl.timestamp);

    // Wait for the duration of the frame.
    usleep(usecs+offset1);

    packet_cnt++;

    if(packet_cnt%1000 == 0) {
      printf("Number of transmitted packets: %d\n", packet_cnt);
    }
  }

  printf("[Tx side] Leaving Tx side thread.\n",0);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void *tx_rx_side_work(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  basic_ctrl_t basic_ctrl_tx, basic_ctrl_rx;
  uint64_t timestamp = get_host_time_now_us();
  int64_t packet_cnt = 0;
  unsigned int tb_size = srslte_ra_get_tb_size_scatter((tput_context->args.phy_bw_idx-1), tput_context->args.mcs);
  int numOfBytes = tput_context->args.nof_slots_to_tx*tb_size;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  unsigned int offset = 1000; // given in ms.
  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);
  uint32_t phy_id = 0, send_to = 0, intf_id = 0;

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl_tx, PHY_TX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, tput_context->args.rf_boost, numOfBytes, data);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl_rx, PHY_RX_ST, send_to, intf_id, phy_id, 66, tput_context->args.phy_bw_idx, (tput_context->args.rx_channel+phy_id), timestamp, 0, tput_context->args.rx_gain, tput_context->args.rf_boost, 1, NULL);

  while(!tput_context->go_exit && packet_cnt != tput_context->args.nof_packets_to_tx) {

    // Retrieve time now.
    timestamp = get_host_time_now_us();

    // Send data to specific PHY.
    basic_ctrl_tx.ch = rand() % 10;
    basic_ctrl_tx.gain = rand() % 31;
    basic_ctrl_tx.timestamp = timestamp + (uint64_t)offset; // Set timestamp
    communicator_send_basic_control(tput_context->handle, &basic_ctrl_tx);

    // Send RX control to PHY.
    basic_ctrl_rx.ch = rand() % 10;
    basic_ctrl_rx.timestamp = timestamp + (uint64_t)offset; // Set timestamp
    communicator_send_basic_control(tput_context->handle, &basic_ctrl_rx);

    //printf("timestamp: %" PRIu64 " - timestamp + Xms: %" PRIu64 "\n", timestamp, basic_ctrl.timestamp);

    // Wait for the duration of the frame.
    usleep(usecs+offset);

    packet_cnt++;
  }

  printf("[Tx/Rx side] Leaving Tx/Rx side thread.\n",0);
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
  args->change_bw = true;
}

int start_tx_side_thread(tput_context_t *tput_context) {
  // Enable Rx side thread.
  tput_context->run_tx_side_thread = true;
  // Create thread attr and Id.
  pthread_attr_init(&tput_context->tx_side_thread_attr);
  pthread_attr_setdetachstate(&tput_context->tx_side_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&tput_context->tx_side_thread_id, &tput_context->tx_side_thread_attr, tx_basic_control_side, (void *)tput_context);
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
  while((opt = getopt(argc, argv, "bgikmnprst0123456789")) != -1) {
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

void *rx_stats_work(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  phy_stat_t phy_rx_stat;
  uchar data[10000];
  bool ret = true;
  uint32_t errors = 0, pkt_cnt = 0;
  float prr = 0.0, rssi = 0.0;

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Loop until otherwise said.
  printf("[Rx stats side] Starting Rx stats side thread loop...\n");
  while(!tput_context->go_exit && tput_context->run_rx_stats_thread) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(!tput_context->go_exit && ret && tput_context->run_rx_stats_thread) {

      if(pkt_cnt % 500 == 0) {
        // Store errors for each one of the PHYs.
        errors = (phy_rx_stat.stat.rx_stat.detection_errors+phy_rx_stat.stat.rx_stat.decoding_errors);
        // Store PRR for each one of the PHYs.
        prr = phy_rx_stat.num_cb_total/phy_rx_stat.stat.rx_stat.total_packets_synchronized;
        // Store RSSI for each one of the PHYs.
        rssi = phy_rx_stat.stat.rx_stat.rssi;
        printf("[Rx side] MCS: %d - RSSI: %1.2f - PRR: %1.2f - # errors: %d\n", phy_rx_stat.mcs, rssi, prr, errors);
      }
      // Increment packet counter.
      pkt_cnt++;

    }
  }

  printf("[Rx stats side] Leaving Rx stats thread.\n",0);
  // Exit thread with result code.
  pthread_exit(NULL);
}

int start_rx_stats_thread(tput_context_t *tput_context) {
  // Enable Rx stats thread.
  tput_context->run_rx_stats_thread = true;
  // Create thread attr and Id.
  pthread_attr_init(&tput_context->rx_stats_thread_attr);
  pthread_attr_setdetachstate(&tput_context->rx_stats_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&tput_context->rx_stats_thread_id, &tput_context->rx_stats_thread_attr, rx_stats_work, (void *)tput_context);
  if(rc) {
    printf("[Rx stats] Return code from Rx stats pthread_create() is %d\n", rc);
    return -1;
  }
  return 0;
}

int stop_rx_stats_thread(tput_context_t *tput_context) {
  // Stop Rx stats thread.
  tput_context->run_rx_stats_thread = false;
  pthread_attr_destroy(&tput_context->rx_stats_thread_attr);
  int rc = pthread_join(tput_context->rx_stats_thread_id, NULL);
  if(rc) {
    printf("[Rx stats] Return code from Rx stats pthread_join() is %d\n", rc);
    return -1;
  }
  return 0;
}
