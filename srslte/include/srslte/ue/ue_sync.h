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

/******************************************************************************
 *  File:         ue_sync.h
 *
 *  Description:  This object automatically manages the cell synchronization
 *                procedure.
 *
 *                The main function is srslte_ue_sync_get_buffer(), which returns
 *                a pointer to the aligned subframe of samples (before FFT). This
 *                function should be called regularly, returning every 1 ms.
 *                It reads from the USRP, aligns the samples to the subframe and
 *                performs time/freq synch.
 *
 *                It is also possible to read the signal from a file using the
 *                init function srslte_ue_sync_init_file(). The sampling frequency
 *                is derived from the number of PRB.
 *
 *                The function returns 1 when the signal is correctly acquired and
 *                the returned buffer is aligned with the subframe.
 *
 *  Reference:
 *****************************************************************************/

#ifndef _UE_SYNC_
#define _UE_SYNC_

#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/sync/cfo.h"
#include "srslte/agc/agc.h"
#include "srslte/ch_estimation/chest_dl.h"
#include "srslte/phch/pbch.h"
#include "srslte/dft/ofdm.h"
#include "srslte/common/timestamp.h"
#include "srslte/io/filesource.h"
#include "srslte/utils/debug.h"
#include "srslte/utils/vector.h"
#include "srslte/sync/sync.h"
#include "srslte/rf/rf.h"

#define ENABLE_UE_SYNC_PRINTS 1

#define UE_SYNC_PROFILE_ENABLE 0

// Number of buffers used to store synchronized and aligned subframes.
#define NUMBER_OF_SUBFRAME_BUFFERS 20000

#define UE_SYNC_PRINT(_fmt, ...) do { if(ENABLE_UE_SYNC_PRINTS) { \
  fprintf(stdout, "[UE SYNC PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_SYNC_DEBUG(_fmt, ...) do { if(ENABLE_UE_SYNC_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[UE SYNC DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_SYNC_INFO(_fmt, ...) do { if(ENABLE_UE_SYNC_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[UE SYNC INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define UE_SYNC_ERROR(_fmt, ...) do { fprintf(stdout, "[UE SYNC ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define UE_SYNC_PROFILE(stream, timestamp) do { if(UE_SYNC_PROFILE_ENABLE) { time_t full_secs; double frac_secs; \
  srslte_rf_get_time((srslte_rf_t*)stream, &full_secs, &frac_secs); \
  srslte_timestamp_sub((srslte_timestamp_t*)&timestamp, full_secs, frac_secs); \
  fprintf(stdout, "[UE SYNC PROFILE]: Diff: %f\n",srslte_timestamp_real((srslte_timestamp_t *)&timestamp)); } } while(0)

#define UE_SYNC_PROFILE2(stream, timestamp) do { if(UE_SYNC_PROFILE_ENABLE) { time_t full_secs; double frac_secs; \
  srslte_rf_get_time((srslte_rf_t*)stream, &full_secs, &frac_secs); \
  fprintf(stdout,"[UE SYNC PROFILE]: FPGA time now: %d : %f - Fisrt sample timestamp: %d : %f\n", full_secs, frac_secs, (srslte_timestamp_t)timestamp.full_secs, (srslte_timestamp_t)timestamp.frac_secs); } } while(0)

#define SYNC_STATE(flag) flag==0 ? "SF_FIND":"SF_TRACK"

typedef enum SRSLTE_API {SF_FIND, SF_TRACK} srslte_ue_sync_state_t;

//#define MEASURE_EXEC_TIME

