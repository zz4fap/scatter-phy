#include "phy_reception.h"

// *********** Global variables ***********
// This handle is used to pass some important objects to the PHY Rx thread work function.
static phy_reception_t *phy_rx_threads[MAX_NUM_CONCURRENT_PHYS] = {NULL};

// Mutex used to control access to the BW change section of the code.
pthread_mutex_t phy_rx_bw_change_mutex = PTHREAD_MUTEX_INITIALIZER;

// *********** Definition of functions ***********
// This function is used to set everything needed for the phy reception thread to run accordinly.
int phy_reception_start_thread(LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args, uint32_t phy_id) {
  // Allocate memory for a new Tx object.
  phy_rx_threads[phy_id] = (phy_reception_t*)srslte_vec_malloc(sizeof(phy_reception_t));
  // Check if memory allocation was correctly done.
  if(phy_rx_threads[phy_id] == NULL) {
    PHY_RX_ERROR("PHY ID: %d - Error allocating memory for Rx context.\n", phy_id);
    return -1;
  }
  // Set PHY Rx context with PHY ID number.
  phy_rx_threads[phy_id]->phy_id = phy_id;
  // Initialize PHY Rx thread context.
  if(phy_reception_init_thread_context(phy_rx_threads[phy_id], handle, rf, args) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when initializing Rx thread context.\n", phy_id);
    return -1;
  }
  // Start synchronization thread.
  if(phy_reception_start_sync_thread(phy_rx_threads[phy_id]) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when starting PHY synchronization thread.\n", phy_id);
    return -1;
  }
  // Start decoding thread.
  if(phy_reception_start_decoding_thread(phy_rx_threads[phy_id]) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when starting PHY decoding thread.\n", phy_id);
    return -1;
  }
  // Enable plot for debugging purposes.
#if(ENBALE_RX_INFO_PLOT==1)
  if(phy_rx_threads[phy_id]->plot_rx_info == true) {
    // Initialize plot.
    init_plot();
  }
#endif
  return 0;
}

