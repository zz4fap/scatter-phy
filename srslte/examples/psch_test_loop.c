#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#define RNTI 0x1234

#define DEFAULT_CFI 1

#define NOF_SNR_VALUES 17

#define NOF_ITERATIONS 10000

typedef struct {
  bool transmit_psch;
  uint32_t mcs;
  bool transmit_pdsch;
  bool add_noise;
  bool transmit_sch;
  uint32_t psch_payload_len;
  uint32_t max_nof_tbi;
  uint32_t bw_idx;
  bool print_pdsch_stats;
  srslte_mod_t psch_modulation;
  uint32_t psch_crc_type;
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
int decode_sch(srslte_sss_synch_t *sss, sch_sync_t *q, cf_t *input, uint32_t peak_pos, bool decode_radio_id);
int update_radl(srslte_ra_dl_dci_t* const ra_dl, uint32_t mcs, uint32_t nof_prb);
unsigned int get_tb_size(uint32_t prb, uint32_t mcs);
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
  app_args->transmit_psch     = false;
  app_args->transmit_sch      = true;
  app_args->transmit_pdsch    = true;
  app_args->add_noise         = true;
  app_args->mcs               = 0;
  app_args->max_nof_tbi       = 5;
  cell.id                     = 0;
  cell.nof_prb                = 25;
  app_args->bw_idx            = 2;
  app_args->print_pdsch_stats = false;
  app_args->psch_payload_len  = 24;               //20; // 20 bits of payload for the case when we use BPSK.
  app_args->psch_modulation   = SRSLTE_MOD_QPSK;  //SRSLTE_MOD_BPSK;
  app_args->psch_crc_type     = SRSLTE_LTE_CRC16; //0; // No CRC to be added.
}

