#ifndef _LBT_H_
#define _LBT_H_

#include <stdlib.h>
#include <time.h>

#include "rf_monitor.h"

#define ENABLE_LBT_PRINTS 1

#define LBT_DEBUG(_fmt, ...) do { if(ENABLE_LBT_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[LBT DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define LBT_INFO(_fmt, ...) do { if(ENABLE_LBT_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[LBT INFO]: " _fmt, __VA_ARGS__); } while(0)

#define LBT_PRINT(_fmt, ...) do { if(ENABLE_LBT_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[LBT PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define LBT_ERROR(_fmt, ...) do { fprintf(stdout, "[LBT ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define NUM_OF_SENSING_SAMPLES 1024

#define MAX_NUMBER_OF_CHANNELS 4

typedef enum {IDLE_ST=0, CHECK_MEDIUM_ST=1, ALLOW_TX_ST=2} lbt_states_t;

typedef enum {LBT_IDLE_EVT=0, LBT_START_EVT=1, LBT_BUSY_EVT=2, LBT_DONE_EVT=3} lbt_events_t;

typedef struct lbt_stats_st {
    uint64_t channel_free_cnt;
    uint64_t channel_busy_cnt;
    double free_energy;
    double busy_energy;
} lbt_stats_t;

// ********************** Declaration of function. **********************
SRSLTE_API bool lbt_execute_procedure(lbt_stats_t *lbt_stats);

void *lbt_work(void *h);

int lbt_initialize(rf_monitor_handle_t *rf_monitor_handle);

int lbt_uninitialize();

void lbt_initialize_fft();

void lbt_uninitialize_fft();

void lbt_apply_fft_on_bb_samples(cf_t *data);

void lbt_initialize_freq_domain_parameters(rf_monitor_handle_t *rf_monitor_handle);

uint32_t lbt_get_initial_channel_fft_bin(uint32_t tx_channel);

lbt_events_t get_lbt_procedure_event();

void set_lbt_procedure_event(lbt_events_t evt);

void wait_lbt_to_be_done();

float lbt_channel_td_power_measurement(cf_t *data, uint32_t num_read_samples);

float lbt_channel_fd_power_measurement(cf_t *data, uint32_t num_read_samples);

uint32_t lbt_get_number_of_samples_to_read(uint32_t nof_prb);

void lbt_init_stats(lbt_stats_t *lbt_stats);

void lbt_copy_stats(lbt_stats_t *lbt_stats, lbt_stats_t *phy_tx_lbt_stats);

#endif // _LBT_H_
