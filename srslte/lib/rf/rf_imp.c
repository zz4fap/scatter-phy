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

#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "srslte/rf/rf.h"
#include "srslte/srslte.h"
#include "rf_dev.h"
#include "rf_uhd_imp.h"
#include "rf_ch_emulator_imp.h"

// Global varibale used to exit the AGC thread.
bool exit_agc_thread = false;

int rf_get_available_devices(char **devnames, int max_strlen) {
  int i=0;
  while(available_devices[i]->name) {
    strncpy(devnames[i], available_devices[i]->name, max_strlen);
    i++;
  }
  return i;
}

double srslte_rf_set_rx_gain_th(srslte_rf_t *rf, double gain) {
  if (gain > rf->new_rx_gain + 0.5 || gain < rf->new_rx_gain - 0.5) {
    pthread_mutex_lock(&rf->mutex);
    rf->new_rx_gain = gain;
    pthread_cond_signal(&rf->cond);
    pthread_mutex_unlock(&rf->mutex);
  }
  return gain;
}

void srslte_rf_set_tx_rx_gain_offset(srslte_rf_t *rf, double offset) {
  rf->tx_rx_gain_offset = offset;
}

/* This thread listens for set_rx_gain commands to the USRP */
typedef struct {
  srslte_rf_t *rf_p;
  size_t channel;
} gain_struct_t;

gain_struct_t gain_struct;

static void* thread_gain_fcn(void *h) {
  gain_struct_t* gain_struct = (gain_struct_t*) h;
  srslte_rf_t* rf = (srslte_rf_t*) gain_struct->rf_p;
  while(!exit_agc_thread) {
    pthread_mutex_lock(&rf->mutex);
    while(rf->cur_rx_gain == rf->new_rx_gain && !exit_agc_thread) {
      pthread_cond_wait(&rf->cond, &rf->mutex);
    }
    if (rf->new_rx_gain != rf->cur_rx_gain) {
      rf->cur_rx_gain = rf->new_rx_gain;
      srslte_rf_set_rx_gain(gain_struct->rf_p, rf->cur_rx_gain, gain_struct->channel);
    }
    if (rf->tx_gain_same_rx) {
      srslte_rf_set_tx_gain(gain_struct->rf_p, rf->cur_rx_gain+rf->tx_rx_gain_offset, gain_struct->channel);
    }
    pthread_mutex_unlock(&rf->mutex);
  }
  pthread_exit(NULL);
}

/* Create auxiliary thread and mutexes for AGC */
int srslte_rf_start_gain_thread(srslte_rf_t *rf, bool tx_gain_same_rx, size_t channel) {
  gain_struct.rf_p = rf;
  gain_struct.channel = channel;
  rf->tx_gain_same_rx = tx_gain_same_rx;
  rf->tx_rx_gain_offset = 0.0;
  if (pthread_mutex_init(&rf->mutex, NULL)) {
    return -1;
  }
  if (pthread_cond_init(&rf->cond, NULL)) {
    return -1;
  }
  if (pthread_create(&rf->thread_gain, NULL, thread_gain_fcn, (void*)&gain_struct)) {
    perror("pthread_create");
    return -1;
  }
  return 0;
}

/* Send signal to finsih and join the thread created to control AGC's gain. */
int srslte_rf_finish_gain_thread(srslte_rf_t *rf) {
  void *res;
  int ret;
  if (!pthread_kill(rf->thread_gain, 0)) {
    // Send signal throught global variable to the AGC thread to finish its execution.
    exit_agc_thread = true;
    pthread_mutex_lock(&rf->mutex);
    pthread_cond_signal(&rf->cond);
    pthread_mutex_unlock(&rf->mutex);
    ret = pthread_join(rf->thread_gain, &res);
    if (ret != 0) {
      perror("pthread_join");
      return -1;
    }
  } else {
    DEBUG("AGC Thread is not running........\n",0);
    return -1;
  }
  return 0;
}

const char* srslte_rf_get_devname(srslte_rf_t *rf) {
  return ((rf_dev_t*) rf->dev)->name;
}

