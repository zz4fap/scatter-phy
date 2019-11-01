#ifndef _MEASURE_THROUGHPUT_FD_H_
#define _MEASURE_THROUGHPUT_FD_H_

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#include "helpers.h"
#include "srslte/intf/intf.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define MAX_TX_SLOTS 10000

typedef struct {
  uint32_t phy_bw_idx;
  uint32_t mcs;
  uint32_t rx_channel;
  uint32_t rx_gain;
  uint32_t tx_channel;
  uint32_t tx_gain;
  uint32_t nof_slots_to_tx;
  uint32_t interval;
  int64_t nof_packets_to_tx;
  bool tx_side;
} tput_test_args_t;

typedef struct {
  LayerCommunicator_handle handle;
  tput_test_args_t args;
  bool run_rx_side_thread;
  pthread_attr_t rx_side_thread_attr;
  pthread_t rx_side_thread_id;
} tput_context_t;

void sig_int_handler(int signo);

void initialize_signal_handler();

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint32_t phy_id, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data);

void default_args(tput_test_args_t *args);

int start_rx_side_thread(tput_context_t *tput_context);

int stop_rx_side_thread(tput_context_t *tput_context);

inline uint64_t get_host_time_now_us();

inline double profiling_diff_time(struct timespec *timestart);

inline double time_diff(struct timespec *start, struct timespec *stop);

void parse_args(tput_test_args_t *args, int argc, char **argv);

void *rx_side(void *h);

void tx_side(tput_context_t *tput_context);

void generateData(uint32_t numOfBytes, uchar *data);

#endif //_MEASURE_THROUGHPUT_FD_H_
