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

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>

#include "srslte/ue/ue_sync.h"

#include "../../examples/helpers.h"

#define MAX_TIME_OFFSET 128
cf_t dummy[MAX_TIME_OFFSET];

#define TRACK_MAX_LOST          4
#define TRACK_FRAME_SIZE        32
#define FIND_NOF_AVG_FRAMES     4
#define DEFAULT_SAMPLE_OFFSET_CORRECT_PERIOD  0
#define DEFAULT_SFO_EMA_COEFF                 0.1

cf_t dummy_offset_buffer[1024*1024];

int srslte_ue_sync_init_file_new(srslte_ue_sync_t *q,
                                 uint32_t nof_prb, char *file_name, int offset_time, float offset_freq,
                                 srslte_cell_t cell,
                                 int (recv_callback)(void*, void*, uint32_t,srslte_timestamp_t*, size_t), int initial_subframe_index)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q                   != NULL &&
      file_name           != NULL &&
      srslte_nofprb_isvalid(nof_prb))
  {
    ret = SRSLTE_ERROR;

    bzero(q, sizeof(srslte_ue_sync_t));

    q->recv_callback = recv_callback;
    q->cell = cell;
    q->fft_size = srslte_symbol_sz(nof_prb);
    q->sf_len = SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb));
    q->file_mode = true;
    q->file_cfo = -offset_freq;
    q->correct_cfo = true;
    q->agc_period = 0;
    q->sample_offset_correct_period = DEFAULT_SAMPLE_OFFSET_CORRECT_PERIOD;
    q->sfo_ema                      = DEFAULT_SFO_EMA_COEFF;
    q->initial_subframe_index       = initial_subframe_index;
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;
    q->subframe_buffer_counter = 0;
    q->previous_subframe_buffer_counter_value = NUMBER_OF_SUBFRAME_BUFFERS-1;
    q->subframe_start_index = 0;

    if(cell.id == 1000) {

      // If the cell is unkown, we search PSS/SSS in 5 ms.
      q->nof_recv_sf = 5;

      q->decode_sss_on_track = true;

    } else {

      // If the cell is known, we work on a 1ms basis.
      q->nof_recv_sf = 1;

      q->decode_sss_on_track = true;
    }

    q->frame_len = q->nof_recv_sf*q->sf_len;

    if(srslte_cfo_init(&q->file_cfo_correct, 1*q->sf_len)) {
      fprintf(stderr, "Error initiating CFO\n");
      goto clean_exit;
    }

    if(srslte_filesource_init(&q->file_source, file_name, SRSLTE_COMPLEX_FLOAT_BIN)) {
      fprintf(stderr, "Error opening file %s\n", file_name);
      goto clean_exit;
    }
    q->stream = &q->file_source;

    INFO("Offseting input file by %d samples and %.1f kHz\n", offset_time, offset_freq/1000);

    srslte_filesource_read(&q->file_source, dummy_offset_buffer, offset_time);

    if(srslte_sync_init(&q->sfind, q->frame_len, q->frame_len, q->fft_size, cell.id, q->initial_subframe_index)) {
      fprintf(stderr, "Error initiating sync find\n");
      goto clean_exit;
    }
    if(cell.id == 1000) {
      if(srslte_sync_init(&q->strack, q->frame_len, TRACK_FRAME_SIZE, q->fft_size, cell.id, q->initial_subframe_index)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    } else {
      if(srslte_sync_init(&q->strack, q->frame_len, SRSLTE_CP_LEN_NORM(1,q->fft_size), q->fft_size, cell.id, q->initial_subframe_index)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    }

    if(cell.id == 1000) {
      /* If the cell id is unknown, enable CP detection on find */
      // FIXME: CP detection not working very well. Not supporting Extended CP right now
      srslte_sync_cp_en(&q->sfind, false);
      srslte_sync_cp_en(&q->strack, false);

      srslte_sync_set_cfo_ema_alpha(&q->sfind, 1);
      srslte_sync_set_cfo_ema_alpha(&q->strack, 1);

      srslte_sync_cfo_i_detec_en(&q->sfind, false);

      q->nof_avg_find_frames = FIND_NOF_AVG_FRAMES;
      srslte_sync_set_threshold(&q->sfind, 2.0);
      srslte_sync_set_threshold(&q->strack, 1.2);

    } else {
      srslte_sync_set_N_id_2(&q->sfind, cell.id%3);
      srslte_sync_set_N_id_2(&q->strack, cell.id%3);
      q->sfind.cp = cell.cp;
      q->strack.cp = cell.cp;
      srslte_sync_cp_en(&q->sfind, false);
      srslte_sync_cp_en(&q->strack, false);

      srslte_sync_cfo_i_detec_en(&q->sfind, false);

      srslte_sync_set_cfo_ema_alpha(&q->sfind, 1);
      srslte_sync_set_cfo_ema_alpha(&q->strack, 1);

      /* In find phase and if the cell is known, do not average pss correlation
       * because we only capture 1 subframe and do not know where the peak is.
       */
      q->nof_avg_find_frames = 1;
      srslte_sync_set_em_alpha(&q->sfind, 1);
      srslte_sync_set_threshold(&q->sfind, 3.0);

      srslte_sync_set_em_alpha(&q->strack, 0.2);
      srslte_sync_set_threshold(&q->strack, 1.2);

    }

    for(int i = 0; i < NUMBER_OF_SUBFRAME_BUFFERS; i++) {
      q->input_buffer[i] = NULL;
    }

    // Allocate memory for the synchronization process.
    ret = srslte_ue_sync_allocate_subframe_buffer(q);
    if(ret == SRSLTE_ERROR) {
      goto clean_exit;
    }

    srslte_ue_sync_reset(q);

    ret = SRSLTE_SUCCESS;
  }
clean_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_ue_sync_free(q);
  }
  return ret;
}

int srslte_ue_sync_init_file(srslte_ue_sync_t *q, uint32_t nof_prb, char *file_name, int offset_time, float offset_freq) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q                   != NULL &&
      file_name           != NULL &&
      srslte_nofprb_isvalid(nof_prb))
  {
    ret = SRSLTE_ERROR;
    bzero(q, sizeof(srslte_ue_sync_t));
    q->file_mode = true;
    q->sf_len = SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb));
    q->file_cfo = -offset_freq;
    q->correct_cfo = true;
    q->fft_size = srslte_symbol_sz(nof_prb);
    q->nof_prb = q->cell.nof_prb;
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;
    q->subframe_buffer_counter = 0;
    q->previous_subframe_buffer_counter_value = NUMBER_OF_SUBFRAME_BUFFERS-1;
    q->subframe_start_index = 0;

    if(srslte_cfo_init(&q->file_cfo_correct, 2*q->sf_len)) {
      fprintf(stderr, "Error initiating CFO\n");
      goto clean_exit;
    }

    if(srslte_filesource_init(&q->file_source, file_name, SRSLTE_COMPLEX_FLOAT_BIN)) {
      fprintf(stderr, "Error opening file %s\n", file_name);
      goto clean_exit;
    }

    for(int i = 0; i < NUMBER_OF_SUBFRAME_BUFFERS; i++) {
      q->input_buffer[i] = NULL;
    }

    // Allocate memory for the synchronization process.
    ret = srslte_ue_sync_allocate_subframe_buffer(q);
    if(ret == SRSLTE_ERROR) {
      goto clean_exit;
    }

    INFO("Offseting input file by %d samples and %.1f kHz\n", offset_time, offset_freq/1000);

    srslte_filesource_read(&q->file_source, dummy_offset_buffer, offset_time);
    srslte_ue_sync_reset(q);

    ret = SRSLTE_SUCCESS;
  }
clean_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_ue_sync_free(q);
  }
  return ret;
}

