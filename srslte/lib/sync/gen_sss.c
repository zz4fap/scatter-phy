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


#include <strings.h>
#include <stdlib.h>

#include "srslte/sync/sss.h"

/**
 * @brief Function documentation: initSSStables()
 * This function generates the scrambling sequences required for generation of
 * SSS sequence according with 3GPP TS 36.211 version 10.5.0 Release 10.
 */
void generate_zsc_tilde(int *z_tilde, int *s_tilde, int *c_tilde) {

  int i;
  int x[SRSLTE_SSS_N];
  bzero(x, sizeof(int) * SRSLTE_SSS_N);
  x[4] = 1;

  for (i = 0; i < 26; i++)
    x[i + 5] = (x[i + 2] + x[i]) % 2;
  for (i = 0; i < SRSLTE_SSS_N; i++)
    s_tilde[i] = 1 - 2 * x[i];

  for (i = 0; i < 26; i++)
    x[i + 5] = (x[i + 3] + x[i]) % 2;
  for (i = 0; i < SRSLTE_SSS_N; i++)
    c_tilde[i] = 1 - 2 * x[i];

  for (i = 0; i < 26; i++)
    x[i + 5] = (x[i + 4] + x[i + 2] + x[i + 1] + x[i]) % 2;
  for (i = 0; i < SRSLTE_SSS_N; i++)
    z_tilde[i] = 1 - 2 * x[i];
}

void generate_m0m1(uint32_t N_id_1, uint32_t *m0, uint32_t *m1) {
  uint32_t q_prime = N_id_1 / (SRSLTE_SSS_N - 1);
  uint32_t q = (N_id_1 + (q_prime * (q_prime + 1) / 2)) / (SRSLTE_SSS_N - 1);
  uint32_t m_prime = N_id_1 + (q * (q + 1) / 2);
  *m0 = m_prime % SRSLTE_SSS_N;
  *m1 = (*m0 + m_prime / SRSLTE_SSS_N + 1) % SRSLTE_SSS_N;
}


/* table[m0][m1-1]=N_id_1 */
void generate_N_id_1_table(uint32_t table[30][30]) {
  uint32_t m0, m1;
  uint32_t N_id_1;
  for (N_id_1=0;N_id_1<168;N_id_1++) {
    generate_m0m1(N_id_1, &m0, &m1);
    table[m0][m1-1] = N_id_1;
  }
}


void generate_s(int *s, int *s_tilde, uint32_t m0_m1) {
  uint32_t i;
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    s[i] = s_tilde[(i + m0_m1) % SRSLTE_SSS_N];
  }
}

void generate_s_all(int s[SRSLTE_SSS_N][SRSLTE_SSS_N], int *s_tilde) {
  uint32_t i;
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    generate_s(s[i], s_tilde, i);
  }
}

void generate_c(int *c, int *c_tilde, uint32_t N_id_2, bool is_c0) {
  uint32_t i;
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    c[i] = c_tilde[(i + N_id_2 + (is_c0 ? 3 : 0)) % SRSLTE_SSS_N];
  }
}

void generate_z(int *z, int *z_tilde, uint32_t m0_m1) {
  uint32_t i;
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    z[i] = z_tilde[(i + (m0_m1 % 8)) % SRSLTE_SSS_N];
  }
}

void generate_z_all(int z[SRSLTE_SSS_N][SRSLTE_SSS_N], int *z_tilde) {
  uint32_t i;
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    generate_z(z[i], z_tilde, i);
  }
}

void generate_sss_all_tables(srslte_sss_tables_t *tables, uint32_t N_id_2) {
  uint32_t i;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];

  generate_zsc_tilde(z_t, s_t, c_t);
  generate_s_all(tables->s, s_t);
  generate_z_all(tables->z1, z_t);
  for (i = 0; i < 2; i++) {
    generate_c(tables->c[i], c_t, N_id_2, i != 0);
  }
}

