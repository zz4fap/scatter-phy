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

#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <string.h>
#include <pthread.h>

#include "srslte/dft/dft.h"
#include "srslte/utils/vector.h"

#define dft_ceil(a,b) ((a-1)/b+1)
#define dft_floor(a,b) (a/b)

pthread_mutex_t dft_fftw_mutex = PTHREAD_MUTEX_INITIALIZER;

int srslte_dft_plan(srslte_dft_plan_t* const plan, const int dft_points, srslte_dft_dir_t dir, srslte_dft_mode_t mode) {
  if(mode == SRSLTE_DFT_COMPLEX){
    return srslte_dft_plan_c(plan,dft_points,dir);
  } else {
    return srslte_dft_plan_r(plan,dft_points,dir);
  }
  // Everything went well.
  return 0;
}

static void allocate(srslte_dft_plan_t* const plan, int size_in, int size_out, int len) {
  plan->in = fftwf_malloc(size_in*len);
  plan->out = fftwf_malloc(size_out*len);
}

int srslte_dft_plan_c(srslte_dft_plan_t* const plan, const int dft_points, srslte_dft_dir_t dir) {
  int ret = 0;
  pthread_mutex_lock(&dft_fftw_mutex);
  allocate(plan, sizeof(fftwf_complex), sizeof(fftwf_complex), dft_points);
  int sign = (dir == SRSLTE_DFT_FORWARD) ? FFTW_FORWARD : FFTW_BACKWARD;
  plan->p = fftwf_plan_dft_1d(dft_points, plan->in, plan->out, sign, 0U);
  if(!plan->p) {
    fprintf(stderr, "[DFT Error] Error creating FFTW Complex plan.\n");
    ret = -1;
    goto exit_dft_plan_c;
  }
  if(dft_points <= 0) {
    fprintf(stderr, "[DFT Error] DFT Complex plan number of points has to be greater than 0: %d.\n", dft_points);
    ret = -1;
    goto exit_dft_plan_c;
  }
  plan->size = dft_points;
  plan->mode = SRSLTE_DFT_COMPLEX;
  plan->dir = dir;
  plan->forward = (dir==SRSLTE_DFT_FORWARD)?true:false;
  plan->mirror = false;
  plan->db = false;
  plan->norm = false;
  plan->dc = false;

exit_dft_plan_c:
  pthread_mutex_unlock(&dft_fftw_mutex);

  return ret;
}

int srslte_dft_plan_r(srslte_dft_plan_t* const plan, const int dft_points, srslte_dft_dir_t dir) {
  int ret = 0;
  pthread_mutex_lock(&dft_fftw_mutex);
  allocate(plan,sizeof(float),sizeof(float), dft_points);
  int sign = (dir == SRSLTE_DFT_FORWARD) ? FFTW_R2HC : FFTW_HC2R;
  plan->p = fftwf_plan_r2r_1d(dft_points, plan->in, plan->out, sign, 0U);
  if(!plan->p) {
    fprintf(stderr, "[DFT Error] Error creating FFTW Real plan.\n");
    ret = -1;
    goto exit_dft_plan_r;
  }
  if(dft_points <= 0) {
    fprintf(stderr, "[DFT Error] DFT Real plan number of points has to be greater than 0: %d.\n", dft_points);
    ret = -1;
    goto exit_dft_plan_r;
  }
  plan->size = dft_points;
  plan->mode = SRSLTE_REAL;
  plan->dir = dir;
  plan->forward = (dir==SRSLTE_DFT_FORWARD)?true:false;
  plan->mirror = false;
  plan->db = false;
  plan->norm = false;
  plan->dc = false;

exit_dft_plan_r:
  pthread_mutex_unlock(&dft_fftw_mutex);

  return ret;
}

void srslte_dft_plan_set_mirror(srslte_dft_plan_t* const plan, bool val){
  pthread_mutex_lock(&dft_fftw_mutex);
  plan->mirror = val;
  pthread_mutex_unlock(&dft_fftw_mutex);
}
void srslte_dft_plan_set_db(srslte_dft_plan_t* const plan, bool val){
  pthread_mutex_lock(&dft_fftw_mutex);
  plan->db = val;
  pthread_mutex_unlock(&dft_fftw_mutex);
}
void srslte_dft_plan_set_norm(srslte_dft_plan_t* const plan, bool val){
  pthread_mutex_lock(&dft_fftw_mutex);
  plan->norm = val;
  pthread_mutex_unlock(&dft_fftw_mutex);
}
void srslte_dft_plan_set_dc(srslte_dft_plan_t* const plan, bool val){
  pthread_mutex_lock(&dft_fftw_mutex);
  plan->dc = val;
  pthread_mutex_unlock(&dft_fftw_mutex);
}

