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
#include "srslte/sync/sync.h"

#define MEANPEAK_EMA_ALPHA      0.1
#define CFO_EMA_ALPHA           0.1
#define CP_EMA_ALPHA            0.1

static bool fft_size_isvalid(uint32_t fft_size) {
  if (fft_size >= SRSLTE_SYNC_FFT_SZ_MIN && fft_size <= SRSLTE_SYNC_FFT_SZ_MAX && (fft_size%64) == 0) {
    return true;
  } else {
    return false;
  }
}

int srslte_sync_init_reentry(srslte_sync_t *q, uint32_t frame_size, uint32_t max_offset, uint32_t fft_size) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL         &&
      frame_size        <= 307200       &&
      fft_size_isvalid(fft_size))
  {
    ret = SRSLTE_ERROR;

    for (int i=0;i<2;i++) {
      q->cfo_i_corr[i] = srslte_vec_malloc(sizeof(cf_t)*q->frame_size);
      if (!q->cfo_i_corr[i]) {
        perror("malloc");
        goto clean_exit;
      }
    }

    DEBUG("SYNC init with frame_size=%d, max_offset=%d and fft_size=%d\n", frame_size, max_offset, fft_size);

    ret = SRSLTE_SUCCESS;
  }  else {
    fprintf(stderr, "Invalid parameters frame_size: %d, fft_size: %d\n", frame_size, fft_size);
  }

clean_exit:
  if (ret == SRSLTE_ERROR) {
    srslte_sync_free(q);
  }
  return ret;
}

int srslte_sync_init(srslte_sync_t *q, uint32_t frame_size, uint32_t max_offset, uint32_t fft_size, uint32_t cell_id, int initial_subframe_index) {
  return srslte_sync_init_generic(q, frame_size, max_offset, fft_size, cell_id, initial_subframe_index, true, 0, 0, false, false, SRSLTE_PSS_LEN, false);
}

int srslte_sync_init_generic(srslte_sync_t *q, uint32_t frame_size, uint32_t max_offset, uint32_t fft_size, uint32_t cell_id, int initial_subframe_index, bool decode_pdcch, uint32_t node_id, uint32_t intf_id, bool phy_filtering, bool use_scatter_sync_seq, uint32_t pss_len, bool enable_second_stage_pss_detection) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL         &&
      frame_size        <= 307200       &&
      fft_size_isvalid(fft_size)        &&
      cell_id          <= MAXIMUM_NUMBER_OF_CELLS)
  {
    ret = SRSLTE_ERROR;

    bzero(q, sizeof(srslte_sync_t));
    q->detect_cp = true;
    q->sss_en = true;
    q->mean_cfo = 0;
    q->mean_cfo2 = 0;
    q->N_id_2 = 1000;
    q->N_id_1 = 1000;
    q->cfo_i = 0;
    q->find_cfo_i = false;
    q->find_cfo_i_initiated = false;
    q->cfo_ema_alpha = CFO_EMA_ALPHA;
    q->fft_size = fft_size;
    q->frame_size = frame_size;
    q->max_offset = max_offset;
    q->sss_alg = SSS_FULL;
    q->node_N_id_1 = cell_id/3;
    q->node_N_id_2 = cell_id%3;
    q->initial_subframe_index = initial_subframe_index;
    q->nof_subframes_to_rx = 0;
    q->cfo_correction_type = CFO_CORRECTION_CP;
    // Values used for SCH decoding.
    q->decode_pdcch = decode_pdcch;
    q->node_id = node_id;
    q->intf_id = intf_id;
    q->mcs = 0;
    q->nof_prb = srslte_nof_prb(q->fft_size);
    q->phy_filtering = phy_filtering;
    q->use_scatter_sync_seq = use_scatter_sync_seq;
    q->pss_len = pss_len;
    q->enable_second_stage_pss_detection = enable_second_stage_pss_detection;

    q->enable_cfo_corr = false;
    if(srslte_cfo_init(&q->cfocorr, q->frame_size)) {
      fprintf(stderr, "Error initiating CFO\n");
      goto clean_exit;
    }
    // Set a CFO tolerance of approx 50 Hz.
    srslte_cfo_set_tol(&q->cfocorr, 50.0/(15000.0*q->fft_size));

    if(srslte_cfo_init(&q->cfocorr2, q->frame_size)) {
      fprintf(stderr, "Error initiating CFO\n");
      goto clean_exit;
    }
    // Set a CFO tolerance of approx 50 Hz.
    srslte_cfo_set_tol(&q->cfocorr2, 50.0/(15000.0*q->fft_size));

#if(USE_FINE_GRAINED_CFO==1)
    // This is a more fine-grained CFO correction frequency generator.
    if(srslte_cfo_init_finer(&q->cfocorr_finer, q->frame_size)) {
      fprintf(stderr, "Error initiating finer CFO.\n");
      goto clean_exit;
    }
    // Set the FFT size.
    srslte_cfo_set_fft_size_finer(&q->cfocorr_finer, q->fft_size);
    // Set a CFO tolerance of approx 50 Hz.
    srslte_cfo_set_tol_finer(&q->cfocorr_finer, 50.0/(15000.0*q->fft_size));
#endif

    for(int i = 0; i < 2; i++) {
      q->cfo_i_corr[i] = srslte_vec_malloc(sizeof(cf_t)*q->frame_size);
      if(!q->cfo_i_corr[i]) {
        perror("malloc");
        goto clean_exit;
      }
    }

    q->temp = srslte_vec_malloc(NUMBER_OF_SUBFRAMES_TO_STORE*sizeof(cf_t)*q->frame_size);
    if(!q->temp) {
      perror("malloc");
      goto clean_exit;
    }

    srslte_sync_set_cp(q, SRSLTE_CP_NORM);

    if(srslte_pss_synch_init_fft_offset_generic(&q->pss, max_offset, fft_size, 0, q->use_scatter_sync_seq, q->pss_len, q->enable_second_stage_pss_detection)) {
      fprintf(stderr, "Error initializing PSS object.\n");
      goto clean_exit;
    }

    if(srslte_sss_synch_init(&q->sss, fft_size)) {
      fprintf(stderr, "Error initializing SSS object.\n");
      goto clean_exit;
    }

    if(srslte_cp_synch_init(&q->cp_synch, fft_size)) {
      fprintf(stderr, "Error initiating CFO.\n");
      goto clean_exit;
    }

    DEBUG("SYNC init with frame_size=%d, max_offset=%d and fft_size=%d\n", frame_size, max_offset, fft_size);

    ret = SRSLTE_SUCCESS;
  }  else {
    fprintf(stderr, "Invalid parameters frame_size: %d, fft_size: %d, radio id: %d\n", frame_size, fft_size, cell_id);
  }

