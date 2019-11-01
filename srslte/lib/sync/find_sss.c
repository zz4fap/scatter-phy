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
#include <complex.h>
#include <math.h>

#include "srslte/utils/vector.h"
#include "srslte/sync/sss.h"

#define MAX_M 3


static void corr_all_zs(cf_t z[SRSLTE_SSS_N], float s[SRSLTE_SSS_N][SRSLTE_SSS_N-1], float output[SRSLTE_SSS_N]) {
  uint32_t m;
  cf_t tmp[SRSLTE_SSS_N];

  for (m = 0; m < SRSLTE_SSS_N; m++) {
    tmp[m] = srslte_vec_dot_prod_cfc(z, s[m], SRSLTE_SSS_N - 1);
  }
  srslte_vec_abs_square_cf(tmp, output, SRSLTE_SSS_N);
}

static void corr_all_sz_partial(cf_t z[SRSLTE_SSS_N], float s[SRSLTE_SSS_N][SRSLTE_SSS_N], uint32_t M, float output[SRSLTE_SSS_N]) {
  uint32_t Nm = SRSLTE_SSS_N/M;
  cf_t tmp[SRSLTE_SSS_N];
  float tmp_abs[MAX_M-1][SRSLTE_SSS_N];
  int j, m;
  float *ptr;

  for (j=0;j<M;j++) {
    for (m = 0; m < SRSLTE_SSS_N; m++) {
      tmp[m] = srslte_vec_dot_prod_cfc(&z[j*Nm], &s[m][j*Nm], Nm);
    }
    if (j == 0) {
      ptr = output;
    } else {
      ptr = tmp_abs[j-1];
    }
    srslte_vec_abs_square_cf(tmp, ptr, SRSLTE_SSS_N);
  }
  for (j=1;j<M;j++) {
    srslte_vec_sum_fff(tmp_abs[j-1], output, output, SRSLTE_SSS_N);
  }
}

static void extract_pair_sss(srslte_sss_synch_t *q, cf_t *input, cf_t *ce, cf_t y[2][SRSLTE_SSS_N]) {
  cf_t input_fft[SRSLTE_SYMBOL_SZ_MAX];

  srslte_dft_run_c(&q->dftp_input, input, input_fft);

  if (ce) {
    srslte_vec_div_ccc(&input_fft[q->fft_size/2-SRSLTE_SSS_N], ce,
                       &input_fft[q->fft_size/2-SRSLTE_SSS_N], 2*SRSLTE_SSS_N);
  }

  for (int i = 0; i < SRSLTE_SSS_N; i++) {
    y[0][i] = input_fft[q->fft_size/2-SRSLTE_SSS_N + 2 * i];
    y[1][i] = input_fft[q->fft_size/2-SRSLTE_SSS_N + 2 * i + 1];
  }

  srslte_vec_prod_cfc(y[0], q->fc_tables[q->N_id_2].c[0], y[0], SRSLTE_SSS_N);
  srslte_vec_prod_cfc(y[1], q->fc_tables[q->N_id_2].c[1], y[1], SRSLTE_SSS_N);

}

int srslte_sss_synch_m0m1_diff(srslte_sss_synch_t *q, cf_t *input, uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value)
{
  return srslte_sss_synch_m0m1_diff_coh(q, input, NULL, m0, m0_value, m1, m1_value);
}

/* Differential SSS estimation.
 * Returns m0 and m1 estimates
 *
 * Source: "SSS Detection Method for Initial Cell Search in 3GPP LTE FDD/TDD Dual Mode Receiver"
 *       Jung-In Kim, Jung-Su Han, Hee-Jin Roh and Hyung-Jin Choi

 *
 */
