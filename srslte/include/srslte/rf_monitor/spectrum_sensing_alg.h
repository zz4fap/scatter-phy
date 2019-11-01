#ifndef _SPECTRUM_SENSING_ALG_H_
#define _SPECTRUM_SENSING_ALG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <fftw3.h>
#include <complex.h>
#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/utils/vector.h"
#include "srslte/utils/debug.h"
#include "srslte/intf/intf.h"

#include "srslte/rf_monitor/statistics_helpers.h"

#include "rf_monitor.h"

#define ENABLE_SENSING_ALG_PRINTS 1

#define SENSING_ALG_PRINT(_fmt, ...) do { if(ENABLE_SENSING_ALG_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[SENSING ALG PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define SENSING_ALG_DEBUG(_fmt, ...) do { if(ENABLE_SENSING_ALG_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[SENSING ALG DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define SENSING_ALG_INFO(_fmt, ...) do { if(ENABLE_SENSING_ALG_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[SENSING ALG INFO]: " _fmt, __VA_ARGS__); } while(0)

#define SENSING_ALG_ERROR(_fmt, ...) do { fprintf(stdout, "[SENSING ALG ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define NUM_OF_SPECTRUM_SENSING_SAMPLES 2048

#define IS_DETECTED(is_detected) (is_detected==true) ? "TRUE":"FALSE"

void spectrum_sensing_alg_init();

void spectrum_sensing_alg_free();

void spectrum_sensing_alg_calculate_subband_energy(cf_t *base_band_samples);

void spectrum_sensing_alg_sort_energy();

float spectrum_sensing_alg_calculate_noise_reference(int* number_of_zref_segs);

float spectrum_sensing_alg_calculate_scale_factor(int x, float pfa);

void spectrum_sensing_alg_detect_primary_user(float alpha, float zref, int Index);

int spectrum_sensing_alg_get_detection_array(uint8_t **data);

int spectrum_sensing_alg_get_subband_energy_array(uint8_t **data);

void spectrum_sensing_alg_print_subband_energy();

void spectrum_sensing_alg_print_sorted_subband_energy();

void spectrum_sensing_alg_print_detection_array(uint8_t *data, int data_length);

void *spectrum_sensing_alg_work(void *h);

#endif // _SPECTRUM_SENSING_ALG_H_
