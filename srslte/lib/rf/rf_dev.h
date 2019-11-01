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

/* RF frontend API */
typedef struct {
  const char *name;
  char*  (*srslte_rf_devname) (void *h);
  bool   (*srslte_rf_rx_wait_lo_locked) (void *h, size_t channel);
  int    (*srslte_rf_start_rx_stream)(void *h, size_t channel);
  int    (*srslte_rf_stop_rx_stream)(void *h, size_t channel);
  void   (*srslte_rf_flush_buffer)(void *h, size_t channel);
  bool   (*srslte_rf_has_rssi)(void *h, size_t channel);
  float  (*srslte_rf_get_rssi)(void *h, size_t channel);
  void   (*srslte_rf_suppress_stdout)(void *h);
  void   (*srslte_rf_register_error_handler)(void *h, srslte_rf_error_handler_t error_handler);
  int    (*srslte_rf_open)(char *args, void **h);
  int    (*srslte_rf_close)(void *h);
  void   (*srslte_rf_set_master_clock_rate)(void *h, double rate);
  bool   (*srslte_rf_is_master_clock_dynamic)(void *h);
  double (*srslte_rf_set_rx_srate)(void *h, double freq, size_t channel);
  double (*srslte_rf_set_rx_gain)(void *h, double gain, size_t channel);
  double (*srslte_rf_set_tx_gain)(void *h, double gain, size_t channel);
  double (*srslte_rf_get_rx_gain)(void *h, size_t channel);
  double (*srslte_rf_get_tx_gain)(void *h, size_t channel);
  double (*srslte_rf_set_rx_freq)(void *h, double freq, size_t channel);
  void   (*srslte_rf_set_fir_taps)(void *h, size_t nof_prb, size_t channel);
  double (*srslte_rf_set_tx_srate)(void *h, double freq, size_t channel);
  double (*srslte_rf_set_tx_freq)(void *h, double freq, size_t channel);
  void   (*srslte_rf_get_time)(void *h, time_t *secs, double *frac_secs);
  int    (*srslte_rf_recv_with_time)(void *h, void *data, uint32_t nsamples, bool blocking, time_t *secs,double *frac_secs, size_t channel);
  int    (*srslte_rf_send_timed)(void *h, void *data, int nsamples,
                     time_t secs, double frac_secs, bool has_time_spec,
                     bool blocking, bool is_start_of_burst, bool is_end_of_burst, bool is_lbt_enabled, void *lbt_stats_void_ptr, size_t channel);
  void   (*srslte_rf_set_tx_cal)(void *h, srslte_rf_cal_t *cal);
  void   (*srslte_rf_set_rx_cal)(void *h, srslte_rf_cal_t *cal);
  double (*srslte_rf_get_rx_srate)(void *h, size_t channel);
  bool   (*srslte_rf_tx_wait_lo_locked)(void *h, size_t channel);
  bool   (*srslte_rf_is_burst_transmitted)(void* h, size_t channel);
  double (*srslte_rf_get_tx_freq)(void *h, size_t channel);
  double (*srslte_rf_get_rx_freq)(void *h, size_t channel);
  void   (*srslte_rf_set_time_now)(void *h, time_t full_secs, double frac_secs);
  double (*srslte_rf_set_tx_freq2)(void *h, double freq, double lo_off, size_t channel);
  double (*srslte_rf_set_rx_freq2)(void *h, double freq, double lo_off, size_t channel);
  double (*srslte_rf_set_tx_freq_cmd)(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);
  double (*srslte_rf_set_rx_freq_cmd)(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);
  double (*srslte_rf_set_tx_gain_cmd)(void *h, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);
  double (*srslte_rf_set_tx_freq_and_gain_cmd)(void *h, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);
  void   (*srslte_rf_rf_monitor_initialize) (void *h, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size);
  void   (*srslte_rf_rf_monitor_uninitialize) (void *h);
  double (*srslte_rf_set_rf_mon_srate)(void *h, double srate);
  double (*srslte_rf_get_rf_mon_srate)(void *h);
  double (*srslte_rf_set_tx_channel_freq)(void *h, double freq, size_t channel);
  double (*srslte_rf_set_rx_channel_freq)(void *h, double freq, size_t channel);
  double (*srslte_rf_set_tx_channel_freq_cmd)(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);
  double (*srslte_rf_set_rx_channel_freq_cmd)(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);
  double (*srslte_rf_set_tx_channel_freq_and_gain_cmd)(void *h, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);
} rf_dev_t;

