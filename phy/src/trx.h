#ifndef _TRX_H_
#define _TRX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <float.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#include "../../../../communicator/cpp/communicator_wrapper.h"
#include "helpers.h"
#include "transceiver.h"
#include "phy_reception.h"
#include "phy_transmission.h"

#ifndef DISABLE_RF
#include "srslte/rf/rf.h"
#include "srslte/rf/rf_utils.h"
#else
#error Compiling PHY with no RF support. Add RF support.
#endif

#define ENABLE_SENSING_THREAD 1

// Set to 1 the macro below to enable the adjustment of FPGA time from time to time.
#define ADJUST_FPGA_TIME 1

#define PHY_DEBUG_1 1
#define PHY_DEBUG_2 2

#define ENABLE_TRX_PRINTS 1

#define ENABLE_CH_EMULATOR_IMPAIRMENTS 0

#define TRX_PRINT(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[TRX PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define TRX_DEBUG(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[TRX DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define TRX_INFO(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[TRX INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define TRX_ERROR(_fmt, ...) do { fprintf(stdout, "[TRX ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define TRX_PRINT_TIME(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[TRX PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define TRX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[TRX DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define TRX_INFO_TIME(_fmt, ...) do { if(ENABLE_TRX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[TRX INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define TRX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[TRX ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// ********************* Declaration of types *********************
typedef struct {
  srslte_rf_t rf;
  transceiver_args_t prog_args;
  bool go_exit;
  timer_t *rx_timer_ids[MAX_NUM_CONCURRENT_PHYS];
  timer_t *tx_timer_ids[MAX_NUM_CONCURRENT_PHYS];
} trx_handle_t;

// ********************* Declaration of functions *********************
void trx_parse_args(transceiver_args_t *args, int argc, char **argv);

void trx_args_default(transceiver_args_t *args);

void trx_usage(transceiver_args_t *args, char *prog);

void trx_sig_int_handler(int signo);

void trx_initialize_signal_handler();

void trx_change_process_priority(int inc);

void trx_get_module_and_target_name(char *module_name, char *target1_name, char *target2_name);

int trx_handle_mac_messages(basic_ctrl_t *basic_ctrl);

void trx_open_rf_device();

void trx_close_rf_device();

void trx_set_master_clock_rate();

void trx_handle_update_env_messages(environment_t *env_update);

void trx_verify_environment_update_file_existence(transceiver_args_t* args);

static void trx_watchdog_handler(int sig, siginfo_t *info, void *ptr);

// Set FPGA time now to host time.
inline void trx_set_fpga_time(srslte_rf_t *rf) {
  struct timespec host_time_now;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_time_now);
  srslte_rf_set_time_now(rf, host_time_now.tv_sec, (double)host_time_now.tv_nsec/1000000000LL);
  TRX_DEBUG("FPGA Time set to: %f\n",((double)(uintmax_t)host_time_now.tv_sec + (double)host_time_now.tv_nsec/1000000000LL));
}

inline static void trx_adjust_fpga_time(srslte_rf_t *rf) {
  static uint64_t time_adjust_cnt = 0;
  // Adjust FPGA time if it is different from host pc.
  time_adjust_cnt++;
  if(time_adjust_cnt==200) {
    uint64_t fpga_time = helpers_get_fpga_timestamp_us(rf);
    uint64_t host_time = helpers_get_host_timestamp();
    if((host_time-fpga_time) > 500) {
      trx_set_fpga_time(rf);
      TRX_DEBUG("FPGA: %" PRIu64 " - Host: %" PRIu64 " - diff: %d\n", fpga_time,host_time, (host_time-fpga_time));
    }
    time_adjust_cnt = 0;
  }
}

#endif // _TRX_H_