clean_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_sync_free(q);
  }
  return ret;
}

void srslte_sync_free_reentry(srslte_sync_t *q) {
  if (q) {
    for (int i = 0; i < 2; i++) {
      if (q->cfo_i_corr[i]) {
        free(q->cfo_i_corr[i]);
      }
    }
  }
}

void srslte_sync_free_except_reentry(srslte_sync_t *q) {
  if(q) {
    srslte_pss_synch_free(&q->pss);
    srslte_sss_synch_free(&q->sss);
    srslte_cfo_free(&q->cfocorr);
    srslte_cfo_free(&q->cfocorr2);
#if(USE_FINE_GRAINED_CFO==1)
    srslte_cfo_free_finer(&q->cfocorr_finer);
#endif
    srslte_cp_synch_free(&q->cp_synch);
    for(int i = 0; i < 2; i++) {
      if(q->cfo_i_corr[i]) {
        free(q->cfo_i_corr[i]);
      }
      if(q->find_cfo_i) {
        srslte_pss_synch_free(&q->pss_i[i]);
      }
    }
    if(q->temp) {
      free(q->temp);
    }
  }
}

void srslte_sync_free(srslte_sync_t *q) {
  if(q) {
    srslte_pss_synch_free(&q->pss);
    srslte_sss_synch_free(&q->sss);
    srslte_cfo_free(&q->cfocorr);
    srslte_cfo_free(&q->cfocorr2);
#if(USE_FINE_GRAINED_CFO==1)
    srslte_cfo_free_finer(&q->cfocorr_finer);
#endif
    srslte_cp_synch_free(&q->cp_synch);
    for(int i = 0; i < 2; i++) {
      if(q->cfo_i_corr[i]) {
        free(q->cfo_i_corr[i]);
      }
      if(q->find_cfo_i) {
        srslte_pss_synch_free(&q->pss_i[i]);
      }
    }
    if(q->temp) {
      free(q->temp);
    }
  }
}

void srslte_sync_set_threshold(srslte_sync_t *q, float threshold) {
  q->threshold = threshold;
  //printf("[SYNC] PSS detection threshold set to: %1.2f\n", q->threshold);
}

void srslte_sync_cfo_i_detec_en(srslte_sync_t *q, bool enabled) {
  q->find_cfo_i = enabled;
  if (enabled && !q->find_cfo_i_initiated) {
    for (int i=0;i<2;i++) {
      int offset=(i==0)?-1:1;
      if (srslte_pss_synch_init_fft_offset(&q->pss_i[i], q->max_offset, q->fft_size, offset)) {
        fprintf(stderr, "Error initializing PSS object\n");
      }
      for (int t=0;t<q->frame_size;t++) {
        q->cfo_i_corr[i][t] = cexpf(-2*_Complex_I*M_PI*offset*(float) t/q->fft_size);
      }
    }
    q->find_cfo_i_initiated = true;
  }
}

void srslte_sync_sss_en(srslte_sync_t *q, bool enabled) {
  q->sss_en = enabled;
}

bool srslte_sync_sss_detected(srslte_sync_t *q) {
  if(q->decode_pdcch) {
    return srslte_N_id_1_isvalid(q->N_id_1);
  } else {
    return true;
  }
}

int srslte_sync_get_cell_id(srslte_sync_t *q) {
  if (srslte_N_id_2_isvalid(q->N_id_2) && srslte_N_id_1_isvalid(q->N_id_1)) {
    return q->N_id_1*3 + q->N_id_2;
  } else {
    return -1;
  }
}

