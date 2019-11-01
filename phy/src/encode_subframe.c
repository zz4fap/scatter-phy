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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/select.h>

#include "srslte/srslte.h"


#define UE_CRNTI 0x1234

char *output_file_name = NULL;

srslte_cell_t cell =
{
	25,						// nof_prb
	1,						// nof_ports
	0,						// bw idx
	0,						// cell_id
	SRSLTE_CP_NORM,			// cyclic prefix
	SRSLTE_PHICH_R_1,		// PHICH resources
	SRSLTE_PHICH_NORM		// PHICH length
};

uint32_t mcs_idx = 1, last_mcs_idx = 1, nof_packets = 1;
srslte_datatype_t type = SRSLTE_COMPLEX_FLOAT_BIN; // SRSLTE_COMPLEX_SHORT_BIN

srslte_filesink_t fsink;
srslte_ofdm_t ifft;
//srslte_pbch_t pbch;
//srslte_pcfich_t pcfich;
//srslte_pdcch_t pdcch;
srslte_pdsch_t pdsch;
srslte_pdsch_cfg_t pdsch_cfg;
srslte_softbuffer_tx_t softbuffer;
//srslte_regs_t regs;
srslte_ra_dl_dci_t ra_dl;
srslte_ra_dl_grant_t grant;

cf_t *sf_buffer = NULL, *output_buffer = NULL;
int sf_n_re, sf_n_samples;


