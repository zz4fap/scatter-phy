#include "sync_decode.h"

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

  // Run Rx side loop.
  start_rx_side(tput_context);

  // Free communicator related objects.
  communicator_free(&tput_context->handle);

  // Free memory used to store Tput context object.
  if(tput_context) {
    free(tput_context);
    tput_context = NULL;
  }

  return 0;
}

void start_rx_side(tput_context_t *tput_context) {

  basic_ctrl_t basic_ctrl;

  phy_stat_t phy_rx_stat;
  uchar data[10000];

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Create basic control message to control Rx chain.
  createBasicControl(&basic_ctrl, PHY_RX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.rx_channel, 0, 0, tput_context->args.rx_gain, 1, NULL);
  basic_ctrl.timestamp = get_host_time_now_us();
  communicator_send_basic_control(tput_context->handle, &basic_ctrl);

  uint64_t end_time = basic_ctrl.timestamp + tput_context->args.nof_packets * tput_context->args.slot_duration + 2000000;
  while(!tput_context->go_exit && get_host_time_now_us() < end_time) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

  }
  printf("%u/%u\n", phy_rx_stat.stat.rx_stat.total_packets_synchronized, phy_rx_stat.num_cb_total);

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
  args->rx_channel = 0;
  args->rx_gain = 7;
  args->nof_packets = 100;
  args->slot_duration = 30000;
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
  while((opt = getopt(argc, argv, "gkdpc")) != -1) {
    switch(opt) {
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Rx gain: %d\n", args->rx_gain);
      break;
    case 'k':
      args->nof_packets = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Number of packets to receive: %d\n", args->nof_packets);
      break;
    case 'd':
      args->slot_duration = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Slot duration in usec: %d\n", args->nof_tbs);
      break;
    case 'p':
      args->phy_bw_idx = get_bw_index_from_bw(atoi(argv[optind]));
      fprintf(stderr, "[Input argument] PHY BW: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
      break;
    case 'c':
      args->rx_channel = atoi(argv[optind]);
      fprintf(stderr, "[Input argument] Rx channel: %d\n", args->rx_channel);
      break;
    default:
      fprintf(stderr, "Error parsing arguments...\n");
      exit(-1);
    }
  }
}

void change_process_priority(int inc) {
  errno = 0;
  if(nice(inc) == -1) {
    if(errno != 0) {
      fprintf(stderr, "Something went wrong with nice(): %s - Perhaps you should run it as root.\n",strerror(errno));
    }
  }
}