int srslte_ue_sync_start_agc(srslte_ue_sync_t *q, double (set_gain_callback)(void*, double), float init_gain_value) {
  uint32_t nframes;
  if(q->nof_recv_sf == 1) {
    nframes = 10;
  } else {
    nframes = 0;
  }
  int n = srslte_agc_init_uhd(&q->agc, SRSLTE_AGC_MODE_PEAK_AMPLITUDE, nframes, set_gain_callback, q->stream);
  q->do_agc = n==0?true:false;
  if(q->do_agc) {
    srslte_agc_set_gain(&q->agc, init_gain_value);
  }
  return n;
}

int srslte_ue_sync_init_reentry_loop(srslte_ue_sync_t *q) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if(q != NULL) {
    // This variable will help switch from the SF_TRACK state back to the SF_FIND state, avoiding any freezing in the ST_TRACK state.
    // Here it is reset everytime the RX procedure is called.
    q->pdsch_num_rxd_bits = 0; // Number of received and decoded PDSCH bits.

    // This counter will be used to count the number of subframes after a peak is detected and first subframe is correctly decoded.
    q->subframe_counter = 0;

    srslte_ue_sync_reset(q);

    // We set last state to TRACK as it was really the TRACK state the last one.
    q->last_state = SF_TRACK;

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

int srslte_ue_sync_init_reentry(srslte_ue_sync_t *q,
                 srslte_cell_t cell,
                 int (recv_callback)(void*, void*, uint32_t,srslte_timestamp_t*,size_t),
                 void *stream_handler)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q                                 != NULL &&
      stream_handler                    != NULL &&
      srslte_nofprb_isvalid(cell.nof_prb)      &&
      recv_callback                     != NULL)
  {
    ret = SRSLTE_ERROR;

    // This variable will help switch from the SF_TRACK state back to the SF_FIND state, avoiding any freezing in the ST_TRACK state.
    // Here it is reset everytime the RX procedure is called.
    q->pdsch_num_rxd_bits = 0; // Number of received and decoded PDSCH bits.
    // This counter will be used to count the number of subframes after a peak is detected and first subframe is correctly decoded.
    q->subframe_counter = 0;

    if(srslte_sync_init_reentry(&q->sfind, q->frame_len, q->frame_len, q->fft_size)) {
      fprintf(stderr, "Error initiating sync find\n");
      goto clean_exit;
    }
    if(cell.id == 1000) {
      if(srslte_sync_init_reentry(&q->strack, q->frame_len, TRACK_FRAME_SIZE, q->fft_size)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    } else {
      if(srslte_sync_init_reentry(&q->strack, q->frame_len, SRSLTE_CP_LEN_NORM(1,q->fft_size), q->fft_size)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    }

    srslte_ue_sync_reset(q);

    ret = SRSLTE_SUCCESS;
  }

clean_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_ue_sync_free(q);
  }
  return ret;
}

int srslte_ue_sync_init(srslte_ue_sync_t *q,
                        srslte_cell_t cell,
                        int (recv_callback)(void*, void*, uint32_t, srslte_timestamp_t*, size_t),
                        void *stream_handler,
                        int initial_subframe_index,
                        bool enable_cfo_correction) {
  return srslte_ue_sync_init_generic(q, cell, recv_callback, stream_handler, initial_subframe_index, enable_cfo_correction, true, 0, 0, false, false, SRSLTE_PSS_LEN, false);
}

int srslte_ue_sync_init_generic(srslte_ue_sync_t *q,
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
                                bool enable_second_stage_pss_detection)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q                                  != NULL &&
     stream_handler                     != NULL &&
     srslte_nofprb_isvalid(cell.nof_prb)      &&
     recv_callback                      != NULL)
  {
    ret = SRSLTE_ERROR;

    bzero(q, sizeof(srslte_ue_sync_t));

    // This variable will help switch from the SF_TRACK state back to the SF_FIND state, avoiding any freezing in the ST_TRACK state.
    // Here it is reset.
    q->pdsch_num_rxd_bits = 0; // Number of received/decoded PDSCH bits.
    q->subframe_counter = 0; // This counter will be used to count the number of subframes after a peak is detected and first subframe is correctly decoded.
    q->stream = stream_handler;
    q->recv_callback = recv_callback;
    q->cell = cell;
    q->fft_size = srslte_symbol_sz(q->cell.nof_prb);
    q->nof_prb = q->cell.nof_prb;
    q->sf_len = SRSLTE_SF_LEN(q->fft_size);
    q->file_mode = false;
    q->correct_cfo = enable_cfo_correction;
    q->agc_period = 0;
    q->sample_offset_correct_period = DEFAULT_SAMPLE_OFFSET_CORRECT_PERIOD;
    q->sfo_ema                      = DEFAULT_SFO_EMA_COEFF;
    q->initial_subframe_index = initial_subframe_index;
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;
    q->subframe_buffer_counter = 0;
    q->previous_subframe_buffer_counter_value = NUMBER_OF_SUBFRAME_BUFFERS-1;
    q->subframe_start_index = 0;
    q->phy_id = intf_id; // PHY ID also known as Radio interface ID (intf_id)
    q->use_scatter_sync_seq = use_scatter_sync_seq;
    q->pss_len = pss_len;
    q->enable_second_stage_pss_detection = enable_second_stage_pss_detection;

    if(cell.id == 1000) {

      // If the cell is unkown, we search PSS/SSS in 5 ms.
      q->nof_recv_sf = 5;

      q->decode_sss_on_track = true;

    } else {

      // If the cell is known, we work on a 1ms basis.
      q->nof_recv_sf = 1;

      q->decode_sss_on_track = true;
    }

    q->frame_len = q->nof_recv_sf*q->sf_len;

    if(srslte_sync_init_generic(&q->sfind, q->frame_len, q->frame_len, q->fft_size, cell.id, q->initial_subframe_index, decode_pdcch, node_id, intf_id, phy_filtering, q->use_scatter_sync_seq, q->pss_len, q->enable_second_stage_pss_detection)) {
      fprintf(stderr, "Error initiating sync find\n");
      goto clean_exit;
    }
    if(cell.id == 1000) {
      if(srslte_sync_init_generic(&q->strack, q->frame_len, TRACK_FRAME_SIZE, q->fft_size, cell.id, q->initial_subframe_index, decode_pdcch, node_id, intf_id, phy_filtering, q->use_scatter_sync_seq, q->pss_len, q->enable_second_stage_pss_detection)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    } else {
      if(srslte_sync_init_generic(&q->strack, q->frame_len, SRSLTE_CP_LEN_NORM(1,q->fft_size), q->fft_size, cell.id, q->initial_subframe_index, decode_pdcch, node_id, intf_id, phy_filtering, q->use_scatter_sync_seq, q->pss_len, q->enable_second_stage_pss_detection)) {
        fprintf(stderr, "Error initiating sync track\n");
        goto clean_exit;
      }
    }

    if(cell.id == 1000) {
      /* If the cell id is unknown, enable CP detection on find */
      // FIXME: CP detection not working very well. Not supporting Extended CP right now
      srslte_sync_cp_en(&q->sfind, false);
      srslte_sync_cp_en(&q->strack, false);

      srslte_sync_set_cfo_ema_alpha(&q->sfind, 1);
      srslte_sync_set_cfo_ema_alpha(&q->strack, 1);

      srslte_sync_cfo_i_detec_en(&q->sfind, false);

      q->nof_avg_find_frames = FIND_NOF_AVG_FRAMES;
      srslte_sync_set_threshold(&q->sfind, 2.0);
      srslte_sync_set_threshold(&q->strack, 1.2);

    } else {
      srslte_sync_set_N_id_2(&q->sfind, cell.id%3);
      srslte_sync_set_N_id_2(&q->strack, cell.id%3);
      q->sfind.cp = cell.cp;
      q->strack.cp = cell.cp;
      srslte_sync_cp_en(&q->sfind, false);
      srslte_sync_cp_en(&q->strack, false);

      srslte_sync_cfo_i_detec_en(&q->sfind, false);

      srslte_sync_set_cfo_ema_alpha(&q->sfind, 1);
      srslte_sync_set_cfo_ema_alpha(&q->strack, 1);

      /* In find phase and if the cell is known, do not average pss correlation
       * because we only capture 1 subframe and do not know where the peak is.
       */
      q->nof_avg_find_frames = 1;
      srslte_sync_set_em_alpha(&q->sfind, 1);
      srslte_sync_set_threshold(&q->sfind, 3.0);

      srslte_sync_set_em_alpha(&q->strack, 0.2);
      srslte_sync_set_threshold(&q->strack, 1.2);
    }

    for(int i = 0; i < NUMBER_OF_SUBFRAME_BUFFERS; i++) {
      q->input_buffer[i] = NULL;
    }

    // Allocate memory for the synchronization process.
    ret = srslte_ue_sync_allocate_subframe_buffer(q);
    if(ret == SRSLTE_ERROR) {
      goto clean_exit;
    }

    srslte_ue_sync_reset(q);

    ret = SRSLTE_SUCCESS;
  }

clean_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_ue_sync_free(q);
  }
  return ret;
}

