#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "rf_ch_emulator_imp.h"
#include "srslte/srslte.h"
#include "srslte/rf/rf.h"

void rf_ch_emulator_suppress_stdout(void *h) {

}

void rf_ch_emulator_register_error_handler(void *h, srslte_rf_error_handler_t new_handler) {

}

char* rf_ch_emulator_devname(void* h) {
  return DEVNAME_X300;
}

bool rf_ch_emulator_rx_wait_lo_locked(void *h, size_t channel) {
  return true;
}

bool rf_ch_emulator_tx_wait_lo_locked(void *h, size_t channel) {
  return true;
}

void rf_ch_emulator_set_tx_cal(void *h, srslte_rf_cal_t *cal) {

}

void rf_ch_emulator_set_rx_cal(void *h, srslte_rf_cal_t *cal) {

}

int rf_ch_emulator_start_rx_stream(void *h, size_t channel) {
  return 0;
}

int rf_ch_emulator_stop_rx_stream(void *h, size_t channel) {
  return 0;
}

void rf_ch_emulator_flush_buffer(void *h, size_t channel) {

}

bool rf_ch_emulator_has_rssi(void *h, size_t channel) {
  return false;
}

bool get_has_rssi(void *h, size_t channel) {
  return false;
}

float rf_ch_emulator_get_rssi(void *h, size_t channel) {
  return 0.0;
}

int rf_ch_emulator_open(char *args, void **h) {
  if(h) {
    *h = NULL;

    rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*)malloc(sizeof(rf_ch_emulator_handler_t));
    if(!handler) {
      CH_EMULATOR_ERROR("Error allocating memory for handler",0);
      return -1;
    }
    // Intialize structure.
    bzero(handler, sizeof(rf_ch_emulator_handler_t));
    // Return pointer to handler.
    *h = handler;

    handler->ch_emulator = (channel_emulator_t*)malloc(sizeof(channel_emulator_t));
    if(!handler->ch_emulator) {
      CH_EMULATOR_ERROR("Error allocating memory for ch_emulator",0);
      return -1;
    }
    // Intialize structure.
    bzero(handler->ch_emulator, sizeof(channel_emulator_t));

    // Set priority to UHD threads
    uhd_set_thread_priority(1.0, true);

    handler->dynamic_rate = false;
    handler->devname = DEVNAME_X300;
    handler->num_of_channels = 1;

    CH_EMULATOR_PRINT("Initializing Channel Emulator...\n",0);

    int ret = channel_emulator_initialization(handler->ch_emulator);

    CH_EMULATOR_PRINT("Channel Emulator initialized.\n",0);

    handler->is_emulator_running = true;

    return ret;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}

int rf_ch_emulator_close(void *h) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  handler->is_emulator_running = false;
  int ret = channel_emulator_uninitialization(handler->ch_emulator);
  // Free channel emulator.
  if(handler->ch_emulator != NULL) {
    free(handler->ch_emulator);
    handler->ch_emulator = NULL;
  }
  // Free handler.
  if(handler != NULL) {
    free(handler);
    handler = NULL;
  }
  CH_EMULATOR_INFO("Channel Emulator uninitialized.\n",0);
  return ret;
}

void rf_ch_emulator_stop_running(void *h, size_t channel) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  handler->is_emulator_running = false;
}

void rf_ch_emulator_set_master_clock_rate(void *h, double rate) {

}

bool rf_ch_emulator_is_master_clock_dynamic(void *h) {
  return false;
}

double rf_ch_emulator_set_rx_srate(void *h, double freq, size_t channel) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  int ret = channel_emulator_set_subframe_length(handler->ch_emulator, freq);
  RF_CH_EMULATOR_PRINT("[Rx] Subframe length set to: %d\n", ret);
  if(ret <= 0) {
    return -1.0;
  }
  return freq;
}

double rf_ch_emulator_get_rx_srate(void *h, size_t channel) {
  return 0.0;
}