int phy_reception_init_thread_context(phy_reception_t* const phy_reception_ctx, LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args) {
  // Instantiate and reserve capacity for circular buffer.
  sync_cb_make(&phy_reception_ctx->rx_synch_handle, NUMBER_OF_SUBFRAME_BUFFERS);
  // Set PHY reception context.
  phy_reception_init_context(phy_reception_ctx, handle, rf, args);
  // Set Rx sample rate according to the number of PRBs.
  if(phy_reception_set_rx_sample_rate(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error setting Rx sample rate.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Configure initial Rx frequency and gains.
  if(phy_reception_set_initial_rx_freq_and_gain(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when setting Rx freq. or gain.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Initialize struture with Cell parameters.
  phy_reception_init_cell_parameters(phy_reception_ctx);
  // Initialize structure with last configured Rx basic control.
  phy_reception_init_last_basic_control(phy_reception_ctx);
  // Do all UE related intilization: SYNC and Downlink.
  if(phy_reception_ue_init(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error initializing synch and decoding structures.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Try to stop Rx Stream if that is open, flush reception buffer and open Rx Stream.
  if(phy_reception_initialize_rx_stream(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error initializing Rx stream.\n", 0);
    return -1;
  }
#ifndef ENABLE_CH_EMULATOR
  // Create a watchdog timer for synchronization thread.
  if(timer_init(&phy_reception_ctx->synch_thread_timer_id) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Not possible to create a timer for synchronization thread.\n", phy_reception_ctx->phy_id);
    return -1;
  }
#endif
  // Initialize mutex for basic control fields access.
  if(pthread_mutex_init(&phy_reception_ctx->rx_last_basic_control_mutex, NULL) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Mutex for basic control access init failed.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Initialize mutex for ue sync structure access.
  if(pthread_mutex_init(&phy_reception_ctx->rx_sync_mutex, NULL) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Mutex for ue sync structure access init failed.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&phy_reception_ctx->rx_sync_cv, NULL)) {
    PHY_RX_ERROR("PHY ID: %d - Conditional variable init failed.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Everything went well.
  return 0;
}

// Free all the resources used by the phy reception module.
int phy_reception_stop_thread(uint32_t phy_id) {
  // Stop synchronization thread.
  if(phy_reception_stop_sync_thread(phy_rx_threads[phy_id]) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when stopping PHY synchronization thread.\n", phy_rx_threads[phy_id]->phy_id);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - Synchronization thread stopped successfully.\n", phy_rx_threads[phy_id]->phy_id);
  // Stop decoding thread.
  if(phy_reception_stop_decoding_thread(phy_rx_threads[phy_id]) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error when stopping PHY decoding thread.\n", phy_rx_threads[phy_id]->phy_id);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - Decoding thread stopped successfully.\n", phy_rx_threads[phy_id]->phy_id);
  // Destroy mutex for basic control fields access.
  pthread_mutex_destroy(&phy_rx_threads[phy_id]->rx_last_basic_control_mutex);
  // Destroy mutex for ue sync structure access.
  pthread_mutex_destroy(&phy_rx_threads[phy_id]->rx_sync_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&phy_rx_threads[phy_id]->rx_sync_cv) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Conditional variable destruction failed.\n", phy_rx_threads[phy_id]->phy_id);
    return -1;
  }
  // Free all related UE Downlink structures.
  phy_reception_ue_free(phy_rx_threads[phy_id]);
  // Stop Rx Stream and Flush reception buffer.
  if(phy_reception_stop_rx_stream_and_flush_buffer(phy_rx_threads[phy_id]) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping and flushing Rx stream.\n", phy_rx_threads[phy_id]->phy_id);
    return -1;
  }
  // Free plot context object if plot is enabled.
#if(ENBALE_RX_INFO_PLOT==1)
  // free plot object.
  if(phy_rx_threads[phy_id]->plot_rx_info == true) {
    free_plot();
  }
#endif
  // Delete circular buffer object.
  sync_cb_free(&phy_rx_threads[phy_id]->rx_synch_handle);
  // Free memory used to store Rx object.
  if(phy_rx_threads[phy_id]) {
    free(phy_rx_threads[phy_id]);
    phy_rx_threads[phy_id] = NULL;
  }
  // Everything went well.
  return 0;
}

int phy_reception_start_decoding_thread(phy_reception_t* const phy_reception_ctx) {
  // Enable receiving thread.
  phy_reception_ctx->run_rx_decoding_thread = true;
  // Create threads to perform phy reception.
  pthread_attr_init(&phy_reception_ctx->rx_decoding_thread_attr);
  pthread_attr_setdetachstate(&phy_reception_ctx->rx_decoding_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread to decode data.
  int rc = pthread_create(&phy_reception_ctx->rx_decoding_thread_id, &phy_reception_ctx->rx_decoding_thread_attr, phy_reception_decoding_work, (void *)phy_reception_ctx);
  if(rc) {
    PHY_RX_ERROR("PHY ID: %d - Return code from PHY reception pthread_create() is %d\n", phy_reception_ctx->phy_id, rc);
    return -1;
  }
  // Everything went well.
  return 0;
}

int phy_reception_stop_decoding_thread(phy_reception_t* const phy_reception_ctx) {
  phy_reception_ctx->run_rx_decoding_thread = false; // Stop decoding thread.
  pthread_attr_destroy(&phy_reception_ctx->rx_decoding_thread_attr);
  int rc = pthread_join(phy_reception_ctx->rx_decoding_thread_id, NULL);
  if(rc) {
    PHY_RX_ERROR("PHY ID: %d - Return code from PHY reception pthread_join() is %d\n", phy_reception_ctx->phy_id, rc);
    return -1;
  }
  // Everything went well.
  return 0;
}

int phy_reception_start_sync_thread(phy_reception_t* const phy_reception_ctx) {
  // Enable synchronization thread.
  phy_reception_ctx->run_rx_synchronization_thread = true;
  // Create threads to perform phy synchronization.
  pthread_attr_init(&phy_reception_ctx->rx_sync_thread_attr);
  pthread_attr_setdetachstate(&phy_reception_ctx->rx_sync_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread to perform synchronization.
  int rc = pthread_create(&phy_reception_ctx->rx_sync_thread_id, &phy_reception_ctx->rx_sync_thread_attr, phy_reception_sync_work, (void *)phy_reception_ctx);
  if(rc) {
    PHY_RX_ERROR("PHY ID: %d - Return code from PHY synchronization pthread_create() is %d\n", phy_reception_ctx->phy_id, rc);
    return -1;
  }
  // Everything went well.
  return 0;
}

int phy_reception_stop_sync_thread(phy_reception_t* const phy_reception_ctx) {
  // Stop synchronization thread.
  phy_reception_ctx->run_rx_synchronization_thread = false;
  // Notify condition variable.
  pthread_cond_signal(&phy_reception_ctx->rx_sync_cv);
  pthread_attr_destroy(&phy_reception_ctx->rx_sync_thread_attr);
  int rc = pthread_join(phy_reception_ctx->rx_sync_thread_id, NULL);
  if(rc) {
    PHY_RX_ERROR("PHY ID: %d - Return code from PHY synchronization pthread_join() is %d\n", phy_reception_ctx->phy_id, rc);
    return -1;
  }
  // Everything went well.
  return 0;
}

// Initialize struture with Cell parameters.
void phy_reception_init_cell_parameters(phy_reception_t* const phy_reception_ctx) {
  phy_reception_ctx->cell_ue.nof_prb         = phy_reception_ctx->nof_prb;            // nof_prb
  phy_reception_ctx->cell_ue.nof_ports       = phy_reception_ctx->nof_ports;          // nof_ports
  phy_reception_ctx->cell_ue.bw_idx          = 0;                                     // bw idx
  phy_reception_ctx->cell_ue.id              = phy_reception_ctx->radio_id;           // cell_id
  phy_reception_ctx->cell_ue.cp              = SRSLTE_CP_NORM;                        // cyclic prefix
  phy_reception_ctx->cell_ue.phich_length    = SRSLTE_PHICH_NORM;                     // PHICH length
  phy_reception_ctx->cell_ue.phich_resources = SRSLTE_PHICH_R_1;                      // PHICH resources
}

// Initialize structure with last configured Rx basic control.
void phy_reception_init_last_basic_control(phy_reception_t* const phy_reception_ctx) {
  phy_reception_ctx->last_rx_basic_control.trx_flag     = PHY_RX_ST;
  phy_reception_ctx->last_rx_basic_control.seq_number   = 0;
  phy_reception_ctx->last_rx_basic_control.send_to      = phy_reception_ctx->node_id;
  phy_reception_ctx->last_rx_basic_control.intf_id      = phy_reception_ctx->intf_id;
  phy_reception_ctx->last_rx_basic_control.bw_idx       = phy_reception_ctx->bw_idx;
  phy_reception_ctx->last_rx_basic_control.ch           = (phy_reception_ctx->default_rx_channel+phy_reception_ctx->phy_id);
  phy_reception_ctx->last_rx_basic_control.frame        = 0;
  phy_reception_ctx->last_rx_basic_control.slot         = 0;
  phy_reception_ctx->last_rx_basic_control.timestamp    = 0;
  phy_reception_ctx->last_rx_basic_control.mcs          = 0;
  phy_reception_ctx->last_rx_basic_control.gain         = phy_reception_ctx->initial_rx_gain;
  phy_reception_ctx->last_rx_basic_control.length       = 1;
}

void phy_reception_init_context(phy_reception_t* const phy_reception_ctx, LayerCommunicator_handle handle, srslte_rf_t *rf, transceiver_args_t *args) {
  phy_reception_ctx->phy_comm_handle                    = handle;
  phy_reception_ctx->rf                                 = rf;
  phy_reception_ctx->initial_rx_gain                    = args->initial_rx_gain;
  phy_reception_ctx->competition_bw                     = args->competition_bw;
  phy_reception_ctx->competition_center_freq            = args->competition_center_frequency;
  phy_reception_ctx->rnti                               = args->rnti;
  phy_reception_ctx->initial_agc_gain                   = args->initial_agc_gain;
  phy_reception_ctx->use_std_carrier_sep                = args->use_std_carrier_sep;
  phy_reception_ctx->initial_subframe_index             = args->initial_subframe_index;
  phy_reception_ctx->add_tx_timestamp                   = args->add_tx_timestamp;
  phy_reception_ctx->plot_rx_info                       = args->plot_rx_info;
  phy_reception_ctx->enable_cfo_correction              = args->enable_cfo_correction;
  phy_reception_ctx->nof_prb                            = args->nof_prb;
  phy_reception_ctx->default_rx_channel                 = args->default_rx_channel;
  phy_reception_ctx->radio_id                           = args->radio_id;
  phy_reception_ctx->nof_ports                          = args->nof_ports;
  phy_reception_ctx->max_turbo_decoder_noi              = args->max_turbo_decoder_noi;
  phy_reception_ctx->max_turbo_decoder_noi_for_high_mcs = args->max_turbo_decoder_noi_for_high_mcs;
  phy_reception_ctx->default_rx_bandwidth               = helpers_get_bw_from_nprb(args->nof_prb);      // Get bandwidth from number of physical resource blocks.
  phy_reception_ctx->bw_idx                             = helpers_get_bw_index_from_prb(args->nof_prb); // Convert from number of Resource Blocks to BW Index.
  phy_reception_ctx->run_rx_decoding_thread             = true; // Variable used to stop phy decoding thread.
  phy_reception_ctx->run_rx_synchronization_thread      = true; // Variable used to stop phy synchronization thread.
  phy_reception_ctx->decode_pdcch                       = args->decode_pdcch; // If enabled, then PDCCH is decoded, otherwise, SCH is decoded.
  phy_reception_ctx->node_id                            = args->node_id; // SRN ID, a number from 0 to 255.
  phy_reception_ctx->intf_id                            = args->intf_id; // Radio Interface ID.
  phy_reception_ctx->phy_filtering                      = args->phy_filtering;
  phy_reception_ctx->threshold                          = args->threshold; // PSS detection threshold.
  phy_reception_ctx->use_scatter_sync_seq               = args->use_scatter_sync_seq;
  phy_reception_ctx->pss_len                            = args->pss_len;
  phy_reception_ctx->enable_avg_psr                     = args->enable_avg_psr; // Enable PSR min/max averaging.
  phy_reception_ctx->enable_second_stage_pss_detection  = args->enable_second_stage_pss_detection;
  phy_reception_ctx->pss_first_stage_threshold          = args->pss_first_stage_threshold;
  phy_reception_ctx->pss_second_stage_threshold         = args->pss_second_stage_threshold;
}

static inline int phy_reception_change_bw(phy_reception_t* const phy_reception_ctx, basic_ctrl_t* const bc) {
  int ret = 0;
  // Lock this section of the code so that PHY Rx threads do not interfere with each other when changing the USRP parameters.
  pthread_mutex_lock(&phy_rx_bw_change_mutex);
  // Stop synchronization thread before configuring a new bandwidth.
  if(phy_reception_stop_sync_thread(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping synchronization thread.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  PHY_RX_PRINT("PHY ID: %d - Change BW - Synchronization thread stopped\n", phy_reception_ctx->phy_id);
  // Stop decoding thread before configuring a new bandwidth.
  if(phy_reception_stop_decoding_thread(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping decoding thread.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  PHY_RX_PRINT("PHY ID: %d - Change BW - Decoding thread stopped\n", phy_reception_ctx->phy_id);
  // Set the new number of PRB based on PRB retrieved from BW index.
  phy_reception_ctx->cell_ue.nof_prb = helpers_get_prb_from_bw_index(bc->bw_idx);
  // Free all Rx related structures.
  phy_reception_ue_free(phy_reception_ctx);
  // Set parameters for new PHY BW.
  phy_reception_ctx->nof_prb                          = phy_reception_ctx->cell_ue.nof_prb;
  phy_reception_ctx->default_rx_bandwidth             = helpers_get_bw_from_nprb(phy_reception_ctx->nof_prb);
  phy_reception_ctx->bw_idx                           = helpers_get_bw_index_from_prb(phy_reception_ctx->nof_prb);
  phy_reception_ctx->run_rx_decoding_thread           = true;
  phy_reception_ctx->run_rx_synchronization_thread    = true;
  // Set the new Rx sample rate based on the new PRB sent by the upper layer.
  // Set Rx sample rate according to the number of PRBs.
  if(phy_reception_set_rx_sample_rate(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error setting Rx sample rate.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  // Initialize all Rx related structures.
  if(phy_reception_ue_init(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error initializing synch and decoding structures.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  // After configuring a new bandwidth the synchronization thread needs to be restarted.
  if(phy_reception_start_sync_thread(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error starting synchronization thread.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  // After configuring a new bandwidth the decoding thread needs to be restarted.
  if(phy_reception_start_decoding_thread(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error starting decoding thread.\n", phy_reception_ctx->phy_id);
    ret = -1;
    goto exit_phy_rx_change_bw;
  }
  // Update last tx basic control structure with current BW index.
  set_bw_index(phy_reception_ctx, bc->bw_idx);

exit_phy_rx_change_bw:
  // Unlock the mutex so that other Tx thread can is able to execute.
  pthread_mutex_unlock(&phy_rx_bw_change_mutex);
  // Everything went well.
  return ret;
}

int phy_reception_change_parameters(basic_ctrl_t* const bc) {
  phy_reception_t* const phy_reception_ctx = phy_rx_threads[bc->phy_id];
  float rx_bandwidth, rx_gain;

  // Check bandwidth.
  rx_bandwidth = helpers_get_bandwidth_float(bc->bw_idx);
  if(rx_bandwidth < 0.0) {
    PHY_RX_ERROR("PHY ID: %d - Undefined BW Index: %d....\n", phy_reception_ctx->phy_id, bc->bw_idx);
    return -1;
  }
  // Check if number of slots is valid.
  if(bc->length <= 0) {
    PHY_RX_ERROR("PHY ID: %d - Invalid number of slots. It MUST be greater than 0. Current value is: %d\n", phy_reception_ctx->phy_id, bc->length);
    return -1;
  }
#if(RX_TIMED_COMMAND_ENABLED==1)
  // Check timestamp.
  if(bc->timestamp == 0) {
    PHY_RX_ERROR("PHY ID: %d - Timestamp is zero....\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Apply Rx timed-command parameters.
  if(phy_reception_change_timed_parameters(phy_reception_ctx, bc) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Problem setting timed parameters....\n", phy_reception_ctx->phy_id);
    return -1;
  }
#else
  // Apply Rx non-timed-command parameters.
  if(phy_reception_change_non_timed_parameters(phy_reception_ctx, bc) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Problem setting non-timed parameters....\n", phy_reception_ctx->phy_id);
    return -1;
  }
#endif
  // If bandwidth has changed then free and initialize UE DL.
  if(phy_reception_ctx->last_rx_basic_control.bw_idx != bc->bw_idx) {
    if(phy_reception_change_bw(phy_reception_ctx, bc) < 0) {
      PHY_RX_ERROR("PHY ID: %d - Error changing BW.\n", phy_reception_ctx->phy_id);
      return -1;
    }
  }
  // If AGC is disabled then we set the new gain sent by MAC in a basic control command.
  // If AGC is enabled in command line with "-g -1" parameter, the gain sent by MAC layer is ignored as there is no point in setting a gain when AGC is ON.
  if(phy_reception_ctx->initial_rx_gain >= 0.0 && bc->gain >= 0) {
    // Checking if last configured gain is different from the current one.
    if(phy_reception_ctx->last_rx_basic_control.gain != bc->gain) {
      // Set new Rx gain.
      rx_gain = srslte_rf_set_rx_gain(phy_reception_ctx->rf, (float)bc->gain, phy_reception_ctx->phy_id);
      if(rx_gain < 0) {
        PHY_RX_ERROR("PHY ID: %d - Error setting Rx gain.\n", phy_reception_ctx->phy_id);
        return -1;
      }
      // Updating last configured Rx basic control.
      set_rx_gain(phy_reception_ctx, bc->gain);
      // Print new Rx gain.
      PHY_RX_INFO_TIME("PHY ID: %d - Rx gain set to: %.1f [dB]\n", phy_reception_ctx->phy_id, rx_gain);
    }
  }
  // Set sequence number.
  set_sequence_number(phy_reception_ctx, bc->seq_number);
  // Everything went well.
  return 0;
}

int phy_reception_change_timed_parameters(phy_reception_t* const phy_reception_ctx, basic_ctrl_t* const bc) {
  double actual_rx_freq, rx_channel_center_freq, lo_offset;
  float rx_bandwidth;

  // Change Center frequency only if one of the parameters have changed.
  if(phy_reception_ctx->last_rx_basic_control.ch != bc->ch || phy_reception_ctx->last_rx_basic_control.bw_idx != bc->bw_idx) {
    // Retrieve PHY Rx bandwidth.
    rx_bandwidth = helpers_get_bandwidth_float(bc->bw_idx);
    // Calculate cntral frequency for the channel.
    rx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_reception_ctx->competition_center_freq, phy_reception_ctx->competition_bw, rx_bandwidth, bc->ch);
#if(ENABLE_HW_RF_MONITOR==1)
    // Always apply offset when HW RF Monitor is enabled.
    // Set default offset frequency for reception.
    lo_offset = (double)PHY_RX_LO_OFFSET;
    // Using set_rx_channel_freq cmds which do not need lo_offset parameter_
    actual_rx_freq = srslte_rf_set_rx_channel_freq_cmd(phy_reception_ctx->rf, rx_channel_center_freq, bc->timestamp, RX_TIME_ADVANCE_FOR_COMMANDS, phy_reception_ctx->phy_id); //Prasanthi 11/20/18
    if(actual_rx_freq < 0.0) {
      PHY_RX_ERROR("PHY ID: %d - Error setting Rx frequency with timed command. Returning error: %f\n", phy_reception_ctx->phy_id, actual_rx_freq);
      return -10;
    }
#else // ENABLE_HW_RF_MONITOR
    // Set default offset frequency for reception.
    lo_offset = phy_reception_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_RX_LO_OFFSET;
    // Set new Rx frequency with timed command.
    actual_rx_freq = srslte_rf_set_rx_freq_cmd(phy_reception_ctx->rf, rx_channel_center_freq, lo_offset, bc->timestamp, RX_TIME_ADVANCE_FOR_COMMANDS, phy_reception_ctx->phy_id);
    if(actual_rx_freq < 0.0) {
      PHY_RX_ERROR("PHY ID: %d - Error setting Rx frequency with timed command. Returning error: %f\n", phy_reception_ctx->phy_id, actual_rx_freq);
      return -10;
    }
#endif // ENABLE_HW_RF_MONITOR
    // Check if actual frequency is inside a range of +/- 10 Hz.
    if(actual_rx_freq < (rx_channel_center_freq - 10.0) || actual_rx_freq > (rx_channel_center_freq + 10.0)) {
       PHY_RX_ERROR("PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]................\n", phy_reception_ctx->phy_id, rx_channel_center_freq, actual_rx_freq);
       return -1;
    }
    // Update last configured channel.
    set_channel_number(phy_reception_ctx, bc->ch);
    // Print some debugging information.
    PHY_RX_INFO_TIME("PHY ID: %d - Rx <--- BW[%d]: %1.1f [MHz] - Channel: %d - Set freq to: %.2f [MHz] - Offset: %.2f [MHz]\n", phy_reception_ctx->phy_id, bc->bw_idx, (rx_bandwidth/1000000.0), bc->ch, (actual_rx_freq/1000000.0), (lo_offset/1000000.0));
  }
  // Everything went well.
  return 0;
}

int phy_reception_change_non_timed_parameters(phy_reception_t* const phy_reception_ctx, basic_ctrl_t* const bc) {
  double actual_rx_freq, rx_channel_center_freq, lo_offset;
  float rx_bandwidth;

  // Change Center frequency only if one of the parameters have changed.
  if(phy_reception_ctx->last_rx_basic_control.ch != bc->ch || phy_reception_ctx->last_rx_basic_control.bw_idx != bc->bw_idx) {
    // Retrieve PHY Rx bandwidth.
    rx_bandwidth = helpers_get_bandwidth_float(bc->bw_idx);
    // Calculate cntral frequency for the channel.
    rx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_reception_ctx->competition_center_freq, phy_reception_ctx->competition_bw, rx_bandwidth, bc->ch);
#if(ENABLE_HW_RF_MONITOR==1)
    // Always apply offset when HW RF Monitor is enabled.
    // Set default offset frequency for reception.
    lo_offset = (double)PHY_RX_LO_OFFSET;
    // Set Rx frequency.
    actual_rx_freq = srslte_rf_set_rx_channel_freq(phy_reception_ctx->rf, rx_channel_center_freq, phy_reception_ctx->phy_id); //Prasanthi 11/20/18
    if(actual_rx_freq < 0.0) {
      PHY_RX_ERROR("PHY ID: %d - Error setting Rx frequency. Returning error: %f\n", phy_reception_ctx->phy_id, actual_rx_freq);
      return -10;
    }
#else // ENABLE_HW_RF_MONITOR
    // Set default offset frequency for reception.
    lo_offset = phy_reception_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_RX_LO_OFFSET;
    // Set Rx frequency.
    actual_rx_freq = srslte_rf_set_rx_freq2(phy_reception_ctx->rf, rx_channel_center_freq, lo_offset, phy_reception_ctx->phy_id);
    if(actual_rx_freq < 0.0) {
      PHY_RX_ERROR("PHY ID: %d - Error setting Rx freq. Returning error: %f\n", phy_reception_ctx->phy_id, actual_rx_freq);
      return -10;
    }
#endif // ENABLE_HW_RF_MONITOR
    // Check if actual frequency is inside a range of +/- 10 Hz.
    if(actual_rx_freq < (rx_channel_center_freq - 10.0) || actual_rx_freq > (rx_channel_center_freq + 10.0)) {
       PHY_RX_ERROR("PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]................\n", phy_reception_ctx->phy_id, rx_channel_center_freq, actual_rx_freq);
       return -1;
    }
    // Update last configured channel.
    set_channel_number(phy_reception_ctx, bc->ch);
    // Print some debugging information.
    PHY_RX_INFO_TIME("PHY ID: %d - Rx <--- BW[%d]: %1.1f [MHz] - Channel: %d - Set freq to: %.2f [MHz] - Offset: %.2f [MHz]\n", phy_reception_ctx->phy_id, bc->bw_idx, (rx_bandwidth/1000000.0), bc->ch, (actual_rx_freq/1000000.0), (lo_offset/1000000.0));
  }
  // Everything went well.
  return 0;
}

void set_sequence_number(phy_reception_t* const phy_reception_ctx, uint64_t seq_number) {
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  phy_reception_ctx->last_rx_basic_control.seq_number = seq_number;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
}

uint64_t get_sequence_number(phy_reception_t* const phy_reception_ctx) {
  uint64_t seq_number;
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  seq_number = phy_reception_ctx->last_rx_basic_control.seq_number;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
  return seq_number;
}

void set_channel_number(phy_reception_t* const phy_reception_ctx, uint32_t channel) {
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  phy_reception_ctx->last_rx_basic_control.ch = channel;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
}

uint32_t get_channel_number(phy_reception_t* const phy_reception_ctx) {
  uint32_t channel;
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  channel = phy_reception_ctx->last_rx_basic_control.ch;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
  return channel;
}

void set_bw_index(phy_reception_t* const phy_reception_ctx, uint32_t bw_index) {
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  phy_reception_ctx->last_rx_basic_control.bw_idx = bw_index;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
}

uint32_t get_bw_index(phy_reception_t* const phy_reception_ctx) {
  uint32_t bw_index;
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  bw_index = phy_reception_ctx->last_rx_basic_control.bw_idx;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
  return bw_index;
}

void set_rx_gain(phy_reception_t* const phy_reception_ctx, uint32_t rx_gain) {
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  phy_reception_ctx->last_rx_basic_control.gain = rx_gain;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
}

uint32_t get_rx_gain(phy_reception_t* const phy_reception_ctx) {
  uint32_t rx_gain;
  // Lock a mutex prior to using the basic control object.
  pthread_mutex_lock(&phy_reception_ctx->rx_last_basic_control_mutex);
  rx_gain = phy_reception_ctx->last_rx_basic_control.gain;
  // Unlock mutex upon using the basic control object.
  pthread_mutex_unlock(&phy_reception_ctx->rx_last_basic_control_mutex);
  return rx_gain;
}

void *phy_reception_decoding_work(void *h) {
  phy_reception_t* phy_reception_ctx = (phy_reception_t*)h;
  int decoded_slot_counter = 0, pdsch_num_rxd_bits;
  float rsrp = 0.0, rsrq = 0.0, noise = 0.0, rssi = 0.0, sinr = 0.0;
  double synch_plus_decoding_time = 0.0, decoding_time = 0.0;
  uint32_t nof_prb = 0, sfn = 0, bw_index = 0;
  uint8_t data[10000];
  phy_stat_t phy_rx_stat;
  short_ue_sync_t short_ue_sync;
  cf_t *subframe_buffer = NULL;

#if(CHECK_TIME_BETWEEN_DEMOD_ITER==1)
  struct timespec time_between_demods;
#endif

  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);

  /****************************** PHY Decoding loop - BEGIN ******************************/
  PHY_RX_DEBUG("PHY ID: %d - Entering PHY Decoding thread loop.\n", phy_reception_ctx->phy_id);
  while(phy_reception_timedwait_and_pop_ue_sync_from_queue(phy_reception_ctx, &short_ue_sync) && phy_reception_ctx->run_rx_decoding_thread) {

#if(CHECK_TIME_BETWEEN_DEMOD_ITER==1)
    double difft = helpers_profiling_diff_time(&time_between_demods);
    clock_gettime(CLOCK_REALTIME, &time_between_demods);
#endif

#if(CHECK_TIME_BETWEEN_DEMOD_ITER==1)
    PHY_RX_PRINT("[DEMOD] PHY ID: %d - subframe_counter: %d - time between demod iterations: %f\n", phy_reception_ctx->phy_id, short_ue_sync.subframe_counter, difft);
#endif

    // Reset number of decoded PDSCH bits every loop iteration.
    pdsch_num_rxd_bits = 0;

    // Reset decoded slot counter every time a new MAC frame starts.
    if(short_ue_sync.subframe_counter == 1) {
      decoded_slot_counter = 0;
    }

    //phy_reception_print_ue_sync(&short_ue_sync,"********** decoding thread **********\n");

    // Create an alias to the input buffer containing the synchronized and aligned subframe.
    subframe_buffer = &phy_reception_ctx->ue_sync.input_buffer[short_ue_sync.buffer_number][short_ue_sync.subframe_start_index];

#if(WRITE_SUBFRAME_SEQUENCE_INTO_FILE==1)
    static unsigned int dump_cnt = 0;
    char output_file_name[200];
    sprintf(output_file_name,"received_subframe_%d_cnt_%d.dat",short_ue_sync.subframe_counter,dump_cnt);
    srslte_filesink_t file_sink;
    printf("write into file nof_subframes_to_rx: %d - subframe_counter: %d\n", short_ue_sync.nof_subframes_to_rx, short_ue_sync.subframe_counter);
    if(dump_cnt < 30 && short_ue_sync.subframe_counter == (short_ue_sync.nof_subframes_to_rx)) {
      filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink, subframe_buffer, short_ue_sync.frame_len);
      // Close file.
      filesink_free(&file_sink);
      dump_cnt++;
      PHY_RX_PRINT("PHY ID: %d - File dumped: %d.\n", phy_reception_ctx->phy_id, dump_cnt);
    }
#endif

    // Retrieve BW index.
    bw_index = get_bw_index(phy_reception_ctx);

    // Change MCS. For PHY BW 1.4 MHz we can not set MCS > 28 for the very first subframe as it carries PSS/SSS and does not have enough "room" for FEC bits.
    if(phy_reception_ctx->decode_pdcch == false && bw_index == BW_IDX_OneDotFour && short_ue_sync.mcs > 28 && short_ue_sync.sf_idx == phy_reception_ctx->initial_subframe_index) {
      // Maximum MCS for 1st subframe of 1.4 MHz PHY BW is 28.
      short_ue_sync.mcs = 28;
    }

    // Having a higher number of iterations for higher MCS values is beneficial.
    if(short_ue_sync.subframe_counter == 1) {
      if(short_ue_sync.mcs >= 25 && phy_reception_ctx->max_turbo_decoder_noi_for_high_mcs > phy_reception_ctx->max_turbo_decoder_noi) {
        // Set the maximum number of turbo decoder iterations to a greater value when MCS is greater than or equal to 25.
        srslte_ue_dl_set_max_noi(&phy_reception_ctx->ue_dl, phy_reception_ctx->max_turbo_decoder_noi_for_high_mcs);
      } else {
        // Set the maximum number of turbo decoder iterations to the default one.
        srslte_ue_dl_set_max_noi(&phy_reception_ctx->ue_dl, phy_reception_ctx->max_turbo_decoder_noi);
      }
    }

    pdsch_num_rxd_bits = srslte_ue_dl_decode_scatter(&phy_reception_ctx->ue_dl,
                                                     subframe_buffer,
                                                     data,
                                                     sfn*10+short_ue_sync.sf_idx,
                                                     short_ue_sync.mcs);

    // Calculate time it takes to decode control and data (PDCCH/PCFICH/PDSCH or SCH/PDSCH).
    decoding_time = helpers_profiling_diff_time(&phy_reception_ctx->ue_dl.decoding_start_timestamp);

    //PHY_PROFILLING_AVG6("PHY ID: %d - Average decoding time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 0.5 [ms]: %d - total counter: %d - perc: %f\n", phy_reception_ctx->phy_id, decoding_time, 0.5, 1000);

    if(pdsch_num_rxd_bits < 0) {
      PHY_RX_ERROR("PHY ID: %d - Error decoding UE DL.\n", phy_reception_ctx->phy_id);
    } else if(pdsch_num_rxd_bits > 0) {

      // Only if packet is really received we update the received slot counter.
      decoded_slot_counter++;
      PHY_RX_DEBUG("PHY ID: %d - Decoded slot counter: %d\n", phy_reception_ctx->phy_id, decoded_slot_counter);

      // Retrieve number pf physical resource blocks.
      nof_prb = helpers_get_prb_from_bw_index(bw_index);
      // Calculate statistics.
      rssi = srslte_vec_avg_power_cf(subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb)));
      rsrq = srslte_chest_dl_get_rsrq(&phy_reception_ctx->ue_dl.chest);
      rsrp = srslte_chest_dl_get_rsrp(&phy_reception_ctx->ue_dl.chest);
      noise = srslte_chest_dl_get_noise_estimate(&phy_reception_ctx->ue_dl.chest);

      // Check if the values are valid numbers, if not, set them to 0.
      if(isnan(rssi)) {
        rssi = 0.0;
      }
      if(isnan(rsrq)) {
        rsrq = 0.0;
      }
      if(isnan(noise)) {
        noise = 0.0;
      }
      if(isnan(rsrp)) {
        rsrp = 0.0;
      }
      // Calculate SNR out of RSRP and noise estimation.
      sinr = 10.0*log10f(rsrp/noise);

      // Set PHY Rx Stats with valid values.
      // When data is correctly decoded return SUCCESS status.
      phy_rx_stat.status                                              = PHY_SUCCESS;                                                              // Status tells upper layers that if successfully received data.
      phy_rx_stat.phy_id                                              = phy_reception_ctx->phy_id;
      phy_rx_stat.seq_number                                          = get_sequence_number(phy_reception_ctx);                                   // Sequence number represents the counter of received slots.
      phy_rx_stat.host_timestamp                                      = helpers_convert_host_timestamp(&short_ue_sync.peak_detection_timestamp);  // Retrieve host's time. Host PC time value when (ch,slot) PHY data are demodulated
      phy_rx_stat.ch                                                  = get_channel_number(phy_reception_ctx);                                    // Set the channel number where the data was received at.
      phy_rx_stat.mcs                                                 = phy_reception_ctx->ue_dl.pdsch_cfg.grant.mcs.idx;	                        // MCS index is decoded when the DCI is found and correctly decoded. Modulation Scheme. Range: [0, 28]. check TBS table num_byte_per_1ms_mcs[29] in intf.h to know MCS
      // TODO: centralize error counting!
      phy_rx_stat.num_cb_total                                        = phy_reception_ctx->ue_dl.nof_detected;	                                  // Number of Code Blocks (CB) received in the (ch, slot)
      phy_rx_stat.num_cb_err                                          = phy_reception_ctx->ue_dl.pkt_errors;		                                  // How many CBs get CRC error in the (ch, slot)
      phy_rx_stat.wrong_decoding_counter                              = phy_reception_ctx->ue_dl.wrong_decoding_counter;
      // Assign the values to Rx Stat structure.
      phy_rx_stat.stat.rx_stat.nof_slots_in_frame                     = short_ue_sync.nof_subframes_to_rx;                                        // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
      phy_rx_stat.stat.rx_stat.slot_counter                           = short_ue_sync.subframe_counter;                                           // This field indicates the slot number inside of a MAC frame.
      phy_rx_stat.stat.rx_stat.gain                                   = get_rx_gain(phy_reception_ctx);                                           // Receiver gain (maybe not important now, but let's reserve it). dB*10. for example, value 789 means 78.9dB
      phy_rx_stat.stat.rx_stat.cqi                                    = srslte_cqi_from_snr(sinr);                                                // Channel Quality Indicator. Range: [1, 15]
      phy_rx_stat.stat.rx_stat.rssi                                   = 10.0*log10f(rssi);			                                                      // Received Signal Strength Indicator. Range: [–2^31, (2^31) - 1]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.rsrp                                   = 10.0*log10f(rsrp);				                                                    // Reference Signal Received Power. Range: [-1400, -400]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.rsrq                                   = 10.0*log10f(rsrq);				                                                    // Reference Signal Receive Quality. Range: [-340, -25]. dB*10. For example, value 301 means 30.1 dB.
      phy_rx_stat.stat.rx_stat.sinr                                   = sinr; 			                                                              // Signal to Interference plus Noise Ratio. Range: [–2^31, (2^31) - 1]. dB*10. For example, value 256 means 25.6 dB.
      phy_rx_stat.stat.rx_stat.cfo                                    = short_ue_sync.cfo/1000.0;                                                 // CFO value given in KHz
      phy_rx_stat.stat.rx_stat.peak_value                             = short_ue_sync.peak_value;
      phy_rx_stat.stat.rx_stat.noise                                  = phy_reception_ctx->ue_dl.noise_estimate;
      phy_rx_stat.stat.rx_stat.last_noi                               = srslte_ue_dl_last_noi(&phy_reception_ctx->ue_dl);
      phy_rx_stat.stat.rx_stat.decoding_time                          = decoding_time;
      phy_rx_stat.stat.rx_stat.detection_errors                       = phy_reception_ctx->ue_dl.wrong_decoding_counter;
      phy_rx_stat.stat.rx_stat.decoding_errors                        = phy_reception_ctx->ue_dl.pkt_errors;                                      // If there was a decoding error, then, check the counters below.
      phy_rx_stat.stat.rx_stat.filler_bits_error                      = phy_reception_ctx->ue_dl.pdsch.dl_sch.filler_bits_error;
      phy_rx_stat.stat.rx_stat.nof_cbs_exceeds_softbuffer_size_error  = phy_reception_ctx->ue_dl.pdsch.dl_sch.nof_cbs_exceeds_softbuffer_size_error;
      phy_rx_stat.stat.rx_stat.rate_matching_error                    = phy_reception_ctx->ue_dl.pdsch.dl_sch.rate_matching_error;
      phy_rx_stat.stat.rx_stat.cb_crc_error                           = phy_reception_ctx->ue_dl.pdsch.dl_sch.cb_crc_error;
      phy_rx_stat.stat.rx_stat.tb_crc_error                           = phy_reception_ctx->ue_dl.pdsch.dl_sch.tb_crc_error;
      phy_rx_stat.stat.rx_stat.total_packets_synchronized             = phy_reception_ctx->ue_dl.pkts_total;                                     // Total number of slots synchronized. It contains correct and wrong slots.
      phy_rx_stat.stat.rx_stat.length                                 = pdsch_num_rxd_bits/8;				                                             // How many bytes are after this header. It should be equal to current TB size.
      phy_rx_stat.stat.rx_stat.data                                   = data;
      // Calculate decoding time based on peak detection timestamp or start of each iteration of subframe TRACK state.
      if(short_ue_sync.subframe_counter == 1) {
        synch_plus_decoding_time = helpers_profiling_diff_time(&short_ue_sync.peak_detection_timestamp);
      } else {
        synch_plus_decoding_time = helpers_profiling_diff_time(&short_ue_sync.subframe_track_start);
      }
      phy_rx_stat.stat.rx_stat.synch_plus_decoding_time = synch_plus_decoding_time;

      //PHY_PROFILLING_AVG3("Avg. sync + decoding time: %f - min: %f - max: %f - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n", synch_plus_decoding_time, 0.5, 1000);

      //PHY_PROFILLING_AVG3("Avg. read samples + sync + decoding time: %f - min: %f - max: %f - max counter %d - diff >= 2ms: %d - total counter: %d - perc: %f\n", helpers_profiling_diff_time(&short_ue_sync.start_of_rx_sample), 2.0, 1000);

      // Information on data decoding process.
      PHY_RX_INFO_TIME("[Rx STATS]: PHY ID: %d - Rx slots: %d - Channel: %d - Rx bytes: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - Noise: %1.4f - RSSI: %1.2f [dBm] - SINR: %4.1f [dB] - RSRP: %1.2f - RSRQ: %1.2f [dB] - CQI: %d - MCS: %d - Total: %d - Error: %d - Last NOI: %d - Avg. NOI: %1.2f - Decoding time: %f [ms]\n", phy_reception_ctx->phy_id, decoded_slot_counter, phy_rx_stat.ch, phy_rx_stat.stat.rx_stat.length, short_ue_sync.cfo/1000.0, short_ue_sync.peak_value, phy_reception_ctx->ue_dl.noise_estimate, phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.sinr, phy_rx_stat.stat.rx_stat.rsrp, phy_rx_stat.stat.rx_stat.rsrq, phy_rx_stat.stat.rx_stat.cqi, phy_rx_stat.mcs, phy_reception_ctx->ue_dl.nof_detected, phy_reception_ctx->ue_dl.pkt_errors, srslte_ue_dl_last_noi(&phy_reception_ctx->ue_dl), srslte_ul_dl_average_noi(&phy_reception_ctx->ue_dl), synch_plus_decoding_time);

      // Uncomment this line to measure the number of packets received in one second.
      //helpers_measure_packets_per_second("Rx");

#if(ENABLE_TX_TO_RX_TIME_DIFF==1)
      // Enable add_tx_timestamp to measure the time it takes for a transmitted packet to be received (i.e., detected/buffered/decoded).
      if(phy_reception_ctx->add_tx_timestamp) {
        uint64_t rx_timestamp, tx_timestamp;
        struct timespec rx_timestamp_struct;
        clock_gettime(CLOCK_REALTIME, &rx_timestamp_struct);
        rx_timestamp = helpers_convert_host_timestamp(&rx_timestamp_struct);
        memcpy((void*)&tx_timestamp, (void*)data, sizeof(uint64_t));
        PHY_PROFILLING_AVG3("Diff between TX and Rx time: %0.4f [s] - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n", (double)((rx_timestamp-tx_timestamp)/1000000000.0), 0.001, 1000);
      }
#endif

#if(ENBALE_RX_INFO_PLOT==1)
      if(phy_reception_ctx->plot_rx_info == true) {
        plot_info(&phy_reception_ctx->ue_dl, &phy_reception_ctx->ue_sync);
      }
#endif

#if(WRITE_CORRECT_SUBFRAME_FILE==1)
      static unsigned int dump_cnt = 0;
      char output_file_name[200] = "correct_detection.dat";
      srslte_filesink_t file_sink;
      if(dump_cnt==0) {
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, subframe_buffer, short_ue_sync.frame_len);
        // Close file.
        filesink_free(&file_sink);
        dump_cnt++;
        PHY_RX_PRINT("PHY ID: %d - File dumped: %d.\n",phy_reception_ctx->phy_id,dump_cnt);
      }
#endif

      // Send phy received (Rx) statistics and TB (data) to upper layers.
      phy_reception_send_rx_statistics(phy_reception_ctx->phy_comm_handle, &phy_rx_stat);
    } else {
      // Calculate statistics even if there was decoding error.
      // There was an error if the code reaches this point: (1) wrong CFI or DCI detected or (2) data was incorrectly decoded.
      rssi = srslte_vec_avg_power_cf(subframe_buffer, short_ue_sync.frame_len);
      rsrp = srslte_chest_dl_get_rsrp(&phy_reception_ctx->ue_dl.chest);
      noise = srslte_chest_dl_get_noise_estimate(&phy_reception_ctx->ue_dl.chest);

      // Check if the values are valid numbers, if not, set them to 0.
      if(isnan(rssi)) {
        rssi = 0.0;
      }
      if(isnan(noise)) {
        noise = 0.0;
      }
      if(isnan(rsrp)) {
        rsrp = 0.0;
      }
      // Calculate SNR out of RSRP and noise estimation.
      sinr = 10.0*log10f(rsrp/noise);

      phy_rx_stat.status                                              = PHY_ERROR;
      phy_rx_stat.phy_id                                              = phy_reception_ctx->phy_id;
      phy_rx_stat.seq_number                                          = get_sequence_number(phy_reception_ctx);
      phy_rx_stat.ch                                                  = get_channel_number(phy_reception_ctx);
      phy_rx_stat.mcs                                                 = short_ue_sync.mcs;  // MCS index is decoded from SSS sequence. Modulation Scheme. Range: [0, 28]. check TBS table num_byte_per_1ms_mcs[29] in intf.h to know MCS.
      phy_rx_stat.stat.rx_stat.nof_slots_in_frame                     = short_ue_sync.nof_subframes_to_rx;  // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
      phy_rx_stat.stat.rx_stat.slot_counter                           = short_ue_sync.subframe_counter;        // This field indicates the slot number inside of a MAC frame.
      // TODO: centralize error counting!
      phy_rx_stat.num_cb_total                                        = phy_reception_ctx->ue_dl.nof_detected;
      phy_rx_stat.num_cb_err                                          = phy_reception_ctx->ue_dl.pkt_errors;
      phy_rx_stat.stat.rx_stat.detection_errors                       = phy_reception_ctx->ue_dl.wrong_decoding_counter;
      phy_rx_stat.stat.rx_stat.decoding_errors                        = phy_reception_ctx->ue_dl.pkt_errors;
      phy_rx_stat.stat.rx_stat.filler_bits_error                      = phy_reception_ctx->ue_dl.pdsch.dl_sch.filler_bits_error;
      phy_rx_stat.stat.rx_stat.nof_cbs_exceeds_softbuffer_size_error  = phy_reception_ctx->ue_dl.pdsch.dl_sch.nof_cbs_exceeds_softbuffer_size_error;
      phy_rx_stat.stat.rx_stat.rate_matching_error                    = phy_reception_ctx->ue_dl.pdsch.dl_sch.rate_matching_error;
      phy_rx_stat.stat.rx_stat.cb_crc_error                           = phy_reception_ctx->ue_dl.pdsch.dl_sch.cb_crc_error;
      phy_rx_stat.stat.rx_stat.tb_crc_error                           = phy_reception_ctx->ue_dl.pdsch.dl_sch.tb_crc_error;
      phy_rx_stat.stat.rx_stat.cqi                                    = srslte_cqi_from_snr(sinr); // Channel Quality Indicator. Range: [1, 15]
      phy_rx_stat.stat.rx_stat.rssi                                   = 10.0*log10f(rssi);	      // Received Signal Strength Indicator. Range: [–2^31, (2^31) - 1]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.rsrp                                   = 10.0*log10f(rsrp);				// Reference Signal Received Power. Range: [-1400, -400]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.sinr                                   = sinr; 			              // Signal to Interference plus Noise Ratio. Range: [–2^31, (2^31) - 1]. dB*10. For example, value 256 means 25.6 dB.
      phy_rx_stat.stat.rx_stat.cfo                                    = short_ue_sync.cfo/1000.0;  // CFO value given in KHz
      phy_rx_stat.stat.rx_stat.peak_value                             = short_ue_sync.peak_value;
      phy_rx_stat.stat.rx_stat.noise                                  = phy_reception_ctx->ue_dl.noise_estimate;
      phy_rx_stat.stat.rx_stat.decoded_cfi                            = phy_reception_ctx->ue_dl.decoded_cfi;
      phy_rx_stat.stat.rx_stat.found_dci                              = phy_reception_ctx->ue_dl.found_dci;
      phy_rx_stat.stat.rx_stat.last_noi                               = srslte_ue_dl_last_noi(&phy_reception_ctx->ue_dl);
      phy_rx_stat.stat.rx_stat.total_packets_synchronized             = phy_reception_ctx->ue_dl.pkts_total;
      // Send phy received (Rx) statistics and TB (data) to upper layers.
      phy_reception_send_rx_statistics(phy_reception_ctx->phy_comm_handle, &phy_rx_stat);
      // Print some wrong decoding information, which is useful for debugging.
      PHY_RX_INFO_TIME("[Rx STATS]: PHY ID: %d - Detection errors: %d - Channel: %d - # slots: %d - slot number: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - RSSI: %3.2f [dBm] - Decoded CFI: %d - Found DCI: %d - Last NOI: %d - Avg. NOI: %1.2f - Noise: %1.4f - Decoding errors: %d\n",phy_reception_ctx->phy_id, phy_reception_ctx->ue_dl.wrong_decoding_counter,get_channel_number(phy_reception_ctx),phy_rx_stat.stat.rx_stat.nof_slots_in_frame,phy_rx_stat.stat.rx_stat.slot_counter, short_ue_sync.cfo/1000.0,short_ue_sync.peak_value,phy_rx_stat.stat.rx_stat.rssi,phy_reception_ctx->ue_dl.decoded_cfi,phy_reception_ctx->ue_dl.found_dci,srslte_ue_dl_last_noi(&phy_reception_ctx->ue_dl), srslte_ul_dl_average_noi(&phy_reception_ctx->ue_dl),phy_reception_ctx->ue_dl.noise_estimate,phy_reception_ctx->ue_dl.pkt_errors);

#if(WRITE_DECT_DECD_ERROR_SUBFRAME_FILE==1)
      static unsigned int dump_cnt = 0;
      char output_file_name[200] = "detection_decoding_error.dat";
      srslte_filesink_t file_sink;
      if(dump_cnt==0) {
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, subframe_buffer, short_ue_sync.frame_len);
        // Close file.
        filesink_free(&file_sink);
        dump_cnt++;
        PHY_RX_PRINT("PHY ID: %d - File dumped: %d.\n",phy_reception_ctx->phy_id,dump_cnt);
      }
#endif

    }
  }
  /****************************** PHY Decoding loop - END ******************************/
  PHY_RX_PRINT("PHY ID: %d - Leaving PHY Decoding thread.\n",phy_reception_ctx->phy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void phy_reception_send_rx_statistics(LayerCommunicator_handle handle, phy_stat_t *phy_rx_stat) {
  // Set values to the Rx Stats Structure.
  uint64_t timestamp = helpers_get_host_time_now();
  // Set frame number.
  phy_rx_stat->frame = 0;
  // Set time slot number.
  phy_rx_stat->slot = 0;
  // Host PC timestamp value when reception.
  phy_rx_stat->host_timestamp = timestamp;
  // Set some default values.
  if(phy_rx_stat->status == PHY_SUCCESS) {
     // Host PC time value for success.
    phy_rx_stat->stat.rx_stat.decoded_cfi = 1;
    phy_rx_stat->stat.rx_stat.found_dci   = 1;
  } else if(phy_rx_stat->status == PHY_TIMEOUT) {
    phy_rx_stat->mcs                      = 100;
    phy_rx_stat->num_cb_total             = 0;
    phy_rx_stat->num_cb_err               = 0;
    phy_rx_stat->stat.rx_stat.cqi         = 100;
    phy_rx_stat->stat.rx_stat.rssi        = 0.0;
    phy_rx_stat->stat.rx_stat.rsrp        = 0.0;
    phy_rx_stat->stat.rx_stat.rsrq        = 0.0;
    phy_rx_stat->stat.rx_stat.sinr        = 0.0;
    phy_rx_stat->stat.rx_stat.length      = 0;
    phy_rx_stat->stat.rx_stat.data        = NULL;
  } else if(phy_rx_stat->status == PHY_ERROR) {
    phy_rx_stat->stat.rx_stat.gain        = 0;
    phy_rx_stat->stat.rx_stat.rsrq        = 0.0;
    phy_rx_stat->stat.rx_stat.length      = 0;
    phy_rx_stat->stat.rx_stat.data        = NULL;
  }
  // Send Rx stats. There is a mutex on this function which prevents TX from sending statistics to PHY at the same time.
  if(communicator_send_phy_stat_message(handle, RX_STAT, phy_rx_stat, NULL) < 0) {
    PHY_RX_ERROR("Rx statistics message not sent due to an error with the communicator module.\n",0);
  }
}

int phy_reception_ue_init(phy_reception_t* const phy_reception_ctx) {
  // Initialize parameters for UE Cell.
  PHY_RX_PRINT("PHY ID: %d - Initializing UE Sync.\n", phy_reception_ctx->phy_id);
  if(srslte_ue_sync_init_generic(&phy_reception_ctx->ue_sync, phy_reception_ctx->cell_ue, srslte_rf_recv_with_time_wrapper, (void*)phy_reception_ctx->rf, phy_reception_ctx->initial_subframe_index, phy_reception_ctx->enable_cfo_correction, phy_reception_ctx->decode_pdcch, phy_reception_ctx->node_id, phy_reception_ctx->phy_id, phy_reception_ctx->phy_filtering, phy_reception_ctx->use_scatter_sync_seq, phy_reception_ctx->pss_len, phy_reception_ctx->enable_second_stage_pss_detection)) {
    PHY_RX_ERROR("PHY ID: %d - Error initiating ue_sync\n", phy_reception_ctx->phy_id);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - UE Sync initialization successful.\n", phy_reception_ctx->phy_id);
  // Set PSS detection threshold.
  srslte_ue_set_pss_synch_find_threshold(&phy_reception_ctx->ue_sync, phy_reception_ctx->threshold);
  PHY_RX_PRINT("PHY ID: %d - PSS detection threshold set to: %1.2f.\n", phy_reception_ctx->phy_id, phy_reception_ctx->threshold);
  // Set Min/Max PSR averaging.
  srslte_ue_sync_set_avg_psr_scatter(&phy_reception_ctx->ue_sync, phy_reception_ctx->enable_avg_psr);
  PHY_RX_PRINT("PHY ID: %d - Min/Max PSR Avg: %s.\n", phy_reception_ctx->phy_id, phy_reception_ctx->enable_avg_psr?"ENABLED":"DISABLED");
  // If two-stages PSS detection is enabled, then set the two thresholds.
  PHY_RX_PRINT("PHY ID: %d - Two-stage PSS detection: %s.\n", phy_reception_ctx->phy_id, phy_reception_ctx->enable_second_stage_pss_detection?"ENABLED":"DISABLED");
  if(phy_reception_ctx->enable_second_stage_pss_detection) {
    srslte_ue_sync_set_pss_synch_find_1st_stage_threshold_scatter(&phy_reception_ctx->ue_sync, phy_reception_ctx->pss_first_stage_threshold);
    PHY_RX_PRINT("PHY ID: %d - 1st stage threshold: %1.2f.\n", phy_reception_ctx->phy_id, phy_reception_ctx->pss_first_stage_threshold);
    srslte_ue_sync_set_pss_synch_find_2nd_stage_threshold_scatter(&phy_reception_ctx->ue_sync, phy_reception_ctx->pss_second_stage_threshold);
    PHY_RX_PRINT("PHY ID: %d - 2nd stage threshold: %1.2f.\n", phy_reception_ctx->phy_id, phy_reception_ctx->pss_second_stage_threshold);
  }
  // Initialize UE DL.
  if(srslte_ue_dl_init_generic(&phy_reception_ctx->ue_dl, phy_reception_ctx->cell_ue, phy_reception_ctx->phy_id, !phy_reception_ctx->phy_filtering)) {
    PHY_RX_ERROR("PHY ID: %d - Error initiating UE downlink processing module\n", phy_reception_ctx->phy_id);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - UE DL initialization successful.\n", phy_reception_ctx->phy_id);
  // Configure downlink receiver for the SI-RNTI since will be the only one we'll use.
  // This is the User RNTI.
  srslte_ue_dl_set_rnti(&phy_reception_ctx->ue_dl, phy_reception_ctx->rnti);
  // Set the expected CFI.
  srslte_ue_dl_set_expected_cfi(&phy_reception_ctx->ue_dl, DEFAULT_CFI);
  // Enable estimation of CFO based on CSR signals.
  srslte_ue_dl_set_cfo_csr(&phy_reception_ctx->ue_dl, false);
  // Set the maximum number of turbo decoder iterations.
  srslte_ue_dl_set_max_noi(&phy_reception_ctx->ue_dl, phy_reception_ctx->max_turbo_decoder_noi);
  // Enable or disable decoding of PDCCH/PCFICH control channels. If true, it decodes PDCCH/PCFICH otherwise, decodes SCH control.
  srslte_ue_dl_set_decode_pdcch(&phy_reception_ctx->ue_dl, phy_reception_ctx->decode_pdcch);
  // Setting EOB PSS sequence length.
  srslte_ue_dl_set_pss_length(&phy_reception_ctx->ue_dl, phy_reception_ctx->pss_len);
  // Start AGC.
  if(phy_reception_ctx->initial_rx_gain < 0.0) {
    srslte_ue_sync_start_agc(&phy_reception_ctx->ue_sync, srslte_rf_set_rx_gain_th_wrapper_, phy_reception_ctx->initial_agc_gain);
  }
  // Set initial CFO for ue_sync to 0.
  srslte_ue_sync_set_cfo(&phy_reception_ctx->ue_sync, 0.0);
  // Everything went well.
  return 0;
}

int phy_reception_stop_rx_stream_and_flush_buffer(phy_reception_t* const phy_reception_ctx) {
  int error;
  if((error = srslte_rf_stop_rx_stream(phy_reception_ctx->rf, phy_reception_ctx->phy_id)) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping Rx stream: %d....\n", phy_reception_ctx->phy_id, error);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - Rx stream stopped.\n", phy_reception_ctx->phy_id);
  srslte_rf_flush_buffer(phy_reception_ctx->rf, phy_reception_ctx->phy_id);
  PHY_RX_PRINT("PHY ID: %d - Rx buffer flushed.\n", phy_reception_ctx->phy_id);
  // Everything went well.
  return 0;
}

int phy_reception_initialize_rx_stream(phy_reception_t* const phy_reception_ctx) {
  int error;
  // Stop Rx Stream and Flush Reception Buffer.
  if(phy_reception_stop_rx_stream_and_flush_buffer(phy_reception_ctx) < 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping and flushing Rx stream.\n", phy_reception_ctx->phy_id);
    return -1;
  }
  // Start Rx Stream.
  if((error = srslte_rf_start_rx_stream(phy_reception_ctx->rf, phy_reception_ctx->phy_id)) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Error starting Rx stream: %d....\n", phy_reception_ctx->phy_id, error);
    return -1;
  }
  // Everything went well.
  return 0;
}

// Free all related UE Downlink structures.
void phy_reception_ue_free(phy_reception_t* const phy_reception_ctx) {
  // Terminate and join AGC thread only if AGC is enabled.
  if(phy_reception_ctx->initial_rx_gain < 0.0) {
    if(srslte_rf_finish_gain_thread(phy_reception_ctx->rf) < 0) {
      PHY_RX_ERROR("PHY ID: %d - Error joining gain thread........\n",phy_reception_ctx->phy_id);
    } else {
      PHY_RX_PRINT("PHY ID: %d - Gain thread correctly joined.\n",phy_reception_ctx->phy_id);
    }
  }
  // Free all UE related structures.
  srslte_ue_dl_free(&phy_reception_ctx->ue_dl);
  PHY_RX_INFO("PHY ID: %d - srslte_ue_dl_free done!\n",phy_reception_ctx->phy_id);
  srslte_ue_sync_free_except_reentry(&phy_reception_ctx->ue_sync);
  PHY_RX_INFO("PHY ID: %d - srslte_ue_sync_free_except_reentry done!\n",phy_reception_ctx->phy_id);
}

int phy_reception_set_rx_sample_rate(phy_reception_t* const phy_reception_ctx) {
  int srate = -1;
  float srate_rf = 0;
  if(phy_reception_ctx->use_std_carrier_sep) {
    srate = srslte_sampling_freq_hz(phy_reception_ctx->nof_prb);
  } else {
    srate = helpers_non_std_sampling_freq_hz(phy_reception_ctx->nof_prb);
    PHY_RX_PRINT("PHY ID: %d - Setting a non-standard sampling rate: %1.2f [MHz]\n",phy_reception_ctx->phy_id,srate/1000000.0);
  }
  if(srate != -1) {
    srate_rf = srslte_rf_set_rx_srate(phy_reception_ctx->rf, (double)srate, phy_reception_ctx->phy_id);
    if(srate_rf != srate) {
      PHY_RX_ERROR("PHY ID: %d - Could not set Rx sampling rate.\n",phy_reception_ctx->phy_id);
      return -1;
    }
    PHY_RX_PRINT("PHY ID: %d - Set Rx sampling rate to: %.2f [MHz]\n", phy_reception_ctx->phy_id, srate_rf/1000000.0);
  } else {
    PHY_RX_ERROR("PHY ID: %d - Invalid number of PRB (Rx): %d\n", phy_reception_ctx->phy_id, phy_reception_ctx->nof_prb);
    return -1;
  }
  // Everything went well.
  return 0;
}

int phy_reception_set_initial_rx_freq_and_gain(phy_reception_t* const phy_reception_ctx) {
  double current_rx_freq, lo_offset, rx_channel_center_freq;
  // Set receiver gain.
  if(phy_reception_ctx->initial_rx_gain >= 0.0) {
    double gain = srslte_rf_set_rx_gain(phy_reception_ctx->rf, phy_reception_ctx->initial_rx_gain, phy_reception_ctx->phy_id);
    PHY_RX_PRINT("PHY ID: %d - Set Rx gain to: %.1f [dB] (AGC NOT ENABLED).\n",phy_reception_ctx->phy_id,gain);
  } else {
    PHY_RX_PRINT("PHY ID: %d - Starting AGC thread...\n",phy_reception_ctx->phy_id);
    if(srslte_rf_start_gain_thread(phy_reception_ctx->rf, false, phy_reception_ctx->phy_id)) {
      PHY_RX_ERROR("PHY ID: %d - Error starting gain thread.\n",phy_reception_ctx->phy_id);
      return -1;
    }
    srslte_rf_set_rx_gain(phy_reception_ctx->rf, phy_reception_ctx->initial_agc_gain, phy_reception_ctx->phy_id);
  }
  // Calculate central frequency for the channel.
  rx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_reception_ctx->competition_center_freq, phy_reception_ctx->competition_bw, phy_reception_ctx->default_rx_bandwidth, (phy_reception_ctx->default_rx_channel + phy_reception_ctx->phy_id));
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  lo_offset = (double)PHY_RX_LO_OFFSET;
  // Set default central frequency for reception.
  double actual_comp_freq = srslte_rf_set_rx_freq2(phy_reception_ctx->rf, phy_reception_ctx->competition_center_freq, lo_offset, phy_reception_ctx->phy_id);   // Prasanthi 11/20/2018
  // Check if actual competition frequency is inside a range of +/- 10 Hz.
  if(actual_comp_freq < (phy_reception_ctx->competition_center_freq - 10.0) || actual_comp_freq > (phy_reception_ctx->competition_center_freq + 10.0)) {
     PHY_RX_ERROR("[Initialization] PHY ID: %d - Requested competition freq.: %1.2f [MHz] - Actual competition freq.: %1.2f [MHz] - Competition BW: %1.2f [MHz] - PHY BW: %1.2f [MHz] - Channel: %d\n", phy_reception_ctx->phy_id, phy_reception_ctx->competition_center_freq/1000000.0, actual_comp_freq/1000000.0, phy_reception_ctx->competition_bw/1000000.0, phy_reception_ctx->default_rx_bandwidth/1000000.0, (phy_reception_ctx->default_rx_channel + phy_reception_ctx->phy_id));
     return -1;
  }
  // Use the DDC to change frequency.
  current_rx_freq = srslte_rf_set_rx_channel_freq(phy_reception_ctx->rf, rx_channel_center_freq, phy_reception_ctx->phy_id);  // Prasanthi 11/20/2018
#else
  // Set default central frequency for reception.
  lo_offset = phy_reception_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_RX_LO_OFFSET;
  current_rx_freq = srslte_rf_set_rx_freq2(phy_reception_ctx->rf, rx_channel_center_freq, lo_offset, phy_reception_ctx->phy_id);   // Prasanthi 11/20/2018
#endif
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(current_rx_freq < (rx_channel_center_freq - 10.0) || current_rx_freq > (rx_channel_center_freq + 10.0)) {
     PHY_RX_ERROR("[Initialization] PHY ID: %d - Requested channel freq.: %1.2f [MHz] - Actual channel freq.: %1.2f [MHz] - Center Frequency: %1.2f [MHz] - Competition BW: %1.2f [MHz] - PHY BW: %1.2f [MHz] - Channel: %d\n", phy_reception_ctx->phy_id, rx_channel_center_freq/1000000.0, current_rx_freq/1000000.0, phy_reception_ctx->competition_center_freq/1000000.0, phy_reception_ctx->competition_bw/1000000.0, phy_reception_ctx->default_rx_bandwidth/1000000.0, (phy_reception_ctx->default_rx_channel + phy_reception_ctx->phy_id));
     return -1;
  }
  srslte_rf_rx_wait_lo_locked(phy_reception_ctx->rf, phy_reception_ctx->phy_id);
  PHY_RX_PRINT("PHY ID: %d - Set initial Rx freq to: %.2f [MHz] with offset of: %.2f [MHz]\n", phy_reception_ctx->phy_id, (current_rx_freq/1000000.0),(lo_offset/1000000.0));
  // Everything went well.
  return 0;
}

// This is the function called when the synchronization thread is called/started.
void *phy_reception_sync_work(void *h) {
  phy_reception_t* phy_reception_ctx = (phy_reception_t*)h;
  short_ue_sync_t short_ue_sync;
  int ret;

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
  struct timespec time_between_synchs;
#endif

  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);

  // Set some constant parameters of short_ue_sync structure.
  short_ue_sync.frame_len = phy_reception_ctx->ue_sync.frame_len;

  // Initialize subframe counter to 0.
  phy_reception_ctx->ue_sync.subframe_counter = 0;

  PHY_RX_DEBUG("PHY ID: %d - Entering PHY synchronization thread loop.\n", phy_reception_ctx->phy_id);
  while(phy_reception_ctx->run_rx_synchronization_thread) {

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
    double diff = helpers_profiling_diff_time(&time_between_synchs);
    clock_gettime(CLOCK_REALTIME, &time_between_synchs);
    PHY_RX_PRINT("[SYNC] PHY ID: %d - time between synch iterations: %f\n",phy_reception_ctx->phy_id,diff);
#endif

    // Timestamp the start of the reception procedure. We calculate the total time it takes to read, sync and decode the subframe on the decoding thread.
    //clock_gettime(CLOCK_REALTIME, &short_ue_sync.start_of_rx_sample);

#ifndef ENABLE_CH_EMULATOR
    // Create watchdog timer for synchronization thread. Always wait for some seconds.
    if(timer_set(&phy_reception_ctx->synch_thread_timer_id, 2) < 0) {
      PHY_RX_ERROR("PHY ID: %d - Not possible to set the watchdog timer for sync thread.\n", phy_reception_ctx->phy_id);
    }
#endif

    // synchronize and align subframes.
    ret = srslte_ue_sync_get_subframe_buffer(&phy_reception_ctx->ue_sync, phy_reception_ctx->phy_id);

    // srslte_ue_sync_get_subframe_buffer() returns 1 if it successfully synchronizes to a slot (also known as subframe).
    if(ret == 1) {
      //PHY_PROFILLING_AVG3("Average synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(ue_sync->sfind.peak_detection_timestamp), 1.0, 1000);

      //struct timespec start_push_queue;
      //clock_gettime(CLOCK_REALTIME, &start_push_queue);

      // We increment the subframe counter the first time if we have just found a peak and the subframe data was correctly decoded.
      if(phy_reception_ctx->ue_sync.last_state == SF_FIND && phy_reception_ctx->ue_sync.state == SF_TRACK && phy_reception_ctx->ue_sync.subframe_counter == 0) {
        phy_reception_ctx->ue_sync.subframe_counter = 1;
        PHY_RX_DEBUG("PHY ID: %d - First increment of subframe counter: %d.\n",phy_reception_ctx->phy_id,phy_reception_ctx->ue_sync.subframe_counter);
      } else if(phy_reception_ctx->ue_sync.last_state == SF_TRACK && phy_reception_ctx->ue_sync.state == SF_TRACK && phy_reception_ctx->ue_sync.subframe_counter > 0) {
        // Increment the subframe counter in order to receive the correct number of subframes.
        phy_reception_ctx->ue_sync.subframe_counter++;
      } else {
        PHY_RX_ERROR("PHY ID: %d - There was something wrong with the subframe_counter.\n", phy_reception_ctx->phy_id);
        continue;
      }

      // Update the short ue sync structure with the current subframe counter number and other parameters.
      short_ue_sync.buffer_number             = phy_reception_ctx->ue_sync.previous_subframe_buffer_counter_value;
      short_ue_sync.subframe_start_index      = phy_reception_ctx->ue_sync.subframe_start_index;
      short_ue_sync.sf_idx                    = phy_reception_ctx->ue_sync.sf_idx;
      short_ue_sync.peak_value                = phy_reception_ctx->ue_sync.sfind.peak_value;
      short_ue_sync.peak_detection_timestamp  = phy_reception_ctx->ue_sync.sfind.peak_detection_timestamp;
      short_ue_sync.cfo                       = srslte_ue_sync_get_carrier_freq_offset(&phy_reception_ctx->ue_sync.sfind); // CFO value given in Hz
      short_ue_sync.nof_subframes_to_rx       = phy_reception_ctx->ue_sync.sfind.nof_subframes_to_rx;
      short_ue_sync.subframe_counter          = phy_reception_ctx->ue_sync.subframe_counter;
      short_ue_sync.subframe_track_start      = phy_reception_ctx->ue_sync.subframe_track_start;
      short_ue_sync.mcs                       = phy_reception_ctx->ue_sync.sfind.mcs;

#if(WRITE_RX_SUBFRAME_INTO_FILE==1)
      static unsigned int dump_cnt = 0;
      char output_file_name[200] = "f_ofdm_rx_side_assessment_5MHz.dat";
      srslte_filesink_t file_sink;
      if(dump_cnt==0) {
         filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
         // Write samples into file.
         filesink_write(&file_sink, &phy_reception_ctx->ue_sync.input_buffer[short_ue_sync.buffer_number][short_ue_sync.subframe_start_index], SRSLTE_SF_LEN(srslte_symbol_sz(helpers_get_prb_from_bw_index(get_bw_index(phy_reception_ctx)))));
         // Close file.
         filesink_free(&file_sink);
      }
      dump_cnt++;
      PHY_RX_PRINT("PHY ID: %d - File dumped: %d.\n",phy_reception_ctx->phy_id,dump_cnt);
#endif

      // Push ue_sync structure to queue (FIFO).
      phy_reception_push_ue_sync_to_queue(phy_reception_ctx, &short_ue_sync);

      // After pushing the ue synch message into the queue, reset ue_synch object if this was the last subframe of a MAC frame.
      if(phy_reception_ctx->ue_sync.subframe_counter >= phy_reception_ctx->ue_sync.sfind.nof_subframes_to_rx) {
        // Light weight way to reset ue_dl for new reception.
        if(srslte_ue_sync_init_reentry_loop(&phy_reception_ctx->ue_sync)) {
          PHY_RX_ERROR("PHY ID: %d - Error re-initiating ue_sync\n",phy_reception_ctx->phy_id);
          continue;
        }
        // Reset subframe counter so that it can be used again.
        phy_reception_ctx->ue_sync.subframe_counter = 0;
      }

      //double diff_queue = helpers_profiling_diff_time(start_push_queue);
      //if(diff_queue > 0.05)
      //  PHY_RX_PRINT("UE Synch enQUEUE time: %f\n",diff_queue);

      //phy_reception_print_ue_sync(&short_ue_sync,"********** synchronization thread **********\n");
    } else if(ret == 0) {
      // No slot synchronized and aligned. We don't do nothing for now.
    } else if(ret < 0) {
      PHY_RX_ERROR("PHY ID: %d - Error calling srslte_ue_sync_get_subframe_buffer(): %d. Leaving SYNCH thread.\n",phy_reception_ctx->phy_id, ret);
      // Leave synchronization thread.
      break;
    }

#ifndef ENABLE_CH_EMULATOR
    // Disarm watchdog timer for synchronization thread.
    if(timer_disarm(&phy_reception_ctx->synch_thread_timer_id) < 0) {
      PHY_RX_ERROR("PHY ID: %d - Not possible to disarm the watchdog timer for sync thread.\n", phy_reception_ctx->phy_id);
    }
#endif

  }

  PHY_RX_PRINT("PHY ID: %d - Leaving PHY synchronization thread.\n", phy_reception_ctx->phy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

int srslte_rf_recv_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel) {
  return srslte_rf_recv(h, data, nsamples, 1, channel);
}

int srslte_rf_recv_with_time_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel) {
  return srslte_rf_recv_with_time(h, data, nsamples, 1, &(t->full_secs), &(t->frac_secs), channel);
}

int srslte_file_recv_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t, size_t channel) {
  return srslte_filesource_read(h, data, nsamples);
}

double srslte_rf_set_rx_gain_th_wrapper_(void *h, double f) {
  return srslte_rf_set_rx_gain_th((srslte_rf_t*) h, f);
}

// Functions to transfer ue sync structure from sync thread to reception thread.
void phy_reception_push_ue_sync_to_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t *short_ue_sync) {
  // Lock mutex so that we can push ue sync to queue.
  pthread_mutex_lock(&phy_reception_ctx->rx_sync_mutex);
  // Push ue sync into queue.
  sync_cb_push_back(phy_reception_ctx->rx_synch_handle, short_ue_sync);
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&phy_reception_ctx->rx_sync_mutex);
  // Notify other thread that ue sync structure was pushed into queue.
  pthread_cond_signal(&phy_reception_ctx->rx_sync_cv);
}

void phy_reception_pop_ue_sync_from_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t *short_ue_sync) {
  // Lock mutex so that we can pop ue sync structure from queue.
  pthread_mutex_lock(&phy_reception_ctx->rx_sync_mutex);
  // Retrieve sync element from circular buffer.
  sync_cb_front(phy_reception_ctx->rx_synch_handle, short_ue_sync);
  // Remove sync element from  circular buffer.
  sync_cb_pop_front(phy_reception_ctx->rx_synch_handle);
  // Unlock mutex.
  pthread_mutex_unlock(&phy_reception_ctx->rx_sync_mutex);
}

bool phy_reception_wait_queue_not_empty(phy_reception_t* const phy_reception_ctx) {
  bool ret = true;
  // Lock mutex so that we can wait for ue sync strucure.
  pthread_mutex_lock(&phy_reception_ctx->rx_sync_mutex);
  // Wait for conditional variable only if container is empty.
  if(sync_cb_empty(phy_reception_ctx->rx_synch_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&phy_reception_ctx->rx_sync_cv, &phy_reception_ctx->rx_sync_mutex);
    // If one of the flags below are false, then leave the Rx threads.
    if(!phy_reception_ctx->run_rx_decoding_thread || !phy_reception_ctx->run_rx_synchronization_thread) {
      ret = false;
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&phy_reception_ctx->rx_sync_mutex);
  return ret;
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool phy_reception_timedwait_and_pop_ue_sync_from_queue(phy_reception_t* const phy_reception_ctx, short_ue_sync_t *short_ue_sync) {
  bool ret = true, is_cb_empty = true;
  struct timespec timeout;
  // Lock mutex.
  pthread_mutex_lock(&phy_reception_ctx->rx_sync_mutex);
  // Wait for conditional variable only if container is empty.
  if(sync_cb_empty(phy_reception_ctx->rx_synch_handle)) {
    do {
      // Timeout in 0.1 ms.
      helpers_get_timeout(10000, &timeout);
      // Timed wait for conditional variable to be true.
      pthread_cond_timedwait(&phy_reception_ctx->rx_sync_cv, &phy_reception_ctx->rx_sync_mutex, &timeout);
      // Check status of the circular buffer again.
      is_cb_empty = sync_cb_empty(phy_reception_ctx->rx_synch_handle);
      // If one of the flags below are false, then leave the Rx threads.
      if(!phy_reception_ctx->run_rx_decoding_thread || !phy_reception_ctx->run_rx_synchronization_thread) {
        ret = false;
      }
    } while(is_cb_empty && ret);
  }

  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve mapped element from container.
    sync_cb_front(phy_reception_ctx->rx_synch_handle, short_ue_sync);
    sync_cb_pop_front(phy_reception_ctx->rx_synch_handle);
  }
  // Unlock mutex.
  pthread_mutex_unlock(&phy_reception_ctx->rx_sync_mutex);
  return ret;
}

void phy_reception_print_ue_sync(short_ue_sync_t *short_ue_sync, char* str) {
  printf("%s",str);
  printf("********** UE Sync Structure **********\n");
  printf("Buffer number: %d\n", short_ue_sync->buffer_number);
  printf("Subframe start index: %d\n", short_ue_sync->subframe_start_index);
  printf("Subframe index: %d\n", short_ue_sync->sf_idx);
  printf("Peak value: %0.2f\n", short_ue_sync->peak_value);
  printf("Timestamp: %" PRIu64 "\n" , short_ue_sync->peak_detection_timestamp);
  printf("CFO: %0.3f\n", short_ue_sync->cfo/1000.0);
  printf("Subframe length: %d\n", short_ue_sync->frame_len);
  printf("# of slots to Rx: %d\n", short_ue_sync->nof_subframes_to_rx);
  printf("MCS: %d\n", short_ue_sync->mcs);
}

void phy_reception_print_decoding_error_counters(phy_stat_t* const phy_rx_stat) {
  printf("********************* Detection/Decoding error stats *********************\n");
  printf("# slot in frame: %d\n",phy_rx_stat->stat.rx_stat.nof_slots_in_frame);
  printf("slot counter: %d\n",phy_rx_stat->stat.rx_stat.slot_counter);
  printf("RSSI: %1.4f\n",phy_rx_stat->stat.rx_stat.rssi);
  printf("Peak value: %1.4f\n",phy_rx_stat->stat.rx_stat.peak_value);
  printf("Noise: %1.5f\n",phy_rx_stat->stat.rx_stat.noise);
  printf("NOI: %d\n",phy_rx_stat->stat.rx_stat.last_noi);
  printf("Detection errors: %d\n",phy_rx_stat->stat.rx_stat.detection_errors);
  printf("\tCFI: %d\n",phy_rx_stat->stat.rx_stat.decoded_cfi);
  printf("\tDCI found: %d\n",phy_rx_stat->stat.rx_stat.found_dci);
  printf("Decoding errors: %d\n",phy_rx_stat->stat.rx_stat.decoding_errors);
  printf("\tFiller bit error: %d\n",phy_rx_stat->stat.rx_stat.filler_bits_error);
  printf("\tSoftbuffer error: %d\n",phy_rx_stat->stat.rx_stat.nof_cbs_exceeds_softbuffer_size_error);
  printf("\tRate matching error: %d\n",phy_rx_stat->stat.rx_stat.rate_matching_error);
  printf("\tCB CRC error: %d\n",phy_rx_stat->stat.rx_stat.cb_crc_error);
  printf("\tTB CRC error: %d\n",phy_rx_stat->stat.rx_stat.tb_crc_error);
  printf("**************************************************************************\n");
}

void phy_reception_update_environment(uint32_t phy_id, environment_t* const env_update) {
  bool competition_center_freq_updated = false, competition_bw_updated = false;
  // Change center frequency if different from previous one.
  if(env_update->competition_center_freq != phy_rx_threads[phy_id]->competition_center_freq) {
    if(env_update->competition_center_freq > 0) {
      double last_value = phy_rx_threads[phy_id]->competition_center_freq;
      phy_rx_threads[phy_id]->competition_center_freq = env_update->competition_center_freq;
      competition_center_freq_updated = true;
      PHY_RX_PRINT("PHY ID: %d - Competition Center Frequency changed from: %1.2f [MHz] to: %1.2f [MHz]\n", phy_id, last_value/1000000.0, env_update->competition_center_freq/1000000.0);
    } else {
      PHY_RX_ERROR("The json file contained the following center frequency: %d\n", env_update->competition_center_freq);
    }
  }
  // Change scenario BW if different from previous one.
  if(env_update->competition_bw != phy_rx_threads[phy_id]->competition_bw) {
    if(env_update->competition_bw > 0) {
      double last_value = phy_rx_threads[phy_id]->competition_bw;
      phy_rx_threads[phy_id]->competition_bw = env_update->competition_bw;
      competition_bw_updated = true;
      PHY_RX_PRINT("PHY ID: %d - Competition BW changed from: %1.2f [MHz] to: %1.2f [MHz]\n", phy_id, last_value/1000000.0, env_update->competition_bw/1000000.0);
    } else {
      PHY_RX_ERROR("The json file contained the following competition bw: %d\n", env_update->competition_bw);
    }
  }
  // Apply modifications if there were any.
  if(competition_center_freq_updated || competition_bw_updated) {
    phy_reception_update_frequency(phy_rx_threads[phy_id], competition_center_freq_updated);
  }
}

void phy_reception_update_frequency(phy_reception_t* const phy_reception_ctx, bool competition_center_freq_updated) {
  double rx_channel_center_freq, lo_offset, actual_rx_freq;

  // Calculate central frequency for the channel.
  rx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_reception_ctx->competition_center_freq, phy_reception_ctx->competition_bw, phy_reception_ctx->default_rx_bandwidth, phy_reception_ctx->last_rx_basic_control.ch);
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  lo_offset = (double)PHY_RX_LO_OFFSET;
  // Set RF Monitor sampling rate.
  float rf_mon_srate;
  if(phy_reception_ctx->competition_bw > 23000000) {
    rf_mon_srate = srslte_rf_set_rf_mon_srate(phy_reception_ctx->rf, 46080000.0); //Prasanthi 11/20/18
  } else {
    rf_mon_srate = srslte_rf_set_rf_mon_srate(phy_reception_ctx->rf, 23040000.0); //Prasanthi 11/20/18
  }
  PHY_RX_PRINT("RF MONITOR sampling rate changed to: %1.2f [MHz] after environment update\n", rf_mon_srate/1000000.0); //Prasanthi 11/20/18
  PHY_RX_PRINT("SENSING GAIN: %f\n", srslte_rf_get_rx_gain(phy_reception_ctx->rf, 1)); //Prasanthi 11/20/18
  // Set RF front-end center frequency only if competiton center frequency has been changed.
  if(competition_center_freq_updated) {
    srslte_rf_set_rx_freq2(phy_reception_ctx->rf, phy_reception_ctx->competition_center_freq, lo_offset, phy_reception_ctx->phy_id); //Prasanthi 11/20/18
  }
  // Set DDC channel center frequency for reception.
  actual_rx_freq = srslte_rf_set_rx_channel_freq(phy_reception_ctx->rf, rx_channel_center_freq, phy_reception_ctx->phy_id); //Prasanthi 11/20/18
#else // ENABLE_HW_RF_MONITOR
  // Set frequency offset for reception.
  lo_offset = phy_reception_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_RX_LO_OFFSET;
  // Set center channel frequency for reception.
  actual_rx_freq = srslte_rf_set_rx_freq2(phy_reception_ctx->rf, rx_channel_center_freq, lo_offset, phy_reception_ctx->phy_id);
#endif // ENABLE_HW_RF_MONITOR
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(actual_rx_freq < (rx_channel_center_freq - 10.0) || actual_rx_freq > (rx_channel_center_freq + 10.0)) {
     PHY_RX_ERROR("PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]................\n", phy_reception_ctx->phy_id, rx_channel_center_freq, actual_rx_freq);
  }
  PHY_RX_PRINT("PHY ID: %d - Channel %d center frequency changed to: %1.2f [MHz]\n", phy_reception_ctx->phy_id, phy_reception_ctx->last_rx_basic_control.ch, actual_rx_freq/1000000.0);
  if(phy_reception_ctx->phy_id == 1){
    PHY_RX_PRINT("RF MONITOR  center frequency changed to: %1.2f [MHz] after environment update\n", actual_rx_freq/1000000.0);
  }
}

int phy_reception_stop_rx_stream(uint32_t phy_id) {
  int error;
  phy_reception_t* const phy_reception_ctx = phy_rx_threads[phy_id];
  // Close and reopen Rx stream.
  PHY_RX_PRINT("PHY ID: %d - Trying to stop the Rx stream.\n", phy_reception_ctx->phy_id);
  if((error = srslte_rf_stop_rx_stream(phy_reception_ctx->rf, phy_reception_ctx->phy_id)) != 0) {
    PHY_RX_ERROR("PHY ID: %d - Error stopping Rx stream: %d....\n", phy_reception_ctx->phy_id, error);
    return -1;
  }
  PHY_RX_PRINT("PHY ID: %d - Rx stream stopped.\n", phy_reception_ctx->phy_id);
  // Everything went well.
  return 0;
}

timer_t* phy_reception_get_timer_id(uint32_t phy_id) {
  return &phy_rx_threads[phy_id]->synch_thread_timer_id;
}