void srslte_ue_sync_set_avg_psr_scatter(srslte_ue_sync_t *q, bool enable_avg_psr) {
  srslte_sync_set_avg_psr_scatter(&q->sfind, enable_avg_psr);
}

void srslte_ue_sync_enable_second_stage_detection_scatter(srslte_ue_sync_t *q, bool enable_second_stage_pss_detection) {
  srslte_sync_enable_second_stage_detection_scatter(&q->sfind, enable_second_stage_pss_detection);
}

void srslte_ue_set_pss_synch_find_threshold(srslte_ue_sync_t *q, float threshold) {
  srslte_sync_set_threshold(&q->sfind, threshold);
}

void srslte_ue_sync_set_pss_synch_find_1st_stage_threshold_scatter(srslte_ue_sync_t *q, float pss_first_stage_threshold) {
  srslte_sync_set_pss_1st_stage_threshold_scatter(&q->sfind, pss_first_stage_threshold);
}

void srslte_ue_sync_set_pss_synch_find_2nd_stage_threshold_scatter(srslte_ue_sync_t *q, float pss_second_stage_threshold) {
  srslte_sync_set_pss_2nd_stage_threshold_scatter(&q->sfind, pss_second_stage_threshold);
}

uint32_t srslte_ue_sync_sf_len(srslte_ue_sync_t *q) {
  return q->frame_len;
}

void srslte_ue_sync_free_reentry(srslte_ue_sync_t *q) {
  if(!q->file_mode) {
    srslte_sync_free_reentry(&q->sfind);
    srslte_sync_free_reentry(&q->strack);
  }
}

void srslte_ue_sync_free_except_reentry(srslte_ue_sync_t *q) {
  UE_SYNC_INFO("Freeing input buffer\n",0);
  srslte_ue_sync_free_subframe_buffer(q);
  if(q->do_agc) {
    srslte_agc_free(&q->agc);
  }
  if(!q->file_mode) {
    srslte_sync_free_except_reentry(&q->sfind);
    srslte_sync_free_except_reentry(&q->strack);
  } else {
    srslte_filesource_free(&q->file_source);
  }
  bzero(q, sizeof(srslte_ue_sync_t));
}

void srslte_ue_sync_free(srslte_ue_sync_t *q) {
  UE_SYNC_INFO("Freeing input buffer.\n",0);
  srslte_ue_sync_free_subframe_buffer(q);
  if(q->do_agc) {
    srslte_agc_free(&q->agc);
  }
  if(!q->file_mode) {
    srslte_sync_free(&q->sfind);
    srslte_sync_free(&q->strack);
  } else {
    srslte_filesource_free(&q->file_source);
  }
  bzero(q, sizeof(srslte_ue_sync_t));
}

void srslte_ue_sync_get_last_timestamp(srslte_ue_sync_t *q, srslte_timestamp_t *timestamp) {
  memcpy(timestamp, &q->last_timestamp, sizeof(srslte_timestamp_t));
}

uint32_t srslte_ue_sync_peak_idx(srslte_ue_sync_t *q) {
  return q->peak_idx;
}

srslte_ue_sync_state_t srslte_ue_sync_get_state(srslte_ue_sync_t *q) {
  return q->state;
}

uint32_t srslte_ue_sync_get_sfidx(srslte_ue_sync_t *q) {
  return q->sf_idx;
}

void srslte_ue_sync_cfo_i_detec_en(srslte_ue_sync_t *q, bool enable) {
  srslte_sync_cfo_i_detec_en(&q->strack, enable);
  srslte_sync_cfo_i_detec_en(&q->sfind, enable);
}

float srslte_ue_sync_get_cfo(srslte_ue_sync_t *q) {
  return 15000 * srslte_sync_get_cfo(&q->strack);
}

float srslte_ue_sync_get_carrier_freq_offset(srslte_sync_t *q) {
  return 15000 * srslte_sync_get_cfo(q);
}

void srslte_ue_sync_set_cfo(srslte_ue_sync_t *q, float cfo) {
  srslte_sync_set_cfo(&q->sfind, cfo/15000);
  srslte_sync_set_cfo(&q->strack, cfo/15000);
}

float srslte_ue_sync_get_sfo(srslte_ue_sync_t *q) {
  return q->mean_sfo/5e-3;
}

int srslte_ue_sync_get_last_sample_offset(srslte_ue_sync_t *q) {
  return q->last_sample_offset;
}

void srslte_ue_sync_set_sample_offset_correct_period(srslte_ue_sync_t *q, uint32_t nof_subframes) {
  q->sample_offset_correct_period = nof_subframes;
}

void srslte_ue_sync_set_sfo_ema(srslte_ue_sync_t *q, float ema_coefficient) {
  q->sfo_ema = ema_coefficient;
}

void srslte_ue_sync_decode_sss_on_track(srslte_ue_sync_t *q, bool enabled) {
  q->decode_sss_on_track = enabled;
}

void srslte_ue_sync_set_N_id_2(srslte_ue_sync_t *q, uint32_t N_id_2) {
  if(!q->file_mode) {
    srslte_ue_sync_reset(q);
    srslte_sync_set_N_id_2(&q->strack, N_id_2);
    srslte_sync_set_N_id_2(&q->sfind, N_id_2);
  }
}

void srslte_ue_sync_set_agc_period(srslte_ue_sync_t *q, uint32_t period) {
  q->agc_period = period;
}

void print_subframe_buffer_synch(cf_t* const buffer, uint32_t length) {
  for(uint32_t i = 0; i < length; i++) {
    printf("sample %d: %1.2f,%1.2f\n",i,crealf(buffer[i]),cimagf(buffer[i]));
  }
}