int srslte_sss_synch_m0m1_diff_coh(srslte_sss_synch_t *q, cf_t *input, cf_t ce[2*SRSLTE_SSS_N], uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value)
{

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL   &&
      input             != NULL   &&
      m0                != NULL   &&
      m1                != NULL)
  {

    cf_t yprod[SRSLTE_SSS_N];
    cf_t y[2][SRSLTE_SSS_N];

    extract_pair_sss(q, input, ce, y);

    srslte_vec_prod_conj_ccc(&y[0][1], y[0], yprod, SRSLTE_SSS_N - 1);
    corr_all_zs(yprod, q->fc_tables[q->N_id_2].sd, q->corr_output_m0);
    *m0 = srslte_vec_max_fi(q->corr_output_m0, SRSLTE_SSS_N);
    if (m0_value) {
      *m0_value = q->corr_output_m0[*m0];
    }

    srslte_vec_prod_cfc(y[1], q->fc_tables[q->N_id_2].z1[*m0], y[1], SRSLTE_SSS_N);
    srslte_vec_prod_conj_ccc(&y[1][1], y[1], yprod, SRSLTE_SSS_N - 1);
    corr_all_zs(yprod, q->fc_tables[q->N_id_2].sd, q->corr_output_m1);
    *m1 = srslte_vec_max_fi(q->corr_output_m1, SRSLTE_SSS_N);
    if (m1_value) {
      *m1_value = q->corr_output_m1[*m1];
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

/* Partial correlation SSS estimation.
 * Returns m0 and m1 estimates
 *
 * Source: "SSS Detection Method for Initial Cell Search in 3GPP LTE FDD/TDD Dual Mode Receiver"
 *       Jung-In Kim, Jung-Su Han, Hee-Jin Roh and Hyung-Jin Choi

 */
int srslte_sss_synch_m0m1_partial(srslte_sss_synch_t *q, cf_t *input, uint32_t M, cf_t ce[2*SRSLTE_SSS_N], uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value)
{

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL   &&
      input             != NULL   &&
      m0                != NULL   &&
      m1                != NULL   &&
      M                 <= MAX_M)
  {
    cf_t y[2][SRSLTE_SSS_N];

    extract_pair_sss(q, input, ce, y);

    corr_all_sz_partial(y[0], q->fc_tables[q->N_id_2].s, M, q->corr_output_m0);
    *m0 = srslte_vec_max_fi(q->corr_output_m0, SRSLTE_SSS_N);
    if (m0_value) {
      *m0_value = q->corr_output_m0[*m0];
    }
    srslte_vec_prod_cfc(y[1], q->fc_tables[q->N_id_2].z1[*m0], y[1], SRSLTE_SSS_N);
    corr_all_sz_partial(y[1], q->fc_tables[q->N_id_2].s, M, q->corr_output_m1);
    *m1 = srslte_vec_max_fi(q->corr_output_m1, SRSLTE_SSS_N);
    if (m1_value) {
      *m1_value = q->corr_output_m1[*m1];
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

void convert_tables(srslte_sss_fc_tables_t *fc_tables, srslte_sss_tables_t *in) {
  uint32_t i, j;

  for (i = 0; i < SRSLTE_SSS_N; i++) {
    for (j = 0; j < SRSLTE_SSS_N; j++) {
      fc_tables->z1[i][j] = (float) in->z1[i][j];
    }
  }
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    for (j = 0; j < SRSLTE_SSS_N; j++) {
      fc_tables->s[i][j] = (float) in->s[i][j];
    }
  }
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    for (j = 0; j < SRSLTE_SSS_N - 1; j++) {
      fc_tables->sd[i][j] = (float) in->s[i][j + 1] * in->s[i][j];
    }
  }
  for (i = 0; i < 2; i++) {
    for (j = 0; j < SRSLTE_SSS_N; j++) {
      fc_tables->c[i][j] = (float) in->c[i][j];
    }
  }
}

//******************************************************************************
//********************* Scatter signal related functions ***********************
//******************************************************************************
void convert_sch_tables(srslte_sch_fc_tables_t *sch_fc_tables, srslte_sch_tables_t *in) {
  uint32_t i, j;

  for(i = 0; i < SRSLTE_SCH_N; i++) {
    for(j = 0; j < SRSLTE_SCH_N; j++) {
      sch_fc_tables->z1[i][j] = (float) in->z1[i][j];
    }
  }
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    for(j = 0; j < SRSLTE_SCH_N; j++) {
      sch_fc_tables->s[i][j] = (float) in->s[i][j];
    }
  }
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    for(j = 0; j < SRSLTE_SCH_N - 1; j++) {
      sch_fc_tables->sd[i][j] = (float) in->s[i][j + 1] * in->s[i][j];
    }
  }
  for(i = 0; i < 2; i++) {
    for(j = 0; j < SRSLTE_SCH_N; j++) {
      sch_fc_tables->c[i][j] = (float) in->c[i][j];
    }
  }
}

