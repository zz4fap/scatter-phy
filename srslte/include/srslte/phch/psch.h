#ifndef PSCH_
#define PSCH_

#include "srslte/config.h"
#include "srslte/common/phy_common.h"
#include "srslte/mimo/precoding.h"
#include "srslte/mimo/layermap.h"
#include "srslte/modem/mod.h"
#include "srslte/modem/demod_soft.h"
#include "srslte/scrambling/scrambling.h"
#include "srslte/fec/rm_conv.h"
#include "srslte/fec/convcoder.h"
#include "srslte/fec/viterbi.h"
#include "srslte/fec/crc.h"
#include "srslte/common/phy_common.h"
#include "srslte/utils/convolution.h"

#define PSCH_RE_LEN 60

#define NOF_CRC_BITS 16

#define PSCH_MAX_NOF_PORTS 4

#define SRSLTE_SCH_PAYLOAD_LEN     24
#define SRSLTE_SCH_PAYLOADCRC_LEN  (SRSLTE_SCH_PAYLOAD_LEN+NOF_CRC_BITS)
#define SRSLTE_SCH_ENCODED_LEN     3*(SRSLTE_SCH_PAYLOADCRC_LEN)

typedef struct SRSLTE_API {
  uint8_t nof_slots;  // 5 bits are necessary.
  uint8_t mcs;        // 5 bits are necessary.
  uint8_t send_to;    // 8 bits are necessary.
  uint8_t radio_id;   // 1 bit is necessary.
} phy_header_t;

// PSCH object.
typedef struct SRSLTE_API {
  srslte_cell_t cell;

  bool crc_checking_enabled;

  uint32_t sch_payload_length;
  uint32_t sch_payload_plus_crc_length;
  uint32_t sch_encoded_length;

  uint32_t nof_symbols;
  uint32_t nof_bits;
  srslte_mod_t modulation;

  uint32_t fft_size;
  uint32_t frame_len;
  uint32_t cp_len;

  // buffers
  float noise_estimate[SRSLTE_MAX_PORTS];
  cf_t *symbols[SRSLTE_MAX_PORTS];
  cf_t *x[SRSLTE_MAX_PORTS];
  cf_t *d;
  float *llr;
  float *temp;
  float rm_f[SRSLTE_SCH_ENCODED_LEN];
  uint8_t *rm_b;
  uint8_t data[SRSLTE_SCH_PAYLOADCRC_LEN];
  uint8_t data_enc[SRSLTE_SCH_ENCODED_LEN];
  uint32_t bits_per_symbol;

  uint32_t frame_idx;

  // tx & rx objects
  srslte_modem_table_t mod;
  srslte_sequence_t seq;
  srslte_viterbi_t decoder;
  srslte_crc_t crc;
  srslte_convcoder_t encoder;
  srslte_dft_plan_t dftp_input;
  bool search_all_ports;

} srslte_psch_t;

SRSLTE_API int srslte_psch_init(srslte_psch_t *q, srslte_mod_t modulation, uint32_t crc_type, uint32_t payload_len, srslte_cell_t cell);

SRSLTE_API void srslte_psch_free(srslte_psch_t *q);

SRSLTE_API int srslte_psch_decode(srslte_psch_t *q, cf_t *slot0_symbols, cf_t *ce_pss[SRSLTE_MAX_PORTS], float noise_estimate, uint8_t *sch_payload);

SRSLTE_API int srslte_psch_encode(srslte_psch_t *q, uint8_t *sch_payload, cf_t *resource_grid[SRSLTE_MAX_PORTS]);

SRSLTE_API void srslte_psch_phy_header_unpack(uint8_t *msg, phy_header_t *phy_header);

SRSLTE_API void srslte_psch_phy_header_pack(phy_header_t *phy_header, uint8_t *payload, uint32_t len);

SRSLTE_API int srslte_psch_put_slot(cf_t *psch, cf_t *slot0_symbols, srslte_cell_t *cell, uint32_t nof_symbols);

SRSLTE_API int srslte_psch_get_slot(cf_t *slot0_symbols, cf_t *psch, srslte_cell_t *cell, uint32_t nof_symbols);

SRSLTE_API float srslte_psch_get_from_subframe(srslte_psch_t *q, cf_t *subframe, cf_t *psch);

#endif // PSCH_