int srslte_rf_open_devname(srslte_rf_t *rf, char *devname, char *args) {
  /* Try to open the device if name is provided */
  if (devname) {
    if (devname[0] != '\0') {
      int i=0;
      while(available_devices[i] != NULL) {
        if (!strcmp(available_devices[i]->name, devname)) {
          rf->dev = available_devices[i];
          return available_devices[i]->srslte_rf_open(args, &rf->handler);
        }
        i++;
      }
      printf("Device %s not found. Switching to auto mode\n", devname);
    }
  }

  /* If in auto mode or provided device not found, try to open in order of apperance in available_devices[] array */
  int i = 0;
  while(available_devices[i] != NULL) {
    if (!available_devices[i]->srslte_rf_open(args, &rf->handler)) {
      rf->dev = available_devices[i];
      rf->rx_nof_samples = 0;
#ifdef ENABLE_CH_EMULATOR
      rf->num_of_channels = ((rf_ch_emulator_handler_t*)rf->handler)->num_of_channels;
      if(rf->num_of_channels > 1) {
        rf->rf_monitor_channel_handler = NULL;
      }
#else
      rf->num_of_channels = ((rf_uhd_handler_t*)rf->handler)->num_of_channels;
      if(rf->num_of_channels > 1) {
        rf->rf_monitor_channel_handler = &((rf_uhd_handler_t*)rf->handler)->channels[1];
        rf->rx_nof_samples = ((rf_uhd_handler_t*)rf->handler)->channels[1].rx_nof_samples;
      }
#endif
      return 0;
    }
    i++;
  }
  fprintf(stderr, "No compatible RF frontend found.\n");
  return -1;
}

void srslte_rf_set_tx_cal(srslte_rf_t *rf, srslte_rf_cal_t *cal) {
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_cal(rf->handler, cal);
}

void srslte_rf_set_rx_cal(srslte_rf_t *rf, srslte_rf_cal_t *cal) {
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_cal(rf->handler, cal);
}

const char* srslte_rf_name(srslte_rf_t *rf) {
  return ((rf_dev_t*) rf->dev)->srslte_rf_devname(rf->handler);
}

bool srslte_rf_rx_wait_lo_locked(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_rx_wait_lo_locked(rf->handler, channel);
}

bool srslte_rf_tx_wait_lo_locked(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_tx_wait_lo_locked(rf->handler, channel);
}

int srslte_rf_start_rx_stream(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_start_rx_stream(rf->handler, channel);
}

int srslte_rf_stop_rx_stream(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_stop_rx_stream(rf->handler, channel);
}

void srslte_rf_flush_buffer(srslte_rf_t *rf, size_t channel)
{
  ((rf_dev_t*) rf->dev)->srslte_rf_flush_buffer(rf->handler, channel);
}

bool srslte_rf_has_rssi(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_has_rssi(rf->handler, channel);
}

float srslte_rf_get_rssi(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_rssi(rf->handler, channel);
}

void srslte_rf_suppress_stdout(srslte_rf_t *rf)
{
  ((rf_dev_t*) rf->dev)->srslte_rf_suppress_stdout(rf->handler);
}

void srslte_rf_register_error_handler(srslte_rf_t *rf, srslte_rf_error_handler_t error_handler)
{
  ((rf_dev_t*) rf->dev)->srslte_rf_register_error_handler(rf->handler, error_handler);
}

int srslte_rf_open(srslte_rf_t *h, char *args)
{
  return srslte_rf_open_devname(h, NULL, args);
}

int srslte_rf_close(srslte_rf_t *rf)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_close(rf->handler);
}

void srslte_rf_set_master_clock_rate(srslte_rf_t *rf, double rate)
{
  ((rf_dev_t*) rf->dev)->srslte_rf_set_master_clock_rate(rf->handler, rate);
}

bool srslte_rf_is_master_clock_dynamic(srslte_rf_t *rf)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_is_master_clock_dynamic(rf->handler);
}

double srslte_rf_set_rx_srate(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_srate(rf->handler, freq, channel);
}

double srslte_rf_get_rx_srate(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_rx_srate(rf->handler, channel);
}

double srslte_rf_set_rx_gain(srslte_rf_t *rf, double gain, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_gain(rf->handler, gain, channel);
}

double srslte_rf_get_rx_gain(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_rx_gain(rf->handler, channel);
}

double srslte_rf_get_tx_gain(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_tx_gain(rf->handler, channel);
}

double srslte_rf_set_rx_freq(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_freq(rf->handler, freq, channel);
}

