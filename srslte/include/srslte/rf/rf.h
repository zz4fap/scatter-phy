/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef _RF_H_
#define _RF_H_

#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "srslte/config.h"

#define ENABLE_LBT_FEATURE 0 // Change this macro to 1 if you want to enable LBT.

#define ENABLE_HW_RF_MONITOR 1 // If we are using HW RF Mon then this macro must be set to 1, otherwise 0.

#define RF_LBT_TIMEOUT_CODE -1000 // This error indicates that LBT procedure timedout without the channel becoming FREE.

#define ENABLE_RF_PRINTS 1

#define RF_DEBUG(_fmt, ...) do { if(ENABLE_RF_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[RF DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define RF_INFO(_fmt, ...) do { if(ENABLE_RF_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[RF INFO]: " _fmt, __VA_ARGS__); } while(0)

#define RF_ERROR(_fmt, ...) do { fprintf(stdout, "[RF ERROR]: " _fmt, __VA_ARGS__); } while(0)

typedef struct {
  void *handler;
  void *dev;
  void *rf_monitor_channel_handler;

  // The following variables are for threaded RX gain control.
  pthread_t thread_gain;
  pthread_cond_t  cond;
  pthread_mutex_t mutex;
  double cur_rx_gain;
  double new_rx_gain;
  bool   tx_gain_same_rx;
  float  tx_rx_gain_offset;
  size_t num_of_channels;
  size_t rx_nof_samples; // Maximum number of samples per packet per buffer.
} srslte_rf_t;

typedef struct {
  float dc_gain;
  float dc_phase;
  float iq_i;
  float iq_q;
} srslte_rf_cal_t;

typedef struct {
  enum {
    SRSLTE_RF_ERROR_LATE,
    SRSLTE_RF_ERROR_UNDERFLOW,
    SRSLTE_RF_ERROR_OVERFLOW,
    SRSLTE_RF_ERROR_OTHER
  } type;
  int opt;
  const char *msg;
} srslte_rf_error_t;

typedef void (*srslte_rf_error_handler_t)(srslte_rf_error_t error);

SRSLTE_API int srslte_rf_open(srslte_rf_t *h, char *args);

SRSLTE_API int srslte_rf_open_devname(srslte_rf_t *h,
                               char *devname,
                               char *args);

SRSLTE_API const char *srslte_rf_name(srslte_rf_t *h);

SRSLTE_API int srslte_rf_start_gain_thread(srslte_rf_t *rf, bool tx_gain_same_rx, size_t channel);

SRSLTE_API int srslte_rf_finish_gain_thread(srslte_rf_t *rf);

SRSLTE_API int srslte_rf_close(srslte_rf_t *h);

SRSLTE_API void srslte_rf_set_tx_cal(srslte_rf_t *h, srslte_rf_cal_t *cal);

SRSLTE_API void srslte_rf_set_rx_cal(srslte_rf_t *h, srslte_rf_cal_t *cal);

SRSLTE_API int srslte_rf_start_rx_stream(srslte_rf_t *h, size_t channel);

SRSLTE_API int srslte_rf_stop_rx_stream(srslte_rf_t *h, size_t channel);

SRSLTE_API void srslte_rf_flush_buffer(srslte_rf_t *h, size_t channel);

SRSLTE_API bool srslte_rf_has_rssi(srslte_rf_t *h, size_t channel);

SRSLTE_API float srslte_rf_get_rssi(srslte_rf_t *h, size_t channel);

SRSLTE_API bool srslte_rf_rx_wait_lo_locked(srslte_rf_t *h, size_t channel);

SRSLTE_API bool srslte_rf_tx_wait_lo_locked(srslte_rf_t *h, size_t channel);

SRSLTE_API void srslte_rf_set_master_clock_rate(srslte_rf_t *h, double rate);

SRSLTE_API bool srslte_rf_is_master_clock_dynamic(srslte_rf_t *h);