static void corr_all_sz_partial_sch(cf_t z[SRSLTE_SCH_N], float s[SRSLTE_SCH_N][SRSLTE_SCH_N], uint32_t M, float output[SRSLTE_SCH_N]) {
  uint32_t Nm = SRSLTE_SCH_N/M;
  cf_t tmp[SRSLTE_SCH_N];
  float tmp_abs[MAX_M-1][SRSLTE_SCH_N];
  int j, m;
  float *ptr;

  for(j = 0; j < M; j++) {
    for(m = 0; m < SRSLTE_SCH_N; m++) {
      tmp[m] = srslte_vec_dot_prod_cfc(&z[j*Nm], &s[m][j*Nm], Nm);
    }
    if(j == 0) {
      ptr = output;
    } else {
      ptr = tmp_abs[j-1];
    }
    srslte_vec_abs_square_cf(tmp, ptr, SRSLTE_SCH_N);
  }
  for(j = 1; j < M; j++) {
    srslte_vec_sum_fff(tmp_abs[j-1], output, output, SRSLTE_SCH_N);
  }
}

static void extract_sch_pair(srslte_sss_synch_t *q, cf_t *input, cf_t *ce, cf_t y[2][SRSLTE_SCH_N]) {
  cf_t input_fft[SRSLTE_SYMBOL_SZ_MAX];

  srslte_dft_run_c(&q->dftp_input, input, input_fft);

  if(ce) {
    srslte_vec_div_ccc(&input_fft[q->fft_size/2-SRSLTE_SCH_N], ce,
                       &input_fft[q->fft_size/2-SRSLTE_SCH_N], 2*SRSLTE_SCH_N);
  }

  for(int i = 0; i < SRSLTE_SCH_N; i++) {
    y[0][i] = input_fft[q->fft_size/2-SRSLTE_SCH_N + 2 * i];
    y[1][i] = input_fft[q->fft_size/2-SRSLTE_SCH_N + 2 * i + 1];
  }

  srslte_vec_prod_cfc(y[0], q->fc_tables[q->sch_table_index].c[0], y[0], SRSLTE_SCH_N);
  srslte_vec_prod_cfc(y[1], q->fc_tables[q->sch_table_index].c[1], y[1], SRSLTE_SCH_N);
}

