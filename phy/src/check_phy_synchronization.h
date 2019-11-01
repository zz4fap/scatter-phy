#ifndef _CHECK_PHY_SYNCHRONIZATION_H_
#define _CHECK_PHY_SYNCHRONIZATION_H_

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

#define MAX_NOF_PHYS 2

#define MAX_TX_SLOTS 100000

#define CHECK_RX_SEQUENCE 1

#define DISABLE_TX_TIMESTAMP 1

#define MAX_DATA_COUNTER_VALUE 256

#define FREQ_BOOST_AMP 1.0/2.0 // 1.0/120.0 (value for channel emulator)

#define ENABLE_DECODING_TIME_PROFILLING 1

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
  uint32_t nof_phys;
  uint32_t frame_type;
  double rf_boost;
  uint32_t node_id;
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
  uint32_t maximum_counter_value;
  uint32_t nof_transmitted_slot_counter;
  channel_emulator_config_t chan_emu_conf;
  bool run_rx_side;
  uint32_t total_nof_detected_slots;
  uint32_t total_nof_decoded_slots;
} tput_context_t;

void sig_int_handler(int signo);

void initialize_signal_handler();

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint32_t send_to, uint32_t intf_id, uint32_t phy_id, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, double rf_boost, uint32_t length, uchar *data);

void default_args(tput_test_args_t *args);

int start_tx_side_thread(tput_context_t *tput_context);

int stop_tx_side_thread(tput_context_t *tput_context);

inline uint64_t get_host_time_now_us();

inline double profiling_diff_time(struct timespec *timestart);

inline double time_diff(struct timespec *start, struct timespec *stop);

void parse_args(tput_test_args_t *args, int argc, char **argv);

void rx_side(tput_context_t *tput_context);

void *tx_side_work(void *h);

void generateData(uint32_t numOfBytes, uchar *data);

void change_process_priority(int inc);

#endif //_CHECK_PHY_SYNCHRONIZATION_H_