double rf_ch_emulator_set_tx_srate(void *h, double freq, size_t channel) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  int ret = channel_emulator_set_subframe_length(handler->ch_emulator, freq);
  RF_CH_EMULATOR_PRINT("[Tx] Subframe length set to: %d\n", ret);
  if(ret <= 0) {
    return -1.0;
  }
  return freq;
}

double rf_ch_emulator_set_rx_gain(void *h, double gain, size_t channel) {
  return gain;
}

double rf_ch_emulator_set_tx_gain(void *h, double gain, size_t channel) {
  return gain;
}

double rf_ch_emulator_get_rx_gain(void *h, size_t channel) {
  return 0.0;
}

double rf_ch_emulator_get_tx_gain(void *h, size_t channel) {
  return 0.0;
}

double rf_ch_emulator_set_rx_freq2(void *h, double freq, double lo_off, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_rx_freq(void *h, double freq, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_freq2(void *h, double freq, double lo_off, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_freq(void *h, double freq, size_t channel) {
  return freq;
}

double rf_ch_emulator_get_tx_freq(void *h, size_t channel) {
  return 0.0;
}

double rf_ch_emulator_get_rx_freq(void *h, size_t channel) {
  return 0.0;
}

void rf_ch_emulator_get_time(void *h, time_t *secs, double *frac_secs) {
  *secs = 0;
  *frac_secs = 0.0;
}

int rf_ch_emulator_recv_with_time(void *h,
                    void *data,
                    uint32_t nof_samples,
                    bool blocking,
                    time_t *secs,
                    double *frac_secs,
                    size_t channel) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  int ret = 0;
  do {
    ret = channel_emulator_recv(handler->ch_emulator, data, nof_samples, blocking, channel);
  } while(ret < 0 && blocking && handler->is_emulator_running);

  return ret;
}

int rf_ch_emulator_recv_channel_with_time(void *h,
                    void *data,
                    uint32_t nof_samples,
                    bool blocking,
                    time_t *secs,
                    double *frac_secs) {
  rf_ch_emulator_handler_t *handler = (rf_ch_emulator_handler_t*) h;
  int ret = 0;
  do {
    ret = channel_emulator_recv(handler->ch_emulator, data, nof_samples, blocking, 1);
  } while(ret < 0 && blocking && handler->is_emulator_running);

  return ret;
}

int rf_ch_emulator_send_timed(void *h,
                     void *data,
                     int nof_samples,
                     time_t secs,
                     double frac_secs,
                     bool has_time_spec,
                     bool blocking,
                     bool is_start_of_burst,
                     bool is_end_of_burst,
                     bool is_lbt_enabled,
                     void *lbt_stats_void_ptr,
                     size_t channel) {
  rf_ch_emulator_handler_t* handler = (rf_ch_emulator_handler_t*) h;
  int ret = channel_emulator_send(handler->ch_emulator, data, nof_samples, blocking, is_start_of_burst, is_end_of_burst, channel);
  return ret;
}

int rf_ch_emulator_check_async_msg(rf_ch_emulator_handler_t* handler, size_t channel) {
  return 0;
}

bool rf_ch_emulator_is_burst_transmitted(void *h, size_t channel) {
  return true;
}

void rf_ch_emulator_set_time_now(void *h, time_t full_secs, double frac_secs) {

}

double rf_ch_emulator_set_tx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_freq_and_gain_cmd(void *h, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_gain_cmd(void *h, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return gain;
}

double rf_ch_emulator_set_rx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}


void rf_ch_emulator_rf_monitor_initialize(void *h, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size) {

}

void rf_ch_emulator_rf_monitor_uninitialize(void *h) {

}

double rf_ch_emulator_set_rf_mon_srate(void *h, double srate) {
  return srate;
}

double rf_ch_emulator_get_rf_mon_srate(void *h) {
  return 0.0;
}

double rf_ch_emulator_set_tx_channel_freq(void *h, double freq, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_rx_channel_freq(void *h, double freq, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_rx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}

double rf_ch_emulator_set_tx_channel_freq_and_gain_cmd(void *h, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  return freq;
}