void srslte_sss_generate(float *signal0, float *signal5, uint32_t cell_id) {

  uint32_t i;
  uint32_t id1 = cell_id / 3;
  uint32_t id2 = cell_id % 3;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_0[SRSLTE_SSS_N], z1_1[SRSLTE_SSS_N];

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  generate_z(z1_0, z_t, m0);
  generate_z(z1_1, z_t, m1);

  for (i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 0*/
    signal0[2 * i] = (float) (s0[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 0*/
    signal0[2 * i + 1] = (float) (s1[i] * c1[i] * z1_0[i]);
  }
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 5*/
    signal5[2 * i] = (float) (s1[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 5*/
    signal5[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
  }
}

// ************************** SCATTER PHY Implementation - START ******************************
void generate_sch_m0m1(uint32_t sch_value, uint32_t *m0, uint32_t *m1);

void srslte_sss_generate_pkt_number(float *signal0, float *signal5, uint32_t cell_id) {

  uint32_t i;
  uint32_t id1 = cell_id / 3;
  uint32_t id2 = cell_id % 3;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_0[SRSLTE_SSS_N], z1_1[SRSLTE_SSS_N];

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  generate_z(z1_0, z_t, m0);
  generate_z(z1_1, z_t, m1);

  for (i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 0*/
    signal0[2 * i] = (float) (s0[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 0*/
    signal0[2 * i + 1] = (float) (s1[i] * c1[i] * z1_0[i]);
  }
  for (i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 5*/
    signal5[2 * i] = (float) (s1[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 5*/
    signal5[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
  }
}

void srslte_sss_sf5_generate_nof_packets(float *signal5, uint32_t number_of_packets) {

  uint32_t i;
  uint32_t id1 = number_of_packets / 3;
  uint32_t id2 = number_of_packets % 3;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_1[SRSLTE_SSS_N];

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  generate_z(z1_1, z_t, m1);

  for(i = 0; i < SRSLTE_SSS_N; i++) {
    // Even Resource Elements: Sub-frame 5
    signal5[2 * i] = (float) (s1[i] * c0[i]);
    // Odd Resource Elements: Sub-frame 5
    signal5[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
  }
}

void srslte_sss_sf0_generate_nof_packets(float *signal0, uint32_t number_of_packets) {

  uint32_t i;
  uint32_t id1 = number_of_packets / 3;
  uint32_t id2 = number_of_packets % 3;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_0[SRSLTE_SSS_N];

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  generate_z(z1_0, z_t, m0);

  for(i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 0*/
    signal0[2 * i] = (float) (s0[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 0*/
    signal0[2 * i + 1] = (float) (s1[i] * c1[i] * z1_0[i]);
  }
}

void srslte_sss_generate_nof_packets_signal(float *signal, uint32_t sf_idx, uint32_t number_of_packets) {
  uint32_t i;
  uint32_t cell_id;
  uint32_t id1;
  uint32_t id2;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_0[SRSLTE_SSS_N], z1_1[SRSLTE_SSS_N];

  // Translate from number of TBs into cell ID.
  cell_id = ((number_of_packets-1)*3);
  id1 = cell_id / 3;
  id2 = cell_id % 3;

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  //printf("Generating SSS: for #slots: %d - sf_idx: %d - cell_id: %d - id1: %d - id2: %d - m0: %d - m1: %d\n",number_of_packets,sf_idx,cell_id,id1,id2,m0,m1);

  if(sf_idx == 5) {
    generate_z(z1_1, z_t, m1);
    for(i = 0; i < SRSLTE_SSS_N; i++) {
      // Even Resource Elements: Sub-frame 5.
      signal[2 * i] = (float) (s1[i] * c0[i]);
      // Odd Resource Elements: Sub-frame 5.
      signal[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
    }
  } else {
    generate_z(z1_0, z_t, m0);
    for(i = 0; i < SRSLTE_SSS_N; i++) {
      // Even Resource Elements: Sub-frame 0
      signal[2 * i] = (float) (s0[i] * c0[i]);
      // Odd Resource Elements: Sub-frame 0
      signal[2 * i + 1] = (float) (s1[i] * c1[i] * z1_0[i]);
    }
  }
}

// -----------------------------------------------------------------------------
// -------------------- Scatter Signal Related Functions -----------------------
// -----------------------------------------------------------------------------
void generate_sch_s(int *s, int *s_tilde, uint32_t m0_m1) {
  uint32_t i;
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    s[i] = s_tilde[(i + m0_m1) % SRSLTE_SCH_N];
  }
}

void generate_sch_c(int *c, int *c_tilde, uint32_t sch_table_index, bool is_c0) {
  uint32_t i;
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    c[i] = c_tilde[(i + sch_table_index + (is_c0 ? 3 : 0)) % SRSLTE_SCH_N];
  }
}

void generate_sch_z(int *z, int *z_tilde, uint32_t m0_m1) {
  uint32_t i;
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    z[i] = z_tilde[(i + (m0_m1 % 8)) % SRSLTE_SCH_N];
  }
}

void generate_sch_s_all(int s[SRSLTE_SCH_N][SRSLTE_SCH_N], int *s_tilde) {
  uint32_t i;
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    generate_sch_s(s[i], s_tilde, i);
  }
}

void generate_sch_z_all(int z[SRSLTE_SCH_N][SRSLTE_SCH_N], int *z_tilde) {
  uint32_t i;
  for(i = 0; i < SRSLTE_SCH_N; i++) {
    generate_sch_z(z[i], z_tilde, i);
  }
}

void generate_sch_zsc_tilde(int *z_tilde, int *s_tilde, int *c_tilde) {

  int i;
  int x[SRSLTE_SCH_N];
  bzero(x, sizeof(int) * SRSLTE_SCH_N);
  x[4] = 1;

  for(i = 0; i < 26; i++)
    x[i + 5] = (x[i + 2] + x[i]) % 2;
  for(i = 0; i < SRSLTE_SCH_N; i++)
    s_tilde[i] = 1 - 2 * x[i];

  for(i = 0; i < 26; i++)
    x[i + 5] = (x[i + 3] + x[i]) % 2;
  for(i = 0; i < SRSLTE_SCH_N; i++)
    c_tilde[i] = 1 - 2 * x[i];

  for(i = 0; i < 26; i++)
    x[i + 5] = (x[i + 4] + x[i + 2] + x[i + 1] + x[i]) % 2;
  for(i = 0; i < SRSLTE_SCH_N; i++)
    z_tilde[i] = 1 - 2 * x[i];
}

void generate_sch_all_tables(srslte_sch_tables_t *tables, uint32_t sch_table_index) {
  uint32_t i;
  int s_t[SRSLTE_SCH_N], c_t[SRSLTE_SCH_N], z_t[SRSLTE_SCH_N];

  generate_sch_zsc_tilde(z_t, s_t, c_t);
  generate_sch_s_all(tables->s, s_t);
  generate_sch_z_all(tables->z1, z_t);
  for(i = 0; i < 2; i++) {
    generate_sch_c(tables->c[i], c_t, sch_table_index, i != 0);
  }
}

// Generate Scatter sequence from pair of values.
uint32_t srslte_sch_generate_from_pair(float *signal, bool encode_radio_id, uint32_t value0, uint32_t value1) {
  uint32_t pair;
  if(encode_radio_id) {
    pair = value0*2 + value1;
  } else {
    pair = value0*26 + value1;
  }
  srslte_sch_generate(signal, pair);
  return pair;
}

// Generate Scatter sequence from single value.
void srslte_sch_generate(float *signal, uint32_t value) {
  uint32_t m0, m1, id2;
  int s_t[SRSLTE_SCH_N], c_t[SRSLTE_SCH_N], z_t[SRSLTE_SCH_N];
  int s0[SRSLTE_SCH_N], s1[SRSLTE_SCH_N], c0[SRSLTE_SCH_N], c1[SRSLTE_SCH_N], z1_1[SRSLTE_SCH_N];

  id2 = (value*3) % 3;

  generate_sch_m0m1(value, &m0, &m1);

  //printf("Generating SCH: for value: %d - id1: %d - id2: %d - m0: %d - m1: %d\n", value, value, id2, m0, m1);

  generate_sch_zsc_tilde(z_t, s_t, c_t);

  generate_sch_s(s0, s_t, m0);
  generate_sch_s(s1, s_t, m1);

#if(0)
  for(int k = 0; k < SRSLTE_SCH_N; k++) {
    printf("s0[%d]: %d \t s1[%d]: %d\n", k, s0[k], k, s1[k]);
  }
#endif


  generate_sch_c(c0, c_t, id2, 0);
  generate_sch_c(c1, c_t, id2, 1);

  generate_sch_z(z1_1, z_t, m1);

  for(uint32_t i = 0; i < SRSLTE_SCH_N; i++) {
    signal[2 * i] = (float) (s1[i] * c0[i]);
    signal[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
  }
}

void generate_sch_m0m1(uint32_t sch_value, uint32_t *m0, uint32_t *m1) {
  *m0 = floor(sch_value/SRSLTE_SCH_N);
  *m1 = sch_value % SRSLTE_SCH_N;
}

// table[m0][m1] = sch_value.
void generate_sch_table(uint32_t table[SRSLTE_SCH_N][SRSLTE_SCH_N]) {
  uint32_t m0, m1;
  for(uint32_t sch_value = 0; sch_value < (SRSLTE_SCH_N*SRSLTE_SCH_N); sch_value++) {
    generate_sch_m0m1(sch_value, &m0, &m1);
    if(m0 < 0 || m1 < 0) {
      fprintf(stderr, "Error generating SCH table: m0: %d - m1: %d.......\n", m0, m1);
      exit(-1);
    }
    table[m0][m1] = sch_value;
  }
}
// ************************** SCATTER PHY Implementation - END ******************************
