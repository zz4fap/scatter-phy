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
 *  File:         ue_dl.h
 *
 *  Description:  UE downlink object.
 *
 *                This module is a frontend to all the downlink data and control
 *                channel processing modules.
 *
 *  Reference:
 *****************************************************************************/

#ifndef UEDL_H
#define UEDL_H

#include <stdbool.h>
#include <time.h>

#include "srslte/ch_estimation/chest_dl.h"
#include "srslte/dft/ofdm.h"
#include "srslte/common/phy_common.h"

#include "srslte/phch/dci.h"
#include "srslte/phch/pcfich.h"
#include "srslte/phch/pdcch.h"
#include "srslte/phch/pdsch.h"
#include "srslte/phch/pdsch_cfg.h"
#include "srslte/phch/phich.h"
#include "srslte/phch/ra.h"
#include "srslte/phch/regs.h"

#include "srslte/sync/cfo.h"

#include "srslte/utils/vector.h"
#include "srslte/utils/debug.h"

#include "srslte/config.h"

#define ENABLE_UE_DL_PRINTS 1

#define UE_DL_PRINT(_fmt, ...) do { if(ENABLE_UE_DL_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[UE DL PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_DL_DEBUG(_fmt, ...) do { if(ENABLE_UE_DL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[UE DL DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_DL_INFO(_fmt, ...) do { if(ENABLE_UE_DL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[UE DL INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_DL_ERROR(_fmt, ...) do { fprintf(stdout, "[UE DL ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define MAX_CANDIDATES_UE  16 // From 36.213 Table 9.1.1-1
#define MAX_CANDIDATES_COM 6 // From 36.213 Table 9.1.1-1
#define MAX_CANDIDATES (MAX_CANDIDATES_UE + MAX_CANDIDATES_COM)

typedef struct {
  srslte_dci_format_t format;
  srslte_dci_location_t loc[MAX_CANDIDATES];
  uint32_t nof_locations;
} dci_blind_search_t;

typedef struct SRSLTE_API {
  srslte_pcfich_t pcfich;
  srslte_pdcch_t pdcch;
  srslte_pdsch_t pdsch;
  srslte_phich_t phich;
  srslte_regs_t regs;
  srslte_ofdm_t fft;
  srslte_chest_dl_t chest;

  srslte_cfo_t sfo_correct;

  srslte_pdsch_cfg_t pdsch_cfg;
  srslte_softbuffer_rx_t softbuffer;
  srslte_ra_dl_dci_t dl_dci;
  srslte_cell_t cell;

  cf_t *sf_symbols;
  cf_t *ce[SRSLTE_MAX_PORTS];

  srslte_dci_format_t dci_format;
  uint64_t pkt_errors;              // Counts the number of times RNTI decoding of data failed.
  uint64_t pkts_total;              // Counts the number of detected and aligned subframe (PSS/SSS synchronized) subframes.
  uint64_t nof_detected;            // Counts the number of correctly decoded subframes, i.e., DCI was found and RNTI decoding was successful.
  uint64_t wrong_decoding_counter;  // Counts the number of subframes correctly detected and aligned but with no data decoded.
  uint32_t decoded_cfi;             // Stores the decoded CFI.
  int32_t found_dci;                // Stores if the DCI was found or not.
  float noise_estimate;             // Stores the noise estimation for last detected and aligned subframe.

  uint16_t current_rnti;
  dci_blind_search_t current_ss_ue[3][10];
  dci_blind_search_t current_ss_common[3];
  srslte_dci_location_t last_location;
  srslte_dci_location_t last_location_ul;

  srslte_dci_msg_t pending_ul_dci_msg;
  uint16_t pending_ul_dci_rnti;

  float sample_offset;

  uint32_t expected_cfi;             // Stores the expected CFI.

  bool decode_pdcch;

  uint32_t phy_id;

  struct timespec decoding_start_timestamp;

  bool add_sch_to_front;

  uint32_t pss_len;
} srslte_ue_dl_t;

/* This function shall be called just after the initial synchronization */
SRSLTE_API int srslte_ue_dl_init_generic(srslte_ue_dl_t *q,
                                         srslte_cell_t cell,
                                         uint32_t phy_id,
                                         bool add_sch_to_front);

SRSLTE_API int srslte_ue_dl_init(srslte_ue_dl_t *q,
                                srslte_cell_t cell);

SRSLTE_API void srslte_ue_dl_free(srslte_ue_dl_t *q);