typedef struct SRSLTE_API {
  srslte_sync_t sfind;
  srslte_sync_t strack;

  uint32_t phy_id;

  srslte_agc_t agc;
  bool do_agc;
  uint32_t agc_period;

  void *stream;
  int (*recv_callback)(void*, void*, uint32_t, srslte_timestamp_t*, size_t);
  srslte_timestamp_t last_timestamp;

  srslte_filesource_t file_source;
  bool async_file_mode;
  bool file_mode;
  float file_cfo;
  srslte_cfo_t file_cfo_correct;

  srslte_ue_sync_state_t state;
  srslte_ue_sync_state_t last_state;

  cf_t *input_buffer[NUMBER_OF_SUBFRAME_BUFFERS]; // This array of pointers is used to hold buffers to synchronized and aligned subframes.

  uint32_t frame_len;
  uint32_t fft_size;
  uint32_t nof_recv_sf;  // Number of subframes received each call to srslte_ue_sync_get_buffer
  uint32_t nof_avg_find_frames;
  uint32_t frame_find_cnt;
  uint32_t sf_len;

  /* These count half frames (5ms) */
  uint64_t frame_ok_cnt;
  uint32_t frame_no_cnt;
  uint32_t frame_total_cnt;

  /* this is the system frame number (SFN) */
  uint32_t frame_number;

  srslte_cell_t cell;
  uint32_t sf_idx;

  bool decode_sss_on_track;
  bool correct_cfo;

  uint32_t peak_idx;
  int next_rf_sample_offset;
  int last_sample_offset;
  float mean_sample_offset;
  float mean_sfo;
  uint32_t sample_offset_correct_period;
  float sfo_ema;

  #ifdef MEASURE_EXEC_TIME
  float mean_exec_time;
  #endif
  uint32_t pdsch_num_rxd_bits;
  uint32_t subframe_counter;
  bool is_find_peak_ok;
  int initial_subframe_index;
  int num_of_samples_still_in_buffer;
  int pos_start_of_samples_still_in_buffer;
  uint32_t subframe_buffer_counter;   // This counter is used to point to the current position an synchronized and aligned subframe should be stored.
  uint32_t previous_subframe_buffer_counter_value;
  uint32_t subframe_start_index;
  struct timespec subframe_track_start;
  uint32_t nof_prb;
  bool use_scatter_sync_seq;
  uint32_t pss_len;
  bool enable_second_stage_pss_detection;
} srslte_ue_sync_t;

typedef struct SRSLTE_API {
  uint32_t buffer_number;
  uint32_t subframe_start_index;
  uint32_t sf_idx;
  float peak_value;
  struct timespec peak_detection_timestamp;
  float cfo;
  uint32_t frame_len;
  struct timespec start_of_rx_sample;
  uint32_t nof_subframes_to_rx;
  uint32_t subframe_counter;
  struct timespec subframe_track_start;
  uint32_t mcs;
} short_ue_sync_t;

SRSLTE_API int srslte_ue_sync_init_reentry(srslte_ue_sync_t *q,
                                           srslte_cell_t cell,
                                           int (recv_callback)(void*, void*, uint32_t, srslte_timestamp_t*, size_t),
                                           void *stream_handler);

SRSLTE_API int srslte_ue_sync_init(srslte_ue_sync_t *q,
                                   srslte_cell_t cell,
                                   int (recv_callback)(void*, void*, uint32_t, srslte_timestamp_t*, size_t),
                                   void *stream_handler,
                                   int initial_subframe_index,
                                   bool enable_cfo_correction);

SRSLTE_API int srslte_ue_sync_init_generic(srslte_ue_sync_t *q,
                                           srslte_cell_t cell,
                                           int (recv_callback)(void*, void*, uint32_t, srslte_timestamp_t*, size_t),
                                           void *stream_handler,
                                           int initial_subframe_index,
                                           bool enable_cfo_correction,
                                           bool decode_pdcch,
                                           uint32_t node_id,
                                           uint32_t intf_id,
                                           bool phy_filtering,
                                           bool use_scatter_sync_seq,
                                           uint32_t pss_len,
                                           bool enable_second_stage_pss_detection);

SRSLTE_API int srslte_ue_sync_init_file_new(srslte_ue_sync_t *q,
                                            uint32_t nof_prb, char *file_name, int offset_time, float offset_freq,
                                            srslte_cell_t cell,
                                            int (recv_callback)(void*, void*, uint32_t,srslte_timestamp_t*, size_t),
                                            int initial_subframe_index);

SRSLTE_API int srslte_ue_sync_init_file(srslte_ue_sync_t *q,
                                        uint32_t nof_prb,
                                        char *file_name,
                                        int offset_time,
                                        float offset_freq);