double srslte_rf_set_rx_freq2(srslte_rf_t *rf, double freq, double lo_off, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_freq2(rf->handler, freq, lo_off, channel);
}

int srslte_rf_recv(srslte_rf_t *rf, void *data, uint32_t nsamples, bool blocking, size_t channel)
{
  return srslte_rf_recv_with_time(rf, data, nsamples, blocking, NULL, NULL, channel);
}

int srslte_rf_recv_with_time(srslte_rf_t *rf,
                    void *data,
                    uint32_t nsamples,
                    bool blocking,
                    time_t *secs,
                    double *frac_secs,
                    size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_recv_with_time(rf->handler, data, nsamples, blocking, secs, frac_secs, channel);
}

int srslte_rf_recv_channel_with_time(srslte_rf_t *rf,
                                    void *data,
                                    uint32_t nsamples,
                                    bool blocking,
                                    time_t *secs,
                                    double *frac_secs)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_recv_with_time(rf->handler, data, nsamples, blocking, secs, frac_secs, 1);
}

void srslte_rf_set_fir_taps(srslte_rf_t *rf, size_t nof_prb, size_t channel)	
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_fir_taps(rf->handler, nof_prb, channel);
}

double srslte_rf_set_tx_gain(srslte_rf_t *rf, double gain, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_gain(rf->handler, gain, channel);
}

double srslte_rf_set_tx_srate(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_srate(rf->handler, freq, channel);
}

double srslte_rf_set_tx_freq(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_freq(rf->handler, freq, channel);
}

double srslte_rf_set_tx_freq2(srslte_rf_t *rf, double freq, double lo_off, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_freq2(rf->handler, freq, lo_off, channel);
}

void srslte_rf_get_time(srslte_rf_t *rf, time_t *secs, double *frac_secs)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_time(rf->handler, secs, frac_secs);
}

int srslte_rf_send_timed3(srslte_rf_t *rf,
                     void *data,
                     int nsamples,
                     time_t secs,
                     double frac_secs,
                     bool has_time_spec,
                     bool blocking,
                     bool is_start_of_burst,
                     bool is_end_of_burst,
                     bool is_lbt_enabled,
                     void *lbt_stats,
                     size_t channel)
{

  return ((rf_dev_t*) rf->dev)->srslte_rf_send_timed(rf->handler, data, nsamples, secs, frac_secs,
                                 has_time_spec, blocking, is_start_of_burst, is_end_of_burst, is_lbt_enabled, lbt_stats, channel);
}

int srslte_rf_send(srslte_rf_t *rf, void *data, uint32_t nsamples, bool blocking, size_t channel)
{
  return srslte_rf_send2(rf, data, nsamples, blocking, true, true, channel);
}

int srslte_rf_send2(srslte_rf_t *rf, void *data, uint32_t nsamples, bool blocking, bool start_of_burst, bool end_of_burst, size_t channel)
{
  return srslte_rf_send_timed3(rf, data, nsamples, 0, 0, false, blocking, start_of_burst, end_of_burst, false, NULL, channel);
}

int srslte_rf_send_timed(srslte_rf_t *rf,
                    void *data,
                    int nsamples,
                    time_t secs,
                    double frac_secs,
                    size_t channel)
{
  return srslte_rf_send_timed2(rf, data, nsamples, secs, frac_secs, true, true, channel);
}

int srslte_rf_send_timed2(srslte_rf_t *rf,
                    void *data,
                    int nsamples,
                    time_t secs,
                    double frac_secs,
                    bool is_start_of_burst,
                    bool is_end_of_burst,
                    size_t channel)
{
  return srslte_rf_send_timed3(rf, data, nsamples, secs, frac_secs, true, true, is_start_of_burst, is_end_of_burst, false, NULL, channel);
}

bool srslte_rf_is_burst_transmitted(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_is_burst_transmitted(rf->handler, channel);
}

double srslte_rf_get_tx_freq(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_tx_freq(rf->handler, channel);
}

double srslte_rf_get_rx_freq(srslte_rf_t *rf, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_rx_freq(rf->handler, channel);
}

void srslte_rf_set_time_now(srslte_rf_t *rf, time_t full_secs, double frac_secs)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_time_now(rf->handler, full_secs, frac_secs);
}

double srslte_rf_set_tx_freq_cmd(srslte_rf_t *rf, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_freq_cmd(rf->handler, freq, lo_off, timestamp, time_advance, channel);
}