static int find_peak_ok(srslte_ue_sync_t *q, size_t channel) {

  int ret = 0, offset, num_of_samples_to_stay, num_of_samples_to_copy, num_of_missing_samples;

  if(srslte_sync_sss_detected(&q->sfind)) {
    // Get the subframe index (0 or 5)
    q->sf_idx = srslte_sync_get_sf_idx(&q->sfind);
  } else {
    DEBUG("Found peak at %d, SSS not detected\n", q->peak_idx);
  }

  // Count the number of detected peaks (positive of false alarms).
  q->frame_find_cnt++;
  DEBUG("Found peak: %d at %d, value: %.3f, Cell_id: %d CP: %s\n",
       q->frame_find_cnt, q->peak_idx,
       srslte_sync_get_last_peak_value(&q->sfind), q->cell.id, srslte_cp_string(q->cell.cp));

  if(q->frame_find_cnt >= q->nof_avg_find_frames || (q->peak_idx-q->frame_len) < 2*q->fft_size) {

    // Reset values before setting.
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;

    INFO("Realigning frame, reading %d samples\n", (q->peak_idx-q->frame_len)+q->sf_len/2);
    // Receive the rest of the subframe so that we are subframe aligned.
    offset = q->peak_idx-(q->sf_len/2);
    num_of_samples_to_stay = NUMBER_OF_SUBFRAMES_TO_STORE*q->sf_len - offset;
    num_of_samples_to_copy = q->sf_len;
    num_of_missing_samples = 0; // number of samples that needs to be read in order to have a full subframe.
    if(num_of_samples_to_stay < q->sf_len) {
      num_of_samples_to_copy = num_of_samples_to_stay;
      num_of_missing_samples = q->sf_len - num_of_samples_to_stay;
    } else {
      q->num_of_samples_still_in_buffer = num_of_samples_to_stay - q->sf_len;
      q->pos_start_of_samples_still_in_buffer = q->peak_idx+(q->sf_len/2);
    }

    //printf("offset: %d - num_of_samples_to_stay: %d - sf_len: %d - num_of_samples_still_in_buffer: %d - pos_start_of_samples_still_in_buffer: %d\n",offset,num_of_samples_to_stay,q->sf_len,q->num_of_samples_still_in_buffer,q->pos_start_of_samples_still_in_buffer);

    // If all the IQ samples of a subframe are already contained in the current buffer, then there is no need to align it to the begining of the buffer, we just use the position of the subframe start.
    if(num_of_missing_samples == 0) {
      // Set the position of the subframe start.
      q->subframe_start_index = offset;
    } else {
      // If there still are samples to be read from the USRP then we align the subframe to the start of the buffer and read the remaining samples.
      // Set the position of the subframe start.
      q->subframe_start_index = 0;
      // Here we align the subframe with the start of the buffer.
      memcpy((uint8_t*)q->input_buffer[q->subframe_buffer_counter], (uint8_t*)(q->input_buffer[q->subframe_buffer_counter]+offset), num_of_samples_to_copy*sizeof(cf_t));
      // We read samples if there still are samples to be read from USRP in order to complete the subframe IQ samples.
      if(srslte_rf_recv_with_time(q->stream, (q->input_buffer[q->subframe_buffer_counter]+num_of_samples_to_copy), num_of_missing_samples, true, &(q->last_timestamp.full_secs), &(q->last_timestamp.frac_secs), channel) < 0) {
        return SRSLTE_ERROR;
      }
    }

    //static uint32_t cnt = 0;
    //printf("cnt: %d - subframe_start_index: %d - num_of_samples_still_in_buffer: %d - pos_start_of_samples_still_in_buffer: %d\n",++cnt,q->subframe_start_index,q->num_of_samples_still_in_buffer,q->pos_start_of_samples_still_in_buffer);

    // Reset variables.
    q->frame_ok_cnt = 0;
    q->frame_no_cnt = 0;
    q->frame_total_cnt = 0;
    q->frame_find_cnt = 0;
    q->mean_sample_offset = 0;

    // Goto Tracking state.
    q->last_state = SF_FIND;
    q->state = SF_TRACK;

    // Initialize track state CFO.
    q->strack.mean_cfo = q->sfind.mean_cfo;
    q->strack.cfo_i    = q->sfind.cfo_i;

    ret = 1;
  }

  return ret;
}

static int track_peak_ok(srslte_ue_sync_t *q, uint32_t track_idx, size_t channel) {

   /* Make sure subframe idx is what we expect */
  if((q->sf_idx != srslte_sync_get_sf_idx(&q->strack)) &&
       q->decode_sss_on_track                           &&
       srslte_sync_sss_detected(&q->strack))
  {
    INFO("Warning: Expected SF idx %d but got %d! (%d frames)\n",
           q->sf_idx, srslte_sync_get_sf_idx(&q->strack), q->frame_no_cnt);
    q->frame_no_cnt++;
    if(q->frame_no_cnt >= TRACK_MAX_LOST) {
      INFO("\n%d frames lost. Going back to FIND\n", (int) q->frame_no_cnt);
      q->state = SF_FIND;
      q->last_state = SF_TRACK;
    }
  } else {
    q->frame_no_cnt = 0;
  }

  // Get sampling time offset
  q->last_sample_offset = ((int) track_idx - (int) q->strack.max_offset/2 - (int) q->strack.fft_size);

  // Adjust sampling time every q->sample_offset_correct_period subframes
  uint32_t frame_idx = 0;
  if(q->sample_offset_correct_period) {
    frame_idx = q->frame_ok_cnt%q->sample_offset_correct_period;
    q->mean_sample_offset += (float) q->last_sample_offset/q->sample_offset_correct_period;
  } else {
    q->mean_sample_offset = q->last_sample_offset;
  }

  // Compute cumulative moving average time offset */
  if(!frame_idx) {
    // Adjust RF sampling time based on the mean sampling offset
    q->next_rf_sample_offset = (int) round(q->mean_sample_offset);

    // Reset PSS averaging if correcting every a period longer than 1
    if(q->sample_offset_correct_period > 1) {
      srslte_sync_reset(&q->strack);
    }

    // Compute SFO based on mean sample offset
    if(q->sample_offset_correct_period) {
      q->mean_sample_offset /= q->sample_offset_correct_period;
    }
    q->mean_sfo = SRSLTE_VEC_EMA(q->mean_sample_offset, q->mean_sfo, q->sfo_ema);

    if(q->next_rf_sample_offset) {
      INFO("Time offset adjustment: %d samples (%.2f), mean SFO: %.2f Hz, %.5f samples/5-sf, ema=%f, length=%d\n",
           q->next_rf_sample_offset, q->mean_sample_offset,
           srslte_ue_sync_get_sfo(q),
           q->mean_sfo, q->sfo_ema, q->sample_offset_correct_period);
    }
    q->mean_sample_offset = 0;
  }

  /* If the PSS peak is beyond the frame (we sample too slowly),
    discard the offseted samples to align next frame */
  if(q->next_rf_sample_offset > 0 && q->next_rf_sample_offset < MAX_TIME_OFFSET) {
    DEBUG("Positive time offset %d samples.\n", q->next_rf_sample_offset);
    if (q->recv_callback(q->stream, dummy, (uint32_t) q->next_rf_sample_offset, &q->last_timestamp, channel) < 0) {
      fprintf(stderr, "Error receiving from USRP\n");
      return SRSLTE_ERROR;
    }
    q->next_rf_sample_offset = 0;
  }

  q->peak_idx = q->frame_len + q->sf_len/2 + q->last_sample_offset;
  q->frame_ok_cnt++;

  return 1;
}

static int track_peak_no(srslte_ue_sync_t *q) {

  /* if we missed too many PSS go back to FIND and consider this frame unsynchronized */
  q->frame_no_cnt++;
  if(q->frame_no_cnt >= TRACK_MAX_LOST) {
    INFO("\n%d frames lost. Going back to FIND\n", (int) q->frame_no_cnt);
    q->state = SF_FIND;
    q->last_state = SF_TRACK;
    return 0;
  } else {
    INFO("Tracking peak not found. Peak %.3f, %d lost\n",
         srslte_sync_get_last_peak_value(&q->strack), (int) q->frame_no_cnt);
    return 1;
  }

}

