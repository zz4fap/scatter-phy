#include "sync_decode.h"

#include <stdio.h>
#include <stdlib.h>

static tput_context_t *tput_context;

int main(int argc, char *argv[]) {

  char source_module[] = "MODULE_MAC";
  char target_module[] = "MODULE_PHY";

  change_process_priority(-20);

  // Allocate memory for a new Tput context object.
  tput_context = (tput_context_t*)srslte_vec_malloc(sizeof(tput_context_t));
  // Check if memory allocation was correctly done.
  if(tput_context == NULL) {
    fprintf(stderr, "Error allocating memory for Tput context object.\n");
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

  // Start Tx thread.
  fprintf(stderr, "[Main] Starting Rx side thread...\n");
  start_rx_stats_thread(tput_context);

  // Run Tx side loop.
  fprintf(stderr, "[Main] Starting Tx side...\n");
  start_tx_side(tput_context);

  // Wait some time so that all packets are transmitted.
  fprintf(stderr, "[Main] Leaving the application in 2 seconds...\n");
  sleep(2);
  // Stop Rx side thread.
  fprintf(stderr, "[Main] Stopping Rx stats thread...\n");
  stop_rx_stats_thread(tput_context);

  // Free communicator related objects.
  communicator_free(&tput_context->handle);

  // Free memory used to store Tput context object.
  if(tput_context) {
    free(tput_context);
    tput_context = NULL;
  }

  return 0;
}

void start_tx_side(tput_context_t *tput_context) {

  basic_ctrl_t basic_ctrl;
  uint64_t timestamp;
  int64_t tx_packet_cnt = 0;
  unsigned int tb_size = get_tb_size((tput_context->args.phy_bw_idx-1), tput_context->args.mcs);
  int numOfBytes = tput_context->args.nof_tbs*tb_size;
  unsigned int usecs = 1000*tput_context->args.nof_tbs;
  unsigned int offset = tput_context->args.slot_duration - 1000*tput_context->args.nof_tbs;

  // Set priority to TX thread.
  uhd_set_thread_priority(1.0, true);

  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl, PHY_TX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, numOfBytes, data);

  while(!tput_context->go_exit && tx_packet_cnt != tput_context->args.nof_packets) {

    // Retrieve time now.
    timestamp = get_host_time_now_us();

    // Send data to PHY.
    basic_ctrl.timestamp = timestamp + (uint64_t)offset; // Set timestamp
    communicator_send_basic_control(tput_context->handle, &basic_ctrl);

    // Wait for the duration of the subframe.
    usleep(usecs+offset);

    tx_packet_cnt++;
  }

  fprintf(stderr, "[Tx side] Leaving Tx side thread.\n",0);

}

void *rx_side_work(void *h) {

  tput_context_t *tput_context = (tput_context_t*)h;
  basic_ctrl_t basic_ctrl;

  phy_stat_t phy_rx_stat;
  uchar data[10000];

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Create basic control message to control Rx chain.
  createBasicControl(&basic_ctrl, PHY_RX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.rx_channel, 0, 0, tput_context->args.rx_gain, 1, NULL);
  basic_ctrl.timestamp = get_host_time_now_us();
  communicator_send_basic_control(tput_context->handle, &basic_ctrl);

//  sleep(1);

  // Loop until otherwise said.
  fprintf(stderr, "[Rx stats side] Starting Rx side loop...\n");
  while(tput_context->run_rx_stats_thread) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

  }
  printf("%u/%u\n", phy_rx_stat.stat.rx_stat.total_packets_synchronized, phy_rx_stat.num_cb_total);

  // Exit thread with result code.
  pthread_exit(NULL);
}

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    tput_context->go_exit = true;
    fprintf(stderr, "SIGINT received. Exiting...\n",0);
  }
}

void initialize_signal_handler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sig_int_handler);
}

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data) {
  basic_ctrl->trx_flag = trx_flag;
  basic_ctrl->seq_number = seq_num;
  basic_ctrl->bw_idx = bw_idx;
  basic_ctrl->ch = ch;
  basic_ctrl->timestamp = timestamp;
  basic_ctrl->mcs = mcs;
  basic_ctrl->gain = gain;
  basic_ctrl->length = length;
  if(data == NULL) {
    basic_ctrl->data = NULL;
  } else {
    basic_ctrl->data = data;
  }
}