SRSLTE_API void srslte_ue_sync_free_reentry(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_free_except_reentry(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_free(srslte_ue_sync_t *q);

SRSLTE_API int srslte_ue_sync_start_agc(srslte_ue_sync_t *q,
                                        double (set_gain_callback)(void*, double),
                                        float init_gain_value);

SRSLTE_API uint32_t srslte_ue_sync_sf_len(srslte_ue_sync_t *q);

SRSLTE_API int srslte_ue_sync_get_buffer_new(srslte_ue_sync_t *q, cf_t **sf_symbols, size_t channel);

SRSLTE_API int srslte_ue_sync_get_buffer(srslte_ue_sync_t *q, cf_t **sf_symbols, size_t channel);

SRSLTE_API void srslte_ue_sync_set_agc_period(srslte_ue_sync_t *q, uint32_t period);

// CAUTION: input_buffer MUST have space for 3 subframes.
SRSLTE_API int srslte_ue_sync_zerocopy_new(srslte_ue_sync_t *q, size_t channel);

SRSLTE_API int srslte_ue_sync_zerocopy(srslte_ue_sync_t *q, cf_t *input_buffer, size_t channel);

SRSLTE_API void srslte_ue_sync_set_cfo(srslte_ue_sync_t *q, float cfo);

SRSLTE_API void srslte_ue_sync_cfo_i_detec_en(srslte_ue_sync_t *q, bool enable);

SRSLTE_API void srslte_ue_sync_reset(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_set_N_id_2(srslte_ue_sync_t *q, uint32_t N_id_2);

SRSLTE_API void srslte_ue_sync_decode_sss_on_track(srslte_ue_sync_t *q, bool enabled);

SRSLTE_API srslte_ue_sync_state_t srslte_ue_sync_get_state(srslte_ue_sync_t *q);

SRSLTE_API uint32_t srslte_ue_sync_get_sfidx(srslte_ue_sync_t *q);

SRSLTE_API float srslte_ue_sync_get_cfo(srslte_ue_sync_t *q);

SRSLTE_API float srslte_ue_sync_get_carrier_freq_offset(srslte_sync_t *q);

SRSLTE_API float srslte_ue_sync_get_sfo(srslte_ue_sync_t *q);

SRSLTE_API int srslte_ue_sync_get_last_sample_offset(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_set_sample_offset_correct_period(srslte_ue_sync_t *q, uint32_t nof_subframes);

SRSLTE_API void srslte_ue_sync_get_last_timestamp(srslte_ue_sync_t *q, srslte_timestamp_t *timestamp);

SRSLTE_API int srslte_ue_sync_init_reentry_loop(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_set_number_of_received_bits(srslte_ue_sync_t *q, uint32_t pdsch_num_rxd_bits);

SRSLTE_API int srslte_ue_sync_get_subframe_buffer(srslte_ue_sync_t *q, size_t channel);

SRSLTE_API int srslte_ue_sync_allocate_subframe_buffer(srslte_ue_sync_t *q);

SRSLTE_API void srslte_ue_sync_free_subframe_buffer(srslte_ue_sync_t *q);

SRSLTE_API int srslte_ue_synchronize(srslte_ue_sync_t *q, size_t channel);

SRSLTE_API void srslte_ue_set_pss_synch_find_threshold(srslte_ue_sync_t *q, float threshold);

SRSLTE_API void srslte_ue_sync_set_avg_psr_scatter(srslte_ue_sync_t *q, bool enable_avg_psr);

SRSLTE_API void srslte_ue_sync_enable_second_stage_detection_scatter(srslte_ue_sync_t *q, bool enable_second_stage_pss_detection);

SRSLTE_API void srslte_ue_sync_set_pss_synch_find_1st_stage_threshold_scatter(srslte_ue_sync_t *q, float pss_first_stage_threshold);

SRSLTE_API void srslte_ue_sync_set_pss_synch_find_2nd_stage_threshold_scatter(srslte_ue_sync_t *q, float pss_second_stage_threshold);

#endif // _UE_SYNC_
