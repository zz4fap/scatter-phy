#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <fftw3.h>
#include <math.h>
#include <complex.h>

#include <uhd.h>

#include "srslte/srslte.h"
#include "srslte/rf/rf.h"
#include "srslte/utils/debug.h"
#include "srslte/common/timestamp.h"
#include "srslte/utils/vector.h"
#include "srslte/io/filesink.h"

#define NUM_OF_SENSING_SAMPLES 1024

// Global variables for FFT.
fftwf_complex *fft_out_samples, *fft_in_samples;
fftwf_plan sensing_fft_plan;
cf_t *freq_samples;

void sensing_apply_fft_on_bb_samples(cf_t *data) {
  // Execute FFT on samples read from USRP.
  // Copy base band samples into fft type for fftw execution.
  memcpy((uint8_t*)fft_in_samples,(uint8_t*)data,sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);
  // Apply FFT to the received base band samples.
  fftwf_execute(sensing_fft_plan);
  // Copy the result from the fft type into the array for sensing.
  memcpy((uint8_t*)freq_samples,(uint8_t*)fft_out_samples,sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);
}

void sensing_initialize_fft() {
  // Allocate Memory for FFT application.
   fft_in_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SENSING_SAMPLES);
   fft_out_samples = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SENSING_SAMPLES);
   // Allocate memory for subbands.
   freq_samples = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * NUM_OF_SENSING_SAMPLES);
   if(!freq_samples) {
     printf("Error allocating freq_samples memory.\n");
     exit(-1);
   }
   // Create a one dimensional FFT plan.
   sensing_fft_plan = fftwf_plan_dft_1d(NUM_OF_SENSING_SAMPLES, fft_in_samples, fft_out_samples, FFTW_FORWARD, FFTW_ESTIMATE);
}

void sensing_uninitialize_fft() {
  // Free allocated FFT resources
  fftwf_destroy_plan(sensing_fft_plan);
  if(fft_in_samples) {
    fftwf_free(fft_in_samples);
  }
  if(fft_out_samples) {
    fftwf_free(fft_out_samples);
  }
  if(freq_samples) {
    free(freq_samples);
  }
}

void print_signal_cf(cf_t * signal) {
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    printf("Sample[%d]: (%f,%f)\n",i,crealf(signal[i]),cimagf(signal[i]));
  }
}

void print_signal_f(float * signal) {
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    printf("Sample[%d]: (%f,%f)\n",i,signal[i]);
  }
}

void print_abs_signal_cf(cf_t * signal) {
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    printf("Sample[%d]: %f\n",i,cabsf(signal[i]));
  }
}

void test_unit_impulse() {

  cf_t data[NUM_OF_SENSING_SAMPLES];

  // Create signal to be transformed.
  bzero(data, sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);
  data[0] = 1.0+0.0*I;

  // Apply FFT to generated signal.
  sensing_apply_fft_on_bb_samples(data);

  // Check if output is correct.
  double error = 0.0, local_error = 0.0;
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    local_error = (freq_samples[i] - (1.0+0.0*I));
    if(local_error > 1e-5) {
      printf("Local error greater than 1e-5\n");
    }
    error = error + local_error;
  }
  printf("Error: %f\n",error/NUM_OF_SENSING_SAMPLES);

  // Print samples.
  //print_signal_cf(freq_samples);
}

void test_window_signal() {
  cf_t data[NUM_OF_SENSING_SAMPLES];

  // Create signal to be transformed.
  bzero(data, sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);

  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    data[i] = 1.0+0.0*I;
  }

  // Apply FFT to generated signal.
  sensing_apply_fft_on_bb_samples(data);

  // Check if output is correct.
  double error = 0.0, local_error = 0.0;
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    if(i == 0) {
      local_error = (freq_samples[i]/NUM_OF_SENSING_SAMPLES - (1.0+0.0*I));
    } else {
      local_error = (freq_samples[i] - (0.0+0.0*I));
    }
    if(local_error > 1e-5) {
      printf("Local error greater than 1e-5\n");
    }
    error = error + local_error;
  }
  printf("Error: %f\n",error/NUM_OF_SENSING_SAMPLES);

  // Print samples.
  //print_signal_cf(freq_samples);
}

void test_complex_exponential(float fs, float fo, float k) {
  cf_t data[NUM_OF_SENSING_SAMPLES];
  float arg;
  float freq_samples_abs[NUM_OF_SENSING_SAMPLES];

  // Create signal to be transformed.
  bzero(data, sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);

  for(int n = 0; n < NUM_OF_SENSING_SAMPLES; n++) {
    arg = (float) 2.0 * M_PI * k * fo * n * (1.0/fs);
    __real__ data[n] = cosf(arg);
    __imag__ data[n] = sinf(arg);
  }

  // Apply FFT to generated signal.
  sensing_apply_fft_on_bb_samples(data);

  // Calculate absolute signal.
  for(int n = 0; n < NUM_OF_SENSING_SAMPLES; n++) {
    freq_samples_abs[n] = cabsf(freq_samples[n])/NUM_OF_SENSING_SAMPLES;
  }

  //srslte_vec_abs_cf(freq_samples, freq_samples_abs, NUM_OF_SENSING_SAMPLES);

  // Print samples.
  print_signal_f(freq_samples_abs);
  printf("fs: %1.3f [MHz] - fo: %1.3f [MHz]\n",fs/1000000.0,(k*fo)/1000000.0);
}

void test_simple() {
  cf_t data[NUM_OF_SENSING_SAMPLES];

  // Zero all samples of the array.
  bzero(data, sizeof(cf_t)*NUM_OF_SENSING_SAMPLES);

  // Create signal to be transformed.
  for(int i = 0; i < NUM_OF_SENSING_SAMPLES; i++) {
    data[i] = ((float)i)+((float)i)*I;
  }

  // Apply FFT to generated signal.
  sensing_apply_fft_on_bb_samples(data);

  // Print absolute signal.
  print_abs_signal_cf(freq_samples);
}

int main(int argc, char **argv) {

  // Create resources for FFT.
  sensing_initialize_fft();

  // Run tests.
  //test_unit_impulse();
  //test_window_signal();
  //float fs = 23040000.0;
  //float k = 1023.0;
  //float fo = (fs/NUM_OF_SENSING_SAMPLES);
  //test_complex_exponential(fs, fo, k);

  test_simple();

  // Free resources for FFT.
  sensing_uninitialize_fft();

  return 0;
}
