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

typedef struct {
  uint32_t phy_bw_idx;
  uint32_t mcs;
  uint32_t rx_channel;
  uint32_t rx_gain;
  uint32_t tx_channel;
  uint32_t tx_gain;
  uint32_t slot_duration;
  uint32_t nof_tbs;
  int64_t nof_packets;
} tput_test_args_t;

typedef struct {
  bool go_exit;
  LayerCommunicator_handle handle;
  tput_test_args_t args;
  bool run_tx_side_thread;
  pthread_attr_t tx_side_thread_attr;
  pthread_t tx_side_thread_id;
  bool run_rx_stats_thread;
  pthread_attr_t rx_stats_thread_attr;
  pthread_t rx_stats_thread_id;
} tput_context_t;

void sig_int_handler(int signo);

void initialize_signal_handler();

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data);

void default_args(tput_test_args_t *args);

inline uint64_t get_host_time_now_us();

unsigned int get_tb_size(uint32_t prb, uint32_t mcs);

void parse_args(tput_test_args_t *args, int argc, char **argv);

void rx_side(tput_context_t *tput_context);

int stop_rx_stats_thread(tput_context_t *tput_context);

int start_rx_stats_thread(tput_context_t *tput_context);

void start_rx_side(tput_context_t *tput_context);

void start_tx_side(tput_context_t *tput_context);

void generateData(uint32_t numOfBytes, uchar *data);

void change_process_priority(int inc);

#endif //_MEASURE_THROUGHPUT_FD_H_