void parse_args(int argc, char **argv, app_args_t *app_args) {
  int opt;
  set_default_arguments(app_args);
  while((opt = getopt(argc, argv, "bcmvw")) != -1) {
    switch(opt) {
    case 'b':
      cell.nof_prb = atoi(argv[optind]);
      app_args->bw_idx = helpers_get_bw_vector_index_from_prb(cell.nof_prb);
      printf("[Input arg.] # PRB: %d - BW index: %d\n", cell.nof_prb, app_args->bw_idx);
      break;
    case 'm':
      app_args->mcs = atoi(argv[optind]);
      printf("[Input arg.] MCS: %d\n", app_args->mcs);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'v':
      srslte_verbose++;
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
  uint32_t fft_size, frame_len, resource_grid_len, sch_value = 0, nof_subframes_to_tx = 0, rx_nof_subframes_to_tx = 0;
  srslte_pss_synch_t pss;
  srslte_sss_synch_t sss;
  srslte_ofdm_t ifft;
  cf_t pss_signal[SRSLTE_PSS_LEN];
  float sch_signal[SRSLTE_SCH_LEN], sss_signal[SRSLTE_SSS_LEN];
  double tx_rssi;
  sch_sync_t sch_sync;
  srslte_ue_dl_t ue_dl;
  srslte_pdsch_t pdsch;
  srslte_pdsch_cfg_t pdsch_cfg;
  srslte_softbuffer_tx_t softbuffer;
  srslte_ra_dl_dci_t ra_dl;
  srslte_ra_dl_grant_t grant;
  srslte_chest_dl_t est;
  uint8_t decoded_data[10000];
  uint8_t *tx_data = NULL;
  uint32_t pdsch_num_rxd_bits = 0, srn_id = 0, intf_id = 0,value0 = 0, value1 = 0;
  int pdsch_comparison = 0, sch_comparison = 0, psch_comparison = 0, sss_comparison = 0;
  bool decode_radio_id;

  // **************************** Initialization *****************************
  float noise_variance = 0.0;
  float noise_variance_vector[NOF_SNR_VALUES] = {0.0001, 0.00025, 0.0005, 0.00075, 0.001, 0.0025, 0.005, 0.0075, 0.01, 0.025, 0.05, 0.075, 0.1, 0.25, 0.5, 0.75, 1};
  double noise_power[NOF_SNR_VALUES];
  double SNR[NOF_SNR_VALUES];
  uint32_t pdsch_prr[NOF_SNR_VALUES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t sss_prr[NOF_SNR_VALUES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t psch_prr[NOF_SNR_VALUES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t sch_prr[NOF_SNR_VALUES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // Make all values random.
  srand(time(NULL));

  // Get command-line arguments.
  parse_args(argc, argv, &app_args);

  // Get FFT size for this PHY BW.
  fft_size = srslte_symbol_sz(cell.nof_prb);

  // Get subframe size.
  frame_len = SRSLTE_SF_LEN(fft_size);

  // Get slot length.
  resource_grid_len = 2*SRSLTE_SLOT_LEN_RE(cell.nof_prb, SRSLTE_CP_NORM);

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

  // Initialize PSS object structure.
  if(srslte_pss_synch_init_fft(&pss, frame_len, fft_size)) {
    fprintf(stderr, "Error initializing PSS object.\n");
    exit(-1);
  }

  if(srslte_sss_synch_init(&sss, fft_size)) {
    printf("Error initializing SSS object.\n");
    exit(-1);
  }

  // Set the correct Nid2.
  if(srslte_pss_synch_set_N_id_2(&pss, cell.id%3)) {
    fprintf(stderr, "Error setting N_id_2 = %d.\n", cell.id%3);
    exit(-1);
  }

  // Initialization of PSCH.
  if(srslte_psch_init(&psch, app_args.psch_modulation, app_args.psch_crc_type, app_args.psch_payload_len, cell)) {
    fprintf(stderr, "Error creating PSCH object.\n");
    exit(-1);
  }

  // Initialize OFDM Tx side.
  if(srslte_ofdm_tx_init(&ifft, cell.cp, cell.nof_prb)) {
    fprintf(stderr, "Error creating iFFT object.\n");
    exit(-1);
  }
  // Set normalization to true in IFFT object.
  srslte_ofdm_set_normalize(&ifft, true);

  // Initialize PDSCH object.
  if(srslte_pdsch_init(&pdsch, cell)) {
    printf("Error creating PDSCH object.\n");
    exit(-1);
  }
  // Set RNTI for PDSCH object.
  srslte_pdsch_set_rnti(&pdsch, RNTI);

  // Initialize softbuffer object.
  if(srslte_softbuffer_tx_init(&softbuffer, cell.nof_prb)) {
    printf("Error initiating soft buffer.\n");
    exit(-1);
  }
  // Reset softbuffer.
  srslte_softbuffer_tx_reset(&softbuffer);

  // Generate CRS signals
  if(srslte_chest_dl_init(&est, cell)) {
    printf("Error initializing equalizer.\n");
    exit(-1);
  }

  // Initialize ue_dl object.
  if(srslte_ue_dl_init(&ue_dl, cell)) {
    fprintf(stderr, "Error initiating UE downlink processing module\n");
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
  // Do not try to decode control channels.
  srslte_ue_dl_set_decode_pdcch(&ue_dl, false);

  for(uint32_t n_idx = 0; n_idx < NOF_SNR_VALUES; n_idx++) {

    // Retrieve noise variance for this iteration.
    noise_variance = noise_variance_vector[n_idx];
    // Calculate noise power in dBW
    noise_power[n_idx] = 10*log10(noise_variance);

    for(uint32_t iter = 0; iter < NOF_ITERATIONS; iter++) {

      // ******************************* Tx side ***********************************
      // Transmit PSCH signal.
      if(app_args.transmit_psch) {
        // Generate random PHY Header data.
        for(int i = 0; i < app_args.psch_payload_len; i++) {
          sch_payload_tx[i] = rand() % 2;
        }

        // Encode the PHY Header and add it to resource grid.
        srslte_psch_encode(&psch, sch_payload_tx, resource_grid);

        // Generate PSS signal.
        srslte_pss_generate(pss_signal, cell.id%3);

        // Add PSS signal to resource grid.
        srslte_pss_put_slot(pss_signal, resource_grid[0], cell.nof_prb, cell.cp);
      } else {
        // Generate SSS signal with number of slots to be transmitted.
        nof_subframes_to_tx = (rand() % 25) + 1;
        // When PSCH is disabled, then we transmit SSS sequence.
        srslte_sss_generate_nof_packets_signal(sss_signal, 5, nof_subframes_to_tx);

        // Insert signal into resource grid.
        srslte_sss_put_slot(sss_signal, &resource_grid[0][0], cell.nof_prb, cell.cp);

        // Print value being transmitted.
        DEBUG("[SSS] Tx Payload: %d\n", nof_subframes_to_tx);
      }

      // Add Scatter Header Signal. Transmitted as an M-Sequence.
      if(app_args.transmit_sch) {

#if(1)
        // Generate SCH signal with SRN ID and Radio Interface ID.
        srn_id = rand() % 256;
        intf_id = rand() % 2;
        value0 = srn_id;
        value1 = intf_id;
        decode_radio_id = true;
        sch_value = srslte_sch_generate_from_pair(sch_signal, decode_radio_id, srn_id, intf_id);
        // Print value being transmitted.
        DEBUG("[SCH] srn_id: %d - intf_id: %d - Tx Payload: %d\n", srn_id, intf_id, sch_value);
#endif

#if(0)
        // Generate SCH signal with MCS and number of slots to be transmitted.
        uint32_t mcs = rand() % 32;
        uint32_t nof_slots = rand() % 26;
        value0 = mcs;
        value1 = nof_slots;
        decode_radio_id = false;
        sch_value = srslte_sch_generate_from_pair(sch_signal, decode_radio_id, mcs, nof_slots);
        // Print value being transmitted.
        DEBUG("[SCH] MCS: %d - # slots: %d - Tx Payload: %d\n", mcs, nof_slots, sch_value);
#endif

        if(sch_value > SRSLTE_SCH_N*SRSLTE_SCH_N) {
          printf("[SCH] Error!!! value is greater than the maximum possible one: 961.\n");
          exit(-1);
        }

        // Add SCH signal to resource grid.
        //srslte_sch_put_slot_generic(sch_signal, &resource_grid[0][resource_grid_len/2], cell.nof_prb, cell.cp, 2);
        srslte_sch_put_slot_front(sch_signal, &resource_grid[0][0], cell.nof_prb, cell.cp);
      }

      // Transmit data only when enabled.
      if(app_args.transmit_pdsch) {
        // Add reference signal (RS).
        srslte_refsignal_cs_put_sf(cell, 0, est.csr_signal.pilots[0][5], resource_grid[0]);

        // Update allocation with number of resource blocks and MCS.
        update_radl(&ra_dl, app_args.mcs, cell.nof_prb);

        // Configure PDSCH to transmit the requested allocation.
        srslte_ra_dl_dci_to_grant(&ra_dl, cell.nof_prb, RNTI, &grant);
        if(srslte_pdsch_cfg(&pdsch_cfg, cell, &grant, DEFAULT_CFI, 5, 0)) {
          printf("Error configuring PDSCH.\n");
          exit(-1);
        }

        // Create data for the defined number of PRBs.
        uint32_t numOfBytes = get_tb_size(app_args.bw_idx, app_args.mcs);
        // Allocate memory for data slots.
        tx_data = (uint8_t*)srslte_vec_malloc(numOfBytes);
        // Generate some data.
        DEBUG("[PDSCH] Creating %d data bytes\n", numOfBytes);
        generateData(numOfBytes, tx_data);

        // Encode PDSCH.
        if(srslte_pdsch_encode(&pdsch, &pdsch_cfg, &softbuffer, tx_data, &resource_grid[0])) {
          printf("Error encoding PDSCH\n");
          exit(-1);
        }
      }

      // Create OFDM symbols.
      srslte_ofdm_tx_sf(&ifft, resource_grid[0], subframe[0]);

      // Normalize transmitted signal.
      float rf_amp = 0.8;
      float norm_factor = (float) cell.nof_prb/15/sqrtf(cell.nof_prb);
      srslte_vec_sc_prod_cfc(subframe[0], (rf_amp*norm_factor), subframe[0], SRSLTE_SF_LEN_PRB(cell.nof_prb));
      DEBUG("[OFDM] Resource grid length: %d - Subframe length: %d - RF amp factor: %1.2f - norm. factor: %1.2f - FFT length: %d\n", resource_grid_len, SRSLTE_SF_LEN_PRB(cell.nof_prb), rf_amp, norm_factor, fft_size);

      // Calcualte Tx RSSI in dBW.
      if(iter == 0) {
        tx_rssi = 10*log10(srslte_vec_avg_power_cf(subframe[0], SRSLTE_SF_LEN(srslte_symbol_sz(cell.nof_prb))));
        SNR[n_idx] = (tx_rssi-noise_power[n_idx]);
        printf("SNR: %1.2f [dB] - Noise power: %1.2f [dBW]\n", (tx_rssi-noise_power[n_idx]), noise_power[n_idx]);
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
      sprintf(output_file_name,"received_psch_resource_grid.dat");
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
        srslte_ch_awgn_c(subframe[0], subframe[0], sqrt(noise_variance), frame_len);
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

      if(app_args.transmit_sch) {
        // Decode SCH sequence signal.
        if(decode_sch(&sss, &sch_sync, &subframe[0][0], frame_len/2, decode_radio_id) < 0) {
          printf("Error detecting SCH.\n");
        }
        // Check decoded value against the transmitted one.
        sch_comparison = -1;
        if(sch_value == sch_sync.sch_value && value0 == sch_sync.value0 && value1 == sch_sync.value1) {
          sch_comparison = 0;
        }
        if(sch_comparison == 0) {
          sch_prr[n_idx]++;
        }

        //else {
        //  printf("[SCH] Expected: %d - MCS: %d - # slots: %d - Received: %d - MCS: %d - # slots: %d\n", sch_value, value0, value1, sch_sync.sch_value, sch_sync.value0, sch_sync.value1);
      //  }

        // Print received value.
        DEBUG("[SCH] Rx Payload: %d\n", sch_sync.sch_value);
      }

      // Decode PSCH signal.
      if(app_args.transmit_psch) {
        // Estimate channel based on PSS signal.
        // We use PSS-based channel estimates here.
        // As PSCH uses only 60 subcarriers and PSS 62, we start from index 1 and use the subsequent 60 estimates,
        // wich match with the PSCH position in frequency domain.
        if(srslte_pss_synch_chest_generic(&pss, &subframe[0][(frame_len/2) - fft_size], ce_pss[0], PSCH_RE_LEN, 1)) {
          fprintf(stderr, "Error computing PSS-based channel estimation.\n");
          exit(-1);
        }

        // Get the accumulated noise power based on null subcarriers (10 in total) above and below the PSS.
        float acc_noise_estimate = srslte_pss_synch_acc_noise(&pss);

  #if(0)
        printf("acc_noise_estimate: %f\n",acc_noise_estimate);
        for(int k = 0; k < PSCH_RE_LEN; k++) {
          printf("%d - %f,%f\n", k, creal(ce_pss[0][k]), cimag(ce_pss[0][k]));
        }
  #endif

        // Combine outputs.
        for(int i = 1; i < cell.nof_ports; i++) {
          for(int j = 0; j < PSCH_RE_LEN; j++) {
            subframe[0][j] += subframe[i][j];
          }
        }

        // Try to decode the PSCH signal.
        if(srslte_psch_decode(&psch, subframe[0], ce_pss, acc_noise_estimate, sch_payload_rx) != 1) {
          fprintf(stderr, "Error decoding PSCH.\n");
          // Print randomly generated PSCH payload.
          printf("[PSCH] Tx payload: ");
          srslte_vec_fprint_hex(stdout, sch_payload_tx, app_args.psch_payload_len);
          printf("[PSCH] Rx payload: ");
          srslte_vec_fprint_hex(stdout, sch_payload_rx, app_args.psch_payload_len);
        }
        psch_comparison = memcmp(sch_payload_rx, sch_payload_tx, sizeof(uint8_t) * app_args.psch_payload_len);
        if(psch_comparison == 0) {
          psch_prr[n_idx]++;
        }
      } else {
        // Decode the number of transmitted subframes encoded in the SSS sequence.
        rx_nof_subframes_to_tx = decode_sss(&sss, &subframe[0][0], frame_len/2, cell.cp, fft_size, 0, SSS_FULL);

        // Check decoded value against the transmitted one.
        sss_comparison = -1;
        if(nof_subframes_to_tx == rx_nof_subframes_to_tx) {
          sss_comparison = 0;
        }
        if(sss_comparison == 0) {
          sss_prr[n_idx]++;
        }

        // Print received value.
        DEBUG("[SSS] Rx Payload: %d\n", rx_nof_subframes_to_tx);
      }

      // Receive data only when it was transmitted.
      if(app_args.transmit_pdsch) {
        // Function srslte_ue_dl_decode() returns the number of bits received.
        pdsch_num_rxd_bits = srslte_ue_dl_decode_scatter(&ue_dl, subframe[0], decoded_data, 5, app_args.mcs);

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
          printf("[PDCSH] MCS: %d - # bytes: %d - CQI: %d - Last TBi: %d - Tx RSSI: %1.2f [dBW] - Rx RSSI: %1.2f [dBW] - RSRQ: %1.2f - RSRP: %1.2f - Noise power: %1.2f [dBW] - Noise: %1.2e - SINR: %1.2f [dB] - SNR: %1.2f [dB].\n", ue_dl.pdsch_cfg.grant.mcs.idx, pdsch_num_rxd_bits/8, srslte_cqi_from_snr(sinr), last_tb_noi, tx_rssi, rssi, rsrq, rsrp, noise_power[n_idx], noise, sinr, (tx_rssi-noise_power[n_idx]));
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
    printf("------- PRB: %d - MCS: %d - SNR: %1.2f [dB] - Noise power: %1.2f [dBW] -------\n", cell.nof_prb, app_args.mcs, SNR[n_idx], noise_power[n_idx]);
    printf("\t\t[PDSCH] PRR: %1.4f - cnt: %d\n", (((float)pdsch_prr[n_idx])/(float)NOF_ITERATIONS), pdsch_prr[n_idx]);
    printf("\t\t[SSS] PRR: %1.4f - cnt: %d\n", (((float)sss_prr[n_idx])/(float)NOF_ITERATIONS), sss_prr[n_idx]);
    printf("\t\t[SCH] PRR: %1.4f - cnt: %d\n", (((float)sch_prr[n_idx])/(float)NOF_ITERATIONS), sch_prr[n_idx]);
    printf("\t\t[PSCH] PRR: %1.4f - cnt: %d\n", (((float)psch_prr[n_idx])/(float)NOF_ITERATIONS), psch_prr[n_idx]);
    printf("------------------------------------------------------------------------------\n\n");
  }

  printf("PRB\tMCS\tSNR\tPDSCH PRR\tSSS PRR\tSCH PRR\tPSCH PRR\t\n");
  for(uint32_t n_idx = 0; n_idx < NOF_SNR_VALUES; n_idx++) {
    printf("%d\t%d\t%1.2f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t\n", cell.nof_prb, app_args.mcs, SNR[n_idx], (((float)pdsch_prr[n_idx])/(float)NOF_ITERATIONS), (((float)sss_prr[n_idx])/(float)NOF_ITERATIONS), (((float)sch_prr[n_idx])/(float)NOF_ITERATIONS), (((float)psch_prr[n_idx])/(float)NOF_ITERATIONS));
  }

  // **************************** Uninitialization *****************************
  srslte_psch_free(&psch);

  srslte_pss_synch_free(&pss);

  srslte_sss_synch_free(&sss);

  srslte_ofdm_tx_free(&ifft);

  srslte_ue_dl_free(&ue_dl);

  srslte_softbuffer_tx_free(&softbuffer);

  srslte_pdsch_free(&pdsch);

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
}

// Returns 1 if the SCH signal is found, 0 if not and -1 if there is not enough space to correlate
int decode_sch(srslte_sss_synch_t *sss, sch_sync_t *q, cf_t *input, uint32_t peak_pos, bool decode_radio_id) {
  int sch_idx, ret, offset;

  srslte_sch_set_table_index(sss, 0);

  offset = (SRSLTE_CP_NSYMB(q->cp) - q->symb_number);
  // Calculate OFDM symbol containing SCH sequence.
  sch_idx = (int) peak_pos - offset*q->fft_size - (offset-1)*SRSLTE_CP_LEN(q->fft_size, (SRSLTE_CP_ISNORM(q->cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  // Make sure we have enough room to find SCH sequence.
  if(sch_idx < 0) {
    printf("Not enough samples to decode SCH (sch_idx=%d, peak_pos=%d)\n", sch_idx, peak_pos);
    return SRSLTE_ERROR;
  }
  DEBUG("[SCH] Searching SCH around sch_idx: %d, peak_pos: %d, symbol number: %d\n", sch_idx, peak_pos, q->symb_number);

  switch(q->sss_alg) {
    case SSS_DIFF:
      srslte_sss_synch_m0m1_diff(sss, &input[sch_idx], &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_PARTIAL_3:
      srslte_sch_synch_m0m1_partial(sss, &input[sch_idx], 3, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
    case SSS_FULL:
      //srslte_sch_synch_m0m1_partial(sss, &input[sch_idx], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      srslte_sch_m0m1_partial_front(sss, q->nof_prb, &input[sch_idx], 1, NULL, &q->m0, &q->m0_value, &q->m1, &q->m1_value);
      break;
  }

  ret = srslte_sch_synch_value(sss, q->m0, q->m1);

  DEBUG("[SCH] m0: %d - m1: %d - value: %d\n", q->m0, q->m1, ret);

  if(ret >= 0) {
    q->sch_value  = (uint32_t) ret;
    if(decode_radio_id) {
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

static const unsigned int tbs_table[6][29] = {{19,26,32,41,51,63,75,89,101,117,117,129,149,169,193,217,225,225,241,269,293,325,349,373,405,437,453,469,549}, // Values for 1.4 MHz BW
{49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383}, // Values for 3 MHz BW
{85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292}, // Values for 5 MHz BW
{173,225,277,357,453,549,645,775,871,999,999,1095,1239,1431,1620,1764,1908,1908,2052,2292,2481,2673,2865,3182,3422,3542,3822,3963,4587}, // Values for 10 MHz BW
{261,341,421,549,669,839,967,1143,1335,1479,1479,1620,1908,2124,2385,2673,2865,2865,3062,3422,3662,4107,4395,4736,5072,5477,5669,5861,6882}, // Values for 15 MHz BW
{349,453,573,717,903,1095,1287,1527,1764,1980,1980,2196,2481,2865,3182,3542,3822,3822,4107,4587,4904,5477,5861,6378,6882,7167,7708,7972,9422}}; // Values for 20 MHz BW

unsigned int get_tb_size(uint32_t prb, uint32_t mcs) {
  return tbs_table[prb][mcs];
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
  sss_idx = (int) peak_pos-2*fft_size-SRSLTE_CP_LEN(fft_size, (SRSLTE_CP_ISNORM(cp)?SRSLTE_CP_NORM_LEN:SRSLTE_CP_EXT_LEN));
  if(sss_idx < 0) {
    DEBUG("Not enough room to decode SSS (sss_idx=%d, peak_pos=%d)\n", sss_idx, peak_pos);
    return -1;
  }
  DEBUG("[SSS] Searching SSS around sss_idx: %d, peak_pos: %d\n", sss_idx, peak_pos);

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

  //uint32_t sf_idx = srslte_sss_synch_subframe(m0, m1);

  ret = srslte_sss_synch_N_id_1(sss, m0, m1);

  DEBUG("[SSS] m0: %d - m1: %d - value: %d\n", m0, m1, (ret + 1));

  if(ret >= 0) {
    return (ret + 1);
  } else {
    return -1;
  }
}
