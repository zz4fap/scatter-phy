#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "prb_dl.h"
#include "srslte/phch/psch.h"
#include "srslte/common/phy_common.h"
#include "srslte/utils/bit.h"
#include "srslte/utils/vector.h"
#include "srslte/utils/debug.h"
#include "srslte/sync/pss.h"

const uint8_t srslte_psch_crc_mask[PSCH_MAX_NOF_PORTS][NOF_CRC_BITS] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 } };

bool srslte_psch_exists(int nframe, int nslot) {
  return (!(nframe % 5) && nslot == 1);
}

cf_t *offset_original;

int srslte_psch_cp(cf_t *input, cf_t *output, srslte_cell_t cell, bool put) {
  int i;
  cf_t *ptr;

  offset_original = input;

  if(put) {
    ptr = input;
    output += cell.nof_prb * SRSLTE_NRE / 2 - 36;
  } else {
    ptr = output;
    input += cell.nof_prb * SRSLTE_NRE / 2 - 36;
  }

  // symbol 0 & 1.
  for(i = 0; i < 2; i++) {
    prb_cp_ref(&input, &output, cell.id % 3, 4, 4*6, put);
    if(put) {
      output += cell.nof_prb * SRSLTE_NRE - 2*36 + (cell.id%3==2?1:0);
    } else {
      input += cell.nof_prb * SRSLTE_NRE - 2*36 + (cell.id%3==2?1:0);
    }
  }
  // symbols 2 & 3.
  if(SRSLTE_CP_ISNORM(cell.cp)) {
    for(i = 0; i < 2; i++) {
      prb_cp(&input, &output, 6);
      if(put) {
        output += cell.nof_prb * SRSLTE_NRE - 2*36;
      } else {
        input += cell.nof_prb * SRSLTE_NRE - 2*36;
      }
    }
  } else {
    prb_cp(&input, &output, 6);
    if(put) {
      output += cell.nof_prb * SRSLTE_NRE - 2*36;
    } else {
      input += cell.nof_prb * SRSLTE_NRE - 2*36;
    }
    prb_cp_ref(&input, &output, cell.id % 3, 4, 4*6, put);
  }
  if(put) {
    return input - ptr;
  } else {
    return output - ptr;
  }
}

/**
 * Puts PSCH in slot number 0.
 *
 * Returns the number of symbols written to slot0_data.
 *
 * @param[in] psch PSCH complex symbols to place in slot0_data
 * @param[out] slot0_data Complex symbol buffer for slot0
 * @param[in] cell Cell configuration
 */
int srslte_psch_put(cf_t *psch, cf_t *slot0_data, srslte_cell_t cell) {
  return srslte_psch_cp(psch, slot0_data, cell, true);
}

/**
 * Extracts PSCH from slot number 0
 *
 * Returns the number of symbols written to psch.
 *
 * @param[in] slot0_data Complex symbols for slot0
 * @param[out] psch Extracted complex PSCH symbols
 * @param[in] cell Cell configuration
 */
int srslte_psch_get(cf_t *slot0_data, cf_t *psch, srslte_cell_t cell) {
  return srslte_psch_cp(slot0_data, psch, cell, false);
}