void usage(char *prog)
{
	printf("Usage: %s [omntpcv]\n", prog);
	printf("\t-o output_file\n");
	printf("\t-m MCS index [Default %d]\n", mcs_idx);
	printf("\t-n number of packet to transmit [Default %d]\n", nof_packets);
	printf("\t-t data type [Default %d]\n", type);
	printf("\t-c cell id [Default %d]\n", cell.id);
	printf("\t-p nof_prb [Default %d]\n", cell.nof_prb);
	printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "omntpcv")) != -1)
	{
		switch (opt)
		{
		case 'o':
			output_file_name = argv[optind];
			break;
		case 'm':
			mcs_idx = atoi(argv[optind]);
			break;
		case 'n':
			nof_packets = atoi(argv[optind]);
			break;
		case 't':
			type = atoi(argv[optind]);
			break;
		case 'p':
			cell.nof_prb = atoi(argv[optind]);
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

	if (!output_file_name)
	{
		usage(argv[0]);
		exit(-1);
	}
}

void base_init()
{
	/* init memory */
	sf_buffer = srslte_vec_malloc(sizeof(cf_t) * sf_n_re);
	if (!sf_buffer)
	{
		perror("malloc");
		exit(-1);
	}
	output_buffer = srslte_vec_malloc(sizeof(cf_t) * sf_n_samples);
	if (!output_buffer)
	{
		perror("malloc");
		exit(-1);
	}
	/* open file or USRP */
	if (output_file_name)
	{
		if (srslte_filesink_init(&fsink, output_file_name, type))
		{
			fprintf(stderr, "Error opening file %s\n", output_file_name);
			exit(-1);
		}
	}
	else
	{
		printf("Select an output file\n");
		exit(-1);
	}

	/* create ifft object */
	if (srslte_ofdm_tx_init(&ifft, SRSLTE_CP_NORM, cell.nof_prb))
	{
		fprintf(stderr, "Error creating iFFT object\n");
		exit(-1);
	}
	srslte_ofdm_set_normalize(&ifft, true);
/*	if (srslte_pbch_init(&pbch, cell))
	{
		fprintf(stderr, "Error creating PBCH object\n");
		exit(-1);
	}

	if (srslte_regs_init(&regs, cell))
	{
		fprintf(stderr, "Error initiating regs\n");
		exit(-1);
	}

	if (srslte_pcfich_init(&pcfich, &regs, cell))
	{
		fprintf(stderr, "Error creating PBCH object\n");
		exit(-1);
	}

	if (srslte_regs_set_cfi(&regs, cfi))
	{
		fprintf(stderr, "Error setting CFI\n");
		exit(-1);
	}

	if (srslte_pdcch_init(&pdcch, &regs, cell))
	{
		fprintf(stderr, "Error creating PDCCH object\n");
		exit(-1);
	}
*/
	if (srslte_pdsch_init(&pdsch, cell))
	{
		fprintf(stderr, "Error creating PDSCH object\n");
		exit(-1);
	}

	srslte_pdsch_set_rnti(&pdsch, UE_CRNTI);

	if (srslte_softbuffer_tx_init(&softbuffer, cell.nof_prb))
	{
		fprintf(stderr, "Error initiating soft buffer\n");
		exit(-1);
	}
}

void base_free()
{
	srslte_softbuffer_tx_free(&softbuffer);
	srslte_pdsch_free(&pdsch);
//	srslte_pdcch_free(&pdcch);
//	srslte_regs_free(&regs);
//	srslte_pbch_free(&pbch);

	srslte_ofdm_tx_free(&ifft);

	if (sf_buffer)
	{
		free(sf_buffer);
	}
	if (output_buffer)
	{
		free(output_buffer);
	}
	if (output_file_name)
	{
		srslte_filesink_free(&fsink);
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

uint32_t prbset_to_bitmask()
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

int update_radl(int sf_idx)
{
	bzero(&ra_dl, sizeof(srslte_ra_dl_dci_t));
	ra_dl.harq_process = 0;
	ra_dl.mcs_idx = mcs_idx;
	ra_dl.ndi = 0;
	ra_dl.rv_idx = 0;
	ra_dl.alloc_type = SRSLTE_RA_ALLOC_TYPE0;
	ra_dl.type0_alloc.rbg_bitmask = prbset_to_bitmask();

	srslte_ra_pdsch_fprint(stdout, &ra_dl, cell.nof_prb);
	srslte_ra_dl_grant_t dummy_grant;
	srslte_ra_nbits_t dummy_nbits;
	srslte_ra_dl_dci_to_grant(&ra_dl, cell.nof_prb, UE_CRNTI, &dummy_grant);
	srslte_ra_dl_grant_to_nbits_new(&dummy_grant, cell, sf_idx, &dummy_nbits);
	srslte_ra_dl_grant_fprint(stdout, &dummy_grant);

	return 0;
}

#define DATA_BUFF_SZ 75376

int main(int argc, char **argv)
{
	int sf_idx=0, N_id_2=0;
	cf_t pss_signal[SRSLTE_PSS_LEN];
	float sss_signal5_nof[SRSLTE_SSS_LEN]; // for subframe 5 nof
	float sss_signal5_mcs[SRSLTE_SSS_LEN]; // for subframe 5 mcs
	int i;
	cf_t *sf_symbols[SRSLTE_MAX_PORTS];
//	srslte_dci_msg_t dci_msg;
//	srslte_dci_location_t locations[SRSLTE_NSUBFRAMES_X_FRAME][30];
	srslte_chest_dl_t est;

	uint8_t data[DATA_BUFF_SZ];

	if (argc < 3)
	{
		usage(argv[0]);
		exit(-1);
	}

	parse_args(argc, argv);

	N_id_2 = cell.id % 3;
	sf_n_re = 2 * SRSLTE_CP_NORM_NSYMB * cell.nof_prb * SRSLTE_NRE;
	sf_n_samples = 2 * SRSLTE_SLOT_LEN(srslte_symbol_sz(cell.nof_prb));

	/* this *must* be called after setting slot_len_* */
	base_init();

	/* Generate PSS/SSS signals */
	sf_idx = 5;
	srslte_pss_generate(pss_signal, N_id_2);
	srslte_sss_generate_nof_packets_signal(sss_signal5_nof, sf_idx, nof_packets);
	srslte_sss_generate_mcs_signal(sss_signal5_mcs, sf_idx, mcs_idx);

	/* Generate CRS signals */
	if (srslte_chest_dl_init(&est, cell))
	{
		fprintf(stderr, "Error initializing equalizer\n");
		exit(-1);
	}

	for (i = 0; i < SRSLTE_MAX_PORTS; i++)   // now there's only 1 port
	{
		sf_symbols[i] = sf_buffer;
	}

	if (update_radl(sf_idx))
	{
		exit(-1);
	}

	/* Initiate valid DCI locations */
//	for (i=0; i<SRSLTE_NSUBFRAMES_X_FRAME; i++)
//	{
//		srslte_pdcch_ue_locations(&pdcch, locations[i], 30, i, cfi, UE_CRNTI);
//	}

	srslte_softbuffer_tx_reset(&softbuffer);

	bzero(sf_buffer, sizeof(cf_t) * sf_n_re);

	srslte_pss_put_slot(pss_signal,  sf_buffer, cell.nof_prb, SRSLTE_CP_NORM);
	srslte_sss_put_slot(sss_signal5_nof, sf_buffer, cell.nof_prb, SRSLTE_CP_NORM);
	srslte_sss_put_slot_mcs(sss_signal5_mcs, sf_buffer, cell.nof_prb, SRSLTE_CP_NORM);

	srslte_refsignal_cs_put_sf(cell, 0, est.csr_signal.pilots[0][sf_idx], sf_buffer);

//	srslte_pcfich_encode(&pcfich, cfi, sf_symbols, sf_idx);

	/* Transmit PDCCH + PDSCH only when there is data to send */
	//-------------to make sure we have some bits for sf 1-------------//
	srslte_ra_dl_dci_to_grant(&ra_dl, cell.nof_prb, UE_CRNTI, &grant);
	if (srslte_pdsch_cfg_new(&pdsch_cfg, cell, &grant, sf_idx, 0))
	{
		fprintf(stderr, "Error configuring PDSCH\n");
		exit(-1);
	}

	INFO("SF: %d, Generating %d random bits\n", sf_idx, pdsch_cfg.grant.mcs.tbs);
	for (i=0; i<pdsch_cfg.grant.mcs.tbs/8; i++)
	{
		data[i] = '*';
	}

	/* Encode PDCCH */
//	INFO("Putting DCI to location: n=%d, L=%d\n", locations[sf_idx][0].ncce, locations[sf_idx][0].L);
//	srslte_dci_msg_pack_pdsch(&ra_dl, SRSLTE_DCI_FORMAT1, &dci_msg, cell.nof_prb, false);
//	if (srslte_pdcch_encode(&pdcch, &dci_msg, locations[sf_idx][0], UE_CRNTI, sf_symbols, sf_idx, cfi))
//	{
//		fprintf(stderr, "Error encoding DCI message\n");
//		exit(-1);
//	}

	/* Encode PDSCH */
	if (srslte_pdsch_encode_new(&pdsch, &pdsch_cfg, &softbuffer, data, sf_symbols))
	{
		fprintf(stderr, "Error encoding PDSCH\n");
		exit(-1);
	}

	/* Transform to OFDM symbols */
	srslte_ofdm_tx_sf(&ifft, sf_buffer, output_buffer);

	/* send to file */
	srslte_filesink_write(&fsink, output_buffer, sf_n_samples);
	usleep(1000);

	base_free();

	printf("Done\n");
	exit(0);
}


