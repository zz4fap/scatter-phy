#ifndef _TRX_FILTER_H_
#define _TRX_FILTER_H_

#include <immintrin.h>

#include "srslte/srslte.h"

#define NUMBER_OF_FILTERS 5 // Filters defined for 1.4, 3, 5, 10 and 20 MHz.

#define MAXIMUM_FILTER_LENGTH 513

#define MAX_SUBFRAME_LEN 5760 // For 5 MHz //TODO Change way coefficients and buffers are allocated as BSS is getting too BIG!!
#define MAX_SIGNAL_BLK_LEN 1000*((4*2*MAX_SUBFRAME_LEN)+3)

#define SIZE_OF_8_FLOATS (8*sizeof(float))

#define ENABLE_TRX_FILTER_PRINTS 1

#define TRX_FILTER_PRINT(_fmt, ...) if(scatter_verbose_level >= 0 && ENABLE_TRX_FILTER_PRINTS) \
  fprintf(stdout, "[TRX FILTER PRINT]: " _fmt, __VA_ARGS__)

#define TRX_FILTER_DEBUG(_fmt, ...) if(scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG && ENABLE_TRX_FILTER_PRINTS) \
  fprintf(stdout, "[TRX FILTER DEBUG]: " _fmt, __VA_ARGS__)

#define TRX_FILTER_INFO(_fmt, ...) if(scatter_verbose_level >= SRSLTE_VERBOSE_INFO && ENABLE_TRX_FILTER_PRINTS) \
  fprintf(stdout, "[TRX FILTER INFO]: " _fmt, __VA_ARGS__)

extern __attribute__ ((aligned (32))) const float *trx_filter_coeffs;

void trx_filter_run_fir_filter_real(float *input, uint32_t input_len, uint32_t bw_idx, uint32_t filter_order, float *output);

void trx_filter_run_fir_filter_complex(cf_t *input, uint32_t input_len, uint32_t bw_idx, uint32_t filter_order, cf_t *output);

void trx_filter_run_fir_filter_sse_real(float *input, uint32_t input_len, float *output);

void trx_filter_create_simd_kernel(uint32_t bw_idx);

void trx_filter_init_coeffs(uint32_t bw_idx);

uint32_t trx_filter_get_filter_length();

void trx_filter_init_filter_length(uint32_t filter_order_idx);

uint32_t trx_filter_get_order(uint32_t filter_idx);

// ********** MM256 **********
void trx_filter_create_tx_simd_kernel_mm256(uint32_t bw_idx);

void trx_filter_create_rx_simd_kernel_mm256(uint32_t bw_idx);

void trx_filter_create_simd_kernel_mm256(uint32_t bw_idx, __m256 *kernel_mm256);

void trx_filter_run_fir_tx_filter_sse_mm256_real(float *input, uint32_t input_len, float *output);

void trx_filter_run_fir_rx_filter_sse_mm256_real(float *input, uint32_t input_len, float *output);

void trx_filter_run_fir_filter_sse_mm256_real(float *input, uint32_t input_len, __m256 *kernel_mm256, float *output);

void trx_filter_run_fir_tx_filter_sse_mm256_complex(cf_t *input_complex, uint32_t input_len, cf_t *output_complex);

void trx_filter_run_fir_rx_filter_sse_mm256_complex(cf_t *input_complex, uint32_t input_len, cf_t *output_complex);

void trx_filter_run_fir_filter_sse_mm256_complex(cf_t *input_complex, uint32_t input_len, __m256 *kernel_mm256, __m256 *signal_block, cf_t *output_complex);

void trx_filter_load_tx_simd_kernel_mm256(float *coeffs, uint32_t filter_size);

void trx_filter_load_rx_simd_kernel_mm256(float *coeffs, uint32_t filter_size);

void trx_filter_load_simd_kernel_mm256(float *coeffs, uint32_t filter_size, __m256 *kernel_mm256);

void trx_filter_run_fir_tx_filter_sse_mm256_complex2(cf_t *input_complex, uint32_t input_len, uint32_t filter_size, cf_t *output_complex);

void trx_filter_run_fir_rx_filter_sse_mm256_complex2(cf_t *input_complex, uint32_t input_len, uint32_t filter_size, cf_t *output_complex);

void trx_filter_run_fir_filter_sse_mm256_complex2(cf_t *input_complex, uint32_t input_len, __m256 *kernel_mm256, uint32_t filter_size, cf_t *output_complex);

void trx_filter_run_fir_tx_filter_sse_mm256_complex3(cf_t *input_complex, uint32_t input_len, cf_t *output_complex, uint32_t packet_cnt, uint32_t packets_to_tx);

void trx_filter_run_fir_rx_filter_sse_mm256_complex3(cf_t *input_complex, uint32_t input_len, cf_t *output_complex, uint32_t packet_cnt, uint32_t packets_to_tx);

void trx_filter_run_fir_filter_sse_mm256_complex3(cf_t *input_complex, uint32_t input_len, __m256 *kernel_mm256, __m256 *signal_block, cf_t *output_complex, uint32_t packet_cnt, uint32_t packets_to_tx);

void trx_filter_free_simd_kernel_mm256();

#endif // _TRX_FILTER_H_