// Initializes the PSCH transmitter and receiver.
int srslte_psch_init(srslte_psch_t *q, srslte_mod_t modulation, uint32_t crc_type, uint32_t payload_len, srslte_cell_t cell) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  uint32_t crc_nof_bits = 0;

  if(q != NULL) {
    ret = SRSLTE_ERROR;

    bzero(q, sizeof(srslte_psch_t));

    q->modulation                   = modulation;
    q->cell                         = cell;
    q->fft_size                     = srslte_symbol_sz(cell.nof_prb);
    q->frame_len                    = SRSLTE_SF_LEN(q->fft_size);
    q->cp_len                       = SRSLTE_CP_ISNORM(cell.cp)?SRSLTE_CP_LEN_NORM(6,q->fft_size):SRSLTE_CP_LEN_EXT(q->fft_size);
    q->crc_checking_enabled         = false;
    q->sch_payload_length           = payload_len;
    q->sch_payload_plus_crc_length  = q->sch_payload_length;

    if(crc_type > 0) {
      switch(crc_type){
        case SRSLTE_LTE_CRC8:
          crc_nof_bits = 8;
          DEBUG("[PSCH init] SRSLTE_LTE_CRC8: %d\n", crc_nof_bits);
          break;
        case SRSLTE_LTE_CRC16:
          crc_nof_bits = 16;
          DEBUG("[PSCH init] SRSLTE_LTE_CRC16: %d\n", crc_nof_bits);
          break;
        case SRSLTE_LTE_CRC24A:
        case SRSLTE_LTE_CRC24B:
          crc_nof_bits = 24;
          DEBUG("[PSCH init] SRSLTE_LTE_CRC24: %d\n", crc_nof_bits);
          break;
        default:
          crc_nof_bits = 16;
      }
      if(srslte_crc_init(&q->crc, crc_type, crc_nof_bits)) {
        goto clean;
      }
      q->crc_checking_enabled = true;
      q->sch_payload_plus_crc_length += crc_nof_bits;
    }
    q->sch_encoded_length = 3*q->sch_payload_plus_crc_length;

    // Calculate number of bits.
    switch(q->modulation) {
      case SRSLTE_MOD_BPSK:
        q->bits_per_symbol = 1;
        q->nof_symbols = q->sch_encoded_length/q->bits_per_symbol;
        q->nof_bits = q->bits_per_symbol*q->nof_symbols;
        DEBUG("Modulation is SRSLTE_MOD_BPSK.\n");
        break;
      case SRSLTE_MOD_QPSK:
        q->bits_per_symbol = 2;
        q->nof_symbols = q->sch_encoded_length/q->bits_per_symbol;
        q->nof_bits = q->bits_per_symbol*q->nof_symbols;
        DEBUG("Modulation is SRSLTE_MOD_QPSK.\n");
        break;
      default:
        q->bits_per_symbol = 2;
        q->nof_symbols = q->sch_encoded_length/q->bits_per_symbol;
        q->nof_bits = q->bits_per_symbol*q->nof_symbols;
        DEBUG("Modulation is SRSLTE_MOD_QPSK.\n");
    }

    if(srslte_modem_table_lte(&q->mod, q->modulation)) {
      goto clean;
    }
    // Generate scrambling/de-scrambling sequence.
    if(srslte_sequence_psch(&q->seq, q->nof_bits, q->cell.id)) {
      goto clean;
    }

    DEBUG("[PSCH init] Payload length: %d - Payload plus CRC: %d - Encoded length: %d - # of symbols: %d - CRC enabled: %d - Total nof bits: %d\n", q->sch_payload_length, q->sch_payload_plus_crc_length, q->sch_encoded_length, q->nof_symbols, q->crc_checking_enabled, q->nof_bits);

    // Instantiate convolutional decoder.
    int poly[3] = { 0x6D, 0x4F, 0x57 };
    if(srslte_viterbi_init(&q->decoder, SRSLTE_VITERBI_37, poly, q->sch_payload_plus_crc_length, true)) {
      goto clean;
    }

    // Set encoder parameters.
    q->encoder.K = 7;
    q->encoder.R = 3;
    q->encoder.tail_biting = true;
    memcpy(q->encoder.poly, poly, 3 * sizeof(int));

    if(srslte_dft_plan(&q->dftp_input, q->fft_size, SRSLTE_DFT_FORWARD, SRSLTE_DFT_COMPLEX)) {
      fprintf(stderr, "Error creating DFT plan.\n");
      goto clean;
    }
    srslte_dft_plan_set_mirror(&q->dftp_input, true);
    srslte_dft_plan_set_dc(&q->dftp_input, true);
    srslte_dft_plan_set_norm(&q->dftp_input, true);

    q->d = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
    if(!q->d) {
      goto clean;
    }
    for(int i = 0; i < q->cell.nof_ports; i++) {
      q->noise_estimate[i] = 0.0;
      q->x[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
      if(!q->x[i]) {
        goto clean;
      }
      q->symbols[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
      if(!q->symbols[i]) {
        goto clean;
      }
    }
    q->llr = srslte_vec_malloc(sizeof(float) * q->nof_symbols * q->bits_per_symbol);
    if(!q->llr) {
      goto clean;
    }
    q->temp = srslte_vec_malloc(sizeof(float) * q->nof_symbols * q->bits_per_symbol);
    if(!q->temp) {
      goto clean;
    }
    q->rm_b = srslte_vec_malloc(sizeof(float) * q->nof_symbols * q->bits_per_symbol);
    if(!q->rm_b) {
      goto clean;
    }
    ret = SRSLTE_SUCCESS;
  }
clean:
  if(ret == SRSLTE_ERROR) {
    srslte_psch_free(q);
  }
  return ret;
}

void srslte_psch_free(srslte_psch_t *q) {
  if(q->d) {
    free(q->d);
  }
  for(int i = 0; i < q->cell.nof_ports; i++) {
    if(q->x[i]) {
      free(q->x[i]);
    }
    if(q->symbols[i]) {
      free(q->symbols[i]);
    }
  }
  if(q->llr) {
    free(q->llr);
  }
  if(q->temp) {
    free(q->temp);
  }
  if(q->rm_b) {
    free(q->rm_b);
  }
  srslte_sequence_free(&q->seq);
  srslte_modem_table_free(&q->mod);
  srslte_viterbi_free(&q->decoder);
  srslte_dft_plan_free(&q->dftp_input);

  bzero(q, sizeof(srslte_psch_t));
}

/**
 * Unpacks PHY Header from PSCH message.
 *
 * @param[in] msg PSCH in an unpacked bit array of size q->sch_payload_length
 * @param[out] PHY Header information will be unpacked here
 */
void srslte_psch_phy_header_unpack(uint8_t *msg, phy_header_t *phy_header) {
  phy_header->nof_slots = srslte_bit_pack(&msg, 5);
  phy_header->mcs       = srslte_bit_pack(&msg, 5);
  phy_header->send_to   = srslte_bit_pack(&msg, 8);
  phy_header->radio_id  = srslte_bit_pack(&msg, 1);
}

/**
 * Packs PHY Header to PSCH message.
 *
 * @param[out] payload Output unpacked bit array of size q->sch_payload_length
 * @param[in] PHY Header configuration to be encoded in PSCH.
 */
void srslte_psch_phy_header_pack(phy_header_t *phy_header, uint8_t *payload, uint32_t len) {
  uint8_t *msg = payload;
  bzero(msg, len);
  srslte_bit_unpack(phy_header->nof_slots,  &msg, 5);
  srslte_bit_unpack(phy_header->mcs,        &msg, 5);
  srslte_bit_unpack(phy_header->send_to,    &msg, 8);
  srslte_bit_unpack(phy_header->radio_id,   &msg, 1);
}

void srslte_psch_crc_set_mask(uint8_t *data, int nof_ports, uint32_t len) {
  for(int i = 0; i < NOF_CRC_BITS; i++) {
    data[len + i] = (data[len + i] + srslte_psch_crc_mask[nof_ports - 1][i]) % 2;
  }
}

/* Checks CRC after applying the mask for the given number of ports.
 * The bits buffer size must be at least q->sch_payload_length bytes.
 * Returns 0 if the data is correct, -1 otherwise
 */
uint32_t srslte_psch_crc_check(srslte_psch_t *q, uint8_t *bits, uint32_t nof_ports) {
  uint8_t data[q->sch_payload_plus_crc_length];
  memcpy(data, bits, q->sch_payload_plus_crc_length * sizeof(uint8_t));
  srslte_psch_crc_set_mask(data, nof_ports, q->sch_payload_length);
  int ret = srslte_crc_checksum(&q->crc, data, q->sch_payload_plus_crc_length);
  if(ret == 0) {
    uint32_t chkzeros=0;
    for(int i = 0; i < q->sch_payload_length; i++) {
      chkzeros += data[i];
    }
    if(chkzeros) {
      return 0;
    } else {
      return SRSLTE_ERROR;
    }
  } else {
    return ret;
  }
}

int srslte_psch_decode_frame(srslte_psch_t *q, uint32_t nof_bits, uint32_t nof_ports) {

  memcpy(q->temp, q->llr, nof_bits * sizeof(float));

  // Descramble.
  srslte_scrambling_f_offset(&q->seq, q->temp, 0, nof_bits);

#if(0)
  printf("----------------------------- Descramble Check -------------------------\n");
  printf("nof_bits: %d\n", nof_bits);
  for(int k = 0; k < nof_bits; k++) {
    printf("%d - %f\n", k, q->temp[k]);
  }
  printf("------------------------------------------------------------------\n");
#endif

  // Unrate matching.
  srslte_rm_conv_rx(q->temp, nof_bits, q->rm_f, q->sch_encoded_length);

#if(0)
  printf("----------------------------- Unrate match Check -------------------------\n");
  for(int k = 0; k < q->sch_encoded_length; k++) {
    printf("%d - %f\n", k, q->rm_f[k]);
  }
  printf("------------------------------------------------------------------\n");
#endif

  // Normalize LLR.
  // NOTE: for some reason it is not necessary to normalize the LLR measurements.
  srslte_vec_sc_prod_fff(q->rm_f, 1.0/((float)1.0), q->rm_f, q->sch_encoded_length);

#if(0)
  printf("----------------------------- Norm LLR Check -------------------------\n");
  for(int k = 0; k < q->sch_encoded_length; k++) {
    printf("%d - %f\n", k, q->rm_f[k]);
  }
  printf("------------------------------------------------------------------\n");
#endif

  // Decode.
  srslte_viterbi_decode_f(&q->decoder, q->rm_f, q->data, q->sch_payload_plus_crc_length);

  if(q->crc_checking_enabled) {
    if(srslte_psch_crc_check(q, q->data, nof_ports) == 0) {
      return 0;
    } else {
      return -1;
    }
  } else {
    return 0;
  }
}

/* Decodes the PSCH channel
 *
 * It tries to decode the PHY Header.
 *
 * Returns 1 if successfully decoded PHY Header, 0 if not and -1 on error.
 */
int srslte_psch_decode(srslte_psch_t *q, cf_t *subframe, cf_t *ce_pss[SRSLTE_MAX_PORTS], float acc_noise_power, uint8_t *sch_payload) {
  cf_t *x[SRSLTE_MAX_LAYERS];
  float noise_power = 0.0;
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q != NULL && subframe != NULL) {

    for(int i = 0; i < q->cell.nof_ports; i++) {
      if(ce_pss[i] == NULL) {
        return SRSLTE_ERROR_INVALID_INPUTS;
      }
    }

    // Number of layers equals number of ports.
    for(int i = 0; i < SRSLTE_MAX_PORTS; i++) {
      x[i] = q->x[i];
    }
    memset(&x[SRSLTE_MAX_PORTS], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - SRSLTE_MAX_PORTS));

    // Extract PSCH OFDM symbol.
    acc_noise_power += srslte_psch_get_from_subframe(q, subframe, q->symbols[0]);

    // Average over all the 10 null subcarriers from PSS plus the 12 null subcarriers from PSCH, totaling 22 null subcarriers.
    noise_power = acc_noise_power/22;

#if(0)
    printf("--------------------------- PSCH Symbols ---------------------------\n");
    for(int k = 0; k < q->nof_symbols; k++) {
      printf("%d - %f,%f\n", k, creal(q->symbols[0][k]), cimag(q->symbols[0][k]));
    }

    printf("\n\n--------------------------- CE Symbols ---------------------------\n");
    for(int k = 0; k < q->nof_symbols; k++) {
      printf("%d - %f,%f\n", k, creal(ce_pss[0][k]), cimag(ce_pss[0][k]));
    }
#endif

    // Try to decode PSCH.
    DEBUG("[PSCH decode] Estimated noise power: %1.4e\n", noise_power);
    DEBUG("[PSCH decode] Trying %d Tx antennas with %d frames and %d symbols.\n", q->cell.nof_ports, 1, q->nof_symbols);

    // In control channels, only diversity is supported.
    if(q->cell.nof_ports == 1) {
      // No need for layer demapping.
      srslte_predecoding_single(q->symbols[0], ce_pss[0], q->d, q->nof_symbols, noise_power);
    } else {
      srslte_predecoding_diversity(q->symbols[0], ce_pss, x, q->cell.nof_ports, q->nof_symbols);
      srslte_layerdemap_diversity(x, q->d, q->cell.nof_ports, q->nof_symbols / q->cell.nof_ports);
    }

    // Demodulate symbols.
    srslte_demod_soft_demodulate(q->modulation, q->d, q->llr, q->nof_symbols);

#if(0)
    printf("------------------------ Predecoding ---------------------\n");
    for(int k = 0; k < q->nof_symbols; k++) {
      printf("%d - %f,%f\n", k, creal(q->d[k]), cimag(q->d[k]));
    }
    printf("----------------------------------------------------------\n");
    printf("\n\n------------------------ LLR -------------------------\n");
    for(int k = 0; k < q->nof_bits; k++) {
      printf("%d - %f\n", k, q->llr[k]);
    }
    printf("----------------------------------------------------------\n");
#endif

    ret = srslte_psch_decode_frame(q, q->nof_bits, q->cell.nof_ports);
    if(ret == 0) {
      if(sch_payload) {
        memcpy(sch_payload, q->data, sizeof(uint8_t) * q->sch_payload_length);
      }
      DEBUG("PSCH decoding successful.\n");
      return 1;
    } else {
      DEBUG("PSCH decoding failed.\n");
    }

  }
  return ret;
}

