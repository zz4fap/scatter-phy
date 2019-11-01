#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <uhd.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#define RNTI 0x1234

#define NOF_PRB 25

#define DEFAULT_CFI 1

#define MAX_NOF_TBI 4

#define SUBFRAME_NUMBER 5

#define SUBFRAME_SIZE 5760

#define NUMBER_OF_FILES 30

#define CFO_ENABLED 1

void create_basis_signal(int freq_idx, cf_t *basis_signal, uint32_t length);
void apply_cfo(cf_t *input, cf_t *output, int freq_idx, uint32_t length);
int cfo_i_estimate(srslte_sync_t *q, cf_t *input, int find_offset, int *peak_pos, float *peak_value);
float cfo_estimate_cp(srslte_sync_t *q, cf_t *input);

bool print_data = false;

float BASIS_SIGNAL_LEN = 38400.0;

int main() {

  uint32_t pdsch_num_rxd_bits = 0;
  srslte_ue_dl_t ue_dl;
  srslte_cell_t cell_ue;
  uint8_t decoded_data[10000];
  cf_t subframe_buffer[SUBFRAME_SIZE];
  srslte_filesource_t q;
  srslte_cfo_t cfocorr;
  srslte_pss_synch_t pss;
  srslte_sync_t sfind;
  int ret = 0;
  char filename[200];

  cell_ue.nof_prb         = NOF_PRB;            // nof_prb
  cell_ue.nof_ports       = 1;                  // nof_ports
  cell_ue.bw_idx          = 0;                  // bw idx
  cell_ue.id              = 0;                  // cell_id
  cell_ue.cp              = SRSLTE_CP_NORM;     // cyclic prefix
  cell_ue.phich_length    = SRSLTE_PHICH_NORM;  // PHICH length
  cell_ue.phich_resources = SRSLTE_PHICH_R_1;   // PHICH resources

  // Initialize ue_dl object.
  if(srslte_ue_dl_init(&ue_dl, cell_ue)) {
    fprintf(stderr, "Error initiating UE downlink processing module\n");
    exit(-1);
  }

  if(srslte_cfo_init_finer(&cfocorr, SUBFRAME_SIZE)) {
    fprintf(stderr, "Error initiating CFO\n");
    exit(-1);
  }

  if(srslte_pss_synch_init_fft(&pss, SUBFRAME_SIZE, srslte_symbol_sz(cell_ue.nof_prb))) {
    fprintf(stderr, "Error initializing PSS object\n");
    exit(-1);
  }

  if(srslte_sync_init(&sfind, SUBFRAME_SIZE, SUBFRAME_SIZE, srslte_symbol_sz(cell_ue.nof_prb), 0, SUBFRAME_NUMBER)) {
    fprintf(stderr, "Error initiating sync find\n");
    exit(-1);
  }

  // Set the number of FFT bins used.
  srslte_cfo_set_fft_size_finer(&cfocorr, srslte_symbol_sz(cell_ue.nof_prb));

  // Configure downlink receiver for the SI-RNTI since will be the only one we'll use.
  // This is the User RNTI.
  srslte_ue_dl_set_rnti(&ue_dl, RNTI);

  // Set the expected CFI.
  srslte_ue_dl_set_expected_cfi(&ue_dl, DEFAULT_CFI);

  // Set the maxium number of turbo decoder iterations.
  srslte_ue_dl_set_max_noi(&ue_dl, MAX_NOF_TBI);

  // Enable estimation of CFO based on CSR signals.
  srslte_ue_dl_set_cfo_csr(&ue_dl, false);

  // Set Nid2.
  srslte_pss_synch_set_N_id_2(&pss, 0);

  srslte_sync_cfo_i_detec_en(&sfind, true);

  srslte_sync_set_N_id_2(&sfind, 0);

  uint32_t nof_correctly_decoded_subframes = 0;
  for(uint32_t file_cnt = 0; file_cnt < NUMBER_OF_FILES; file_cnt++) {

    // Create filename to be read.
    //sprintf(filename, "/home/zz4fap/work/dcf_tdma/scatter/build/transmitted_subframe_1_cnt_%d.dat",file_cnt);
    sprintf(filename, "/home/zz4fap/work/dcf_tdma/scatter/build/received_subframe_1_cnt_%d.dat",file_cnt);

    // Create reading object.
    ret = srslte_filesource_init(&q, filename, SRSLTE_COMPLEX_FLOAT_BIN);
    if(ret < 0) {
      printf("Error opening file......\n");
      exit(-1);
    }

    // Read the samples from file.
    ret = srslte_filesource_read(&q, subframe_buffer, SUBFRAME_SIZE);
    if(ret < 0) {
      printf("Error reading file......\n");
      exit(-1);
    }

    // Free reading object.
    srslte_filesource_free(&q);

#if(CFO_ENABLED==1)

    // Apply CFO to the subframe signal.
    float freq = 0.3;
    srslte_cfo_correct_finer(&cfocorr, subframe_buffer, subframe_buffer, freq/384.0);
    printf("Applying CFO of %f [Hz] to the subframe.\n",freq*15000.0);

    // Find PSS correlation peak.
    uint32_t peak_position;
    int offset = (int)SUBFRAME_SIZE;
    srslte_sync_find_ret_t sync_ret = srslte_sync_find(&sfind, subframe_buffer, -offset, &peak_position);
    printf("[APP] sync_ret: %d - Peak value: %1.2f - Peak position: %d\n", sync_ret, sfind.peak_value, peak_position);

    // PSS-based Integer CFO estimation: this estimator is able to find integer CFO in the range: -1, 0 and 1.
    if(fabs(freq) >= 0.0) {
      int peak_pos = 0;
      float peak_value = 0.0;
      int cfo_i = cfo_i_estimate(&sfind, subframe_buffer, 0, &peak_pos, &peak_value);
      printf("----> PSS-based Integer CFO estimation: %d - peak_pos: %d - peak_value: %f\n", cfo_i, peak_pos, peak_value);
    }

    // PSS-based CFO estimation: this type of estimation works for CFO frequencies less than 15 KHz and
    // Therefore, can be used also as integer CFO estimator of at most one subcarrier.
    float pss_based_cfo = srslte_pss_synch_cfo_compute(&pss, &subframe_buffer[peak_position - srslte_symbol_sz(cell_ue.nof_prb)]);
    printf("----> PSS-based CFO estimation: %1.4f\n", pss_based_cfo*15000.0);

    // PSS-based CFO estimation: this type of estimation works for CFO frequencies less than 15 KHz and
    // Therefore, can be used also as integer CFO estimator of at most one subcarrier.
    float pss_based_cfo2 = srslte_pss_synch_cfo_compute2(&pss, &subframe_buffer[peak_position - srslte_symbol_sz(cell_ue.nof_prb)]);
    printf("----> PSS-based CFO estimation 2: %1.4f\n", pss_based_cfo2*15000.0);

    // CP-based CFO estimation: this type of estimation works for CFO frequencies less than 7.5 KHz.
    float cp_based_cfo = cfo_estimate_cp(&sfind, subframe_buffer);
    printf("----> CP-based CFO estimation: %1.4f\n", cp_based_cfo*15000.0);

    // Apply CFO to the signal.
    srslte_cfo_correct_finer(&cfocorr, subframe_buffer, subframe_buffer, -pss_based_cfo/384.0);
#endif

    // The function bwlow returns the number of correcly decoded bits.
    pdsch_num_rxd_bits = srslte_ue_dl_decode(&ue_dl, subframe_buffer, decoded_data, SUBFRAME_NUMBER);

    double rssi = 10*log10(srslte_vec_avg_power_cf(subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(cell_ue.nof_prb))));
    double rsrq = 10*log10(srslte_chest_dl_get_rsrq(&ue_dl.chest));
    double rsrp = 10*log10(srslte_chest_dl_get_rsrp(&ue_dl.chest));
    double noise = srslte_chest_dl_get_noise_estimate(&ue_dl.chest);

    printf("MCS: %d - RSSI: %1.2f - RSRQ: %1.2f - RSRP: %1.2f - Noise: %1.2e\n", ue_dl.pdsch_cfg.grant.mcs.idx, rssi, rsrq, rsrp, noise);

    printf("file_cnt: %d\n", file_cnt);
    if(pdsch_num_rxd_bits > 0) {
      printf("pdsch_num_rxd_bits: %d\n",pdsch_num_rxd_bits/8);
      if(print_data) {
        for(uint32_t k = 0; k < (pdsch_num_rxd_bits/8); k++) {
          printf("Rx data[%d]: %d\n",k,decoded_data[k]);
        }
      }
      printf("***********************************************\n");
      nof_correctly_decoded_subframes++;
    } else {
      printf("Error decoding file number: %d !!!!!!\n",file_cnt);
      printf("pdsch_num_rxd_bits: %d !!!!!!!!\n",pdsch_num_rxd_bits);
    }
  }

  printf("------> Nof correctly decoded subframes: %d\n", nof_correctly_decoded_subframes);

  // Free all UE related structures.
  srslte_ue_dl_free(&ue_dl);
  // Free all CFO related structures.
  srslte_cfo_free_finer(&cfocorr);
  // Free all PSS related structures.
  srslte_pss_synch_free(&pss);
  // Free all Sync related structures.
  srslte_sync_free(&sfind);

  return 0;
}

void create_basis_signal(int freq_idx, cf_t *basis_signal, uint32_t length) {
  for(uint32_t n = 0; n < length; n++) {
    basis_signal[n] = cexpf(_Complex_I * 2 * M_PI * (float) freq_idx * (float) n / BASIS_SIGNAL_LEN);
  }
}

void apply_cfo(cf_t *input, cf_t *output, int freq_idx, uint32_t length) {
  cf_t *basis_signal = (cf_t*)srslte_vec_malloc(length*sizeof(cf_t));
  if(!basis_signal) {
    perror("malloc");
    exit(-1);
  }
  create_basis_signal(freq_idx, basis_signal, length);
  srslte_vec_prod_ccc(basis_signal, input, output, length);
  if(basis_signal) {
    free(basis_signal);
  }
}

// Integer CFO estimation with PSS signal.
int cfo_i_estimate(srslte_sync_t *q, cf_t *input, int find_offset, int *peak_pos, float *peak_value) {
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

// CFO estimation based on CP correlation.
float cfo_estimate_cp(srslte_sync_t *q, cf_t *input) {
  cf_t cp_corr = srslte_cp_corr(&q->cp_synch, input, 7);
  float cfo = -carg(cp_corr) / M_PI / 2;
  return cfo;
}
