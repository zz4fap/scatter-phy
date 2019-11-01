#ifndef _PHY_RECEPTION_H_
#define _PHY_RECEPTION_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <uhd.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#include "../../../../communicator/cpp/communicator_wrapper.h"
#include "helpers.h"
#include "transceiver.h"
#include "plot.h"

// ************************** Definition of macros *****************************
// RX local offset.
#define PHY_RX_LO_OFFSET +42.0e6

// Flag used to enable the use of timed-commands for Rx configuration paramters.
#define RX_TIMED_COMMAND_ENABLED 1

// Amount of time in advance used to apply the timed-command configurations.
#define RX_TIME_ADVANCE_FOR_COMMANDS 500 //350

// Number of Rx basic control messages to be stored in the circular buffer.
#define NUMBER_OF_CONTROL_MSGS_TO_STORE 1000

// ***************************** Debugging macros ******************************
#define CHECK_TIME_BETWEEN_DEMOD_ITER 0

#define CHECK_TIME_BETWEEN_SYNC_ITER 0

#define ENBALE_RX_INFO_PLOT 0

#define WRITE_RX_SUBFRAME_INTO_FILE 0

#define WRITE_CORRECT_SUBFRAME_FILE 0

#define WRITE_DECT_DECD_ERROR_SUBFRAME_FILE 0

#define WRITE_SUBFRAME_SEQUENCE_INTO_FILE 0

// ***************************** INFO/DEBUG MACROS *****************************
#define ENABLE_PHY_RX_PRINTS 1

#define PHY_RX_PRINT(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PHY RX PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_DEBUG(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PHY RX DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_INFO(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PHY RX INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_ERROR(_fmt, ...) do { fprintf(stdout, "[PHY RX ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define PHY_RX_PRINT_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// ****************** Definition of types ******************
typedef struct {
  LayerCommunicator_handle phy_comm_handle;
  srslte_rf_t *rf;
  uint32_t phy_id;
  float initial_rx_gain;
  double competition_bw;
  double competition_center_freq;
  uint16_t rnti;
  float initial_agc_gain;
  bool use_std_carrier_sep;
  int initial_subframe_index; // Set the subframe index number to be used to start from.
  bool add_tx_timestamp;
  bool plot_rx_info;
  bool enable_cfo_correction;
  uint32_t nof_prb;
  uint32_t default_rx_channel;
  double default_rx_bandwidth;
  uint32_t radio_id;
  bool decode_pdcch;
  uint32_t node_id;
  uint32_t intf_id;
  uint32_t nof_ports;
  uint32_t bw_idx;
  uint32_t max_turbo_decoder_noi;
  uint32_t max_turbo_decoder_noi_for_high_mcs;
  bool phy_filtering;

  pthread_attr_t rx_decoding_thread_attr;
  pthread_t rx_decoding_thread_id;

  pthread_attr_t rx_sync_thread_attr;
  pthread_t rx_sync_thread_id;

  // Variable used to stop phy decoding thread.
  volatile sig_atomic_t run_rx_decoding_thread;
  // Variable used to stop phy synchronization thread.
  volatile sig_atomic_t run_rx_synchronization_thread;

  // This basic controls stores the last configured values.
  basic_ctrl_t last_rx_basic_control;

  // Structures used to decode subframes.
  srslte_ue_dl_t ue_dl;
  srslte_ue_sync_t ue_sync;
  srslte_cell_t cell_ue;

  // This mutex is used to synchronize the access to the last configured basic control.
  pthread_mutex_t rx_last_basic_control_mutex;

  // Mutex used to synchronize between synchronization and decoding thread.
  pthread_mutex_t rx_sync_mutex;
  // Condition variable used to synchronize between synchronization and decoding thread.
  pthread_cond_t rx_sync_cv;
  // Circular buffer handle for storage of synchronization messages.
  sync_cb_handle rx_synch_handle;

  // Timer for watchdog.
  timer_t synch_thread_timer_id;

  // PSS detection related parameters.
  float threshold; // PSS detection threshold.
  bool enable_avg_psr;
  bool use_scatter_sync_seq;
  uint32_t pss_len;
  bool enable_second_stage_pss_detection;
  float pss_first_stage_threshold;
  float pss_second_stage_threshold;
  
} phy_reception_t;

