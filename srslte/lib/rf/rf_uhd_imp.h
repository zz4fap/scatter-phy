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

#ifndef _RF_UHD_IMP_H_
#define _RF_UHD_IMP_H_

#include <stdbool.h>
#include <stdint.h>

#include <uhd.h>
#include "srslte/config.h"
#include "srslte/rf/rf.h"
#include "srslte/rf_monitor/lbt.h"

#define DEVNAME_B200 "uhd_b200"
#define DEVNAME_X300 "uhd_x300"

#define ENABLE_RF_UHD_PRINTS 1

#define RF_UHD_PRINT(_fmt, ...) do { if(ENABLE_RF_UHD_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[RF UHD PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define RF_UHD_DEBUG(_fmt, ...) do { if(ENABLE_RF_UHD_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[RF UHD DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define RF_UHD_INFO(_fmt, ...) do { if(ENABLE_RF_UHD_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[RF UHD INFO]: " _fmt, __VA_ARGS__); } while(0)

#define RF_UHD_ERROR(_fmt, ...) do { fprintf(stdout, "[RF UHD ERROR]: " _fmt, __VA_ARGS__); } while(0)

// Struct used to store data related to each one of the USRP channels.
typedef struct {
  uhd_rx_streamer_handle rx_stream;
  uhd_tx_streamer_handle tx_stream;
  uhd_rx_metadata_handle rx_md;
  uhd_rx_metadata_handle rx_md_first;
  uhd_tx_metadata_handle tx_md;
  uhd_meta_range_handle rx_gain_range;
  uhd_async_metadata_handle async_md;
  size_t rx_nof_samples;
  size_t tx_nof_samples;
  double tx_rate;
  bool has_rssi;
  uhd_sensor_value_handle rssi_value;
} rf_uhd_channel_handler_t;

typedef struct {
  char *devname;                                // Field not channel dependent
  uhd_usrp_handle usrp;                         // Field not channel dependent
  bool dynamic_rate;                            // Field not channel dependent
  srslte_rf_error_handler_t uhd_error_handler;  // Field not channel dependent
  size_t num_of_channels;                       // USRP's number of supported and opened channels.
  rf_uhd_channel_handler_t channels[2];         // One USRP can have at most two channels, so, we store up to two channels here.
} rf_uhd_handler_t;

SRSLTE_API int rf_uhd_open(char *args, void **handler);

SRSLTE_API char* rf_uhd_devname(void *h);

SRSLTE_API int rf_uhd_close(void *h);

SRSLTE_API void rf_uhd_set_tx_cal(void *h, srslte_rf_cal_t *cal);

SRSLTE_API void rf_uhd_set_rx_cal(void *h, srslte_rf_cal_t *cal);

SRSLTE_API int rf_uhd_start_rx_stream(void *h, size_t channel);

SRSLTE_API int rf_uhd_start_rx_stream_nsamples(void *h, uint32_t nsamples);

SRSLTE_API int rf_uhd_stop_rx_stream(void *h, size_t channel);

SRSLTE_API void rf_uhd_flush_buffer(void *h, size_t channel);

SRSLTE_API bool rf_uhd_has_rssi(void *h, size_t channel);

SRSLTE_API float rf_uhd_get_rssi(void *h, size_t channel);

SRSLTE_API bool rf_uhd_rx_wait_lo_locked(void *h, size_t channel);

SRSLTE_API bool rf_uhd_tx_wait_lo_locked(void *h, size_t channel);

SRSLTE_API void rf_uhd_set_master_clock_rate(void *h, double rate);

SRSLTE_API bool rf_uhd_is_master_clock_dynamic(void *h);

SRSLTE_API double rf_uhd_set_rx_srate(void *h, double freq, size_t channel);

SRSLTE_API double rf_uhd_get_rx_srate(void *h, size_t channel);

SRSLTE_API double rf_uhd_set_rx_gain(void *h, double gain, size_t channel);

SRSLTE_API double rf_uhd_get_rx_gain(void *h, size_t channel);

SRSLTE_API double rf_uhd_get_tx_gain(void *h, size_t channel);

SRSLTE_API void rf_uhd_suppress_stdout(void *h);

SRSLTE_API void rf_uhd_register_error_handler(void *h, srslte_rf_error_handler_t error_handler);

SRSLTE_API double rf_uhd_set_rx_freq(void *h, double freq, size_t channel);

SRSLTE_API int rf_uhd_recv_with_time(void *h,
                                  void *data,
                                  uint32_t nsamples,
                                  bool blocking,
                                  time_t *secs,
                                  double *frac_secs,
                                  size_t channel);

SRSLTE_API int rf_uhd_recv_channel_with_time(void *h,
                                            void *data,
                                            uint32_t nsamples,
                                            bool blocking,
                                            time_t *secs,
                                            double *frac_secs);

SRSLTE_API void rf_uhd_set_fir_taps(void *h, size_t nof_prb, size_t channel);

SRSLTE_API double rf_uhd_set_tx_srate(void *h, double rate, size_t channel);

SRSLTE_API double rf_uhd_set_tx_gain(void *h, double gain, size_t channel);

SRSLTE_API double rf_uhd_set_tx_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_uhd_get_tx_freq(void *h, size_t channel);

SRSLTE_API double rf_uhd_get_rx_freq(void *h, size_t channel);

SRSLTE_API void rf_uhd_get_time(void *h,
                              time_t *secs,
                              double *frac_secs);

SRSLTE_API int  rf_uhd_send_timed(void *h,
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

SRSLTE_API int rf_uhd_check_async_msg(rf_uhd_handler_t* handler, size_t channel);

SRSLTE_API bool rf_uhd_is_burst_transmitted(void *h, size_t channel);

SRSLTE_API void rf_uhd_set_time_now(void *h, time_t full_secs, double frac_secs);

SRSLTE_API double rf_uhd_set_tx_freq2(void *h, double freq, double lo_off, size_t channel);

SRSLTE_API double rf_uhd_set_rx_freq2(void *h, double freq, double lo_off, size_t channel);

SRSLTE_API double rf_uhd_set_tx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_rx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_tx_gain_cmd(void *h, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_tx_freq_and_gain_cmd(void *h, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API void rf_uhd_rf_monitor_initialize(void *h, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size);

SRSLTE_API void rf_uhd_rf_monitor_uninitialize(void *h);

SRSLTE_API double rf_uhd_set_tx_channel_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_uhd_set_rx_channel_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_uhd_set_tx_channel_freq_and_gain_cmd(void *h, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_rx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_tx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_uhd_set_rf_mon_srate(void *h, double srate);

SRSLTE_API double rf_uhd_get_rf_mon_srate(void *h);

#endif //_RF_UHD_IMP_H_
