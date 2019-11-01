#ifndef _MEASURE_THROUGHPUT_H_
#define _MEASURE_THROUGHPUT_H_

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
  bool tx_side;
} tput_test_args_t;

typedef struct {
  LayerCommunicator_handle handle;
  tput_test_args_t args;
} tput_context_t;

void default_args(tput_test_args_t *args);

inline uint64_t get_host_time_now_us();

inline double profiling_diff_time(struct timespec *timestart);

inline double time_diff(struct timespec *start, struct timespec *stop);

void parse_args(tput_test_args_t *args, int argc, char **argv);

void rx_side(tput_context_t *tput_context);

void tx_side(tput_context_t *tput_context);

void generateData(uint32_t numOfBytes, uchar *data);

#endif //MEASURE_THROUGHPUT_H_
