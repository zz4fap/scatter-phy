#ifndef _RF_CH_EMULATOR_IMP_H_
#define _RF_CH_EMULATOR_IMP_H_

#include <stdbool.h>
#include <stdint.h>

#include <uhd.h>
#include "srslte/config.h"
#include "srslte/rf/rf.h"

#include "../../examples/helpers.h"

#define DEVNAME_B200 "uhd_b200"
#define DEVNAME_X300 "uhd_x300"

#define ENABLE_RF_CH_EMULATOR_PRINTS 1

#define RF_CH_EMULATOR_PRINT(_fmt, ...) do { if(ENABLE_RF_CH_EMULATOR_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[RF CH EMULATOR PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define RF_CH_EMULATOR_DEBUG(_fmt, ...) do { if(ENABLE_RF_CH_EMULATOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[RF CH EMULATOR DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define RF_CH_EMULATOR_INFO(_fmt, ...) do { if(ENABLE_RF_CH_EMULATOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[RF CH EMULATOR INFO]: " _fmt, __VA_ARGS__); } while(0)

#define RF_CH_EMULATOR_ERROR(_fmt, ...) do { fprintf(stdout, "[RF CH EMULATOR ERROR]: " _fmt, __VA_ARGS__); } while(0)

// Struct used to store data related to each one of the USRP channels.
typedef struct {
  size_t rx_nof_samples;
  size_t tx_nof_samples;
  double tx_rate;
  bool has_rssi;
} rf_ch_emulator_channel_handler_t;

typedef struct {
  char *devname;                    // Field not channel dependent
  channel_emulator_t *ch_emulator;  // Field not channel dependent
  bool dynamic_rate;                // Field not channel dependent
  size_t num_of_channels;           // USRP's number of supported and opened channels.
  bool is_emulator_running;         // Flag used to check if the emulator is still running.
} rf_ch_emulator_handler_t;

SRSLTE_API int rf_ch_emulator_open(char *args, void **handler);

SRSLTE_API char* rf_ch_emulator_devname(void *h);

SRSLTE_API int rf_ch_emulator_close(void *h);

SRSLTE_API void rf_ch_emulator_set_tx_cal(void *h, srslte_rf_cal_t *cal);

SRSLTE_API void rf_ch_emulator_set_rx_cal(void *h, srslte_rf_cal_t *cal);

SRSLTE_API int rf_ch_emulator_start_rx_stream(void *h, size_t channel);

SRSLTE_API int rf_ch_emulator_start_rx_stream_nsamples(void *h, uint32_t nsamples);

SRSLTE_API int rf_ch_emulator_stop_rx_stream(void *h, size_t channel);

SRSLTE_API void rf_ch_emulator_flush_buffer(void *h, size_t channel);

SRSLTE_API bool rf_ch_emulator_has_rssi(void *h, size_t channel);

SRSLTE_API float rf_ch_emulator_get_rssi(void *h, size_t channel);

SRSLTE_API bool rf_ch_emulator_rx_wait_lo_locked(void *h, size_t channel);

SRSLTE_API bool rf_ch_emulator_tx_wait_lo_locked(void *h, size_t channel);

SRSLTE_API void rf_ch_emulator_set_master_clock_rate(void *h, double rate);

SRSLTE_API bool rf_ch_emulator_is_master_clock_dynamic(void *h);

SRSLTE_API double rf_ch_emulator_set_rx_srate(void *h, double freq, size_t channel);

SRSLTE_API double rf_ch_emulator_get_rx_srate(void *h, size_t channel);

SRSLTE_API double rf_ch_emulator_set_rx_gain(void *h, double gain, size_t channel);

SRSLTE_API double rf_ch_emulator_get_rx_gain(void *h, size_t channel);

SRSLTE_API double rf_ch_emulator_get_tx_gain(void *h, size_t channel);

SRSLTE_API void rf_ch_emulator_suppress_stdout(void *h);

SRSLTE_API void rf_ch_emulator_register_error_handler(void *h, srslte_rf_error_handler_t error_handler);

SRSLTE_API double rf_ch_emulator_set_rx_freq(void *h, double freq, size_t channel);

SRSLTE_API int rf_ch_emulator_recv_with_time(void *h,
                                  void *data,
                                  uint32_t nsamples,
                                  bool blocking,
                                  time_t *secs,
                                  double *frac_secs,
                                  size_t channel);

SRSLTE_API int rf_ch_emulator_recv_channel_with_time(void *h,
                                            void *data,
                                            uint32_t nsamples,
                                            bool blocking,
                                            time_t *secs,
                                            double *frac_secs);

SRSLTE_API double rf_ch_emulator_set_tx_srate(void *h, double freq, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_gain(void *h, double gain, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_ch_emulator_get_tx_freq(void *h, size_t channel);

SRSLTE_API double rf_ch_emulator_get_rx_freq(void *h, size_t channel);

SRSLTE_API void rf_ch_emulator_get_time(void *h,
                              time_t *secs,
                              double *frac_secs);

SRSLTE_API int  rf_ch_emulator_send_timed(void *h,
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

SRSLTE_API int rf_ch_emulator_check_async_msg(rf_ch_emulator_handler_t* handler, size_t channel);

SRSLTE_API bool rf_ch_emulator_is_burst_transmitted(void *h, size_t channel);

SRSLTE_API void rf_ch_emulator_set_time_now(void *h, time_t full_secs, double frac_secs);

SRSLTE_API double rf_ch_emulator_set_tx_freq2(void *h, double freq, double lo_off, size_t channel);

SRSLTE_API double rf_ch_emulator_set_rx_freq2(void *h, double freq, double lo_off, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_ch_emulator_set_rx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_gain_cmd(void *h, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_freq_and_gain_cmd(void *h, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API void rf_ch_emulator_rf_monitor_initialize(void *h, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size);

SRSLTE_API void rf_ch_emulator_rf_monitor_uninitialize(void *h);

SRSLTE_API double rf_ch_emulator_set_rf_mon_srate(void *h, double srate);

SRSLTE_API double rf_ch_emulator_get_rf_mon_srate(void *h);

SRSLTE_API double rf_ch_emulator_set_tx_channel_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_ch_emulator_set_rx_channel_freq(void *h, double freq, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_ch_emulator_set_rx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API double rf_ch_emulator_set_tx_channel_freq_and_gain_cmd(void *h, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel);

SRSLTE_API void rf_ch_emulator_stop_running(void *h, size_t channel);

#endif //_RF_CH_EMULATOR_IMP_H_