// Convert the PHY header message to symbols mapped to SLOT #0 ready for transmission.
int srslte_psch_encode(srslte_psch_t *q, uint8_t *sch_payload, cf_t *resource_grid[SRSLTE_MAX_PORTS]) {
  cf_t *x[SRSLTE_MAX_LAYERS];

  if(q != NULL && sch_payload != NULL) {
    for(int i = 0; i < q->cell.nof_ports; i++) {
      if(resource_grid[i] == NULL) {
        return SRSLTE_ERROR_INVALID_INPUTS;
      }
    }

    // Number of layers equals number of ports.
    for(int i = 0; i < q->cell.nof_ports; i++) {
      x[i] = q->x[i];
    }
    memset(&x[q->cell.nof_ports], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - q->cell.nof_ports));

    memcpy(q->data, sch_payload, sizeof(uint8_t) * q->sch_payload_length);

    // Encode & modulate.
    if(q->crc_checking_enabled) {
      srslte_crc_attach(&q->crc, q->data, q->sch_payload_length);
      srslte_psch_crc_set_mask(q->data, q->cell.nof_ports, q->sch_payload_length);
    }

    // Output of the convolutional encoder is equal to 3*q->sch_payload_plus_crc_length.
    srslte_convcoder_encode(&q->encoder, q->data, q->data_enc, q->sch_payload_plus_crc_length);

    // Rate Matching.
    srslte_rm_conv_tx(q->data_enc, q->sch_encoded_length, q->rm_b, q->nof_bits);

#if(0)
    printf("----------------------- Rate Matching ------------------\n");
    printf("input len: %d - output len: %d\n", q->sch_encoded_length, q->nof_bits);
    for(int k = 0; k < q->nof_bits; k++) {
      //printf("%d - data_enc: %d - rm_b: %d\n", k, q->data_enc[k], q->rm_b[k]);
      printf("%d - rm_b: %d\n", k, q->rm_b[k]);
    }
    printf("--------------------------------------------------------\n");
#endif

    srslte_scrambling_b_offset(&q->seq, q->rm_b, 0, q->nof_bits);
    srslte_mod_modulate(&q->mod, q->rm_b, q->d, q->nof_bits);

    // Layer mapping & precoding.
    if(q->cell.nof_ports > 1) {
      srslte_layermap_diversity(q->d, x, q->cell.nof_ports, q->nof_symbols);
      srslte_precoding_diversity(x, q->symbols, q->cell.nof_ports, q->nof_symbols / q->cell.nof_ports);
    } else {
      memcpy(q->symbols[0], q->d, q->nof_symbols * sizeof(cf_t));
    }

    // Map to resource elements.
    for(int i = 0; i < q->cell.nof_ports; i++) {

#if(0)
      printf("----------------------- Transmitted PSCH Symbols -----------------------\n");
      printf("nof_symbols: %d\n",q->nof_symbols);
      for(int k = 0; k < q->nof_symbols; k++) {
        printf("%d - %f,%f\n", k, creal(q->symbols[i][k]), cimag(q->symbols[i][k]));
      }
      printf("-----------------------------------------------------------------------\n");
#endif

      srslte_psch_put_slot(q->symbols[i], resource_grid[i], &q->cell, q->nof_symbols);
    }
    return SRSLTE_SUCCESS;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}