static inline int receive_samples_old(srslte_ue_sync_t *q, size_t channel) {

  // A negative time offset means there are samples in our buffer for the next subframe, because we are sampling too fast.
  if(q->next_rf_sample_offset < 0) {
    UE_SYNC_ERROR("q->next_rf_sample_offset < 0!!!!: %d\n",q->next_rf_sample_offset);
    q->next_rf_sample_offset = -q->next_rf_sample_offset;
    exit(-1);
  }

  // Calculate the number of samples that must stay in the buffer.
  int offset = q->frame_len - q->next_rf_sample_offset;
  int num_of_samples_to_stay = (NUMBER_OF_SUBFRAMES_TO_STORE-1)*q->frame_len + q->next_rf_sample_offset;
  memcpy((uint8_t*)q->input_buffer[q->subframe_buffer_counter], (uint8_t*)(q->input_buffer[q->subframe_buffer_counter] + offset), num_of_samples_to_stay*sizeof(cf_t));

  //struct timespec start_time;
  //clock_gettime(CLOCK_REALTIME, &start_time);

  //time_t full_secs; double frac_secs;
  //srslte_rf_get_time(q->stream, &full_secs, &frac_secs);

  // Get N subframes from the USRP getting more samples and keeping the previous samples, if any
  if(srslte_rf_recv_with_time(q->stream, &q->input_buffer[q->subframe_buffer_counter][num_of_samples_to_stay], offset, true, &(q->last_timestamp.full_secs), &(q->last_timestamp.frac_secs), channel) < 0) {
    UE_SYNC_ERROR("Error receive_samples at receive_samples() - subframe_buffer_counter: %d - num_of_samples_to_stay: %d - offset: %d\n", q->subframe_buffer_counter, num_of_samples_to_stay, offset);
    return SRSLTE_ERROR;
  }

  //srslte_timestamp_sub(&q->last_timestamp, full_secs, frac_secs);
  //double avg_time_diff = srslte_timestamp_real(&q->last_timestamp);
  //PHY_PROFILLING_AVG2("Avg. diff fpga time now and 1st sample timestamp: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",avg_time_diff);

  //fprintf(stdout, "[UE SYNC PROFILE]: Diff fpga time now and 1st sample timestamp: %f\n",);

  //PHY_PROFILLING_AVG("recv_callback: %f\n",helpers_profiling_diff_time(start_time));

  // Compare FPGA time now and timestamp of received fisrt sample.
  //UE_SYNC_PROFILE(q->stream, q->last_timestamp);

  // reset time offset.
  q->next_rf_sample_offset = 0;

  return SRSLTE_SUCCESS;
}

static inline int receive_samples(srslte_ue_sync_t *q, size_t channel) {

  int offset = 0, num_of_samples_to_stay = 0;

  // A negative time offset means there are samples in our buffer for the next subframe, because we are sampling too fast.
  if(q->next_rf_sample_offset < 0) {
    UE_SYNC_ERROR("q->next_rf_sample_offset < 0!!!!: %d\n",q->next_rf_sample_offset);
    q->next_rf_sample_offset = -q->next_rf_sample_offset;
    exit(-1);
  }

  // Move to the current buffer samples from previous buffer that were not used to find the peak.
  if(q->num_of_samples_still_in_buffer > 0) {
    // Move samples from previous buffer into the current one.
    memcpy((uint8_t*)(q->input_buffer[q->subframe_buffer_counter]+q->frame_len), (uint8_t*)(q->input_buffer[q->previous_subframe_buffer_counter_value]+q->pos_start_of_samples_still_in_buffer), q->num_of_samples_still_in_buffer*sizeof(cf_t));
    // Calculate the number of samples we still have to read from the USRP.
    offset = (NUMBER_OF_SUBFRAMES_TO_STORE-1)*q->frame_len - q->num_of_samples_still_in_buffer;
    if(offset < 0) {
      UE_SYNC_ERROR("receive_samples(): offset should never be negative....\n",0);
      exit(-1);
    }
    // Set the pointer to the end of the moved samples so that we can read and position the USRP samples at the correct position within the buffer.
    num_of_samples_to_stay = q->frame_len + q->num_of_samples_still_in_buffer;
    // Reset counters.
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;
  } else {
    // Calculate the number of samples that must stay in the buffer.
    offset = q->frame_len - q->next_rf_sample_offset;
    num_of_samples_to_stay = (NUMBER_OF_SUBFRAMES_TO_STORE-1)*q->frame_len + q->next_rf_sample_offset;
    memcpy((uint8_t*)q->input_buffer[q->subframe_buffer_counter], (uint8_t*)(q->input_buffer[q->subframe_buffer_counter] + offset), num_of_samples_to_stay*sizeof(cf_t));
  }

  //struct timespec start_time;
  //clock_gettime(CLOCK_REALTIME, &start_time);

  //time_t full_secs; double frac_secs;
  //srslte_rf_get_time(q->stream, &full_secs, &frac_secs);

  // Get N subframes from the USRP getting more samples and keeping the previous samples, if any
  if(srslte_rf_recv_with_time(q->stream, &q->input_buffer[q->subframe_buffer_counter][num_of_samples_to_stay], offset, true, &(q->last_timestamp.full_secs), &(q->last_timestamp.frac_secs), channel) < 0) {
    UE_SYNC_ERROR("PHY ID: %d - Error at receive_samples at receive_samples() - subframe_buffer_counter: %d - num_of_samples_to_stay: %d - offset: %d\n", channel, q->subframe_buffer_counter, num_of_samples_to_stay, offset);
    return SRSLTE_ERROR;
  }

  //srslte_timestamp_sub(&q->last_timestamp, full_secs, frac_secs);
  //double avg_time_diff = srslte_timestamp_real(&q->last_timestamp);
  //PHY_PROFILLING_AVG2("Avg. diff fpga time now and 1st sample timestamp: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",avg_time_diff);

  //fprintf(stdout, "[UE SYNC PROFILE]: Diff fpga time now and 1st sample timestamp: %f\n",);

  //PHY_PROFILLING_AVG("recv_callback: %f\n",helpers_profiling_diff_time(start_time));

  // Compare FPGA time now and timestamp of received fisrt sample.
  //UE_SYNC_PROFILE(q->stream, q->last_timestamp);

  // reset time offset.
  q->next_rf_sample_offset = 0;

  return SRSLTE_SUCCESS;
}

static inline int receive_samples_after_peak_found(srslte_ue_sync_t *q, size_t channel) {

  // Transfer the IQ samples still in the previous buffer to the current buffer.
  if(q->num_of_samples_still_in_buffer > 0) {
    memcpy((uint8_t*)(q->input_buffer[q->subframe_buffer_counter]+q->frame_len), (uint8_t*)(q->input_buffer[q->previous_subframe_buffer_counter_value]+q->pos_start_of_samples_still_in_buffer), q->num_of_samples_still_in_buffer*sizeof(cf_t));
  }

  //struct timespec start_time;
  //clock_gettime(CLOCK_REALTIME, &start_time);

  if(q->num_of_samples_still_in_buffer > q->frame_len) {
    UE_SYNC_PRINT("receive_samples_after_peak_found() - subframe_buffer_counter: %d - num_of_samples_still_in_buffer: %d - frame_len: %d - sf_len: %d\n", q->subframe_buffer_counter, q->num_of_samples_still_in_buffer, q->frame_len,q->sf_len);
    UE_SYNC_PRINT("peak_idx: %d - peak_value: %f - nof_subframes_to_rx: %d\n", q->peak_idx, q->sfind.peak_value, q->sfind.nof_subframes_to_rx);
  }

  // Check if we still have to read more samples from USRP in order to have a full subframe in the buffer.
  if(q->num_of_samples_still_in_buffer < q->frame_len) {
    // Read samples from the USRP in order to have at least one full subframe in the buffer.
    if(srslte_rf_recv_with_time(q->stream, (q->input_buffer[q->subframe_buffer_counter]+q->frame_len+q->num_of_samples_still_in_buffer), (q->sf_len-q->num_of_samples_still_in_buffer), true, &(q->last_timestamp.full_secs), &(q->last_timestamp.frac_secs), channel) < 0) {
      UE_SYNC_ERROR("Error receiving samples at receive_samples_after_peak_found() - subframe_buffer_counter: %d - num_of_samples_still_in_buffer: %d - num_of_samples_still_in_buffer: %d\n",q->subframe_buffer_counter,q->num_of_samples_still_in_buffer,q->num_of_samples_still_in_buffer);
      return SRSLTE_ERROR;
    }
    // Reset counters.
    q->num_of_samples_still_in_buffer = 0;
    q->pos_start_of_samples_still_in_buffer = 0;
  } else { // There is a full subframe in the buffer, no need to read more samples from the USRP.
    // Calculate number of samples still in the buffer that need to be copied into the next buffer.
    q->num_of_samples_still_in_buffer = q->num_of_samples_still_in_buffer - q->frame_len;
    q->pos_start_of_samples_still_in_buffer = 2*q->frame_len;
  }

  // After peak is found, the subframe start index will always point to the second buffer.
  q->subframe_start_index = q->frame_len;

  //PHY_PROFILLING_AVG("recv_callback: %f\n",helpers_profiling_diff_time(start_time));

  // Compare FPGA time now and timestamp of received fisrt sample.
  //UE_SYNC_PROFILE(q->stream, q->last_timestamp);

  return SRSLTE_SUCCESS;
}