int srslte_sch_synch_m0m1_partial(srslte_sss_synch_t *q, cf_t *input, uint32_t M, cf_t ce[2*SRSLTE_SCH_N], uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL   &&
      input             != NULL   &&
      m0                != NULL   &&
      m1                != NULL   &&
      M                 <= MAX_M)
  {
    cf_t y[2][SRSLTE_SCH_N];

    extract_sch_pair(q, input, ce, y);

    corr_all_sz_partial_sch(y[0], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m0);
    *m0 = srslte_vec_max_fi(q->corr_output_sch_m0, SRSLTE_SCH_N);
    if (m0_value) {
      *m0_value = q->corr_output_sch_m0[*m0];
    }
    srslte_vec_prod_cfc(y[1], q->sch_fc_tables[q->sch_table_index].z1[*m0], y[1], SRSLTE_SCH_N);
    corr_all_sz_partial_sch(y[1], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m1);
    *m1 = srslte_vec_max_fi(q->corr_output_sch_m1, SRSLTE_SCH_N);
    if (m1_value) {
      *m1_value = q->corr_output_sch_m1[*m1];
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

// Extract SCH symbols from the front of the subframe (symbols 0 or 1).
static void extract_sch_pair_front(srslte_sss_synch_t *q, uint32_t nof_prb, cf_t *input, cf_t *ce, cf_t y[2][SRSLTE_SCH_N]) {
  cf_t input_fft[SRSLTE_SYMBOL_SZ_MAX];

  srslte_dft_run_c(&q->dftp_input, input, input_fft);

  // We do not have channel estimates for symbols 0 and 1.
  if(nof_prb != 6) {
    for(int i = 0; i < SRSLTE_SCH_N; i++) {
      y[0][i] = input_fft[q->fft_size/2-(SRSLTE_SCH_N + (SRSLTE_SCH_N/2 + 1)) + 3 * i];
      y[1][i] = input_fft[q->fft_size/2-(SRSLTE_SCH_N + (SRSLTE_SCH_N/2 + 1)) + 3 * i + 1];
    }
  } else {
    for(int i = 0; i < SRSLTE_SCH_N; i++) {
      y[0][i] = input_fft[q->fft_size/2-SRSLTE_SCH_N + 2 * i];
      y[1][i] = input_fft[q->fft_size/2-SRSLTE_SCH_N + 2 * i + 1];
    }
  }

#if(0)
  printf("[SCH pair generic] nof_prb: %d\n", nof_prb);
  for(uint32_t k = 0; k < SRSLTE_SCH_N; k++) {
    printf("%d - %f,%f - %f,%f\n", k, creal(y[0][k]), cimag(y[0][k]), creal(y[1][k]), cimag(y[1][k]));
  }
#endif

  srslte_vec_prod_cfc(y[0], q->fc_tables[q->sch_table_index].c[0], y[0], SRSLTE_SCH_N);
  srslte_vec_prod_cfc(y[1], q->fc_tables[q->sch_table_index].c[1], y[1], SRSLTE_SCH_N);
}

// Find m0 and m1 values for SCH signal sequence added to the front of the subframe (symbols 0 or 1).
int srslte_sch_m0m1_partial_front(srslte_sss_synch_t *q, uint32_t nof_prb, cf_t *input, uint32_t M, cf_t ce[2*SRSLTE_SCH_N], uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL   &&
      input             != NULL   &&
      m0                != NULL   &&
      m1                != NULL   &&
      M                 <= MAX_M)
  {
    cf_t y[2][SRSLTE_SCH_N];

    extract_sch_pair_front(q, nof_prb, input, ce, y);

    corr_all_sz_partial_sch(y[0], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m0);
    *m0 = srslte_vec_max_fi(q->corr_output_sch_m0, SRSLTE_SCH_N);
    if(m0_value) {
      *m0_value = q->corr_output_sch_m0[*m0];
    }
    srslte_vec_prod_cfc(y[1], q->sch_fc_tables[q->sch_table_index].z1[*m0], y[1], SRSLTE_SCH_N);
    corr_all_sz_partial_sch(y[1], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m1);
    *m1 = srslte_vec_max_fi(q->corr_output_sch_m1, SRSLTE_SCH_N);
    if(m1_value) {
      *m1_value = q->corr_output_sch_m1[*m1];
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

static void extract_sch_pair_combining(srslte_sss_synch_t *q, uint32_t nof_prb, cf_t *input0, cf_t *input1, cf_t *ce, cf_t y[2][SRSLTE_SCH_N]) {
  cf_t input_fft0[SRSLTE_SYMBOL_SZ_MAX], input_fft1[SRSLTE_SYMBOL_SZ_MAX];

  // Front SCH sequence.
  srslte_dft_run_c(&q->dftp_input, input0, input_fft0);
  // Middle SCH sequence.
  srslte_dft_run_c(&q->dftp_input, input1, input_fft1);
  // Channel Equalization (PSS-based channel estimates) can be applied to the middle SCH sequence as it is beside the PSS.
  if(ce) {
    srslte_vec_div_ccc(&input_fft1[q->fft_size/2-SRSLTE_SCH_N], ce,
                       &input_fft1[q->fft_size/2-SRSLTE_SCH_N], 2*SRSLTE_SCH_N);
  }

#if(0)
  for(uint32_t k = 0; k < q->fft_size; k++) {
    printf("%d - %1.4f,%1.4f - %1.4f,%1.4f\n", k, creal(input_fft0[k]), cimag(input_fft0[k]), creal(input_fft1[k]), cimag(input_fft1[k]));
  }
#endif

  for(int i = 0; i < SRSLTE_SCH_N; i++) {
    y[0][i] = (input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * i]     + input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * i])/2.0;
    y[1][i] = (input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * i + 1] + input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * i + 1])/2.0;
  }

#if(0)
  for(uint32_t k = 0; k < SRSLTE_SCH_N; k++) {
    printf("%d \t y00: %+1.4f,%+1.4f \t y01: %+1.4f,%+1.4f \t y0comb: %+1.4f,%+1.4f\n", k, creal(input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * k]), cimag(input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * k]), creal(input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * k]), cimag(input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * k]), creal(y[0][k]), cimag(y[0][k]));
  }
  printf("\n\n");
  for(uint32_t k = 0; k < SRSLTE_SCH_N; k++) {
    printf("%d \t y10: %+1.4f,%+1.4f \t y11: %+1.4f,%+1.4f \t y1comb: %+1.4f,%+1.4f\n", k, creal(input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * k + 1]), cimag(input_fft0[q->fft_size/2-SRSLTE_SCH_N + 2 * k + 1]), creal(input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * k + 1]), cimag(input_fft1[q->fft_size/2-SRSLTE_SCH_N + 2 * k + 1]), creal(y[1][k]), cimag(y[1][k]));
  }