// *************** Declaration of functions ***************
int phy_reception_start_thread(LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args, uint32_t phy_id);

int phy_reception_init_thread_context(phy_reception_t* const phy_reception_ctx, LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args);

int phy_reception_stop_thread(uint32_t phy_id);

int phy_reception_start_decoding_thread(phy_reception_t* const phy_reception_ctx);

int phy_reception_stop_decoding_thread(phy_reception_t* const phy_reception_ctx);

int phy_reception_start_sync_thread(phy_reception_t* const phy_reception_ctx);

int phy_reception_stop_sync_thread(phy_reception_t* const phy_reception_ctx);

void phy_reception_init_cell_parameters(phy_reception_t* const phy_reception_ctx);

void phy_reception_init_last_basic_control(phy_reception_t* const phy_reception_ctx);

void phy_reception_init_context(phy_reception_t* const phy_reception_ctx, LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args);

int phy_reception_change_parameters(basic_ctrl_t* const bc);

void set_sequence_number(phy_reception_t* const phy_reception_ctx, uint64_t seq_number);

uint64_t get_sequence_number(phy_reception_t* const phy_reception_ctx);

void set_channel_number(phy_reception_t* const phy_reception_ctx, uint32_t channel);

uint32_t get_channel_number(phy_reception_t* const phy_reception_ctx);

void set_bw_index(phy_reception_t* const phy_reception_ctx, uint32_t bw_index);

uint32_t get_bw_index(phy_reception_t* const phy_reception_ctx);

void set_rx_gain(phy_reception_t* const phy_reception_ctx, uint32_t rx_gain);

uint32_t get_rx_gain(phy_reception_t* const phy_reception_ctx);

void *phy_reception_decoding_work(void *h);

void phy_reception_send_rx_statistics(LayerCommunicator_handle handle, phy_stat_t* const phy_rx_stat);

int phy_reception_ue_init(phy_reception_t* const phy_reception_ctx);

int phy_reception_stop_rx_stream_and_flush_buffer(phy_reception_t* const phy_reception_ctx);

int phy_reception_initialize_rx_stream(phy_reception_t* const phy_reception_ctx);

void phy_reception_ue_free(phy_reception_t* const phy_reception_ctx);

int phy_reception_set_rx_sample_rate(phy_reception_t* const phy_reception_ctx);

int phy_reception_set_initial_rx_freq_and_gain(phy_reception_t* const phy_reception_ctx);

void *phy_reception_sync_work(void *h);

void phy_reception_push_ue_sync_to_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t* const short_ue_sync);

void phy_reception_pop_ue_sync_from_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t* const short_ue_sync);

bool phy_reception_wait_queue_not_empty(phy_reception_t* const phy_reception_ctx);

bool phy_reception_timedwait_and_pop_ue_sync_from_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t* const short_ue_sync);

void phy_reception_print_ue_sync(short_ue_sync_t* const short_ue_sync, char* const str);

void phy_reception_print_decoding_error_counters(phy_stat_t* const phy_rx_stat);

int srslte_rf_recv_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel);

int srslte_rf_recv_with_time_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel);

int srslte_file_recv_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel);

double srslte_rf_set_rx_gain_th_wrapper_(void *h, double f);

void phy_reception_update_environment(uint32_t phy_id, environment_t* const env_update);

void phy_reception_update_frequency(phy_reception_t* const phy_reception_ctx, bool competition_center_freq_updated);

int phy_reception_stop_rx_stream(uint32_t phy_id);

timer_t* phy_reception_get_timer_id(uint32_t phy_id);

int phy_reception_change_timed_parameters(phy_reception_t* const phy_reception_ctx, basic_ctrl_t* const bc);

#endif // _PHY_RECEPTION_H_
