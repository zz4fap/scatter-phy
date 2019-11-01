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

/******************************************************************************
 *  File:         pss.h
 *
 *  Description:  Primary synchronization signal (PSS) generation and detection.
 *
 *                The srslte_pss_synch_t object provides functions for fast
 *                computation of the crosscorrelation between the PSS and received
 *                signal and CFO estimation. Also, the function srslte_pss_synch_tperiodic()
 *                is designed to be called periodically every subframe, taking
 *                care of the correct data alignment with respect to the PSS sequence.
 *
 *                The object is designed to work with signals sampled at 1.92 Mhz
 *                centered at the carrier frequency. Thus, downsampling is required
 *                if the signal is sampled at higher frequencies.
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 6.11.1
 *****************************************************************************/

#ifndef PSS_
#define PSS_

#include <stdint.h>
#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/common/phy_common.h"
#include "srslte/utils/convolution.h"

#define CONVOLUTION_FFT

#define SRSLTE_PSS_LEN            62
#define SRSLTE_PSS_GUARD_BAND_LEN 10
#define SRSLTE_TOTAL_PSS_LEN      (SRSLTE_PSS_LEN+SRSLTE_PSS_GUARD_BAND_LEN)
#define SRSLTE_PSS_RE             (6*12)

#define PSS_ABS_PEAK_MAX          3.0

/* PSS processing options */

#define SRSLTE_PSS_ACCUMULATE_ABS   // If enabled, accumulates the correlation absolute value on consecutive calls to srslte_pss_synch_find_pss

#define SRSLTE_PSS_ABS_SQUARE   // If enabled, compute abs square, otherwise computes absolute value only

#define SRSLTE_PSS_RETURN_PSR  // If enabled returns peak to side-lobe ratio, otherwise returns absolute peak value


/* Low-level API */
typedef struct SRSLTE_API {

  srslte_dft_plan_t dftp_input;

#ifdef CONVOLUTION_FFT
  srslte_conv_fft_cc_t conv_fft;
#endif

  cf_t *rx_pss_fft;

  uint32_t frame_size;
  uint32_t N_id_2;
  uint32_t fft_size;

  cf_t *pss_signal_time[3];
  cf_t *pss_signal_freq[3]; // One sequence for each N_id_2
  cf_t *tmp_input;
  cf_t *conv_output;
  float *conv_output_abs;
  float ema_alpha;
  float *conv_output_avg;
  float *conv_output_avg_plot;
  float peak_value;
  bool enable_avg_psr;
  // 2nd stage PSS detection.
  bool enable_second_stage_pss_detection;
  float pss_first_stage_threshold;
  float pss_second_stage_threshold;
  cf_t *tmp_input_2nd_stage;
  cf_t *conv_output_2nd_stage;
  float *conv_output_avg_2nd_stage;
  float pss_mean_abs_1st_stage_peak;
  uint32_t peak_1st_stage_cnt;
  uint32_t peak_1st_stage_window_len;
} srslte_pss_synch_t;

typedef enum { PSS_TX, PSS_RX } pss_direction_t;

/* Basic functionality */
SRSLTE_API int srslte_pss_synch_init_fft(srslte_pss_synch_t *q,
                                         uint32_t frame_size,
                                         uint32_t fft_size);

SRSLTE_API int srslte_pss_synch_init_fft_offset(srslte_pss_synch_t *q,
                                                uint32_t frame_size,
                                                uint32_t fft_size,
                                                int cfo_i);

SRSLTE_API int srslte_pss_synch_init(srslte_pss_synch_t *q,
                                     uint32_t frame_size);

SRSLTE_API void srslte_pss_synch_free(srslte_pss_synch_t *q);

SRSLTE_API void srslte_pss_synch_reset(srslte_pss_synch_t *q);

SRSLTE_API int srslte_pss_generate(cf_t *signal,
                                   uint32_t N_id_2);

SRSLTE_API void srslte_pss_get_slot(cf_t *slot,
                                    cf_t *pss_signal,
                                    uint32_t nof_prb,
                                    srslte_cp_t cp);

SRSLTE_API void srslte_pss_put_slot(cf_t *pss_signal,
                                    cf_t *slot,
                                    uint32_t nof_prb,
                                    srslte_cp_t cp);

SRSLTE_API void srslte_pss_synch_set_ema_alpha(srslte_pss_synch_t *q,
                                               float alpha);

SRSLTE_API int srslte_pss_synch_set_N_id_2(srslte_pss_synch_t *q,
                                           uint32_t N_id_2);

SRSLTE_API int srslte_pss_synch_find_pss(srslte_pss_synch_t *q,
                                         cf_t *input,
                                         float *corr_peak_value);

SRSLTE_API int srslte_pss_synch_chest(srslte_pss_synch_t *q,
                                      cf_t *input,
                                      cf_t ce[SRSLTE_PSS_LEN]);

SRSLTE_API float srslte_pss_synch_cfo_compute(srslte_pss_synch_t* q,
                                              cf_t *pss_recv);

SRSLTE_API int srslte_pss_synch_init_fft_offset_generic(srslte_pss_synch_t *q, uint32_t frame_size, uint32_t fft_size, int offset, bool use_scatter_sync_seq, uint32_t pss_len, bool enable_second_stage_pss_detection);

//---------------------------------------------------------
//******* Scatter System Customized Implementations *******
//---------------------------------------------------------
SRSLTE_API float srslte_pss_synch_cfo_compute2(srslte_pss_synch_t* q, cf_t *pss_recv);

SRSLTE_API float srslte_pss_synch_acc_noise(srslte_pss_synch_t *q);

SRSLTE_API int srslte_pss_synch_chest_generic(srslte_pss_synch_t *q, cf_t *input, cf_t *ce, uint32_t len, uint32_t offset);

SRSLTE_API int srslte_pss_synch_find_pss_scatter(srslte_pss_synch_t *q, cf_t *input, float *corr_peak_value);

SRSLTE_API int srslte_pss_generate_scatter(cf_t *signal, uint32_t N_id_2, uint32_t length);

SRSLTE_API void srslte_pss_put_slot_scatter(cf_t *pss_signal, cf_t *slot, uint32_t nof_prb, srslte_cp_t cp, uint32_t seq_len);

SRSLTE_API void srslte_pss_get_slot_scatter(cf_t *slot, cf_t *pss_signal, uint32_t nof_prb, srslte_cp_t cp, uint32_t seq_len);

SRSLTE_API int srslte_pss_synch_init_N_id_2_scatter(cf_t *pss_signal_freq, cf_t *pss_signal_time, uint32_t N_id_2, uint32_t fft_size, int cfo_i, uint32_t seq_len);

SRSLTE_API void srslte_pss_set_avg_psr_scatter(srslte_pss_synch_t *q, bool enable_avg_psr);

SRSLTE_API float srslte_pss_get_psr_scatter(float *conv_output_avg, uint32_t corr_peak_pos, uint32_t conv_output_len, bool enable_avg_psr);

SRSLTE_API void srslte_pss_enable_second_stage_detection_scatter(srslte_pss_synch_t *q, bool enable_second_stage_pss_detection);

SRSLTE_API void srslte_pss_set_1st_stage_threshold_scatter(srslte_pss_synch_t *q, float pss_first_stage_threshold);

SRSLTE_API void srslte_pss_set_2nd_stage_threshold_scatter(srslte_pss_synch_t *q, float pss_second_stage_threshold);

#endif // PSS_
