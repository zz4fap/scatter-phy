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
#define DEFAULT_CFI 1
#define MAX_NOF_TBI 4
#define SUBFRAME_NUMBER 5

srslte_ra_dl_dci_t ra_dl;
char *input_file_name = NULL;
uint32_t nof_prb = 25;

void usage(char *prog)
{
	printf("Usage: %s [ipv]\n", prog);
	printf("\t-i input_file\n");
	printf("\t-p nof_prb [Default %d]\n", nof_prb);
	printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "ipv")) != -1)
	{
		switch (opt)
		{
		case 'i':
			input_file_name = argv[optind];
			break;
		case 'p':
			nof_prb = atoi(argv[optind]);
			break;
		case 'v':
			srslte_verbose++;
			break;
		default:
			usage(argv[0]);
			exit(-1);
		}
	}

	if (!input_file_name)
	{
		usage(argv[0]);
		exit(-1);
	}
}

unsigned int reverse(register unsigned int x)
{
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	return((x >> 16) | (x << 16));

}

uint32_t prbset_to_bitmask(srslte_cell_t cell)
{
	uint32_t mask=0;
	int nb = (int) ceilf((float) cell.nof_prb / srslte_ra_type0_P(cell.nof_prb));
	for (int i=0; i<nb; i++)
	{
		if (i >= 0 && i < nb)
		{
			mask = mask | (0x1<<i);
		}
	}
	return reverse(mask)>>(32-nb);
}

int update_radl(srslte_cell_t cell, int mcs_idx)
{
	bzero(&ra_dl, sizeof(srslte_ra_dl_dci_t));
	ra_dl.harq_process = 0;
	ra_dl.mcs_idx = mcs_idx;
	ra_dl.ndi = 0;
	ra_dl.rv_idx = 0;
	ra_dl.alloc_type = SRSLTE_RA_ALLOC_TYPE0;
	ra_dl.type0_alloc.rbg_bitmask = prbset_to_bitmask(cell);
	return 0;
}

int main(int argc, char **argv) {

  uint32_t pdsch_num_rxd_bits = 0;
  srslte_ue_dl_t ue_dl;
  srslte_cell_t cell_ue;
  uint8_t decoded_data[10000];
  uint32_t subframe_size, peak_position;
  float cfo_est;
  cf_t *subframe_buffer;
  srslte_filesource_t q;
  srslte_cfo_t cfocorr;
  srslte_pss_synch_t pss;
  srslte_sync_t sfind;
  srslte_dci_msg_t dci_msg;
  int ret = 0, sf_idx, subframe_counter;

  parse_args(argc, argv);

  subframe_size = 15*srslte_symbol_sz(nof_prb);
  subframe_buffer = srslte_vec_malloc(sizeof(cf_t) * subframe_size);
  if (!subframe_buffer)
  {
    perror("malloc");
    exit(-1);
  }

  cell_ue.nof_prb         = nof_prb;            // nof_prb
  cell_ue.nof_ports       = 1;                  // nof_ports
  cell_ue.bw_idx          = 0;                  // bw idx
  cell_ue.id              = 0;                  // cell_id
  cell_ue.cp              = SRSLTE_CP_NORM;     // cyclic prefix
  cell_ue.phich_length    = SRSLTE_PHICH_NORM;  // PHICH length
  cell_ue.phich_resources = SRSLTE_PHICH_R_1;   // PHICH resources

  // Initialize ue_dl object.
  if(srslte_ue_dl_init_new(&ue_dl, cell_ue)) {
    fprintf(stderr, "Error initiating UE downlink processing module\n");
    exit(-1);
  }

  if(srslte_cfo_init_finer(&cfocorr, subframe_size)) {
    fprintf(stderr, "Error initiating CFO\n");
    exit(-1);
  }

  if(srslte_pss_synch_init_fft(&pss, subframe_size, srslte_symbol_sz(cell_ue.nof_prb))) {
    fprintf(stderr, "Error initializing PSS object\n");
    exit(-1);
  }

  if(srslte_sync_init(&sfind, subframe_size, subframe_size, srslte_symbol_sz(cell_ue.nof_prb), 0, SUBFRAME_NUMBER)) {
    fprintf(stderr, "Error initiating sync find\n");
    exit(-1);
  }

  // Configure downlink receiver for the SI-RNTI since will be the only one we'll use.
  // This is the User RNTI.
  srslte_ue_dl_set_rnti_new(&ue_dl, RNTI);

  // Set the expected CFI.
  srslte_ue_dl_set_expected_cfi(&ue_dl, DEFAULT_CFI);

  // Set the maxium number of turbo decoder iterations.
  srslte_ue_dl_set_max_noi(&ue_dl, MAX_NOF_TBI);

  // Enable estimation of CFO based on CSR signals.
  srslte_ue_dl_set_cfo_csr(&ue_dl, false);

  // Set the number of FFT bins used.
  srslte_cfo_set_fft_size_finer(&cfocorr, srslte_symbol_sz(cell_ue.nof_prb));

  // Set Nid2.
  srslte_pss_synch_set_N_id_2(&pss, 0);

  srslte_sync_cfo_i_detec_en(&sfind, true);

  srslte_sync_set_N_id_2(&sfind, 0);

  // Create reading object.
  ret = srslte_filesource_init(&q, input_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
  if(ret < 0) {
    printf("Error opening file......\n");
    exit(-1);
  }

  subframe_counter = 0;
  do{
    // Read the subframe 0 from file.
    ret = srslte_filesource_read(&q, subframe_buffer, subframe_size);
    if(ret < 0) {
      printf("Error reading file......\n");
      exit(-1);
    }

    // Retrieve mcs and nof_subframes_to_rx from subframe 0
    if(subframe_counter == 0)
    {
      srslte_sync_find_new(&sfind, subframe_buffer, -subframe_size, &peak_position);
      cfo_est = srslte_sync_cfo_estimate_cp(&sfind, subframe_buffer);
      // Apply CFO to the signal.
      srslte_cfo_correct_finer(&cfocorr, subframe_buffer, subframe_buffer, -cfo_est/384.0);

      sf_idx = sfind.sf_idx;
    }
    else
    {
      sf_idx = sfind.sf_idx + 1;
    }

    update_radl(cell_ue, sfind.mcs_idx);
    srslte_dci_msg_pack_pdsch(&ra_dl, SRSLTE_DCI_FORMAT1, &dci_msg, cell_ue.nof_prb, false);

    // Function srslte_ue_dl_decode() returns the number of bits received.
    pdsch_num_rxd_bits = srslte_ue_dl_decode_new(&ue_dl, subframe_buffer, decoded_data, sf_idx, dci_msg);

    if(pdsch_num_rxd_bits > 0) {
      for(uint32_t k = 0; k < (pdsch_num_rxd_bits/8); k++)
        printf("%c", decoded_data[k]);
    } else {
      printf("Error decoding file !!!!!!\n");
      printf("pdsch_num_rxd_bits: %d !!!!!!!!\n",pdsch_num_rxd_bits);
      exit(-1);
    }

    subframe_counter++;

  }while(subframe_counter < sfind.nof_subframes_to_rx);

  // Free reading object.
  srslte_filesource_free(&q);

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