int srslte_ue_sync_get_subframe_buffer(srslte_ue_sync_t *q, size_t channel) {
  int ret = srslte_ue_synchronize(q, channel);
  if(ret == 1) {
    // Update the previous counter value so that it can be used the read the last synchronized subframe.
    q->previous_subframe_buffer_counter_value = q->subframe_buffer_counter;
    // Increase the subframe buffer counter.
    q->subframe_buffer_counter = (q->subframe_buffer_counter + 1)%NUMBER_OF_SUBFRAME_BUFFERS;

    //struct timespec bzero_time;
    //clock_gettime(CLOCK_REALTIME, &bzero_time);

    // Zero buffer so that we don't find ghost peaks.
    bzero(q->input_buffer[q->subframe_buffer_counter], (NUMBER_OF_SUBFRAMES_TO_STORE*q->frame_len*sizeof(cf_t)));
    //PHY_PROFILLING_AVG3("Average bzero time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(&bzero_time), 0.5, 1000);
  }
  return ret;
}

int srslte_ue_sync_get_buffer_new(srslte_ue_sync_t *q, cf_t **sf_symbols, size_t channel) {
  int ret = srslte_ue_synchronize(q, channel);
  if(sf_symbols) {
    *sf_symbols = q->input_buffer[q->subframe_buffer_counter];
  }
  return ret;
}

// Returns 1 if the subframe is synchronized in time, 0 otherwise.
int srslte_ue_synchronize(srslte_ue_sync_t *q, size_t channel) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q != NULL) {

    // Uncomment the lines below to measure the time it take to read samples and synchronize to a read buffer with IQ samples.
    //struct timespec read_plus_synch_time;
    //clock_gettime(CLOCK_REALTIME, &read_plus_synch_time);

    switch(q->state) {
      case SF_FIND:
      {
        // Read samples from USRP and keep looking for peak.
        if(receive_samples(q, channel)) {
          UE_SYNC_ERROR("PHY ID: %d - Error receiving samples at srslte_ue_synchronize().\n", channel);
          return SRSLTE_ERROR;
        }

        // Uncomment the lines below to measure the time it take to synchronize to a read buffer with IQ samples.
        //struct timespec synch_time;
        //clock_gettime(CLOCK_REALTIME, &synch_time);

        int sync_ret = srslte_sync_find(&q->sfind, q->input_buffer[q->subframe_buffer_counter], 0, &q->peak_idx);
        if(sync_ret != SRSLTE_SYNC_FOUND) {
          sync_ret = srslte_sync_find(&q->sfind, q->input_buffer[q->subframe_buffer_counter], (q->sf_len/2), &q->peak_idx);
        }
        switch(sync_ret) {
          case SRSLTE_SYNC_ERROR:
            ret = SRSLTE_ERROR;
            UE_SYNC_ERROR("Error finding correlation peak: (%d)\n", ret);
            break;
          case SRSLTE_SYNC_FOUND:
            ret = find_peak_ok(q, channel);
            break;
          case SRSLTE_SYNC_FOUND_NOSPACE:
            // If a peak was found but there is not enough space for SSS/CP detection, rearrange a few samples.
            srslte_sync_reset(&q->sfind);
            UE_SYNC_ERROR("No space for SSS/CP detection. Exiting.\n",0);
            exit(-1);
            ret = SRSLTE_SUCCESS;
            break;
          default:
            ret = SRSLTE_SUCCESS;
            break;
        }
        // Change last state to correspond to SF_FIND as now it will be the last one.
        if(ret != 1 && q->last_state == SF_TRACK && q->state == SF_FIND) {
          q->last_state = SF_FIND;
        }
        if(q->do_agc) {
          srslte_agc_process(&q->agc, q->input_buffer[q->subframe_buffer_counter]+2*q->sf_len, q->sf_len);
        }

        // Used to measure CFO estimation and correction time.
        //struct timespec est_time;
        //clock_gettime(CLOCK_REALTIME, &est_time);

        // Estimate CFO based on CP if it is enabled and if subframe was detected and synchronized.
        if(sync_ret == SRSLTE_SYNC_FOUND && ret == 1 && srslte_sync_get_cfo_correction_type(&q->sfind) == CFO_CORRECTION_CP) {
          srslte_sync_set_cfo_new(&q->sfind, srslte_sync_cfo_estimate_cp(&q->sfind, (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index)));
        }
        //PHY_PROFILLING_AVG3("Avg. CFO estimation time: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(&est_time), 0.2, 1000);

        // Apply CFO correction to data subframe in case the synchronization subframe is detected and aligned.
        // Data is always decoded from the first buffer (we store subframes in three buffers with the length of subframe each one.)
        if(q->correct_cfo && sync_ret == SRSLTE_SYNC_FOUND && ret == 1) {
          float cfo_estimate = srslte_sync_get_cfo_new(&q->sfind);
          // Correct CFO based on the estimation done for the subframe via PSS or CP.
          if(fabs(cfo_estimate) > 0.01) { // Only correct if CFO is greater than or equal to 150 Hz (i.e., 0.01).
#if(USE_FINE_GRAINED_CFO==1)
            //UE_SYNC_PRINT("CFO before correction: %1.4f\n",cfo_estimate*15000.0);
            // Apply fine-grained CFO correction.
            srslte_cfo_correct_finer(&q->sfind.cfocorr_finer,
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          -cfo_estimate / q->fft_size);
            //float cfo_est = srslte_sync_cfo_estimate_cp(&q->sfind, (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index));
            //printf("CFO before: %f - CFO after: %f\n", cfo_estimate*15000.0, cfo_est*15000.0);
            //UE_SYNC_PRINT("CFO after correction: %1.4f\n",srslte_sync_cfo_estimate_cp(&q->sfind, (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index))*15000.0);
#else
            srslte_cfo_correct(&q->sfind.cfocorr,
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          -cfo_estimate / q->fft_size);
#endif
          }
          //PHY_PROFILLING_AVG3("Avg. synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(synch_time), 1.0, 1000);
          //PHY_PROFILLING_AVG3("Avg. read samples + synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(read_plus_synch_time), 1.0, 1000);
        }
        //PHY_PROFILLING_AVG3("[SF FIND] Avg. CFO estimation/correction time: %f - min: %f - max: %f - max counter %d - diff >= 100us: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(&est_time), 0.1, 10000);
      }
      break;
      case SF_TRACK:
      {
        // Timestamp the start of the TRACK state.
        clock_gettime(CLOCK_REALTIME, &q->subframe_track_start);
        // Receive samples in synchronized way as the peak was found.
        if(receive_samples_after_peak_found(q, channel)) {
          UE_SYNC_ERROR("Error receiving samples after peak found.\n",0);
          return SRSLTE_ERROR;
        }
        // Make sure we try to decode the subframe.
        ret = 1;
        // Update subframe number only the first time as all the subsequente ones will have the same subframe number.
        if(q->last_state == SF_FIND) {
          q->sf_idx = q->sf_idx + 1;
        }
        // Update the last state.
        q->last_state = SF_TRACK;

        // Used to measure CFO estimation and correction time.
        //struct timespec est_time_track;
        //clock_gettime(CLOCK_REALTIME, &est_time_track);

        // Estimate CFO based on CP if it is enabled.
        if(srslte_sync_get_cfo_correction_type(&q->sfind) == CFO_CORRECTION_CP) {
          srslte_sync_set_cfo_new(&q->sfind, srslte_sync_cfo_estimate_cp(&q->sfind, (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index)));
        }
        // Apply CFO correction to data subframe in case the synchronization subframe is detected and aligned.
        if(q->correct_cfo) {
          float cfo_estimate = srslte_sync_get_cfo_new(&q->sfind);
          // Correct CFO based on the estimation done for the subframe via PSS or CP.
          if(fabs(cfo_estimate) > 0.01) { // Only correct if CFO is greater than or equal to 150 Hz (i.e., 0.01).
#if(USE_FINE_GRAINED_CFO==1)
            // Apply fine-grained CFO correction.
            srslte_cfo_correct_finer(&q->sfind.cfocorr_finer,
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          -cfo_estimate / q->fft_size);
#else
            srslte_cfo_correct(&q->sfind.cfocorr,
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          (q->input_buffer[q->subframe_buffer_counter] + q->subframe_start_index),
                          -cfo_estimate / q->fft_size);
#endif
          }
        }
        //PHY_PROFILLING_AVG3("[SF TRACK] Avg. CFO estimation/correction time: %f - min: %f - max: %f - max counter %d - diff >= 100us: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(&est_time_track), 0.1, 10000);
      }
      break;
    }
  }
  return ret;
}

