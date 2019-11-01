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
 *  File:         sch.h
 *
 *  Description:  Common UL and DL shared channel encode/decode functions.
 *
 *  Reference:    3GPP TS 36.212 version 10.0.0 Release 10
 *****************************************************************************/

#ifndef SCH_
#define SCH_

#include "srslte/config.h"
#include "srslte/common/phy_common.h"
#include "srslte/fec/rm_turbo.h"
#include "srslte/fec/turbocoder.h"
#include "srslte/fec/turbodecoder.h"
#include "srslte/fec/crc.h"
#include "srslte/phch/pdsch_cfg.h"
#include "srslte/phch/pusch_cfg.h"
#include "srslte/phch/uci.h"

#define ENABLE_SCH_PRINTS 0

#define SCH_PRINT(_fmt, ...) do { if(ENABLE_SCH_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[SCH PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define SCH_DEBUG(_fmt, ...) do { if(ENABLE_SCH_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[SCH DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define SCH_INFO(_fmt, ...) do { if(ENABLE_SCH_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[SCH INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define SCH_ERROR(_fmt, ...) do { fprintf(stdout, "[SCH ERROR]: " _fmt, __VA_ARGS__); } while(0)

#ifndef SRSLTE_RX_NULL
#define SRSLTE_RX_NULL 10000
#endif

#ifndef SRSLTE_TX_NULL
#define SRSLTE_TX_NULL 100
#endif

/* DL-SCH AND UL-SCH common functions */
typedef struct SRSLTE_API {

  uint32_t max_iterations;
  uint32_t nof_iterations;
  float average_nof_iterations;

  bool llr_is_8bit;

  /* buffers */
  uint8_t *cb_in;
  uint8_t *parity_bits;
  void *e;
  uint8_t *temp_g_bits;
  uint32_t *ul_interleaver;
  srslte_uci_bit_t ack_ri_bits[12*288];
  uint32_t nof_ri_ack_bits;

  srslte_tcod_t encoder;
  srslte_tdec_t decoder;
  srslte_crc_t crc_tb;
  srslte_crc_t crc_cb;

  srslte_uci_cqi_pusch_t uci_cqi;

  // Some error counters.
  uint64_t filler_bits_error;
  uint64_t nof_cbs_exceeds_softbuffer_size_error;
  uint64_t rate_matching_error;
  uint64_t cb_crc_error;
  uint64_t tb_crc_error;

  uint32_t phy_id;

} srslte_sch_t;

SRSLTE_API int srslte_sch_init_generic(srslte_sch_t *q, uint32_t phy_id);

SRSLTE_API int srslte_sch_init(srslte_sch_t *q);

SRSLTE_API void srslte_sch_free(srslte_sch_t *q);


SRSLTE_API void srslte_sch_set_max_noi(srslte_sch_t *q,
                                       uint32_t max_iterations);

SRSLTE_API float srslte_sch_average_noi(srslte_sch_t *q);

SRSLTE_API uint32_t srslte_sch_last_noi(srslte_sch_t *q);

SRSLTE_API int srslte_dlsch_encode(srslte_sch_t *q,
                                   srslte_pdsch_cfg_t *cfg,
                                   srslte_softbuffer_tx_t *softbuffer,
                                   uint8_t *data,
                                   uint8_t *e_bits);

SRSLTE_API int srslte_dlsch_encode2(srslte_sch_t *q,
                                    srslte_pdsch_cfg_t *cfg,
                                    srslte_softbuffer_tx_t *softbuffer,
                                    uint8_t *data,
                                    uint8_t *e_bits,
                                    int codeword_idx);

SRSLTE_API int srslte_dlsch_decode(srslte_sch_t *q,
                                   srslte_pdsch_cfg_t *cfg,
                                   srslte_softbuffer_rx_t *softbuffer,
                                   int16_t *e_bits,
                                   uint8_t *data);

SRSLTE_API int srslte_dlsch_decode2(srslte_sch_t *q,
                                   srslte_pdsch_cfg_t *cfg,
                                   srslte_softbuffer_rx_t *softbuffer,
                                   int16_t *e_bits,
                                   uint8_t *data,
                                   int codeword_idx);

SRSLTE_API int srslte_ulsch_encode(srslte_sch_t *q,
                                   srslte_pusch_cfg_t *cfg,
                                   srslte_softbuffer_tx_t *softbuffer,
                                   uint8_t *data,
                                   uint8_t *g_bits,
                                   uint8_t *q_bits);

SRSLTE_API int srslte_ulsch_uci_encode(srslte_sch_t *q,
                                       srslte_pusch_cfg_t *cfg,
                                       srslte_softbuffer_tx_t *softbuffer,
                                       uint8_t *data,
                                       srslte_uci_data_t uci_data,
                                       uint8_t *g_bits,
                                       uint8_t *q_bits);

SRSLTE_API int srslte_ulsch_decode(srslte_sch_t *q,
                                   srslte_pusch_cfg_t *cfg,
                                   srslte_softbuffer_rx_t *softbuffer,
                                   int16_t *q_bits,
                                   int16_t *g_bits,
                                   uint8_t *data);

SRSLTE_API int srslte_ulsch_uci_decode(srslte_sch_t *q,
                                       srslte_pusch_cfg_t *cfg,
                                       srslte_softbuffer_rx_t *softbuffer,
                                       int16_t *q_bits,
                                       int16_t *g_bits,
                                       uint8_t *data,
                                       srslte_uci_data_t *uci_data);

SRSLTE_API int srslte_ulsch_uci_decode_ri_ack(srslte_sch_t *q,
                                              srslte_pusch_cfg_t *cfg,
                                              srslte_softbuffer_rx_t *softbuffer,
                                              int16_t *q_bits,
                                              uint8_t *c_seq,
                                              srslte_uci_data_t *uci_data);

SRSLTE_API float srslte_sch_beta_cqi(uint32_t I_cqi);

SRSLTE_API uint32_t srslte_sch_find_Ioffset_ack(float beta);

SRSLTE_API uint32_t srslte_sch_find_Ioffset_cqi(float beta);

SRSLTE_API uint32_t srslte_sch_find_Ioffset_ri(float beta);

#endif // SCH_
