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
 *  File:         sync.h
 *
 *  Description:  Time and frequency synchronization using the PSS and SSS signals.
 *
 *                The object is designed to work with signals sampled at 1.92 Mhz
 *                centered at the carrier frequency. Thus, downsampling is required
 *                if the signal is sampled at higher frequencies.
 *
 *                Correlation peak is detected comparing the maximum at the output
 *                of the correlator with a threshold. The comparison accepts two
 *                modes: absolute value or peak-to-mean ratio, which are configured
 *                with the functions sync_pss_det_absolute() and sync_pss_det_peakmean().
 *
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 6.11.1, 6.11.2
 *****************************************************************************/

#ifndef _SYNC_
#define _SYNC_

#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <complex.h>

#include "srslte/config.h"
#include "srslte/sync/pss.h"
#include "srslte/sync/sss.h"
#include "srslte/sync/cp.h"
#include "srslte/sync/cfo.h"
#include "srslte/utils/debug.h"
#include "srslte/common/phy_common.h"
#include "srslte/utils/vector.h"
#include "srslte/io/filesink.h"

#include "../../../examples/transceiver.h"

// ****************** Definition of macros ***********************
#define ENABLE_SYNC_PRINTS 1

#define SYNC_PRINT(_fmt, ...) do { if(ENABLE_SYNC_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[SYNC PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define SYNC_DEBUG(_fmt, ...) do { if(ENABLE_SYNC_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[SYNC DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define SYNC_INFO(_fmt, ...) do { if(ENABLE_SYNC_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[SYNC INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define SYNC_ERROR(_fmt, ...) do { fprintf(stdout, "[SYNC ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define SRSLTE_SYNC_FFT_SZ_MIN    64
#define SRSLTE_SYNC_FFT_SZ_MAX    2048

#define NUMBER_OF_SUBFRAMES_TO_STORE 3 // Number of subframes to store in the read buffer.

#define MAXIMUM_NUMBER_OF_SUBFRAMES 30

#define USE_FINE_GRAINED_CFO 1

typedef enum {SSS_DIFF=0, SSS_PARTIAL_3=2, SSS_FULL=1} sss_alg_t;

typedef enum SRSLTE_API {CFO_CORRECTION_PSS=0, CFO_CORRECTION_CP}  srslte_sync_cfo_correction_type_t;

typedef struct SRSLTE_API {
  srslte_pss_synch_t pss;
  srslte_pss_synch_t pss_i[2];
  srslte_sss_synch_t sss;
  srslte_cp_synch_t cp_synch;
  cf_t *cfo_i_corr[2];

  float threshold;
  float peak_value;
  uint32_t N_id_2; // Received/Detected N_id_2
  uint32_t N_id_1; // Received/Detected N_id_1
  uint32_t sf_idx;
  uint32_t fft_size;
  uint32_t frame_size;
  uint32_t max_offset;
  bool enable_cfo_corr;
  float mean_cfo;
  float mean_cfo2;
  int cfo_i;
  bool find_cfo_i;
  bool find_cfo_i_initiated;
  float cfo_ema_alpha;
  uint32_t nof_symbols;
  uint32_t cp_len;
  srslte_cfo_t cfocorr;
  srslte_cfo_t cfocorr2;
  srslte_cfo_t cfocorr_finer;
  sss_alg_t sss_alg;
  bool detect_cp;
  bool sss_en;
  srslte_cp_t cp;
  uint32_t m0;
  uint32_t m1;
  float m0_value;
  float m1_value;
  float M_norm_avg;
  float M_ext_avg;
  cf_t  *temp;
  uint32_t node_N_id_1; // Node's N_id_1
  uint32_t node_N_id_2; // Node's N_id_2
  int initial_subframe_index; // Subframe index to look for.
  struct timespec peak_detection_timestamp; // timestamp used to calculate time between detection and sending data to upper layer.
  uint32_t nof_subframes_to_rx;
  uint32_t cfo_correction_type;
  bool decode_pdcch;
  uint32_t node_id;
  uint32_t intf_id;
  uint32_t mcs;
  uint32_t nof_prb;
  bool phy_filtering;
  bool use_scatter_sync_seq;
  uint32_t pss_len;
  bool enable_second_stage_pss_detection;
} srslte_sync_t;

typedef enum {
  SRSLTE_SYNC_FOUND = 1,
  SRSLTE_SYNC_FOUND_NOSPACE = 2,
  SRSLTE_SYNC_NOFOUND = 0,
  SRSLTE_SYNC_ERROR = -1
} srslte_sync_find_ret_t;

SRSLTE_API int srslte_sync_init_reentry(srslte_sync_t *q,
                                uint32_t frame_size,
                                uint32_t max_offset,
                                uint32_t fft_size);

SRSLTE_API int srslte_sync_init(srslte_sync_t *q,
                                uint32_t frame_size,
                                uint32_t max_offset,
                                uint32_t fft_size,
                                uint32_t cell_id,
                                int initial_subframe_index);

SRSLTE_API void srslte_sync_free_reentry(srslte_sync_t *q);

SRSLTE_API void srslte_sync_free_except_reentry(srslte_sync_t *q);

SRSLTE_API void srslte_sync_free(srslte_sync_t *q);

SRSLTE_API void srslte_sync_reset(srslte_sync_t *q);

/* Finds a correlation peak in the input signal around position find_offset */
SRSLTE_API srslte_sync_find_ret_t srslte_sync_find(srslte_sync_t *q,
                                                   cf_t *input,
                                                   int find_offset,
                                                   uint32_t *peak_position);