SRSLTE_API int srslte_ue_dl_decode_fft_estimate(srslte_ue_dl_t *q,
                                                cf_t *input,
                                                uint32_t sf_idx,
                                                uint32_t *cfi,
                                                float *cfi_corr);

SRSLTE_API int srslte_ue_dl_decode_estimate(srslte_ue_dl_t *q,
                                            uint32_t sf_idx,
                                            uint32_t *cfi,
                                            float *cfi_corr);

SRSLTE_API int srslte_ue_dl_cfg_grant(srslte_ue_dl_t *q,
                                      srslte_ra_dl_grant_t *grant,
                                      uint32_t cfi,
                                      uint32_t sf_idx,
                                      uint32_t rvidx);

SRSLTE_API int srslte_ue_dl_find_ul_dci(srslte_ue_dl_t *q,
                                        uint32_t cfi,
                                        uint32_t sf_idx,
                                        uint16_t rnti,
                                        srslte_dci_msg_t *dci_msg);

SRSLTE_API int srslte_ue_dl_find_dl_dci(srslte_ue_dl_t *q,
                                        uint32_t cfi,
                                        uint32_t sf_idx,
                                        uint16_t rnti,
                                        srslte_dci_msg_t *dci_msg);

SRSLTE_API int srslte_ue_dl_find_dl_dci_type(srslte_ue_dl_t *q,
                                             uint32_t cfi,
                                             uint32_t sf_idx,
                                             uint16_t rnti,
                                             srslte_rnti_type_t rnti_type,
                                             srslte_dci_msg_t *dci_msg);

SRSLTE_API uint32_t srslte_ue_dl_get_ncce(srslte_ue_dl_t *q);

SRSLTE_API void srslte_ue_dl_set_sample_offset(srslte_ue_dl_t * q,
                                               float sample_offset);

SRSLTE_API int srslte_ue_dl_decode(srslte_ue_dl_t * q,
                                   cf_t *input,
                                   uint8_t *data,
                                   uint32_t tti);

SRSLTE_API int srslte_ue_dl_decode_rnti(srslte_ue_dl_t * q,
                                        cf_t *input,
                                        uint8_t *data,
                                        uint32_t tti,
                                        uint16_t rnti);

SRSLTE_API bool srslte_ue_dl_decode_phich(srslte_ue_dl_t *q,
                                          uint32_t sf_idx,
                                          uint32_t n_prb_lowest,
                                          uint32_t n_dmrs);

SRSLTE_API void srslte_ue_dl_reset(srslte_ue_dl_t *q);

SRSLTE_API void srslte_ue_dl_set_rnti(srslte_ue_dl_t *q,
                                      uint16_t rnti);

SRSLTE_API void srslte_ue_dl_save_signal(srslte_ue_dl_t *q,
                                         srslte_softbuffer_rx_t *softbuffer,
                                         uint32_t tti,
                                         uint32_t rv_idx,
                                         uint16_t rnti,
                                         uint32_t cfi);

SRSLTE_API void srslte_ue_dl_set_max_noi(srslte_ue_dl_t *q, uint32_t max_iterations);

SRSLTE_API float srslte_ul_dl_average_noi(srslte_ue_dl_t *q);

SRSLTE_API uint32_t srslte_ue_dl_last_noi(srslte_ue_dl_t *q);

SRSLTE_API void srslte_ue_dl_set_expected_cfi(srslte_ue_dl_t *q, uint32_t expected_cfi);

SRSLTE_API void srslte_ue_dl_set_cfo_csr(srslte_ue_dl_t *q, bool flag);

//******************************************************************************
//***************** Scatter system customized implementations ******************
//******************************************************************************
SRSLTE_API void srslte_ue_dl_set_decode_pdcch(srslte_ue_dl_t *q, bool flag);

SRSLTE_API int srslte_ue_dl_decode_scatter(srslte_ue_dl_t *q, cf_t *input, uint8_t *data, uint32_t tti, uint32_t mcs);

SRSLTE_API int srslte_ue_dl_decode_rnti_scatter(srslte_ue_dl_t *q, cf_t *input, uint8_t *data, uint32_t tti, uint16_t rnti, int mcs);

SRSLTE_API int srslte_ue_dl_cfg_grant_scatter(srslte_ue_dl_t *q, srslte_ra_dl_grant_t *grant, uint32_t cfi, uint32_t sf_idx, uint32_t rvidx);

SRSLTE_API void srslte_ue_dl_set_pss_length(srslte_ue_dl_t *q, uint32_t pss_len);

#endif