#endif

#if(0)
  printf("[SCH pair generic] nof_prb: %d\n", nof_prb);
  for(uint32_t k = 0; k < SRSLTE_SCH_N; k++) {
    printf("%d - %f,%f - %f,%f\n", k, creal(y[0][k]), cimag(y[0][k]), creal(y[1][k]), cimag(y[1][k]));
  }
#endif

  srslte_vec_prod_cfc(y[0], q->fc_tables[q->sch_table_index].c[0], y[0], SRSLTE_SCH_N);
  srslte_vec_prod_cfc(y[1], q->fc_tables[q->sch_table_index].c[1], y[1], SRSLTE_SCH_N);
}

// Find m0 and m1 values for SCH signal sequences added to the front amd middle of the subframe (OFDM symbols 1 and 6).
// Use temporal receive combing to improve the reception ratio.
// We send the same sequence twice but in different parts of the first subframe (OFDM symbols 1 and 6).
int srslte_sch_m0m1_partial_combining(srslte_sss_synch_t *q, uint32_t nof_prb, cf_t *input0, cf_t *input1, uint32_t M, cf_t ce[2*SRSLTE_SCH_N], uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL   &&
      input0            != NULL   &&
      input1            != NULL   &&
      m0                != NULL   &&
      m1                != NULL   &&
      M                 <= MAX_M)
  {
    cf_t y[2][SRSLTE_SCH_N];

    // Extract SCH sequences from front and middle of the subframe and then combine them.
    extract_sch_pair_combining(q, nof_prb, input0, input1, ce, y);

    corr_all_sz_partial_sch(y[0], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m0);
    *m0 = srslte_vec_max_fi(q->corr_output_sch_m0, SRSLTE_SCH_N);
    if(m0_value) {
      *m0_value = q->corr_output_sch_m0[*m0];
    }
    srslte_vec_prod_cfc(y[1], q->sch_fc_tables[q->sch_table_index].z1[*m0], y[1], SRSLTE_SCH_N);
    corr_all_sz_partial_sch(y[1], q->sch_fc_tables[q->sch_table_index].s, M, q->corr_output_sch_m1);
    *m1 = srslte_vec_max_fi(q->corr_output_sch_m1, SRSLTE_SCH_N);
    if(m1_value) {
      *m1_value = q->corr_output_sch_m1[*m1];
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}
// ************************** SCATTER PHY Implementation - END ******************************