/* Estimates the CP length */
SRSLTE_API srslte_cp_t srslte_sync_detect_cp(srslte_sync_t *q,
                                             cf_t *input,
                                             uint32_t peak_pos);

/* Sets the threshold for peak comparison */
SRSLTE_API void srslte_sync_set_threshold(srslte_sync_t *q,
                                          float threshold);

/* Gets the subframe idx (0 or 5) */
SRSLTE_API uint32_t srslte_sync_get_sf_idx(srslte_sync_t *q);

/* Gets the last peak value */
SRSLTE_API float srslte_sync_get_last_peak_value(srslte_sync_t *q);

/* Gets the mean peak value */
SRSLTE_API float srslte_sync_get_peak_value(srslte_sync_t *q);

/* Choose SSS detection algorithm */
SRSLTE_API void srslte_sync_set_sss_algorithm(srslte_sync_t *q,
                                              sss_alg_t alg);

/* Sets PSS exponential averaging alpha weight */
SRSLTE_API void srslte_sync_set_em_alpha(srslte_sync_t *q,
                                         float alpha);

/* Sets the N_id_2 to search for */
SRSLTE_API int srslte_sync_set_N_id_2(srslte_sync_t *q,
                                      uint32_t N_id_2);

/* Gets the Physical CellId from the last call to synch_run() */
SRSLTE_API int srslte_sync_get_cell_id(srslte_sync_t *q);

/* Gets the CFO estimation from the last call to synch_run() */
SRSLTE_API float srslte_sync_get_cfo_new(srslte_sync_t *q);

/* Gets the CFO estimation from the last call to synch_run() */
SRSLTE_API float srslte_sync_get_cfo(srslte_sync_t *q);

/* Sets known CFO to avoid long transients due to average */
SRSLTE_API void srslte_sync_set_cfo(srslte_sync_t *q, float cfo);

/* Set integer CFO */
SRSLTE_API void srslte_sync_set_cfo_i(srslte_sync_t *q,
                                      int cfo_i);

SRSLTE_API void srslte_sync_set_cfo_enable(srslte_sync_t *q,
                                           bool enable);

/* Sets the exponential moving average coefficient for CFO averaging */
SRSLTE_API void srslte_sync_set_cfo_ema_alpha(srslte_sync_t *q,
                                              float alpha);

/* Gets the CP length estimation from the last call to synch_run() */
SRSLTE_API srslte_cp_t srslte_sync_get_cp(srslte_sync_t *q);

/* Sets the CP length estimation (must do it if disabled) */
SRSLTE_API void srslte_sync_set_cp(srslte_sync_t *q,
                                   srslte_cp_t cp);

/* Enable integer CFO detection */
SRSLTE_API void srslte_sync_cfo_i_detec_en(srslte_sync_t *q,
                                           bool enabled);

/* Enables/Disables SSS detection  */
SRSLTE_API void srslte_sync_sss_en(srslte_sync_t *q,
                                   bool enabled);

SRSLTE_API srslte_pss_synch_t* srslte_sync_get_cur_pss_obj(srslte_sync_t *q);

SRSLTE_API bool srslte_sync_sss_detected(srslte_sync_t *q);

SRSLTE_API bool srslte_sync_sss_is_en(srslte_sync_t *q);

/* Enables/Disables CP detection  */
SRSLTE_API void srslte_sync_cp_en(srslte_sync_t *q,
                                  bool enabled);

SRSLTE_API float srslte_sync_cfo_estimate_cp(srslte_sync_t *q, cf_t *input);

SRSLTE_API void srslte_sync_set_cfo_correction_type(srslte_sync_t *q, uint32_t cfo_correction_type);

SRSLTE_API uint32_t srslte_sync_get_cfo_correction_type(srslte_sync_t *q);

SRSLTE_API void srslte_sync_set_cfo_new(srslte_sync_t *q, float cfo);

SRSLTE_API srslte_sync_find_ret_t srslte_sync_find_legacy(srslte_sync_t *q, cf_t *input, int find_offset, uint32_t *peak_position);

//******************************************************************************
//****************** Scatter system's customized functions *********************
//******************************************************************************
SRSLTE_API int srslte_sync_init_generic(srslte_sync_t *q, uint32_t frame_size, uint32_t max_offset, uint32_t fft_size, uint32_t cell_id, int initial_subframe_index, bool decode_pdcch, uint32_t node_id, uint32_t intf_id, bool phy_filtering, bool use_scatter_sync_seq, uint32_t pss_len, bool enable_second_stage_pss_detection);

SRSLTE_API srslte_sync_find_ret_t srslte_sync_find_sch(srslte_sync_t *q, cf_t *input, int find_offset, uint32_t *peak_position);

SRSLTE_API int srslte_sync_decode_sch(srslte_sync_t *q, cf_t *input, bool decode_radio_id, uint32_t peak_pos, uint32_t *value0, uint32_t *value1, uint32_t* sch_value);

SRSLTE_API void srslte_sync_set_avg_psr_scatter(srslte_sync_t *q, bool enable_avg_psr);

SRSLTE_API void srslte_sync_enable_second_stage_detection_scatter(srslte_sync_t *q, bool enable_second_stage_pss_detection);

SRSLTE_API void srslte_sync_set_pss_1st_stage_threshold_scatter(srslte_sync_t *q, float pss_first_stage_threshold);

SRSLTE_API void srslte_sync_set_pss_2nd_stage_threshold_scatter(srslte_sync_t *q, float pss_second_stage_threshold);

#endif // _SYNC_