SRSLTE_API double srslte_rf_set_rx_srate(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_get_rx_srate(srslte_rf_t *h, size_t channel);

SRSLTE_API double srslte_rf_set_rx_gain(srslte_rf_t *h, double gain, size_t channel);

SRSLTE_API void srslte_rf_set_tx_rx_gain_offset(srslte_rf_t *h, double offset);

SRSLTE_API double srslte_rf_set_rx_gain_th(srslte_rf_t *h, double gain);

SRSLTE_API double srslte_rf_get_rx_gain(srslte_rf_t *h, size_t channel);

SRSLTE_API double srslte_rf_get_tx_gain(srslte_rf_t *h, size_t channel);

SRSLTE_API void srslte_rf_suppress_stdout(srslte_rf_t *h);

SRSLTE_API void srslte_rf_register_error_handler(srslte_rf_t *h, srslte_rf_error_handler_t error_handler);

SRSLTE_API double srslte_rf_set_rx_freq(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_set_rx_freq2(srslte_rf_t *h, double freq, double lo_off, size_t channel);

SRSLTE_API double srslte_rf_get_tx_freq(srslte_rf_t *h, size_t channel);

SRSLTE_API double srslte_rf_get_rx_freq(srslte_rf_t *h, size_t channel);

SRSLTE_API int srslte_rf_recv(srslte_rf_t *h,
                       void *data,
                       uint32_t nsamples,
                       bool blocking,
                       size_t channel);

SRSLTE_API int srslte_rf_recv_with_time(srslte_rf_t *h,
                                 void *data,
                                 uint32_t nsamples,
                                 bool blocking,
                                 time_t *secs,
                                 double *frac_secs,
                                 size_t channel);

SRSLTE_API int srslte_rf_recv_channel_with_time(srslte_rf_t *h,
                                                void *data,
                                                uint32_t nsamples,
                                                bool blocking,
                                                time_t *secs,
                                                double *frac_secs);

SRSLTE_API void srslte_rf_set_fir_taps(srslte_rf_t *h, size_t nof_prb, size_t channel);

SRSLTE_API double srslte_rf_set_tx_srate(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_set_tx_gain(srslte_rf_t *h, double gain, size_t channel);

SRSLTE_API double srslte_rf_set_tx_freq(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_set_tx_freq2(srslte_rf_t *h, double freq, double lo_off, size_t channel);

SRSLTE_API void srslte_rf_get_time(srslte_rf_t *h,
                            time_t *secs,
                            double *frac_secs);

SRSLTE_API int srslte_rf_send(srslte_rf_t *h,
                       void *data,
                       uint32_t nsamples,
                       bool blocking,
                       size_t channel);

SRSLTE_API int srslte_rf_send2(srslte_rf_t *h,
                        void *data,
                        uint32_t nsamples,
                        bool blocking,
                        bool start_of_burst,
                        bool end_of_burst,
                        size_t channel);

SRSLTE_API int srslte_rf_send(srslte_rf_t *h,
                       void *data,
                       uint32_t nsamples,
                       bool blocking,
                       size_t channel);

SRSLTE_API int srslte_rf_send_timed(srslte_rf_t *h,
                             void *data,
                             int nsamples,
                             time_t secs,
                             double frac_secs,
                             size_t channel);

SRSLTE_API int srslte_rf_send_timed2(srslte_rf_t *h,
                              void *data,
                              int nsamples,
                              time_t secs,
                              double frac_secs,
                              bool is_start_of_burst,
                              bool is_end_of_burst,
                              size_t channel);

SRSLTE_API int srslte_rf_send_timed3(srslte_rf_t *rf,
                              void *data,
                              int nsamples,
                              time_t secs,
                              double frac_secs,
                              bool has_time_spec,
                              bool blocking,
                              bool is_start_of_burst,
                              bool is_end_of_burst,
                              bool is_lbt_enabled,
                              void *lbt_stats_void_ptr,
                              size_t channel);

SRSLTE_API bool srslte_rf_is_burst_transmitted(srslte_rf_t *rf, size_t channel);

SRSLTE_API void srslte_rf_set_time_now(srslte_rf_t *rf, time_t full_secs, double frac_secs);

SRSLTE_API double srslte_rf_set_tx_freq_cmd(srslte_rf_t *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double srslte_rf_set_rx_freq_cmd(srslte_rf_t *rf, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double srslte_rf_set_tx_gain_cmd(srslte_rf_t *rf, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double srslte_rf_set_tx_freq_and_gain_cmd(srslte_rf_t *rf, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API void srslte_rf_rf_monitor_initialize(srslte_rf_t *rf, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size);

SRSLTE_API void srslte_rf_rf_monitor_uninitialize(srslte_rf_t *rf);

SRSLTE_API double srslte_rf_set_rf_mon_srate(srslte_rf_t *h, double freq);

SRSLTE_API double srslte_rf_get_rf_mon_srate(srslte_rf_t *h);

SRSLTE_API double srslte_rf_set_tx_channel_freq(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_set_rx_channel_freq(srslte_rf_t *h, double freq, size_t channel);

SRSLTE_API double srslte_rf_set_tx_channel_freq_cmd(srslte_rf_t *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double srslte_rf_set_rx_channel_freq_cmd(srslte_rf_t *rf, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double srslte_rf_set_tx_channel_freq_and_gain_cmd(srslte_rf_t *rf, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

//******************************************************************************
// Functions only used with Channel Emulator.
//******************************************************************************
SRSLTE_API void rf_channel_emulator_add_awgn(srslte_rf_t *rf, float noise_floor, float SNRdB);

SRSLTE_API void rf_channel_emulator_add_simple_awgn(srslte_rf_t *rf, float noise_variance);

SRSLTE_API void rf_channel_emulator_add_carrier_offset(srslte_rf_t *rf, float dphi, float phi);

SRSLTE_API void rf_channel_emulator_add_multipath(srslte_rf_t *rf, cf_t *hc, unsigned int hc_len);

SRSLTE_API void rf_channel_emulator_add_shadowing(srslte_rf_t *rf, float sigma, float fd);

SRSLTE_API void rf_channel_emulator_print_channel(srslte_rf_t *rf);

SRSLTE_API void rf_channel_emulator_estimate_psd(srslte_rf_t *rf, cf_t *sig, uint32_t nof_samples, unsigned int nfft, float *psd);

SRSLTE_API void rf_channel_emulator_create_psd_script(srslte_rf_t *rf, unsigned int nfft, float *psd);

SRSLTE_API void rf_channel_emulator_set_channel_impairments(srslte_rf_t *rf, bool flag);

SRSLTE_API void rf_channel_emulator_set_simple_awgn_channel(srslte_rf_t *rf, bool flag);

SRSLTE_API void rf_channel_emulator_set_cfo_freq(srslte_rf_t *rf, float cfo_freq);

SRSLTE_API void rf_channel_emulator_stop_running(srslte_rf_t *rf, size_t channel);

#endif // _RF_H_