int srslte_psch_put_slot(cf_t *psch, cf_t *slot0_symbols, srslte_cell_t *cell, uint32_t nof_symbols) {
  int k;
  k = (SRSLTE_CP_NSYMB(cell->cp) - 2) * cell->nof_prb * SRSLTE_NRE + cell->nof_prb * SRSLTE_NRE / 2 - (nof_symbols/2);
  memcpy(&slot0_symbols[k], psch, nof_symbols * sizeof(cf_t));
  return nof_symbols;
}

int srslte_psch_get_slot(cf_t *slot0_symbols, cf_t *psch, srslte_cell_t *cell, uint32_t nof_symbols) {
  int k;
  k = (SRSLTE_CP_NSYMB(cell->cp) - 2) * cell->nof_prb * SRSLTE_NRE + cell->nof_prb * SRSLTE_NRE / 2 - (nof_symbols/2);
  memcpy(psch, &slot0_symbols[k], nof_symbols * sizeof(cf_t));
  return nof_symbols;
}

// Retrieve PSCH symbols from time-domain subframe and accumulated noise power.
float srslte_psch_get_from_subframe(srslte_psch_t *q, cf_t *subframe, cf_t *psch) {
  float acc_noise_power = 0.0;
  cf_t temp[q->fft_size];
  int k = (q->frame_len/2) - (2*q->fft_size + q->cp_len);

#if(0)
  printf("--------------------------- Subframe ---------------------------\n");
  for(int j = 0; j < q->fft_size; j++) {
    printf("%d - %f,%f\n", j, creal(subframe[k+j]), cimag(subframe[k+j]));
  }
#endif

  // Transform OFDM symbol into frequency-domain.
  srslte_dft_run_c(&q->dftp_input, &subframe[k], temp);
  // Copy only the useful part of the PSCH symbol.
  memcpy(psch, &temp[(q->fft_size-q->nof_symbols)/2], q->nof_symbols * sizeof(cf_t));
  // Accumulate noise power based on the 6 null subcarriers below the PSCH.
  acc_noise_power += crealf(srslte_vec_dot_prod_conj_ccc(&temp[((q->fft_size-q->nof_symbols)/2) - 6], &temp[((q->fft_size-q->nof_symbols)/2) - 6], 6));
  // Accumulate noise power based on the 6 null subcarriers above the PSCH.
  acc_noise_power += crealf(srslte_vec_dot_prod_conj_ccc(&temp[(q->fft_size+q->nof_symbols)/2],       &temp[(q->fft_size+q->nof_symbols)/2],       6));

#if(0)
  printf("--------------------------- Subframe ---------------------------\n");
  for(int j = 0; j < q->fft_size; j++) {
    printf("%d - %f,%f\n", j, creal(subframe[k+j]), cimag(subframe[k+j]));
  }
  printf("\n\n--------------------------- FFT ---------------------------\n");
  printf("fft size: %d\n", q->fft_size);
  for(int j = 0; j < q->fft_size; j++) {
    printf("%d - %f,%f\n", j, creal(temp[j]), cimag(temp[j]));
  }
  printf("\n\n--------------------------- PSCH ---------------------------\n");
  for(int j = -2; j < (q->nof_symbols+2); j++) {
    printf("%d - %f,%f\n", j, creal(temp[((q->fft_size-q->nof_symbols)/2) + j]), cimag(temp[((q->fft_size-q->nof_symbols)/2) + j]));
  }
  printf("\n\n--------------------------- PSCH symbs ---------------------------\n");
  for(int j = 0; j < q->nof_symbols; j++) {
    printf("%d - %f,%f\n", j, creal(psch[j]), cimag(psch[j]));
  }
#endif

  return acc_noise_power;
}
