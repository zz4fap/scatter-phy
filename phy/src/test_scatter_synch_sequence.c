#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#define ENABLE_APP_PRINTS 0

#define APP_PRINT(_fmt, ...) do { if(1) { \
  fprintf(stdout, "[APP PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define APP_DEBUG(_fmt, ...) do { if(ENABLE_APP_PRINTS) { \
  fprintf(stdout, "[APP DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define APP_INFO(_fmt, ...) do { if(ENABLE_APP_PRINTS) { \
  fprintf(stdout, "[APP INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define APP_ERROR(_fmt, ...) do { fprintf(stdout, "[APP ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define RNTI 0x1234

#define DEFAULT_CFI 1

#define NOF_SNR_VALUES 26

#define NOF_ITERATIONS 10000

typedef struct {
  bool transmit_pss;
  uint32_t mcs;
  bool transmit_pdsch;
  bool add_noise;
  bool transmit_sch;
  bool sch_temporal_diversity;
  uint32_t psch_payload_len;
  uint32_t max_nof_tbi;
  uint32_t bw_idx;
  bool decode_pdcch;
  bool print_pdsch_stats;
  srslte_mod_t psch_modulation;
  uint32_t psch_crc_type;
  uint32_t initial_subframe_index;
  uint32_t subframe_number;
  bool use_scatter_sync_seq;
  uint32_t scatter_pss_length;
  float pss_boost_factor;
  bool add_only_noise;
  float pss_threshold;
  bool avg_psr_scatter;
  bool enable_second_stage_pss_detection;
  float pss_first_stage_threshold;
  float pss_second_stage_threshold;
} app_args_t;

typedef struct {
  uint32_t sch_value;
  uint32_t value0;
  uint32_t value1;
  uint32_t sch_table_index;
  uint32_t nof_prb;
  uint32_t symb_number;
  uint32_t fft_size;
  sss_alg_t sss_alg;
  srslte_cp_t cp;
  uint32_t m0;
  uint32_t m1;
  float m0_value;
  float m1_value;
} sch_sync_t;

int decode_sss(srslte_sss_synch_t *sss, cf_t *input, uint32_t peak_pos, srslte_cp_t cp, uint32_t fft_size, uint32_t N_id_2, sss_alg_t sss_alg);
int decode_sch(srslte_sss_synch_t *sss, sch_sync_t *q, cf_t *input, uint32_t peak_pos, bool decode_front);
int decode_sch_combining(srslte_sss_synch_t *sss, sss_alg_t sss_alg, cf_t *input, bool decode_radio_id, bool sch_temporal_diversity, uint32_t peak_pos, uint32_t nof_prb, uint32_t fft_size, srslte_cp_t cp, uint32_t *value0, uint32_t *value1, uint32_t* sch_value);
int update_radl(srslte_ra_dl_dci_t* const ra_dl, uint32_t mcs, uint32_t nof_prb);
void generateData(uint32_t numOfBytes, uint8_t *data);

srslte_cell_t cell = {
  .nof_prb          = 25,                 // nof_prb
  .nof_ports        = 1,                  // nof_ports
  .bw_idx           = 3,                  // BW index
  .id               = 0,                  // cell_id
  .cp               = SRSLTE_CP_NORM,     // cyclic prefix
  .phich_resources  = SRSLTE_PHICH_R_1,   // PHICH resources
  .phich_length     = SRSLTE_PHICH_NORM   // PHICH length
};

void usage(char *prog) {
  printf("Usage: %s [cpv]\n", prog);
  printf("\t-c  Set cell id [Default %d]\n", cell.id);
  printf("\t-n  Set number of PRB [Default %d]\n", cell.nof_prb);
  printf("\t-v  [set srslte_verbose to debug, default none]\n");
}

void set_default_arguments(app_args_t *app_args) {
  app_args->transmit_pss                      = true;
  app_args->transmit_sch                      = true;
  app_args->sch_temporal_diversity            = true;
  app_args->transmit_pdsch                    = true;
  app_args->add_noise                         = true;
  app_args->add_only_noise                    = false;
  app_args->mcs                               = 0;
  app_args->max_nof_tbi                       = 10;
  cell.id                                     = 0;
  cell.nof_prb                                = 25;
  app_args->bw_idx                            = 2;
  app_args->print_pdsch_stats                 = false;
  app_args->decode_pdcch                      = false;
  app_args->psch_payload_len                  = 24;               //20; // 20 bits of payload for the case when we use BPSK.
  app_args->psch_modulation                   = SRSLTE_MOD_QPSK;  //SRSLTE_MOD_BPSK;
  app_args->psch_crc_type                     = SRSLTE_LTE_CRC16; //0; // No CRC to be added.
  app_args->initial_subframe_index            = 5;
  app_args->subframe_number                   = 5;
  app_args->avg_psr_scatter                   = true;
  app_args->use_scatter_sync_seq              = true;
  app_args->scatter_pss_length                = 72;
  app_args->pss_boost_factor                  = 1.0;
  app_args->pss_threshold                     = 3.0;
  app_args->enable_second_stage_pss_detection = true;
  app_args->pss_first_stage_threshold         = 2.0; // Threshold of the first stage in the two-stage PSS detection mechanism.
  app_args->pss_second_stage_threshold        = 3.5; // Threshold of the second stage in the two-stage PSS detection mechanism.
}

void parse_args(int argc, char **argv, app_args_t *app_args) {
  int opt;
  set_default_arguments(app_args);
  while((opt = getopt(argc, argv, "bcmstpvw")) != -1) {
    switch(opt) {
    case 'b':
      cell.nof_prb = atoi(argv[optind]);
      app_args->bw_idx = helpers_get_bw_vector_index_from_prb(cell.nof_prb);
      APP_PRINT("[Input arg.] # PRB: %d - BW index: %d\n", cell.nof_prb, app_args->bw_idx);
      break;
    case 'm':
      app_args->mcs = atoi(argv[optind]);
      APP_PRINT("[Input arg.] MCS: %d\n", app_args->mcs);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      APP_PRINT("[Input arg.] Cell ID: %d\n", cell.id);
      break;
    case 'v':
      srslte_verbose++;
      break;
    case 's':
      app_args->subframe_number = atoi(argv[optind]);
      APP_PRINT("[Input arg.] Subframe number: %d\n", app_args->subframe_number);
      break;
    case 't':
      app_args->max_nof_tbi = atoi(argv[optind]);
      APP_PRINT("[Input arg.] Number of turbo decoder iterations: %d\n", app_args->max_nof_tbi);
      break;
    case 'p':
      app_args->scatter_pss_length = atoi(argv[optind]);
      APP_PRINT("[Input arg.] PSS sequence length: %d\n", app_args->scatter_pss_length);
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
}

int main(int argc, char **argv) {
  app_args_t app_args;
  srslte_psch_t psch;
  uint8_t *sch_payload_tx, *sch_payload_rx;
  cf_t *ce_pss[SRSLTE_MAX_PORTS], *resource_grid[SRSLTE_MAX_PORTS], *subframe[SRSLTE_MAX_PORTS];
  uint32_t fft_size, frame_len, resource_grid_len, sch_value = 0;
  srslte_sss_synch_t sss;
  srslte_ofdm_t ifft;
  float sch_signal[SRSLTE_SCH_LEN];
  double tx_rssi;
  sch_sync_t sch_sync;
  srslte_ue_dl_t ue_dl;
  srslte_pdsch_t pdsch;
  srslte_pdsch_cfg_t pdsch_cfg;
  srslte_softbuffer_tx_t softbuffer;
  srslte_ra_dl_dci_t ra_dl;
  srslte_ra_dl_grant_t grant;
  srslte_chest_dl_t est;
  srslte_sync_t sfind;
  uint32_t peak_position;
  srslte_sync_find_ret_t sync_ret;
  uint8_t decoded_data[10000];
  uint8_t *tx_data = NULL;
  uint32_t numOfBytes = 0, pdsch_num_rxd_bits = 0, mcs = 0, nof_slots = 0, value0 = 0, value1 = 0, combining_value0 = 0, combining_value1 = 0, combining_sch_value = 0;
  int pdsch_comparison = 0, single_sch_comparison = 0, dual_sch_comparison = 0;
  bool decode_front;

  // ***************************** Initialization ******************************
  double SNR_VECTOR[NOF_SNR_VALUES] = {27.21,23.23,20.25,18.45,17.21,13.26,10.23,8.47,7.22,3.26,0.24,-1.54,-2.15,-2.49,-2.79,-3.52,-4.14,-4.69,-5.23,-6.78,-8.52,-9.76,-10.73,-11.54,-12.16,-12.74};
  double noise_power[NOF_SNR_VALUES];
  float noise_variance_vector[NOF_SNR_VALUES];
  bzero(noise_variance_vector, sizeof(float)*NOF_SNR_VALUES);
  uint32_t pdsch_prr[NOF_SNR_VALUES];
  bzero(pdsch_prr, sizeof(uint32_t)*NOF_SNR_VALUES);
  uint32_t pss_prr[NOF_SNR_VALUES];
  bzero(pss_prr, sizeof(uint32_t)*NOF_SNR_VALUES);
  uint32_t single_sch_prr[NOF_SNR_VALUES];
  bzero(single_sch_prr, sizeof(uint32_t)*NOF_SNR_VALUES);
  uint32_t dual_sch_prr[NOF_SNR_VALUES];
  bzero(dual_sch_prr, sizeof(uint32_t)*NOF_SNR_VALUES);
  double pss_peak_vector[NOF_SNR_VALUES][NOF_ITERATIONS];
  bzero(pss_peak_vector, sizeof(double)*NOF_SNR_VALUES*NOF_ITERATIONS);

  // Make all values random.
  srand(12041988);

  // Get command-line arguments.
  parse_args(argc, argv, &app_args);

  // Get FFT size for this PHY BW.
  fft_size = srslte_symbol_sz(cell.nof_prb);

  // Get subframe size.
  frame_len = SRSLTE_SF_LEN(fft_size);

  // Get slot length.
  resource_grid_len = 2*SRSLTE_SLOT_LEN_RE(cell.nof_prb, SRSLTE_CP_NORM);

  // Create data for the defined number of PRBs.
  numOfBytes = srslte_ra_get_tb_size_scatter(app_args.bw_idx, app_args.mcs);

  APP_PRINT("Frame length: %d - FFT size: %d - Resource grid length: %d - MCS: %d - numOfBytes: %d - numOfBits: %d\n", frame_len, fft_size, resource_grid_len, app_args.mcs, numOfBytes, numOfBytes*8);

  // Set SCH synchronization parameters.
  sch_sync.sch_value        = 2000;
  sch_sync.sch_table_index  = 0;
  sch_sync.cp               = SRSLTE_CP_NORM;
  sch_sync.fft_size         = fft_size;
  sch_sync.sss_alg          = SSS_FULL;
  sch_sync.nof_prb          = cell.nof_prb;
  sch_sync.symb_number      = cell.nof_prb==6 ? 1:0;

  // Init memory.
  for(int i = 0; i < cell.nof_ports; i++) {
    // Allocate memory for resource grid, i.e., in frequency domain.
    resource_grid[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * resource_grid_len);
    if(!resource_grid[i]) {
      perror("malloc subframe");
      exit(-1);
    }
    bzero(resource_grid[i], sizeof(cf_t) * resource_grid_len);
    // Allocate for OFDM symbols.
    subframe[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * frame_len);
    if(!subframe[i]) {
      perror("malloc subframe");
      exit(-1);
    }
    bzero(subframe[i], sizeof(cf_t) * frame_len);
    // Allocate memory for channel estimation.
    ce_pss[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * PSCH_RE_LEN);
    if(!ce_pss[i]) {
      perror("malloc ce_pss");
      exit(-1);
    }
    bzero(ce_pss[i], sizeof(cf_t) * PSCH_RE_LEN);
  }

  sch_payload_tx = (uint8_t*)srslte_vec_malloc(sizeof(uint8_t)*app_args.psch_payload_len);
  if(!sch_payload_tx) {
    perror("malloc sch_payload_tx");
    exit(-1);
  }
  sch_payload_rx = (uint8_t*)srslte_vec_malloc(sizeof(uint8_t)*app_args.psch_payload_len);
  if(!sch_payload_rx) {
    perror("malloc sch_payload_rx");
    exit(-1);
  }

  // Allocate memory for data slots.
  tx_data = (uint8_t*)srslte_vec_malloc(sizeof(uint8_t)*numOfBytes);
  if(!tx_data) {
    perror("malloc tx_data");
    exit(-1);
  }

  if(srslte_sss_synch_init(&sss, fft_size)) {
    APP_ERROR("Error initializing SSS object.\n", 0);
    exit(-1);
  }

  // Initialization of PSCH.
  if(srslte_psch_init(&psch, app_args.psch_modulation, app_args.psch_crc_type, app_args.psch_payload_len, cell)) {
    APP_ERROR("Error creating PSCH object.\n", 0);
    exit(-1);
  }

  // Initialize OFDM Tx side.
  if(srslte_ofdm_tx_init(&ifft, cell.cp, cell.nof_prb)) {
    APP_ERROR("Error creating iFFT object.\n", 0);
    exit(-1);
  }
  // Set normalization to true in IFFT object.
  srslte_ofdm_set_normalize(&ifft, true);

  // Initialize PDSCH object.
  if(srslte_pdsch_init_generic(&pdsch, cell, 0, app_args.sch_temporal_diversity)) {
    APP_ERROR("Error creating PDSCH object.\n", 0);
    exit(-1);
  }
  // Set RNTI for PDSCH object.
  srslte_pdsch_set_rnti(&pdsch, RNTI);

  // Initialize softbuffer object.
  if(srslte_softbuffer_tx_init_scatter(&softbuffer, cell.nof_prb)) {
    APP_ERROR("Error initiating soft buffer.\n", 0);
    exit(-1);
  }
  // Reset softbuffer.
  srslte_softbuffer_tx_reset(&softbuffer);

  // Generate CRS signals.
  if(srslte_chest_dl_init(&est, cell)) {
    APP_ERROR("Error initializing equalizer.\n", 0);
    exit(-1);
  }

  // Initialize ue_dl object.
  if(srslte_ue_dl_init_generic(&ue_dl, cell, 0, app_args.sch_temporal_diversity)) {
    APP_ERROR("Error initiating UE downlink processing module\n", 0);
    exit(-1);
  }
  // Configure downlink receiver for the SI-RNTI since will be the only one we'll use.
  // This is the User RNTI.
  srslte_ue_dl_set_rnti(&ue_dl, RNTI);
  // Set the expected CFI.
  srslte_ue_dl_set_expected_cfi(&ue_dl, DEFAULT_CFI);
  // Set the maxium number of turbo decoder iterations.
  srslte_ue_dl_set_max_noi(&ue_dl, app_args.max_nof_tbi);
  // Enable estimation of CFO based on CSR signals.
  srslte_ue_dl_set_cfo_csr(&ue_dl, false);
  // Do not try to decode control channels (PDCCH/PCFICH).
  srslte_ue_dl_set_decode_pdcch(&ue_dl, app_args.decode_pdcch);

  // Generate PSS signal.
  cf_t *pss_signal = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * app_args.scatter_pss_length);
  if(!pss_signal) {
    perror("malloc pss_signal");
    exit(-1);
  }
  bzero(pss_signal, sizeof(cf_t) * app_args.scatter_pss_length);

  if(srslte_pss_generate_scatter(pss_signal, cell.id%3, app_args.scatter_pss_length) < 0) {
    APP_ERROR("Error initiating PSS sequence.\n", 0);
    exit(-1);
  }

  // Boost Scatter PSS sequence.
  if(app_args.pss_boost_factor > 1.0) {
    srslte_vec_sc_prod_cfc(pss_signal, app_args.pss_boost_factor, pss_signal, app_args.scatter_pss_length);
  }

  // Initialize PSS detector object.
  uint32_t node_id = 0;
  uint32_t intf_id = 0;
  bool phy_filtering = false;
  if(srslte_sync_init_generic(&sfind, frame_len, frame_len, srslte_symbol_sz(cell.nof_prb), 0, app_args.initial_subframe_index, app_args.decode_pdcch, node_id, intf_id, phy_filtering, app_args.use_scatter_sync_seq, app_args.scatter_pss_length, app_args.enable_second_stage_pss_detection)) {
    APP_ERROR("Error initiating sync find.\n", 0);
    exit(-1);
  }
  // Disbale SSS detection/decoding as we do not sent it anymore as it was before.
  srslte_sync_sss_en(&sfind, false);
  // Disable integer CFO estimation.
  srslte_sync_cfo_i_detec_en(&sfind, false);
  // Set cell ID to 0.
  srslte_sync_set_N_id_2(&sfind, cell.id%3);
  sfind.cp = cell.cp;
  srslte_sync_cp_en(&sfind, false);
  srslte_sync_set_cfo_ema_alpha(&sfind, 1);
  srslte_sync_set_em_alpha(&sfind, 1);
  srslte_sync_set_threshold(&sfind, app_args.pss_threshold);
  srslte_sync_reset(&sfind);
  srslte_sync_set_avg_psr_scatter(&sfind, app_args.avg_psr_scatter);
  // If two-stages PSS detection is enabled, then set the two thresholds.
  if(app_args.enable_second_stage_pss_detection) {
    srslte_sync_set_pss_1st_stage_threshold_scatter(&sfind, app_args.pss_first_stage_threshold);
    srslte_sync_set_pss_2nd_stage_threshold_scatter(&sfind, app_args.pss_second_stage_threshold);
  }

  // *************************** Start of main loop ********************************
  for(uint32_t n_idx = 0; n_idx < NOF_SNR_VALUES; n_idx++) {

    for(uint32_t iter = 0; iter < NOF_ITERATIONS; iter++) {

      // ******************************* Tx side ***********************************
      // Transmit PSS signal.
      if(app_args.transmit_pss && app_args.subframe_number == 5) {
        // Add PSS signal to resource grid.
        srslte_pss_put_slot_scatter(pss_signal, resource_grid[0], cell.nof_prb, cell.cp, app_args.scatter_pss_length);
      }

      // Add Scatter Header Signal. Transmitted as an M-Sequence.
      if(app_args.transmit_sch && app_args.subframe_number == 5) {

        // Generate SCH signal with MCS and number of slots to be transmitted.
        mcs = rand() % 32;
        nof_slots = rand() % 26;
        value0 = mcs;
        value1 = nof_slots;
        decode_front = false;
        sch_value = srslte_sch_generate_from_pair(sch_signal, decode_front, mcs, nof_slots);
        // Print value being transmitted.
        APP_DEBUG("[Tx SCH] MCS: %d - # slots: %d - Tx SCH Payload: %d\n", mcs, nof_slots, sch_value);

        if(sch_value > SRSLTE_SCH_N*SRSLTE_SCH_N) {
          APP_ERROR("[SCH] Error!!! value is greater than the maximum possible one: %d.\n", (SRSLTE_SCH_N*SRSLTE_SCH_N));
          exit(-1);
        }

        // Add SCH sequence to the original SSS position in the resource grid, i.e., to the 6th OFDM symbol.
        srslte_sch_put_slot_generic(sch_signal, &resource_grid[0][0], cell.nof_prb, cell.cp, 2);

        // Add SCH sequence to the front of the resource grid, to the 2nd OFDM symbol.
        if(app_args.sch_temporal_diversity) {
          srslte_sch_put_slot_generic(sch_signal, &resource_grid[0][0], cell.nof_prb, cell.cp, 6);
        }

#if(0)
        char output_file_name[200];
        sprintf(output_file_name,"sch_resource_grid.dat");
        srslte_filesink_t file_sink;
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, resource_grid[0], resource_grid_len);
        // Close file.
        filesink_free(&file_sink);
#endif

      }

      // Transmit data only when enabled.
      if(app_args.transmit_pdsch) {
        // Add reference signal (RS).
        srslte_refsignal_cs_put_sf(cell, 0, est.csr_signal.pilots[0][app_args.subframe_number], resource_grid[0]);

        // Update allocation with number of resource blocks and MCS.
        update_radl(&ra_dl, app_args.mcs, cell.nof_prb);

        // Configure PDSCH to transmit the requested allocation.
        srslte_ra_dl_dci_to_grant_scatter(&ra_dl, cell.nof_prb, RNTI, &grant);
        if(srslte_pdsch_cfg_scatter(&pdsch_cfg, app_args.sch_temporal_diversity, app_args.scatter_pss_length, cell, &grant, DEFAULT_CFI, app_args.subframe_number, 0)) {
          APP_ERROR("Error configuring PDSCH.\n", 0);
          exit(-1);
        }

        // Generate some data.
        APP_DEBUG("[PDSCH] Creating %d data bytes\n", numOfBytes);
        // Clean data vector.
        bzero(tx_data, sizeof(uint8_t)*numOfBytes);
        // Generate random data.
        generateData(numOfBytes, tx_data);

        // Encode PDSCH.
        if(srslte_pdsch_encode_scatter(&pdsch, &pdsch_cfg, &softbuffer, tx_data, &resource_grid[0])) {
          APP_ERROR("Error encoding PDSCH.\n", 0);
          exit(-1);
        }

#if(0)
        char output_file_name[200];
        sprintf(output_file_name,"pdsch_resource_grid.dat");
        srslte_filesink_t file_sink;
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, resource_grid[0], resource_grid_len);
        // Close file.
        filesink_free(&file_sink);
#endif

      }

      // Create OFDM symbols.
      srslte_ofdm_tx_sf(&ifft, resource_grid[0], subframe[0]);

      // Normalize transmitted signal.
      float rf_amp = 0.8;
      float norm_factor = (float) cell.nof_prb/15/sqrtf(cell.nof_prb);
      srslte_vec_sc_prod_cfc(subframe[0], (rf_amp*norm_factor), subframe[0], SRSLTE_SF_LEN_PRB(cell.nof_prb));
      APP_DEBUG("[OFDM] Resource grid length: %d - Subframe length: %d - RF amp factor: %1.2f - norm. factor: %1.2f - FFT length: %d\n", resource_grid_len, SRSLTE_SF_LEN_PRB(cell.nof_prb), rf_amp, norm_factor, fft_size);

      // Calculate noise power for a given SNR value.
      if(iter == 0) {
        // Calcualte Tx Signal RSSI in dBW.
        tx_rssi = 10*log10(srslte_vec_avg_power_cf(subframe[0], SRSLTE_SF_LEN(srslte_symbol_sz(cell.nof_prb))));
        // Calculate noise power in dBW.
        noise_power[n_idx] = tx_rssi - SNR_VECTOR[n_idx];
        // Calculate noise variance.
        noise_variance_vector[n_idx] = pow(10.0, noise_power[n_idx]/10.0);
        // Print calculated values.
        APP_PRINT("SNR: %1.2f [dB] - Signal power: %1.2f [dBW] - Noise power: %1.2f [dBW] - Noise variance: %1.2e\n", (tx_rssi-noise_power[n_idx]), tx_rssi, noise_power[n_idx], noise_variance_vector[n_idx]);
      }

#if(0)
      for(int k = 0; k < 14; k++) {
        printf("################################ Symbol: %d ################################\n",k);
        for(int j = 0; j < 300; j++) {
          printf("%d-%d: %f,%f\n",k,j,creal(resource_grid[0][k*300 + j]),cimag(resource_grid[0][k*300 + j]));
        }
        printf("---------------------------------------------------\n\n");
      }
#endif

#if(0)
      char output_file_name[200];
      sprintf(output_file_name,"received_resource_grid.dat");
      srslte_filesink_t file_sink;
      filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink, resource_grid[0], resource_grid_len);
      // Close file.
      filesink_free(&file_sink);
#endif

#if(0)
      char output_file_name[200];
      sprintf(output_file_name,"received_psch_subframe.dat");
      srslte_filesink_t file_sink;
      filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink, subframe[0], frame_len);
      // Close file.
      filesink_free(&file_sink);
#endif

      // ***************************** AWGN Channel ********************************
      if(app_args.add_noise) {
        if(app_args.add_only_noise) {
          bzero(subframe[0], sizeof(cf_t) * frame_len);
          srslte_ch_awgn_c(subframe[0], subframe[0], sqrt(noise_variance_vector[n_idx]), frame_len);
        } else {
          srslte_ch_awgn_c(subframe[0], subframe[0], sqrt(noise_variance_vector[n_idx]), frame_len);
        }
      }

#if(0)
      char output_file_name[200];
      sprintf(output_file_name,"noisy_received_psch_subframe.dat");
      srslte_filesink_t file_sink;
      filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink, subframe[0], frame_len);
      // Close file.
      filesink_free(&file_sink);
#endif

      // ******************************* Rx side ***********************************
#if(0)
      for(int k = 0; k < fft_size+2; k++) {
        printf("%d - %f,%f\n", k, creal(subframe[0][((frame_len/2) - fft_size) + k]), cimag(subframe[0][((frame_len/2) - fft_size) + k]));
      }
#endif

      // Detect PSS sequence.
      if(app_args.transmit_pss && app_args.subframe_number == 5) {
        // Try to find PSS sequence and synchronize subframe.
        sync_ret = srslte_sync_find(&sfind, subframe[0], -frame_len, &peak_position);
        // Check if peak was detected.
        if(sfind.peak_value >= app_args.pss_threshold) {
          pss_prr[n_idx]++;
        }
        if(sfind.peak_value >= app_args.pss_threshold && sync_ret != SRSLTE_SYNC_FOUND) {
          printf("------------------> Peak >= threshold but SYNC ret is different from SRSLTE_SYNC_FOUND.\n");
        }
        // Keep PSS peak.
        pss_peak_vector[n_idx][iter] = sfind.peak_value;

#if(0)
        if(sync_ret != SRSLTE_SYNC_FOUND) {
          printf("[PSS] SNR: %d - iter: %d - peak_value: %1.2f - peak_position: %d - sync_ret: %d\n", n_idx, iter, sfind.peak_value, peak_position, sync_ret);
        }
#endif

        // Reset PSS object after every detection.
        srslte_sync_reset(&sfind);
      }

      // Decode SCH sequence.
      if(app_args.transmit_sch && app_args.subframe_number == 5) {

        if(decode_sch_combining(&sss, sch_sync.sss_alg, &subframe[0][0], false, app_args.sch_temporal_diversity, frame_len/2, cell.nof_prb, fft_size, cell.cp, &combining_value0, &combining_value1, &combining_sch_value) < 0) {
          APP_ERROR("Error detecting dual SCH.\n", 0);
        }
        APP_DEBUG("[Rx SCH combining]: MCS: %d - # slots: %d - Rx SCH Payload: %d\n", combining_value0, combining_value1, combining_sch_value);

        // Check decoded value against the transmitted one.
        dual_sch_comparison = -1;
        if(sch_value == combining_sch_value && value0 == combining_value0 && value1 == combining_value1) {
          dual_sch_comparison = 0;
        }
        if(dual_sch_comparison == 0) {
          dual_sch_prr[n_idx]++;
        }

        // Decode SCH sequence signal.
        if(decode_sch(&sss, &sch_sync, &subframe[0][0], frame_len/2, decode_front) < 0) {
          APP_ERROR("Error detecting single SCH.\n", 0);
        }
        APP_DEBUG("[Rx SCH single]: MCS: %d - # slots: %d - Rx SCH Payload: %d\n", sch_sync.value0, sch_sync.value1, sch_sync.sch_value);

        // Check decoded value against the transmitted one.
        single_sch_comparison = -1;
        if(sch_value == sch_sync.sch_value && value0 == sch_sync.value0 && value1 == sch_sync.value1) {
          single_sch_comparison = 0;
        }
        if(single_sch_comparison == 0) {
          single_sch_prr[n_idx]++;
        }

#if(0)
        if(single_sch_comparison != 0) {
          printf("[SCH] Expected: %d - MCS: %d - # slots: %d - Received: %d - MCS: %d - # slots: %d\n", sch_value, value0, value1, sch_sync.sch_value, sch_sync.value0, sch_sync.value1);
        }
#endif

        // Print received value.
        APP_DEBUG("[SCH] Rx Payload: %d\n", sch_sync.sch_value);
      }

      // Receive data only when it was transmitted.
      if(app_args.transmit_pdsch) {
        // Function srslte_ue_dl_decode() returns the number of bits received.
        pdsch_num_rxd_bits = srslte_ue_dl_decode_scatter(&ue_dl, subframe[0], decoded_data, app_args.subframe_number, app_args.mcs);

        APP_DEBUG("[PDSCH] pdsch_num_rxd_bits: %d\n", pdsch_num_rxd_bits);

        if(app_args.print_pdsch_stats) {
          double rssi = 10*log10(srslte_vec_avg_power_cf(subframe[0], SRSLTE_SF_LEN(srslte_symbol_sz(cell.nof_prb))));
          double rsrq = 10*log10(srslte_chest_dl_get_rsrq(&ue_dl.chest));
          double rsrp = 10*log10(srslte_chest_dl_get_rsrp(&ue_dl.chest));
          double noise = srslte_chest_dl_get_noise_estimate(&ue_dl.chest);
          // Calculate SNR out of RSRP and noise estimation.
          double sinr = 10*log10(rsrp/noise);
          // Retrieve last number of turbo decoding attempts.
          uint32_t last_tb_noi = srslte_ue_dl_last_noi(&ue_dl);
          // Print PDSCH statistics.
          APP_PRINT("[PDCSH] MCS: %d - # bytes: %d - CQI: %d - Last TBi: %d - Tx RSSI: %1.2f [dBW] - Rx RSSI: %1.2f [dBW] - RSRQ: %1.2f - RSRP: %1.2f - Noise power: %1.2f [dBW] - Noise: %1.2e - SINR: %1.2f [dB] - SNR: %1.2f [dB].\n", ue_dl.pdsch_cfg.grant.mcs.idx, pdsch_num_rxd_bits/8, srslte_cqi_from_snr(sinr), last_tb_noi, tx_rssi, rssi, rsrq, rsrp, noise_power[n_idx], noise, sinr, (tx_rssi-noise_power[n_idx]));
        }

        pdsch_comparison = -1;
        if(pdsch_num_rxd_bits > 0) {
          pdsch_comparison = memcmp(decoded_data, tx_data, sizeof(uint8_t) * pdsch_num_rxd_bits/8);
          if(pdsch_comparison == 0) {
            pdsch_prr[n_idx]++;
          }
        }
      }

      // Clean buffers.
      for(int i = 0; i < cell.nof_ports; i++) {
        bzero(resource_grid[i], sizeof(cf_t) * resource_grid_len);
        bzero(subframe[i], sizeof(cf_t) * frame_len);
      }

    }

  }

  // **************************** Print Results *****************************
  for(uint32_t n_idx = 0; n_idx < NOF_SNR_VALUES; n_idx++) {
    printf("------- PRB: %d - MCS: %d - SNR: %1.2f [dB] - Noise power: %1.2f [dBW] -------\n", cell.nof_prb, app_args.mcs, SNR_VECTOR[n_idx], noise_power[n_idx]);
    printf("\t\t[PDSCH] PRR: %1.4f - cnt: %d\n", (((float)pdsch_prr[n_idx])/(float)NOF_ITERATIONS), pdsch_prr[n_idx]);
    printf("\t\t[Single SCH] PRR: %1.4f - cnt: %d\n", (((float)single_sch_prr[n_idx])/(float)NOF_ITERATIONS), single_sch_prr[n_idx]);
    printf("\t\t[Dual SCH] PRR: %1.4f - cnt: %d\n", (((float)dual_sch_prr[n_idx])/(float)NOF_ITERATIONS), dual_sch_prr[n_idx]);
    printf("\t\t[PSS] PRR: %1.4f - cnt: %d\n", (((float)pss_prr[n_idx])/(float)NOF_ITERATIONS), pss_prr[n_idx]);
    printf("------------------------------------------------------------------------------\n\n");
  }

  FILE *fid;
  if(app_args.add_only_noise) {
    fid = fopen("peak_values_noise_only.dat", "w");
  } else {
    fid = fopen("peak_values_signal_present.dat", "w");
  }

  if(fid == NULL) {
    fprintf(stderr, "Error opening file!\n");
    exit(-1);
  }

  printf("PRB\tMCS\tSNR\tPDSCH PRR\tSingle SCH PRR\tDual SCH PRR\tPSS PRR\tPeak mean\tPeak var\t\n");
  for(uint32_t n_idx = 0; n_idx < NOF_SNR_VALUES; n_idx++) {
    double peak_mean = 0.0;
    for(uint32_t index = 0; index < NOF_ITERATIONS; index++) {
      peak_mean += pss_peak_vector[n_idx][index];
      // Write peak into file.
      fprintf(fid, "%f\n", pss_peak_vector[n_idx][index]);
    }
    peak_mean /= NOF_ITERATIONS;

    double peak_var = 0.0;
    for(uint32_t index = 0; index < NOF_ITERATIONS; index++) {
      peak_var += pow(pss_peak_vector[n_idx][index] - peak_mean, 2);
    }
    peak_var /= NOF_ITERATIONS;

    printf("%d\t%d\t%1.2f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t\n", cell.nof_prb, app_args.mcs, SNR_VECTOR[n_idx], (((float)pdsch_prr[n_idx])/(float)NOF_ITERATIONS), (((float)single_sch_prr[n_idx])/(float)NOF_ITERATIONS), (((float)dual_sch_prr[n_idx])/(float)NOF_ITERATIONS), (((float)pss_prr[n_idx])/(float)NOF_ITERATIONS), peak_mean, peak_var);
  }

  // **************************** Uninitialization *****************************
  fclose(fid);

  srslte_psch_free(&psch);

  srslte_sss_synch_free(&sss);

  srslte_ofdm_tx_free(&ifft);

  srslte_ue_dl_free(&ue_dl);

  srslte_softbuffer_tx_free_scatter(&softbuffer);

  srslte_pdsch_free(&pdsch);

  srslte_sync_free(&sfind);

  for(int i = 0; i < cell.nof_ports; i++) {
    if(ce_pss[i]) {
      free(ce_pss[i]);
    }
    if(subframe[i]) {
      free(subframe[i]);
    }
    if(resource_grid[i]) {
      free(resource_grid[i]);
    }
  }
  free(tx_data);
  free(sch_payload_tx);
  free(sch_payload_rx);
  free(pss_signal);
}

// Returns 1 if the SCH signal is found, 0 if not and -1 if there is not enough space to correlate
int decode_sch(srslte_sss_synch_t *sss, sch_sync_t *q, cf_t *input, uint32_t peak_pos, bool decode_front) {
  int sch_idx, ret, offset;

  srslte_sch_set_table_index(sss, 0);

  // Get position of SCH sequence at the 6th OFDM symbol.
  offset = 2;
  // Calculate OFDM symbol containing SCH sequence.
  sch_idx = (int) peak_pos - offset*q->fft_size - (offset-1)*SRSLTE_CP_LEN(q->fft_size, (SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  // Make sure we have enough room to find SCH sequence.
  if(sch_idx < 0) {
    APP_ERROR("Not enough samples to decode SCH (sch_idx=%d, peak_pos=%d)\n", sch_idx, peak_pos);
    return SRSLTE_ERROR;
  }
  APP_DEBUG("[SCH] Searching SCH around sch_idx: %d, peak_pos: %d, symbol number: %d\n", sch_idx, peak_pos, q->symb_number);

  switch(q->sss_alg) {
    case SSS_DIFF:
      srslte_sss_synch_m0m1_diff(sss, &input[sch_idx], &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_PARTIAL_3:
      srslte_sch_synch_m0m1_partial(sss, &input[sch_idx], 3, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_FULL:
      srslte_sch_synch_m0m1_partial(sss, &input[sch_idx], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
  }

  ret = srslte_sch_synch_value(sss, q->m0, q->m1);

  APP_DEBUG("[SCH] m0: %d - m1: %d - value: %d\n", q->m0, q->m1, ret);

  if(ret >= 0) {
    q->sch_value  = (uint32_t) ret;
    if(decode_front) {
      q->value0 = floor(ret/2); // SRN ID
      q->value1 = ret % 2;      // Radio Interface ID
    } else {
      q->value0 = floor(ret/26); // MCS
      q->value1 = ret % 26;      // # slots
    }
    return 0;
  } else {
    q->sch_value  = 2000;
    q->value0     = 2000;
    q->value1     = 2000;
    return -1;
  }
}

unsigned int reverse(register unsigned int x) {
  x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
  x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
  x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
  x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
  return((x >> 16) | (x << 16));
}

uint32_t prbset_to_bitmask(uint32_t nof_prb) {
  uint32_t mask = 0;
  int nb = (int) ceilf((float) nof_prb / srslte_ra_type0_P(nof_prb));
  for(int i = 0; i < nb; i++) {
    if(i >= 0 && i < nb) {
      mask = mask | (0x1<<i);
    }
  }
  return reverse(mask)>>(32-nb);
}

int update_radl(srslte_ra_dl_dci_t* const ra_dl, uint32_t mcs, uint32_t nof_prb) {
  bzero(ra_dl, sizeof(srslte_ra_dl_dci_t));
  ra_dl->harq_process = 0;
  ra_dl->mcs_idx = mcs;
  ra_dl->ndi = 0;
  ra_dl->rv_idx = 0;
  ra_dl->alloc_type = SRSLTE_RA_ALLOC_TYPE0;
  ra_dl->type0_alloc.rbg_bitmask = prbset_to_bitmask(nof_prb);
  // Everything went well.
  return 0;
}

void generateData(uint32_t numOfBytes, uint8_t *data) {
  // Create some data.
  for(int i = 0; i < numOfBytes; i++) {
    data[i] = (uint8_t)(rand() % 256);
  }
}

// Returns decoded value if the SSS is found, -1 if there was an error.
int decode_sss(srslte_sss_synch_t *sss, cf_t *input, uint32_t peak_pos, srslte_cp_t cp, uint32_t fft_size, uint32_t N_id_2, sss_alg_t sss_alg) {
  int sss_idx, ret;
  uint32_t m0, m1;
  float m0_value;
  float m1_value;

  srslte_sss_synch_set_N_id_2(sss, N_id_2);

  // Make sure we have enough room to find SSS sequence.
  sss_idx = (int) peak_pos - 2*fft_size - SRSLTE_CP_LEN(fft_size, (SRSLTE_CP_ISNORM(cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  if(sss_idx < 0) {
    APP_ERROR("Not enough room to decode SSS (sss_idx=%d, peak_pos=%d)\n", sss_idx, peak_pos);
    return -1;
  }
  APP_DEBUG("[SSS] Searching SSS around sss_idx: %d, peak_pos: %d\n", sss_idx, peak_pos);

  switch(sss_alg) {
    case SSS_DIFF:
      srslte_sss_synch_m0m1_diff(sss, &input[sss_idx], &m0, &m0_value, &m1, &m1_value);
      break;
    case SSS_PARTIAL_3:
      srslte_sss_synch_m0m1_partial(sss, &input[sss_idx], 3, NULL, &m0, &m0_value, &m1, &m1_value);
      break;
    case SSS_FULL:
      srslte_sss_synch_m0m1_partial(sss, &input[sss_idx], 1, NULL, &m0, &m0_value, &m1, &m1_value);
      break;
  }

  ret = srslte_sss_synch_N_id_1(sss, m0, m1);

  APP_DEBUG("[SSS] m0: %d - m1: %d - value: %d\n", m0, m1, (ret + 1));

  if(ret >= 0) {
    return (ret + 1);
  } else {
    return -1;
  }
}

// Returns 1 if the SCH signal is found, 0 if not and -1 if there is not enough space to correlate
int decode_sch_combining(srslte_sss_synch_t *sss, sss_alg_t sss_alg, cf_t *input, bool decode_radio_id, bool sch_temporal_diversity, uint32_t peak_pos, uint32_t nof_prb, uint32_t fft_size, srslte_cp_t cp, uint32_t *value0, uint32_t *value1, uint32_t* sch_value) {
  int sch_idx0, sch_idx1, ret, offset0, offset1;
  uint32_t m0, m1;
  float m0_value, m1_value;

  srslte_sch_set_table_index(sss, 0);

  // Calculate offset for front OFDM symbol containing SCH sequence, i.e., the 2nd OFDM symbol.
  offset0 = 6;
  sch_idx0 = (int) peak_pos - offset0*fft_size - (offset0-1)*SRSLTE_CP_LEN(fft_size, (SRSLTE_CP_ISNORM(cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));

  // Calculate offset for middle OFDM symbol containing SCH sequence, i.e., the 6th OFDM symbol.
  offset1 = 2;
  sch_idx1 = (int) peak_pos - offset1*fft_size - (offset1-1)*SRSLTE_CP_LEN(fft_size, (SRSLTE_CP_ISNORM(cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));

  // Make sure we have enough room to find SCH sequence.
  if(sch_idx0 < 0 || sch_idx1 < 0) {
    APP_ERROR("Not enough samples to decode SCH (sch_idx0: %d - sch_idx1: %d - peak_pos: %d)\n", sch_idx0, sch_idx1, peak_pos);
    return SRSLTE_ERROR;
  }
  APP_DEBUG("[SCH] Searching SCH around sch_idx: %d - peak_pos0: %d - peak_pos1: %d\n", sch_idx0, sch_idx1, peak_pos);

  switch(sss_alg) {
    case SSS_PARTIAL_3:
      srslte_sch_synch_m0m1_partial(sss, &input[sch_idx1], 3, NULL, &m0, &m0_value, &m1, &m1_value);
      break;
    case SSS_FULL:
      if(sch_temporal_diversity) {
        srslte_sch_m0m1_partial_combining(sss, nof_prb, &input[sch_idx0], &input[sch_idx1], 1, NULL, &m0, &m0_value, &m1, &m1_value);
      } else {
        srslte_sch_synch_m0m1_partial(sss, &input[sch_idx1], 1, NULL, &m0, &m0_value, &m1, &m1_value);
      }
      break;
    case SSS_DIFF:
    default:
      APP_ERROR("SCH decoding algorithm not supported.\n", 0);
      return SRSLTE_ERROR;
  }

  ret = srslte_sch_synch_value(sss, m0, m1);

  APP_DEBUG("[SCH] m0: %d - m1: %d - value: %d\n", m0, m1, ret);

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