// Returns 1 if the subframe is synchronized in time, 0 otherwise.
int srslte_ue_sync_zerocopy_new(srslte_ue_sync_t *q, size_t channel) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q != NULL) {

    if(q->file_mode) {
      if(receive_samples(q, channel)) {
        fprintf(stderr, "Error receiving samples\n");
        return SRSLTE_ERROR;
      }
      switch(q->state) {
        case SF_FIND:
          switch(srslte_sync_find(&q->sfind, q->input_buffer[q->subframe_buffer_counter]+q->frame_len, 0, &q->peak_idx)) {
            case SRSLTE_SYNC_ERROR:
              ret = SRSLTE_ERROR;
              fprintf(stderr, "Error finding correlation peak (%d)\n", ret);
              return SRSLTE_ERROR;
            case SRSLTE_SYNC_FOUND:
              ret = find_peak_ok(q, channel);
              break;
            case SRSLTE_SYNC_FOUND_NOSPACE:
              /* If a peak was found but there is not enough space for SSS/CP detection, discard a few samples */
              INFO("No space for SSS/CP detection. Realigning frame...\n",0);
              //q->recv_callback(q->stream, dummy_offset_buffer, q->frame_len/2, NULL);
              srslte_sync_reset(&q->sfind);
              ret = SRSLTE_SUCCESS;
              break;
            default:
              ret = SRSLTE_SUCCESS;
              break;
          }
          if(q->do_agc) {
            srslte_agc_process(&q->agc, q->input_buffer[q->subframe_buffer_counter], q->sf_len);
          }

          if(q->correct_cfo && ret == 1) {
            srslte_cfo_correct(&q->sfind.cfocorr,
                          q->input_buffer[q->subframe_buffer_counter]+q->frame_len,
                          q->input_buffer[q->subframe_buffer_counter]+q->frame_len,
                          -srslte_sync_get_cfo_new(&q->sfind) / q->fft_size);
          }

        break;
        case SF_TRACK:

          ret = 1;

          q->sf_idx = (q->sf_idx + q->nof_recv_sf) % 10;

          if(q->correct_cfo) {
            srslte_cfo_correct(&q->sfind.cfocorr,
                          q->input_buffer[q->subframe_buffer_counter]+q->frame_len,
                          q->input_buffer[q->subframe_buffer_counter]+q->frame_len,
                          -srslte_sync_get_cfo_new(&q->sfind) / q->fft_size);
          }
        break;
      }
    } else {

      // Uncomment the lines below to measure the time it take to read samples and synchronize to a read buffer with IQ samples.
      //struct timespec read_plus_synch_time;
      //clock_gettime(CLOCK_REALTIME, &read_plus_synch_time);

#if SCATTER_DEBUG_MODE
      if(q->last_state != q->state) {
        UE_SYNC_DEBUG("SYNC STATE: %s\n", SYNC_STATE(q->state));
      }
#endif

      switch(q->state) {
        case SF_FIND:
        {
          // Read samples from USRP and keep looking for peak.
          if(receive_samples(q, channel)) {
            UE_SYNC_ERROR("PHY ID: %d - Error receiving samples at srslte_ue_sync_zerocopy_new().\n", channel);
            return SRSLTE_ERROR;
          }

          // Uncomment the lines below to measure the time it take to synchronize to a read buffer with IQ samples.
          //struct timespec synch_time;
          //clock_gettime(CLOCK_REALTIME, &synch_time);

          int sync_ret = srslte_sync_find(&q->sfind, q->input_buffer[q->subframe_buffer_counter], 0, &q->peak_idx);
          if(sync_ret != SRSLTE_SYNC_FOUND) {
            sync_ret = srslte_sync_find(&q->sfind, q->input_buffer[q->subframe_buffer_counter], (q->sf_len/2), &q->peak_idx);
          }
          switch(sync_ret) {
            case SRSLTE_SYNC_ERROR:
              ret = SRSLTE_ERROR;
              UE_SYNC_ERROR("Error finding correlation peak: (%d)\n", ret);
              return SRSLTE_ERROR;
            case SRSLTE_SYNC_FOUND:
              ret = find_peak_ok(q, channel);
              break;
            case SRSLTE_SYNC_FOUND_NOSPACE:
              // If a peak was found but there is not enough space for SSS/CP detection, rearrange a few samples.
              UE_SYNC_PRINT("No space for SSS/CP detection. Exiting.\n",0);
              exit(-1);
              srslte_sync_reset(&q->sfind);
              ret = SRSLTE_SUCCESS;
              break;
            default:
              ret = SRSLTE_SUCCESS;
              break;
          }
          if(q->do_agc) {
            srslte_agc_process(&q->agc, q->input_buffer[q->subframe_buffer_counter]+2*q->sf_len, q->sf_len);
          }
          // Apply CFO correction to data subframe in case the synchronization subframe is detected and aligned.
          // Data is always decoded from the first buffer (we store subframes in three buffers with the length of subframe each one.)
          if(q->correct_cfo && ret == 1) {
            srslte_cfo_correct(&q->sfind.cfocorr,
                          q->input_buffer[q->subframe_buffer_counter],
                          q->input_buffer[q->subframe_buffer_counter],
                          -srslte_sync_get_cfo_new(&q->sfind) / q->fft_size);

            //PHY_PROFILLING_AVG3("Avg. synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(synch_time), 1.0, 1000);

            //PHY_PROFILLING_AVG3("Avg. read samples + synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(read_plus_synch_time), 1.0, 1000);
          }
        }
        break;
        case SF_TRACK:
        {

          if(receive_samples_after_peak_found(q, channel)) {
            UE_SYNC_ERROR("Error receiving samples after peak found.\n",0);
            return SRSLTE_ERROR;
          }

          // Make sure we try to decode the subframe.
          ret = 1;
          // Update subframe number only the first time as all the subsequente ones will have the same subframe number.
          if(q->last_state == SF_FIND) {
            q->sf_idx = q->sf_idx + 1;
          }
          // Update the last state.
          q->last_state = SF_TRACK;
          // Apply CFO correction to data subframe in case the synchronization subframe is detected and aligned.
          if(q->correct_cfo) {
            srslte_cfo_correct(&q->sfind.cfocorr,
                          q->input_buffer[q->subframe_buffer_counter],
                          q->input_buffer[q->subframe_buffer_counter],
                          -srslte_sync_get_cfo_new(&q->sfind) / q->fft_size);
          }
        }
        break;
      }
    }
  }
  return ret;
}