int srslte_sync_set_N_id_2(srslte_sync_t *q, uint32_t N_id_2) {
  if (srslte_N_id_2_isvalid(N_id_2)) {
    q->N_id_2 = N_id_2;
    return SRSLTE_SUCCESS;
  } else {
    fprintf(stderr, "Invalid N_id_2=%d\n", N_id_2);
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}

uint32_t srslte_sync_get_sf_idx(srslte_sync_t *q) {
  return q->sf_idx;
}

float srslte_sync_get_cfo(srslte_sync_t *q) {
  //SYNC_DEBUG("srslte_sync_get_cfo mean_cfo2 %f cfo_i %d\n", q->mean_cfo2, q->cfo_i);
  return q->mean_cfo2 + q->cfo_i;
}

float srslte_sync_get_cfo_new(srslte_sync_t *q) {
  //SYNC_DEBUG("srslte_sync_get_cfo_new mean_cfo2: %f - cfo_i: %d - mean_cfo: %f - total: %f\n", q->mean_cfo2, q->cfo_i, q->mean_cfo, (q->mean_cfo2+q->cfo_i+q->mean_cfo));
  return q->mean_cfo2 + q->cfo_i + q->mean_cfo;
}

void srslte_sync_set_cfo_new(srslte_sync_t *q, float cfo) {
  q->mean_cfo2 = cfo;
}

void srslte_sync_set_cfo(srslte_sync_t *q, float cfo) {
  q->mean_cfo = cfo;
}

void srslte_sync_set_cfo_i(srslte_sync_t *q, int cfo_i) {
  q->cfo_i = cfo_i;
}

void srslte_sync_set_cfo_enable(srslte_sync_t *q, bool enable) {
  q->enable_cfo_corr = enable;
}

void srslte_sync_set_cfo_ema_alpha(srslte_sync_t *q, float alpha) {
  q->cfo_ema_alpha = alpha;
}

float srslte_sync_get_last_peak_value(srslte_sync_t *q) {
  return q->peak_value;
}

float srslte_sync_get_peak_value(srslte_sync_t *q) {
  return q->peak_value;
}

void srslte_sync_cp_en(srslte_sync_t *q, bool enabled) {
  q->detect_cp = enabled;
}

bool srslte_sync_sss_is_en(srslte_sync_t *q) {
  return q->sss_en;
}

void srslte_sync_set_em_alpha(srslte_sync_t *q, float alpha) {
  srslte_pss_synch_set_ema_alpha(&q->pss, alpha);
}

srslte_cp_t srslte_sync_get_cp(srslte_sync_t *q) {
  return q->cp;
}

void srslte_sync_set_cfo_correction_type(srslte_sync_t *q, uint32_t cfo_correction_type) {
  q->cfo_correction_type = cfo_correction_type;
}

uint32_t srslte_sync_get_cfo_correction_type(srslte_sync_t *q) {
  return (uint32_t)q->cfo_correction_type;
}

void srslte_sync_set_cp(srslte_sync_t *q, srslte_cp_t cp) {
  q->cp = cp;
  q->cp_len = SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_LEN_NORM(1,q->fft_size):SRSLTE_CP_LEN_EXT(q->fft_size);
  if (q->frame_size < q->fft_size) {
    q->nof_symbols = 1;
  } else {
    q->nof_symbols = q->frame_size/(q->fft_size+q->cp_len)-1;
  }
}

void srslte_sync_set_sss_algorithm(srslte_sync_t *q, sss_alg_t alg) {
  q->sss_alg = alg;
}

/* CP detection algorithm taken from:
 * "SSS Detection Method for Initial Cell Search in 3GPP LTE FDD/TDD Dual Mode Receiver"
 * by Jung-In Kim et al.
 */
srslte_cp_t srslte_sync_detect_cp(srslte_sync_t *q, cf_t *input, uint32_t peak_pos)
{
  float R_norm=0, R_ext=0, C_norm=0, C_ext=0;
  float M_norm=0, M_ext=0;

  uint32_t cp_norm_len = SRSLTE_CP_LEN_NORM(7, q->fft_size);
  uint32_t cp_ext_len = SRSLTE_CP_LEN_EXT(q->fft_size);

  uint32_t nof_symbols = peak_pos/(q->fft_size+cp_ext_len);

  if (nof_symbols > 3) {
    nof_symbols = 3;
  }

  if (nof_symbols > 0) {

    cf_t *input_cp_norm = &input[peak_pos-nof_symbols*(q->fft_size+cp_norm_len)];
    cf_t *input_cp_ext = &input[peak_pos-nof_symbols*(q->fft_size+cp_ext_len)];

    for (int i=0;i<nof_symbols;i++) {
      R_norm  += crealf(srslte_vec_dot_prod_conj_ccc(&input_cp_norm[q->fft_size], input_cp_norm, cp_norm_len));
      C_norm  += cp_norm_len * srslte_vec_avg_power_cf(input_cp_norm, cp_norm_len);
      input_cp_norm += q->fft_size+cp_norm_len;
    }
    if (C_norm > 0) {
      M_norm = R_norm/C_norm;
    }

    q->M_norm_avg = SRSLTE_VEC_EMA(M_norm/nof_symbols, q->M_norm_avg, CP_EMA_ALPHA);

    for (int i=0;i<nof_symbols;i++) {
      R_ext  += crealf(srslte_vec_dot_prod_conj_ccc(&input_cp_ext[q->fft_size], input_cp_ext, cp_ext_len));
      C_ext  += cp_ext_len * srslte_vec_avg_power_cf(input_cp_ext, cp_ext_len);
      input_cp_ext += q->fft_size+cp_ext_len;
    }
    if (C_ext > 0) {
      M_ext = R_ext/C_ext;
    }

    q->M_ext_avg = SRSLTE_VEC_EMA(M_ext/nof_symbols, q->M_ext_avg, CP_EMA_ALPHA);

    if (q->M_norm_avg > q->M_ext_avg) {
      return SRSLTE_CP_NORM;
    } else if (q->M_norm_avg < q->M_ext_avg) {
      return SRSLTE_CP_EXT;
    } else {
      if (R_norm > R_ext) {
        return SRSLTE_CP_NORM;
      } else {
        return SRSLTE_CP_EXT;
      }
    }
  } else {
    return SRSLTE_CP_NORM;
  }
}

/* Returns 1 if the SSS is found, 0 if not and -1 if there is not enough space
 * to correlate
 */
int sync_sss(srslte_sync_t *q, cf_t *input, uint32_t peak_pos, srslte_cp_t cp) {
  int sss_idx, ret;

  srslte_sss_synch_set_N_id_2(&q->sss, q->N_id_2);

  /* Make sure we have enough room to find SSS sequence */
  sss_idx = (int) peak_pos-2*q->fft_size-SRSLTE_CP_LEN(q->fft_size, (SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  if (sss_idx < 0) {
    DEBUG("Not enough room to decode SSS (sss_idx=%d, peak_pos=%d)\n", sss_idx, peak_pos);
    return SRSLTE_ERROR;
  }
  DEBUG("Searching SSS around sss_idx: %d, peak_pos: %d\n", sss_idx, peak_pos);

  switch(q->sss_alg) {
    case SSS_DIFF:
      srslte_sss_synch_m0m1_diff(&q->sss, &input[sss_idx], &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_PARTIAL_3:
      srslte_sss_synch_m0m1_partial(&q->sss, &input[sss_idx], 3, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_FULL:
      srslte_sss_synch_m0m1_partial(&q->sss, &input[sss_idx], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
  }

  q->sf_idx = srslte_sss_synch_subframe(q->m0, q->m1);

  ret = srslte_sss_synch_N_id_1(&q->sss, q->m0, q->m1);
  if (ret >= 0) {
    q->N_id_1 = (uint32_t) ret;
    DEBUG("SSS detected N_id_1=%d, sf_idx=%d, %s CP\n",
      q->N_id_1, q->sf_idx, SRSLTE_CP_ISNORM(q->cp)?"Normal":"Extended");
    return 1;
  } else {
    q->N_id_1 = 1000;
    return SRSLTE_SUCCESS;
  }
}

srslte_pss_synch_t* srslte_sync_get_cur_pss_obj(srslte_sync_t *q) {
 srslte_pss_synch_t *pss_obj[3] = {&q->pss_i[0], &q->pss, &q->pss_i[1]};
 return pss_obj[q->cfo_i+1];
}

static float cfo_estimate(srslte_sync_t *q, cf_t *input) {
  uint32_t cp_offset = 0;
  cp_offset = srslte_cp_synch(&q->cp_synch, input, q->max_offset, 2, SRSLTE_CP_LEN_NORM(1,q->fft_size));
  cf_t cp_corr_max = srslte_cp_synch_corr_output(&q->cp_synch, cp_offset);
  float cfo = -carg(cp_corr_max) / M_PI / 2;
  return cfo;
}

static int cfo_i_estimate(srslte_sync_t *q, cf_t *input, int find_offset, int *peak_pos, float *peak_value) {
  float peak_value_aux;
  float max_peak_value = -99;
  int max_cfo_i = 0;
  srslte_pss_synch_t *pss_obj[3] = {&q->pss_i[0], &q->pss, &q->pss_i[1]};
  for(int cfo_i = 0; cfo_i < 3; cfo_i++) {
    srslte_pss_synch_set_N_id_2(pss_obj[cfo_i], q->N_id_2);
    int p = srslte_pss_synch_find_pss(pss_obj[cfo_i], &input[find_offset], &peak_value_aux);
    if(peak_value_aux > max_peak_value) {
      max_peak_value = peak_value_aux;
      if(peak_pos) {
        *peak_pos = p;
      }
      *peak_value = peak_value_aux;
      max_cfo_i = cfo_i-1;
    }
  }
  return max_cfo_i;
}

float srslte_sync_cfo_estimate(srslte_sync_t *q, cf_t *input, int find_offset) {
  float cfo_f = cfo_estimate(q, input);
  float peak_value = 0;
  int cfo_i = 0;
  if (q->find_cfo_i) {
    cfo_i = cfo_i_estimate(q, input, find_offset, NULL, &peak_value);
  }
  return (float) cfo_i + cfo_f;
}

float srslte_sync_cfo_estimate_cp(srslte_sync_t *q, cf_t *input) {
  cf_t cp_corr = srslte_cp_corr(&q->cp_synch, input, 7);
  float cfo = -carg(cp_corr) / M_PI / 2;
  return cfo;
}

/** Finds the PSS sequence previously defined by a call to srslte_sync_set_N_id_2()
 * around the position find_offset in the buffer input.
 * Returns 1 if the correlation peak exceeds the threshold set by srslte_sync_set_threshold()
 * or 0 otherwise. Returns a negative number on error (if N_id_2 has not been set)
 *
 * The maximum of the correlation peak is always stored in *peak_position
 */
srslte_sync_find_ret_t srslte_sync_find(srslte_sync_t *q, cf_t *input, int find_offset, uint32_t *peak_position) {
  if(q->decode_pdcch) {
    // If PDCCH/PCFICH decoding is enabled, then there is no need to decode SCH.
    return srslte_sync_find_legacy(q, input, find_offset, peak_position);
  } else {
    // Decode SCH signals for MCS/Number of transmitted slots and SRN ID/Interface ID.
    return srslte_sync_find_sch(q, input, find_offset, peak_position);
  }
}

srslte_sync_find_ret_t srslte_sync_find_legacy(srslte_sync_t *q, cf_t *input, int find_offset, uint32_t *peak_position) {
  srslte_sync_find_ret_t ret = SRSLTE_SYNC_ERROR;

  if (!q) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  if(input             != NULL     &&
      srslte_N_id_2_isvalid(q->N_id_2) &&
      fft_size_isvalid(q->fft_size))
  {
    int peak_pos = 0;

    q->sf_idx = 1000;

    ret = SRSLTE_SUCCESS;

    if(peak_position) {
      *peak_position = 0;
    }

    cf_t *input_cfo = input;

    float cfo = 0;
    if(q->enable_cfo_corr) {
      /* compute exponential moving average CFO */
      q->mean_cfo = SRSLTE_VEC_EMA(cfo, q->mean_cfo, q->cfo_ema_alpha);
      /* Correct CFO with the averaged CFO estimation */
      srslte_cfo_correct(&q->cfocorr2, input - q->frame_size, q->temp, -q->mean_cfo / q->fft_size);
      srslte_cfo_correct(&q->cfocorr2, input, q->temp + q->frame_size, -q->mean_cfo / q->fft_size);
      /* Return CFO corrected samples to input_cfo. */
      input_cfo = q->temp;
      SYNC_ERROR("CFO Corr should be disabled!!!!!\n",0);
      return SRSLTE_ERROR;
    }

    // If integer CFO is enabled, find max PSS correlation for shifted +1/0/-1 integer versions
    if(q->find_cfo_i && q->enable_cfo_corr) {
      q->cfo_i = cfo_i_estimate(q, input_cfo + q->frame_size, find_offset, &peak_pos, &q->peak_value);
      if(q->cfo_i != 0) {
        srslte_vec_prod_ccc(input_cfo, q->cfo_i_corr[q->cfo_i<0?0:1], input_cfo, q->frame_size);
        srslte_vec_prod_ccc(input_cfo + q->frame_size, q->cfo_i_corr[q->cfo_i<0?0:1], input_cfo + q->frame_size, q->frame_size);
        INFO("Compensating cfo_i=%d\n", q->cfo_i);
      }
      SYNC_ERROR("Integer CFO should not be enabled!!!!\n",0);
      return SRSLTE_ERROR;
    } else {
      srslte_pss_synch_set_N_id_2(&q->pss, q->N_id_2);
      peak_pos = srslte_pss_synch_find_pss(&q->pss, &input_cfo[q->frame_size + find_offset], &q->peak_value);
      if(peak_pos < 0) {
        SYNC_ERROR("Error finding PSS sequence.\n",0);
        return SRSLTE_ERROR;
      }

      // Add frame size because peak is now in the second subframe, i.e., there is an offset of one subframe in the peak position.
      peak_pos = peak_pos + q->frame_size + find_offset;
    }

    // If peak is over threshold, compute CFO and SSS
    if(q->peak_value >= q->threshold) {

      // Timestamps the peak detection instant for reception statistics.
      clock_gettime(CLOCK_REALTIME, &q->peak_detection_timestamp);

      DEV_DEBUG("frame_size: %d - peak_value: %f - threshold: %f - find_offset: %d - non-offset peak pos: %d - peak position: %d\n", q->frame_size, q->peak_value, q->threshold, find_offset, (peak_pos - q->frame_size), peak_pos);

      // Try to detect SSS.
      if(q->sss_en) {
        // Set an invalid N_id_1 indicating SSS is yet to be detected
        q->N_id_1 = 1000;
        // Decode SSS and find N_id_1.
        if(sync_sss(q, input_cfo, peak_pos, q->cp) < 0) {
          SYNC_INFO("No space for SSS processing. Frame starts at %d\n", peak_pos);
        }
      } else {
        // If SSS detection is disabled then subframe index is always equal to the initial one.
        q->sf_idx = q->initial_subframe_index;
      }

      if(q->detect_cp) {
        if(peak_pos >= 2*(q->fft_size + SRSLTE_CP_LEN_EXT(q->fft_size))) {
          srslte_sync_set_cp(q, srslte_sync_detect_cp(q, input_cfo, peak_pos));
        } else {
          SYNC_INFO("Not enough room to detect CP length. Peak position: %d\n", peak_pos);
        }
      }

      if(peak_pos >= 2*(q->fft_size + SRSLTE_CP_LEN_EXT(q->fft_size))) {
        if(q->cfo_correction_type == CFO_CORRECTION_PSS) {
          q->mean_cfo2 = srslte_pss_synch_cfo_compute2(&q->pss, input_cfo + peak_pos - q->fft_size);
        }
        ret = SRSLTE_SYNC_FOUND;
        SYNC_DEBUG("Peak was found and will be reported to UE sync.\n",0);
      } else {
        ret = SRSLTE_SYNC_FOUND_NOSPACE;
        DEV_DEBUG("SRSLTE_SYNC_FOUND_NOSPACE!\n",0);
      }
    } else {
      ret = SRSLTE_SYNC_NOFOUND;
    }

    INFO("SYNC ret=%d N_id_2=%d find_offset=%d frame_len=%d, pos=%d peak=%.2f threshold=%.2f sf_idx=%d, CFO=%.3f kHz\n",
         ret, q->N_id_2, find_offset, q->frame_size, peak_pos, q->peak_value,
         q->threshold, q->sf_idx, 15*(q->cfo_i+q->mean_cfo+q->mean_cfo2));

    if(peak_position) {
      *peak_position = (uint32_t)peak_pos;
    }

    // PHY Filtering if enabled (by default it is not enabled). We check if the received PSS/SSS (N_id_2) and SF index were sent to this node.
    if(q->N_id_2 != q->node_N_id_2 || q->sf_idx != q->initial_subframe_index) {
      if(ret == SRSLTE_SYNC_FOUND) {
        SYNC_DEBUG("Peak was found but either cell ID or SF is incorrect.\n",0);
        ret = SRSLTE_SYNC_NOFOUND;
      }
    }

    // Only check detected SSS value if SSS detection is enabled.
    if(q->sss_en) {
      // Set the number of subframes to be received in sequence. Only the first one carries PSS/SSS, the other ones have only data, and I hope we can reach MCS 28. ;)
      q->nof_subframes_to_rx = 0;
      if(ret == SRSLTE_SYNC_FOUND) {
        q->nof_subframes_to_rx = q->N_id_1+1;
        if(q->nof_subframes_to_rx > MAXIMUM_NUMBER_OF_SUBFRAMES) {
          SYNC_DEBUG("Received number of subframes: %d is bigger than the maximum expected: %d.\n",q->nof_subframes_to_rx,MAXIMUM_NUMBER_OF_SUBFRAMES);
          ret = SRSLTE_SYNC_NOFOUND;
        }
      }
    }

  } else if(srslte_N_id_2_isvalid(q->N_id_2)) {
    SYNC_ERROR("Must call srslte_sync_set_N_id_2() first!\n",0);
  }

  return ret;
}

void srslte_sync_reset(srslte_sync_t *q) {
  q->M_ext_avg = 0;
  q->M_norm_avg = 0;
  q->nof_subframes_to_rx = 0;
  q->mcs = 0;
  srslte_pss_synch_reset(&q->pss);
}

//******************************************************************************
//****************** Scatter system's customized functions *********************
//******************************************************************************
srslte_sync_find_ret_t srslte_sync_find_sch(srslte_sync_t *q, cf_t *input, int find_offset, uint32_t *peak_position) {
  srslte_sync_find_ret_t ret = SRSLTE_SYNC_ERROR;
  uint32_t srn_id, interface_id, mcs, nof_subframes_to_rx, sch_value;

  if(!q) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  if(input != NULL && srslte_N_id_2_isvalid(q->N_id_2) && fft_size_isvalid(q->fft_size)) {
    int peak_pos = 0, rc = 0;

    q->sf_idx = q->initial_subframe_index;

    ret = SRSLTE_SUCCESS;

    if(peak_position) {
      *peak_position = 0;
    }

    cf_t *input_cfo = input;

    float cfo = 0;
    if(q->enable_cfo_corr) {
      /* compute exponential moving average CFO */
      q->mean_cfo = SRSLTE_VEC_EMA(cfo, q->mean_cfo, q->cfo_ema_alpha);
      /* Correct CFO with the averaged CFO estimation */
      srslte_cfo_correct(&q->cfocorr2, input - q->frame_size, q->temp, -q->mean_cfo / q->fft_size);
      srslte_cfo_correct(&q->cfocorr2, input, q->temp + q->frame_size, -q->mean_cfo / q->fft_size);
      /* Return CFO corrected samples to input_cfo. */
      input_cfo = q->temp;
      SYNC_ERROR("CFO Corr should be disabled!!!!!\n",0);
      return SRSLTE_ERROR;
    }

    // If integer CFO is enabled, find max PSS correlation for shifted +1/0/-1 integer versions
    if(q->find_cfo_i && q->enable_cfo_corr) {
      q->cfo_i = cfo_i_estimate(q, input_cfo + q->frame_size, find_offset, &peak_pos, &q->peak_value);
      if(q->cfo_i != 0) {
        srslte_vec_prod_ccc(input_cfo, q->cfo_i_corr[q->cfo_i<0?0:1], input_cfo, q->frame_size);
        srslte_vec_prod_ccc(input_cfo + q->frame_size, q->cfo_i_corr[q->cfo_i<0?0:1], input_cfo + q->frame_size, q->frame_size);
        INFO("Compensating cfo_i=%d\n", q->cfo_i);
      }
      SYNC_ERROR("Integer CFO should not be enabled!!!!\n",0);
      return SRSLTE_ERROR;
    } else {
      srslte_pss_synch_set_N_id_2(&q->pss, q->N_id_2);
      peak_pos = srslte_pss_synch_find_pss_scatter(&q->pss, &input_cfo[q->frame_size + find_offset], &q->peak_value);
      if(peak_pos < 0) {
        SYNC_ERROR("Error finding PSS sequence.\n",0);
        return SRSLTE_ERROR;
      }

      // Add frame size because peak is now in the second subframe, i.e., there is an offset of one subframe in the peak position.
      peak_pos = peak_pos + q->frame_size + find_offset;
    }

    // If peak is over threshold, compute CFO and SSS
    if(q->peak_value >= q->threshold) {

      // Timestamps the peak detection instant for reception statistics.
      clock_gettime(CLOCK_REALTIME, &q->peak_detection_timestamp);

      DEV_DEBUG("frame_size: %d - peak_value: %f - threshold: %f - find_offset: %d - non-offset peak pos: %d - peak position: %d\n", q->frame_size, q->peak_value, q->threshold, find_offset, (peak_pos - q->frame_size), peak_pos);

      // If PHY filtering is enabled then we try to decode SRN ID and Radio Interface ID.
      if(q->phy_filtering) {
        // Decode SCH sequence carrying SRN ID and Radio Interface ID.
        rc = srslte_sync_decode_sch(q, input_cfo, true, peak_pos, &srn_id, &interface_id, &sch_value);
        if(rc < 0) {
          SYNC_ERROR("[SYNCH] PHY ID: %d - SRN ID/Intf. ID SCH decoding error....\n", q->intf_id);
        }

        // Drop packets not addressed to this specific node.
        if(srn_id != q->node_id && srn_id != BROADCAST_ID) {
          SYNC_DEBUG("Packet with node ID: %d not addressed to this node with ID: %d, dropping it.\n", srn_id, q->node_id);
          return SRSLTE_SYNC_NOFOUND;
        }

        // Drop packets not addressed to this specifc Radio Interface.
        if(interface_id != q->intf_id) {
          SYNC_DEBUG("Packet with radio interface ID: %d not addressed to this radio interface with ID: %d, dropping it.\n", interface_id, q->intf_id);
          return SRSLTE_SYNC_NOFOUND;
        }
      }

#if(0)
      if(rc >= 0) {
        printf("[SYNCH] SCH Decoded: SRN ID: %d - Interface ID: %d - sch_value: %d\n", srn_id, interface_id, sch_value);
      }
      if(srn_id == q->node_id) {
        printf("Received SRN ID: %d matches SRN ID: %d\n", srn_id, q->node_id);
      }
      if(interface_id == q->intf_id) {
        printf("Received Radio Interface ID: %d matches Radio Interface ID: %d\n", interface_id, q->intf_id);
      }
#endif

      // Decode SCH sequence carrying MCS and number of transmitted slots.
      rc = srslte_sync_decode_sch(q, input_cfo, false, peak_pos, &mcs, &nof_subframes_to_rx, &sch_value);
      if(rc < 0) {
        SYNC_ERROR("[SYNCH] PHY ID: %d - MCS/# slots SCH decoding error....\n", q->intf_id);
      }

#if(0)
      if(rc >= 0) {
        printf("[SYNCH] SCH Decoded: MCS: %d - # slots: %d - sch_value: %d\n", mcs, nof_subframes_to_rx, sch_value);
      }
#endif

      if(peak_pos >= 2*(q->fft_size + SRSLTE_CP_LEN_EXT(q->fft_size))) {
        if(q->cfo_correction_type == CFO_CORRECTION_PSS) {
          q->mean_cfo2 = srslte_pss_synch_cfo_compute2(&q->pss, input_cfo + peak_pos - q->fft_size);
        }
        ret = SRSLTE_SYNC_FOUND;
        SYNC_DEBUG("Peak was found and will be reported to UE sync.\n",0);
      } else {
        ret = SRSLTE_SYNC_FOUND_NOSPACE;
        DEV_DEBUG("SRSLTE_SYNC_FOUND_NOSPACE!\n",0);
      }
    } else {
      ret = SRSLTE_SYNC_NOFOUND;
    }

    INFO("SYNC ret=%d N_id_2=%d find_offset=%d frame_len=%d, pos=%d peak=%.2f threshold=%.2f sf_idx=%d, CFO=%.3f kHz\n",
         ret, q->N_id_2, find_offset, q->frame_size, peak_pos, q->peak_value,
         q->threshold, q->sf_idx, 15*(q->cfo_i+q->mean_cfo+q->mean_cfo2));

    if(peak_position) {
      *peak_position = (uint32_t)peak_pos;
    }

    // Set the number of subframes to be received in sequence. Only the first one carries PSS/SSS, the other ones have only data, and I hope we can reach MCS 28. ;)
    q->nof_subframes_to_rx = 0;
    q->mcs = 0;
    if(ret == SRSLTE_SYNC_FOUND) {
      q->nof_subframes_to_rx = nof_subframes_to_rx;
      q->mcs = mcs;
      if(q->nof_subframes_to_rx > MAXIMUM_NUMBER_OF_SUBFRAMES || q->mcs > 31) {
        SYNC_DEBUG("Received number of subframes: %d is bigger than the maximum expected: %d or MCS: %d greater than 31.\n", q->nof_subframes_to_rx, MAXIMUM_NUMBER_OF_SUBFRAMES, q->mcs);
        ret = SRSLTE_SYNC_NOFOUND;
      }
    }
  } else if(srslte_N_id_2_isvalid(q->N_id_2)) {
    SYNC_ERROR("Must call srslte_sync_set_N_id_2() first!\n",0);
  }

  return ret;
}

// Returns 1 if the SCH signal is found, 0 if not and -1 if there is not enough space to correlate
int srslte_sync_decode_sch(srslte_sync_t *q, cf_t *input, bool decode_radio_id, uint32_t peak_pos, uint32_t *value0, uint32_t *value1, uint32_t* sch_value) {
  int sch_idx0 = -1, sch_idx1, ret, offset;

  srslte_sch_set_table_index(&q->sss, 0);

  // Calculate the respective offset.
  if(decode_radio_id) {
    // SCH sequence for Radio and Interface ID is in OFDM symbol #2, i.e., 6 OFDM symbols behind the peak position.
    offset = 6;
  } else {
    // SCH sequence for MCS and # of slots is in OFDM symbol #6, i.e., 2 OFDM symbols behind the peak position.
    offset = 2;
  }
  // Calculate OFDM symbol containing SCH sequence.
  sch_idx1 = (int) peak_pos - offset*q->fft_size - (offset-1)*SRSLTE_CP_LEN(q->fft_size, (SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  // Make sure we have enough room to find SCH sequence at the middle of the first subframe, i.e., on the 5th OFDM symbol.
  if(sch_idx1 < 0) {
    fprintf(stderr, "Not enough samples to decode middle SCH (sch_idx1: %d - peak_pos: %d)\n", sch_idx1, peak_pos);
    return SRSLTE_ERROR;
  }
  DEBUG("[SCH] Searching middle SCH around sch_idx1: %d - peak_pos: %d\n", sch_idx1, peak_pos);

  // If PHY filtering is not enabled, then the front of the subframe carries SCH sequence for temporal Rx combining.
  if(!q->phy_filtering) {
    // Calculate offset for front OFDM symbol containing SCH sequence, i.e., OFDM symbol #2.
    offset = 6;
    sch_idx0 = (int) peak_pos - offset*q->fft_size - (offset-1)*SRSLTE_CP_LEN(q->fft_size, (SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
    // Make sure we have enough room to find the front SCH sequence.
    if(sch_idx0 < 0) {
      fprintf(stderr, "Not enough samples to decode front SCH (sch_idx0: %d - peak_pos: %d)\n", sch_idx0, peak_pos);
      return SRSLTE_ERROR;
    }
    DEBUG("[SCH] Searching front SCH around sch_idx0: %d - peak_pos: %d\n", sch_idx0, peak_pos);
  }

  switch(q->sss_alg) {
    case SSS_PARTIAL_3:
      srslte_sch_synch_m0m1_partial(&q->sss, &input[sch_idx1], 3, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_FULL:
      if(decode_radio_id) {
        srslte_sch_m0m1_partial_front(&q->sss, q->nof_prb, &input[sch_idx1], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      } else {
        // If PHY filtering is enabled, the SCH sequence in the front of the subframe carries SRN and Interface IDs, so no reason to decode/combine.
        if(q->phy_filtering) {
          srslte_sch_synch_m0m1_partial(&q->sss, &input[sch_idx1], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
        } else {
          // When PHY Filtering is not enabled, then we combine the two SCH sequences being sent in the front and at the middle of the 1st subframe.
          if(sch_idx0 < 0) {
            fprintf(stderr, "[SYNC ERROR] Index for front SCH decoding is invalid.\n");
            return SRSLTE_ERROR;
          }
          // Combine the two SCH signals into one so that the probability of correct detection/decoding of MCS/# of slots is higher.
          srslte_sch_m0m1_partial_combining(&q->sss, q->nof_prb, &input[sch_idx0], &input[sch_idx1], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
        }
      }
      break;
    case SSS_DIFF:
    default:
      fprintf(stderr, "[SYNC ERROR] SCH decoding algorithm not supported.\n");
      return SRSLTE_ERROR;
  }

  ret = srslte_sch_synch_value(&q->sss, q->m0, q->m1);

  DEBUG("[SCH] m0: %d - m1: %d - value: %d\n", q->m0, q->m1, ret);

  if(ret >= 0) {
    *sch_value  = (uint32_t) ret;
    if(decode_radio_id) {
      *value0 = floor(ret/2); // SRN ID
      *value1 = ret % 2;      // Radio Interface ID
    } else {
      *value0 = floor(ret/26); // MCS
      *value1 = ret % 26;      // # slots
    }
    return 0;
  } else {
    return SRSLTE_ERROR;
  }
}

void srslte_sync_set_avg_psr_scatter(srslte_sync_t *q, bool enable_avg_psr) {
  srslte_pss_set_avg_psr_scatter(&q->pss, enable_avg_psr);
}

void srslte_sync_enable_second_stage_detection_scatter(srslte_sync_t *q, bool enable_second_stage_pss_detection) {
  srslte_pss_enable_second_stage_detection_scatter(&q->pss, enable_second_stage_pss_detection);
}

void srslte_sync_set_pss_1st_stage_threshold_scatter(srslte_sync_t *q, float pss_first_stage_threshold) {
  srslte_pss_set_1st_stage_threshold_scatter(&q->pss, pss_first_stage_threshold);
}

void srslte_sync_set_pss_2nd_stage_threshold_scatter(srslte_sync_t *q, float pss_second_stage_threshold) {
  srslte_pss_set_2nd_stage_threshold_scatter(&q->pss, pss_second_stage_threshold);
}
