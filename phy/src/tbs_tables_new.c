/* 
  This program is used to create new transport block table as used in scatter PHY.
  By harvesting control resource elements from legancy LTE resource grid,
  we will modify the tranport block table.

  Starting from legacy lte TB table, we
  1. calculate subframe RE bits with sync symbols (PSS/SSS) included
  2. calculate code rate (TB size / RE size)
  3. calculate new subframe RE bits with control symbols removed, and sync symbols (PSS/SSS) and MCS (SSS) included
  4. calculate modified TB size
  5. align modified TB size to turbo code block sizes
  6. calculate new TB size
*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>

#include "srslte/fec/turbodecoder_gen.h"
#include "srslte/fec/cbsegm.h"
#include "srslte/common/phy_common.h"

#define ENABLE_CTRL_RES_HARVESTING 0
#include "../lib/phch/tbs_tables.h"

#define CFI 1

const uint32_t tc_cb_sizes[SRSLTE_NOF_TC_CB_SIZES] = { 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120,
    128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232,
    240, 248, 256, 264, 272, 280, 288, 296, 304, 312, 320, 328, 336, 344,
    352, 360, 368, 376, 384, 392, 400, 408, 416, 424, 432, 440, 448, 456,
    464, 472, 480, 488, 496, 504, 512, 528, 544, 560, 576, 592, 608, 624,
    640, 656, 672, 688, 704, 720, 736, 752, 768, 784, 800, 816, 832, 848,
    864, 880, 896, 912, 928, 944, 960, 976, 992, 1008, 1024, 1056, 1088,
    1120, 1152, 1184, 1216, 1248, 1280, 1312, 1344, 1376, 1408, 1440, 1472,
    1504, 1536, 1568, 1600, 1632, 1664, 1696, 1728, 1760, 1792, 1824, 1856,
    1888, 1920, 1952, 1984, 2016, 2048, 2112, 2176, 2240, 2304, 2368, 2432,
    2496, 2560, 2624, 2688, 2752, 2816, 2880, 2944, 3008, 3072, 3136, 3200,
    3264, 3328, 3392, 3456, 3520, 3584, 3648, 3712, 3776, 3840, 3904, 3968,
    4032, 4096, 4160, 4224, 4288, 4352, 4416, 4480, 4544, 4608, 4672, 4736,
    4800, 4864, 4928, 4992, 5056, 5120, 5184, 5248, 5312, 5376, 5440, 5504,
    5568, 5632, 5696, 5760, 5824, 5888, 5952, 6016, 6080, 6144 };

int main()
{
	uint32_t tb, tb_new, nof_prb, nof_pdcch, bits_per_sym, RE_bits, RE_bits_new;
	double code_rate;
	uint32_t Bp, Bp_new, B, B_new, s_C;

	int i, j, cbindex;
	for (i = 0; i < 27; i++)
	{
		printf("{");
		for (j = 0; j < 110; j++)
		{
			// transport block size
			tb = tbs_table[i][j];

			// We only consider physical resource blocks that can accomodate PSS/SSS
			if(j < 5)
			{
				tb_new = tb;
			}
			else
			{
				// Number of physical resource blocks
				nof_prb = j + 1;
				// Number of Physical Downlink Control Channel
				nof_pdcch = nof_prb<10 ? CFI+1 : CFI;

				// modulated Bits per symbol
				// QPSK modulation
				if(i <= 9)
					bits_per_sym = 2;
				// 16 QAM modulation
				else if(i <= 15)
					bits_per_sym = 4;
				// 64 QAM modulation
				else if(i <= 26)
					bits_per_sym = 6;

				// subframe Resource Element bits with sync symbols (PSS/SSS) included
				RE_bits = bits_per_sym*(((2*SRSLTE_CP_NORM_NSYMB - nof_pdcch) * SRSLTE_NRE - 6) * nof_prb - 144);

				code_rate = (double)tb / RE_bits;

				// new subframe Resource Element bits with control symbols removed, and sync symbols (PSS/SSS) and MCS (SSS) included
				RE_bits_new = bits_per_sym*((2*SRSLTE_CP_NORM_NSYMB * SRSLTE_NRE - 8) * nof_prb - 216);

				// Calculate modified TB size
				tb_new = (uint32_t) RE_bits_new * code_rate;

				// Align modified TB size to turbo code block sizes
				// Code block segmentation
				B = tb_new + 24;
				if (B <= SRSLTE_TCOD_MAX_LEN_CB)
				{
					s_C = 1;
					Bp = B;
				}
				else
				{
					s_C = (uint32_t) ceilf((float) B / (SRSLTE_TCOD_MAX_LEN_CB - 24));
					Bp = B + 24 * s_C;
				}

				// code block segmentation upper bound index
				cbindex = srslte_cbsegm_cbindex((Bp-1) / s_C + 1);

				// Calculate new TB table
				Bp_new = tc_cb_sizes[cbindex] * s_C;
				B_new = (B <= SRSLTE_TCOD_MAX_LEN_CB) ? Bp_new : Bp_new - (24*s_C);
				tb_new = B_new - 24;
			}

			if(j < 109)
				printf("%5u,", tb_new);
			else
				printf("%5u", tb_new);
		}
		if(i < 26)
			printf("},\n");
		else
			printf("}\n");
	}

    return 0;
}