void srslte_ue_sync_set_number_of_received_bits(srslte_ue_sync_t *q, uint32_t pdsch_num_rxd_bits) {
  q->pdsch_num_rxd_bits = pdsch_num_rxd_bits;
}

int srslte_ue_sync_get_buffer(srslte_ue_sync_t *q, cf_t **sf_symbols, size_t channel) {
  int ret = srslte_ue_sync_zerocopy(q, q->input_buffer[q->subframe_buffer_counter], channel);
  if(sf_symbols) {
    *sf_symbols = q->input_buffer[q->subframe_buffer_counter];
  }
  return ret;
}

// Returns 1 if the subframe is synchronized in time, 0 otherwise.
int srslte_ue_sync_zerocopy(srslte_ue_sync_t *q, cf_t *input_buffer, size_t channel) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  uint32_t track_idx;

  if(q            != NULL   &&
      input_buffer != NULL)
  {

    if(q->file_mode) {
      int n = srslte_filesource_read(&q->file_source, input_buffer, q->sf_len);
      if (n < 0) {
        fprintf(stderr, "Error reading input file\n");
        return SRSLTE_ERROR;
      }
      if(n == 0) {
        srslte_filesource_seek(&q->file_source, 0);
        q->sf_idx = 9;
        int n = srslte_filesource_read(&q->file_source, input_buffer, q->sf_len);
        if (n < 0) {
          fprintf(stderr, "Error reading input file\n");
          return SRSLTE_ERROR;
        }
      }
      if(q->correct_cfo) {
        srslte_cfo_correct(&q->file_cfo_correct,
                    input_buffer,
                    input_buffer,
                    q->file_cfo / 15000 / q->fft_size);

      }
      q->sf_idx++;
      if (q->sf_idx == 10) {
        q->sf_idx = 0;
      }
      INFO("Reading %d samples. sf_idx = %d\n", q->sf_len, q->sf_idx);
      ret = 1;
    } else {
      if(receive_samples(q, channel)) {
        fprintf(stderr, "Error receiving samples\n");
        return SRSLTE_ERROR;
      }

      switch(q->state) {
        case SF_FIND:
          switch(srslte_sync_find(&q->sfind, input_buffer, 0, &q->peak_idx)) {
            case SRSLTE_SYNC_ERROR:
              ret = SRSLTE_ERROR;
              fprintf(stderr, "Error finding correlation peak (%d)\n", ret);
              return SRSLTE_ERROR;
            case SRSLTE_SYNC_FOUND:
              ret = find_peak_ok(q, channel);
              break;
            case SRSLTE_SYNC_FOUND_NOSPACE:
              /* If a peak was found but there is not enough space for SSS/CP detection, discard a few samples */
              INFO("No space for SSS/CP detection. Realigning frame...\n",0);
              q->recv_callback(q->stream, dummy_offset_buffer, q->frame_len/2, NULL, channel);
              srslte_sync_reset(&q->sfind);
              ret = SRSLTE_SUCCESS;
              break;
            default:
              ret = SRSLTE_SUCCESS;
              break;
          }
          if(q->do_agc) {
            srslte_agc_process(&q->agc, input_buffer, q->sf_len);
          }

        break;
        case SF_TRACK:

          ret = 1;

          srslte_sync_sss_en(&q->strack, q->decode_sss_on_track);

          q->sf_idx = (q->sf_idx + q->nof_recv_sf) % 10;

          // Every SF idx 0 and 5, find peak around known position q->peak_idx.
          if(q->sf_idx == 0 || q->sf_idx == 5) {

            if (q->do_agc && (q->agc_period == 0 ||
                             (q->agc_period && (q->frame_total_cnt%q->agc_period) == 0)))
            {
              srslte_agc_process(&q->agc, input_buffer, q->sf_len);
            }

            #ifdef MEASURE_EXEC_TIME
            struct timeval t[3];
            gettimeofday(&t[1], NULL);
            #endif

            track_idx = 0;

            /* Track PSS/SSS around the expected PSS position
             * In tracking phase, the subframe carrying the PSS is always the last one of the frame
             */
            switch(srslte_sync_find(&q->strack, input_buffer,
                                    q->frame_len - q->sf_len/2 - q->fft_size - q->strack.max_offset/2,
                                    &track_idx))
            {
              case SRSLTE_SYNC_ERROR:
                ret = SRSLTE_ERROR;
                fprintf(stderr, "Error tracking correlation peak\n");
                return SRSLTE_ERROR;
              case SRSLTE_SYNC_FOUND:
                ret = track_peak_ok(q, track_idx, channel);
                break;
              case SRSLTE_SYNC_FOUND_NOSPACE:
                // It's very very unlikely that we fall here because this event should happen at FIND phase only
                ret = 0;
                q->state = SF_FIND;
                printf("Warning: No space for SSS/CP while in tracking phase\n");
                break;
              case SRSLTE_SYNC_NOFOUND:
                ret = track_peak_no(q);
                break;
            }

            #ifdef MEASURE_EXEC_TIME
            gettimeofday(&t[2], NULL);
            get_time_interval(t);
            q->mean_exec_time = (float) SRSLTE_VEC_CMA((float) t[0].tv_usec, q->mean_exec_time, q->frame_total_cnt);
            #endif

            if(ret == SRSLTE_ERROR) {
              fprintf(stderr, "Error processing tracking peak\n");
              q->state = SF_FIND;
              return SRSLTE_SUCCESS;
            }

            q->frame_total_cnt++;
          }
          if(q->correct_cfo) {
            srslte_cfo_correct(&q->sfind.cfocorr,
                          input_buffer,
                          input_buffer,
                          -srslte_sync_get_cfo(&q->strack) / q->fft_size);
          }
        break;
      }

    }
  }
  return ret;
}

void srslte_ue_sync_reset(srslte_ue_sync_t *q) {

  if(!q->file_mode) {
    srslte_sync_reset(&q->sfind);
    srslte_sync_reset(&q->strack);
  } else {
    q->sf_idx = 9;
  }
  q->state = SF_FIND;
  q->last_state = SF_FIND;
  q->frame_ok_cnt = 0;
  q->frame_no_cnt = 0;
  q->frame_total_cnt = 0;
  q->mean_sample_offset = 0.0;
  q->next_rf_sample_offset = 0;
  q->frame_find_cnt = 0;
  #ifdef MEASURE_EXEC_TIME
  q->mean_exec_time = 0;
  #endif
}

// Allocate buffer that will be used to store synchronized and aligned subframes.
int srslte_ue_sync_allocate_subframe_buffer(srslte_ue_sync_t *q) {
   for(int i = 0; i < NUMBER_OF_SUBFRAME_BUFFERS; i++) {
      q->input_buffer[i] = (cf_t*)srslte_vec_malloc(NUMBER_OF_SUBFRAMES_TO_STORE*q->frame_len*sizeof(cf_t));
      if(!q->input_buffer[i]) {
         UE_SYNC_ERROR("Impossible to allocate memory for subframe buffer.\n",0);
         return SRSLTE_ERROR;
      }
   }
   return SRSLTE_SUCCESS;
}

// Free buffer that will be used to store synchronized and aligned subframes.
void srslte_ue_sync_free_subframe_buffer(srslte_ue_sync_t *q) {
   for(int i = 0; i < NUMBER_OF_SUBFRAME_BUFFERS; i++) {
      if(q->input_buffer[i] != NULL) {
         free(q->input_buffer[i]);
         q->input_buffer[i] = NULL;
      }
   }
}