// Swap upper part of the resource grid (positive frequencies) with the lower part of the resource grid (negative frequencies)
static void copy_pre(uint8_t *dst, uint8_t *src, int size_d, int len, bool forward, bool mirror, bool dc) {
  int offset = dc?1:0;
  if(mirror && !forward){
    int hlen = dft_floor(len,2);
    // Set DC subcarrier if it is enabled.
    memset(dst,0,size_d*offset);
    // Next two lines swap contents of the resource grid so that it conforms with the FFT/IFFT processing, i.e., 0 to +fs/2 (lower part) and -fs/2 to 0 (upper part).
    // Copy the upper part of the resource grid to lower part of the buffer sent to the IFFT/FFT.
    memcpy(&dst[size_d*offset], &src[size_d*hlen], size_d*(len-hlen-offset));
    // Copy the lower part of the resource grid to upper part of the buffer sent to the IFFT/FFT.
    memcpy(&dst[(len-hlen)*size_d], src, size_d*hlen);
  } else {
    memcpy(dst,src,size_d*len);
  }
}

static void copy_post(uint8_t *dst, uint8_t *src, int size_d, int len, bool forward, bool mirror, bool dc) {
  int offset = dc?1:0;
  if(mirror && forward){
    int hlen = dft_ceil(len,2);
    memcpy(dst, &src[size_d*hlen], size_d*(len-hlen));
    memcpy(&dst[(len-hlen)*size_d], &src[size_d*offset], size_d*(hlen-offset));
  } else {
    memcpy(dst,src,size_d*len);
  }
}

void srslte_dft_run(srslte_dft_plan_t* const plan, void *in, void *out) {
  if(plan->mode == SRSLTE_DFT_COMPLEX) {
    srslte_dft_run_c(plan,in,out);
  } else {
    srslte_dft_run_r(plan,in,out);
  }
}

void srslte_dft_run_c_zerocopy(srslte_dft_plan_t* const plan, cf_t *in, cf_t *out) {
  fftwf_execute_dft(plan->p, in, out);
}

void srslte_dft_run_c(srslte_dft_plan_t* const plan, cf_t *in, cf_t *out) {
  float norm;
  int i;
  fftwf_complex *f_out = plan->out;

  copy_pre((uint8_t*)plan->in, (uint8_t*)in, sizeof(cf_t), plan->size, plan->forward, plan->mirror, plan->dc);
  fftwf_execute(plan->p);
  if (plan->norm) {
    norm = 1.0/sqrtf(plan->size);
    srslte_vec_sc_prod_cfc(f_out, norm, f_out, plan->size);
  }
  if (plan->db) {
    for (i=0;i<plan->size;i++) {
      f_out[i] = 10*log10(f_out[i]);
    }
  }
  copy_post((uint8_t*)out, (uint8_t*)plan->out, sizeof(cf_t), plan->size, plan->forward, plan->mirror, plan->dc);
}

void srslte_dft_run_r(srslte_dft_plan_t* const plan, float *in, float *out) {
  float norm;
  int i;
  int len = plan->size;
  float *f_out = plan->out;

  memcpy(plan->in,in,sizeof(float)*plan->size);
  fftwf_execute(plan->p);
  if (plan->norm) {
    norm = 1.0/plan->size;
    srslte_vec_sc_prod_fff(f_out, norm, f_out, plan->size);
  }
  if (plan->db) {
    for (i=0;i<len;i++) {
      f_out[i] = 10*log10(f_out[i]);
    }
  }
  memcpy(out,plan->out,sizeof(float)*plan->size);
}

void srslte_dft_plan_free(srslte_dft_plan_t* const plan) {
  pthread_mutex_lock(&dft_fftw_mutex);
  if(!plan) {
    fprintf(stderr, "[DFT Error] DFT plan is NULL..\n");
    goto exit_dft_plan_free;
  }
  if(!plan->size) {
    fprintf(stderr, "[DFT Error] Invalid fft size: %d\n", plan->size);
    goto exit_dft_plan_free;
  }
  if(plan->in) {
    fftwf_free(plan->in);
    plan->in = NULL;
  }
  if(plan->out) {
    fftwf_free(plan->out);
    plan->out = NULL;
  }
  if(plan->p) {
    fftwf_destroy_plan(plan->p);
    plan->p = NULL;
  }
  bzero(plan, sizeof(srslte_dft_plan_t));

exit_dft_plan_free:
  pthread_mutex_unlock(&dft_fftw_mutex);
}