void default_args(tput_test_args_t *args) {
  args->phy_bw_idx = BW_IDX_Three;
  args->mcs = 0;
  args->rx_channel = 2;
  args->rx_gain = 7;
  args->tx_channel = 0;
  args->tx_gain = 24;
  args->slot_duration = 30000;
  args->nof_tbs = 23;
  args->nof_packets = 100;
}

int start_rx_stats_thread(tput_context_t *tput_context) {
  // Create thread attr and Id.
  pthread_attr_init(&tput_context->rx_stats_thread_attr);
  pthread_attr_setdetachstate(&tput_context->tx_side_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&tput_context->rx_stats_thread_id, &tput_context->rx_stats_thread_attr, rx_side_work, (void *)tput_context);
  if(rc) {
    fprintf(stderr, "[Rx side] Return code from Rx side pthread_create() is %d\n", rc);
    return -1;
  }

  // Enable Rx side thread.
  tput_context->run_rx_stats_thread = true;
  return 0;
}

int stop_rx_stats_thread(tput_context_t *tput_context) {
  tput_context->run_rx_stats_thread = false; // Stop Rx side thread.
  pthread_attr_destroy(&tput_context->tx_side_thread_attr);
  return 0;
}

// This function returns timestamp with microseconds precision.
inline uint64_t get_host_time_now_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
}

inline uint32_t get_bw_index_from_bw(uint32_t bw) {
  uint32_t bw_idx;
  switch(bw) {
    case 1400000:
      bw_idx = BW_IDX_OneDotFour;
      break;
    case 3000000:
      bw_idx = BW_IDX_Three;
      break;
    case 5000000:
      bw_idx = BW_IDX_Five;
      break;
    case 10000000:
      bw_idx = BW_IDX_Ten;
      break;
    case 15000000:
      bw_idx = BW_IDX_Fifteen;
      break;
    case 20000000:
      bw_idx = BW_IDX_Twenty;
      break;
    default:
      bw_idx = 0;
  }
  return bw_idx;
}

void parse_args(tput_test_args_t *args, int argc, char **argv) {
  int opt;
  default_args(args);
  while((opt = getopt(argc, argv, "bgkmdnprt")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Tx gain: %d\n", args->tx_gain);
      break;
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Rx gain: %d\n", args->rx_gain);
      break;
    case 'k':
      args->nof_packets = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Number of packets to transmit: %d\n", args->nof_packets);
      break;
    case 'm':
      args->mcs = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] MCS: %d\n", args->mcs);
      break;
    case 'd':
      args->slot_duration = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Slot duration in usec: %d\n", args->nof_tbs);
      break;
    case 'n':
      args->nof_tbs = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Number of consecutive slots to be transmitted: %d\n", args->nof_tbs);
      break;
    case 'p':
      args->phy_bw_idx = get_bw_index_from_bw(atoi(argv[optind]));
      fprintf(stderr, "[Input argument] PHY BW: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
      break;
    case 'r':
      args->rx_channel = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Rx channel: %d\n", args->rx_channel);
      break;
    case 't':
      args->tx_channel = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Tx channel: %d\n", args->tx_channel);
      break;
    default:
      fprintf(stderr, "Error parsing arguments...\n");
      exit(-1);
    }
  }
}

const int mcs_tbs_idx_table[29] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 11, 12, 13, 14, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
unsigned int get_tb_size(uint32_t bw_idx, uint32_t mcs) {
  return srslte_ra_tbs_from_idx(mcs_tbs_idx_table[mcs], helpers_get_prb_from_bw_index(bw_idx + 1)) / 8;
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  fprintf(stderr, "Creating %d data bytes\n",numOfBytes);
  for(int i = 0; i < numOfBytes; i++) {
    data[i] = (uchar)(rand() % 256);
  }
}

void change_process_priority(int inc) {
  errno = 0;
  if(nice(inc) == -1) {
    if(errno != 0) {
      fprintf(stderr, "Something went wrong with nice(): %s - Perhaps you should run it as root.\n",strerror(errno));
    }
  }
  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);
}