/* Define implementation for UHD */
#ifdef ENABLE_UHD

#include "rf_uhd_imp.h"

static rf_dev_t dev_uhd = {
  "UHD",
  rf_uhd_devname,
  rf_uhd_rx_wait_lo_locked,
  rf_uhd_start_rx_stream,
  rf_uhd_stop_rx_stream,
  rf_uhd_flush_buffer,
  rf_uhd_has_rssi,
  rf_uhd_get_rssi,
  rf_uhd_suppress_stdout,
  rf_uhd_register_error_handler,
  rf_uhd_open,
  rf_uhd_close,
  rf_uhd_set_master_clock_rate,
  rf_uhd_is_master_clock_dynamic,
  rf_uhd_set_rx_srate,
  rf_uhd_set_rx_gain,
  rf_uhd_set_tx_gain,
  rf_uhd_get_rx_gain,
  rf_uhd_get_tx_gain,
  rf_uhd_set_rx_freq,
  rf_uhd_set_fir_taps,
  rf_uhd_set_tx_srate,
  rf_uhd_set_tx_freq,
  rf_uhd_get_time,
  rf_uhd_recv_with_time,
  rf_uhd_send_timed,
  rf_uhd_set_tx_cal,
  rf_uhd_set_rx_cal,
  rf_uhd_get_rx_srate,
  rf_uhd_tx_wait_lo_locked,
  rf_uhd_is_burst_transmitted,
  rf_uhd_get_tx_freq,
  rf_uhd_get_rx_freq,
  rf_uhd_set_time_now,
  rf_uhd_set_tx_freq2,
  rf_uhd_set_rx_freq2,
  rf_uhd_set_tx_freq_cmd,
  rf_uhd_set_rx_freq_cmd,
  rf_uhd_set_tx_gain_cmd,
  rf_uhd_set_tx_freq_and_gain_cmd,
  rf_uhd_rf_monitor_initialize,
  rf_uhd_rf_monitor_uninitialize,
  rf_uhd_set_rf_mon_srate,
  rf_uhd_get_rf_mon_srate,
  rf_uhd_set_tx_channel_freq,
  rf_uhd_set_rx_channel_freq,
  rf_uhd_set_tx_channel_freq_cmd,
  rf_uhd_set_rx_channel_freq_cmd,
  rf_uhd_set_tx_channel_freq_and_gain_cmd
};
#endif

// Define Blade RF only if UHD is not present.
#ifndef ENABLE_UHD

/* Define implementation for channel emulator */
#ifdef ENABLE_CH_EMULATOR

#include "rf_ch_emulator_imp.h"