double srslte_rf_set_rx_freq_cmd(srslte_rf_t *rf, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_freq_cmd(rf->handler, freq, lo_off, timestamp, time_advance, channel);
}

double srslte_rf_set_tx_gain_cmd(srslte_rf_t *rf, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_gain_cmd(rf->handler, gain, timestamp, time_advance, channel);
}

double srslte_rf_set_tx_freq_and_gain_cmd(srslte_rf_t *rf, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_freq_and_gain_cmd(rf->handler, freq, lo_off, gain, timestamp, time_advance, channel);
}

void srslte_rf_rf_monitor_initialize(srslte_rf_t *rf, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_rf_monitor_initialize(rf->handler, freq, rate, lo_off, fft_size, avg_size);
}

void srslte_rf_rf_monitor_uninitialize(srslte_rf_t *rf)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_rf_monitor_uninitialize(rf->handler);
}

double srslte_rf_set_tx_channel_freq(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_channel_freq(rf->handler, freq, channel);
}

double srslte_rf_set_rx_channel_freq(srslte_rf_t *rf, double freq, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_channel_freq(rf->handler, freq,  channel);
}

double srslte_rf_set_tx_channel_freq_cmd(srslte_rf_t *rf, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_channel_freq_cmd(rf->handler, freq, timestamp, time_advance, channel);
}

double srslte_rf_set_rx_channel_freq_cmd(srslte_rf_t *rf, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rx_channel_freq_cmd(rf->handler, freq, timestamp, time_advance, channel);
}

double srslte_rf_set_tx_channel_freq_and_gain_cmd(srslte_rf_t *rf, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_tx_channel_freq_and_gain_cmd(rf->handler, freq, gain, timestamp, time_advance, channel);
}

double srslte_rf_set_rf_mon_srate(srslte_rf_t *rf, double srate)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_set_rf_mon_srate(rf->handler, srate);
}

double srslte_rf_get_rf_mon_srate(srslte_rf_t *rf)
{
  return ((rf_dev_t*) rf->dev)->srslte_rf_get_rf_mon_srate(rf->handler);
}

//******************************************************************************
// Functions only used with Channel Emulator.
//******************************************************************************
void rf_channel_emulator_add_awgn(srslte_rf_t *rf, float noise_floor, float SNRdB) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.add_awgn_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, noise_floor, SNRdB);
}

void rf_channel_emulator_add_simple_awgn(srslte_rf_t *rf, float noise_variance) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.add_simple_awgn_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, noise_variance);
}

void rf_channel_emulator_add_carrier_offset(srslte_rf_t *rf, float dphi, float phi) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.add_carrier_offset_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, dphi, phi);
}

void rf_channel_emulator_add_multipath(srslte_rf_t *rf, cf_t *hc, unsigned int hc_len) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.add_multipath_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, hc, hc_len);
}

void rf_channel_emulator_add_shadowing(srslte_rf_t *rf, float sigma, float fd) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.add_shadowing_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, sigma, fd);
}

void rf_channel_emulator_print_channel(srslte_rf_t *rf) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.print_channel_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator);
}

void rf_channel_emulator_estimate_psd(srslte_rf_t *rf, cf_t *sig, uint32_t nof_samples, unsigned int nfft, float *psd) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.estimate_psd_func_ptr(sig, nof_samples, nfft, psd);
}

void rf_channel_emulator_create_psd_script(srslte_rf_t *rf, unsigned int nfft, float *psd) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.create_psd_script_func_ptr(nfft, psd);
}

void rf_channel_emulator_set_channel_impairments(srslte_rf_t *rf, bool flag) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->set_channel_impairments_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, flag);
}

void rf_channel_emulator_set_simple_awgn_channel(srslte_rf_t *rf, bool flag) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->set_channel_simple_awgn_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, flag);
}

void rf_channel_emulator_set_cfo_freq(srslte_rf_t *rf, float cfo_freq) {
  ((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator->channel_impairments.set_cfo_freq_func_ptr(((rf_ch_emulator_handler_t*) rf->handler)->ch_emulator, cfo_freq);
}

void rf_channel_emulator_stop_running(srslte_rf_t *rf, size_t channel) {
#ifdef ENABLE_CH_EMULATOR
  rf_ch_emulator_stop_running((void *)rf->handler, channel);
#endif
}