static rf_dev_t dev_ch_emulator = {
  "CH_EMULATOR",
  rf_ch_emulator_devname,
  rf_ch_emulator_rx_wait_lo_locked,
  rf_ch_emulator_start_rx_stream,
  rf_ch_emulator_stop_rx_stream,
  rf_ch_emulator_flush_buffer,
  rf_ch_emulator_has_rssi,
  rf_ch_emulator_get_rssi,
  rf_ch_emulator_suppress_stdout,
  rf_ch_emulator_register_error_handler,
  rf_ch_emulator_open,
  rf_ch_emulator_close,
  rf_ch_emulator_set_master_clock_rate,
  rf_ch_emulator_is_master_clock_dynamic,
  rf_ch_emulator_set_rx_srate,
  rf_ch_emulator_set_rx_gain,
  rf_ch_emulator_set_tx_gain,
  rf_ch_emulator_get_rx_gain,
  rf_ch_emulator_get_tx_gain,
  rf_ch_emulator_set_rx_freq,
  rf_ch_emulator_set_tx_srate,
  rf_ch_emulator_set_tx_freq,
  rf_ch_emulator_get_time,
  rf_ch_emulator_recv_with_time,
  rf_ch_emulator_send_timed,
  rf_ch_emulator_set_tx_cal,
  rf_ch_emulator_set_rx_cal,
  rf_ch_emulator_get_rx_srate,
  rf_ch_emulator_tx_wait_lo_locked,
  rf_ch_emulator_is_burst_transmitted,
  rf_ch_emulator_get_tx_freq,
  rf_ch_emulator_get_rx_freq,
  rf_ch_emulator_set_time_now,
  rf_ch_emulator_set_tx_freq2,
  rf_ch_emulator_set_rx_freq2,
  rf_ch_emulator_set_tx_freq_cmd,
  rf_ch_emulator_set_rx_freq_cmd,
  rf_ch_emulator_set_tx_gain_cmd,
  rf_ch_emulator_set_tx_freq_and_gain_cmd,
  rf_ch_emulator_rf_monitor_initialize,
  rf_ch_emulator_rf_monitor_uninitialize,
  rf_ch_emulator_set_rf_mon_srate,
  rf_ch_emulator_get_rf_mon_srate,
  rf_ch_emulator_set_tx_channel_freq,
  rf_ch_emulator_set_rx_channel_freq,
  rf_ch_emulator_set_tx_channel_freq_cmd,
  rf_ch_emulator_set_rx_channel_freq_cmd,
  rf_ch_emulator_set_tx_channel_freq_and_gain_cmd
};

#endif

/* Define implementation for bladeRF */
#ifdef ENABLE_BLADERF

#include "rf_blade_imp.h"

static rf_dev_t dev_blade = {
  "bladeRF",
  rf_blade_devname,
  rf_blade_rx_wait_lo_locked,
  rf_blade_start_rx_stream,
  rf_blade_stop_rx_stream,
  rf_blade_flush_buffer,
  rf_blade_has_rssi,
  rf_blade_get_rssi,
  rf_blade_suppress_stdout,
  rf_blade_register_error_handler,
  rf_blade_open,
  rf_blade_close,
  rf_blade_set_master_clock_rate,
  rf_blade_is_master_clock_dynamic,
  rf_blade_set_rx_srate,
  rf_blade_set_rx_gain,
  rf_blade_set_tx_gain,
  rf_blade_get_rx_gain,
  rf_blade_get_tx_gain,
  rf_blade_set_rx_freq,
  rf_blade_set_tx_srate,
  rf_blade_set_tx_freq,
  rf_blade_get_time,
  rf_blade_recv_with_time,
  rf_blade_send_timed,
  rf_blade_set_tx_cal,
  rf_blade_set_rx_cal
};
#endif

#endif //ifndef ENABLE_UHD

//#define ENABLE_DUMMY_DEV

#ifdef ENABLE_DUMMY_DEV
int dummy_rcv() {
  usleep(100000);
  return 1;
}
void dummy_fnc() {

}

static rf_dev_t dev_dummy = {
  "dummy",
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_rcv,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc,
  dummy_fnc
};
#endif

static rf_dev_t *available_devices[] = {
#ifdef ENABLE_UHD
  &dev_uhd,
#endif
#ifndef ENABLE_UHD
#ifdef ENABLE_CH_EMULATOR
  &dev_ch_emulator,
#endif // #ifdef ENABLE_CH_EMULATOR
#ifdef ENABLE_BLADERF
  &dev_blade,
#endif
#endif
#ifdef ENABLE_DUMMY_DEV
  &dev_dummy,
#endif
  NULL
};
