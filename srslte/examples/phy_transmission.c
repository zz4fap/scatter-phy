#include "phy_transmission.h"

// ***************************** Global variables ******************************
// This handle is used to pass some important objects to the PHY Tx thread work function.
static phy_transmission_t* phy_tx_threads[MAX_NUM_CONCURRENT_PHYS] = {NULL};

// Mutex used to control access to the BW change section of the code.
pthread_mutex_t phy_tx_bw_change_mutex = PTHREAD_MUTEX_INITIALIZER;

// *********************** Definition of functions *****************************
// This function is used to set everything needed for the PHY transmission thread to run accordinly.
int phy_transmission_start_thread(LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args, uint32_t phy_id) {
  // Allocate memory for a new Tx object.
  phy_tx_threads[phy_id] = (phy_transmission_t*)srslte_vec_malloc(sizeof(phy_transmission_t));
  // Check if memory allocation was correctly done.
  if(phy_tx_threads[phy_id] == NULL) {
    PHY_TX_ERROR("PHY ID: %d - Error allocating memory for Tx context.\n", phy_id);
    return -1;
  }
  // Set PHY Tx context with PHY ID number.
  phy_tx_threads[phy_id]->phy_id = phy_id;
  // Initialize PHY Tx thread context.
  phy_transmission_init_thread_context(phy_tx_threads[phy_id], handle, rf, args);
  // Set encoding/transmission thread flag to run.
  phy_tx_threads[phy_id]->run_tx_encoding_thread = true;
  // Initialize thread objects to perform encoding and transmission.
  pthread_attr_init(&phy_tx_threads[phy_id]->tx_encoding_thread_attr);
  pthread_attr_setdetachstate(&phy_tx_threads[phy_id]->tx_encoding_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Spwan thread to to perform encoding and transmission.
  int rc = pthread_create(&phy_tx_threads[phy_id]->tx_encoding_thread_id, &phy_tx_threads[phy_id]->tx_encoding_thread_attr, phy_transmission_work, (void*)phy_tx_threads[phy_id]);
  if(rc) {
    PHY_TX_ERROR("PHY ID: %d - Return code from PHY encoding/transmission pthread_create() is %d\n", phy_id, rc);
    return -1;
  }
  PHY_TX_PRINT("PHY ID: %d - PHY Tx intialization done!\n", phy_id);
  // Everything went well.
  return 0;
}

int phy_transmission_init_thread_context(phy_transmission_t* const phy_transmission_ctx, LayerCommunicator_handle handle, srslte_rf_t* const rf, transceiver_args_t* const args) {
  // Instantiate and reserve positions for Tx basic control information.
  tx_cb_make(&phy_transmission_ctx->tx_basic_control_handle, NUMBER_OF_USER_DATA_BUFFERS/2);
  // Set PHY transmission context.
  phy_transmission_init_context(phy_transmission_ctx, handle, rf, args);
  // Set Tx sample rate for Tx chain accoring to the number of PRBs.
  phy_transmission_set_tx_sample_rate(phy_transmission_ctx);
  // Configure initial Tx frequency and gain.
  if(phy_transmission_set_initial_tx_freq_and_gain(phy_transmission_ctx) < 0) {
    PHY_TX_ERROR("PHY ID: %d - Error initializing frequency and/or gain.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Initilize cell struture with transmission values.
  phy_transmission_init_cell_parameters(phy_transmission_ctx);
  // Initialize structure with last configured Tx basic control.
  phy_transmission_init_last_basic_control(phy_transmission_ctx);
  // Initialize the filter length based on the index passed through command line.
#if(ENABLE_PHY_TX_FILTERING==1)
  if(phy_transmission_ctx->trx_filter_idx > 0) {
    trx_filter_init_filter_length(phy_transmission_ctx->trx_filter_idx);
  }
#endif
  // Allocate memory for transmission buffers.
  phy_transmission_init_buffers(phy_transmission_ctx);
  // Initialize base station structures.
  phy_transmission_base_init(phy_transmission_ctx);
  PHY_TX_PRINT("PHY ID: %d - phy_transmission_base_init done!\n", phy_transmission_ctx->phy_id);
  // Initial update of allocation with MCS 0 and initial number of resource blocks.
  phy_transmission_update_radl(phy_transmission_ctx, 0, args->nof_prb);
#ifndef ENABLE_CH_EMULATOR
  // Create a watchdog timer for transmission thread.
  if(timer_init(&phy_transmission_ctx->tx_thread_timer_id) < 0) {
    PHY_TX_ERROR("PHY ID: %d - Not possible to create a timer for transmission thread.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
#endif
  // Initialize conditional variables.
  if(pthread_mutex_init(&phy_transmission_ctx->tx_basic_control_mutex, NULL) != 0) {
    PHY_TX_ERROR("PHY ID: %d - Encoding/transmission mutex init failed.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  if(pthread_mutex_init(&phy_transmission_ctx->tx_env_update_mutex, NULL) != 0) {
    PHY_TX_ERROR("PHY ID: %d - Environment update mutex init failed.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&phy_transmission_ctx->tx_basic_control_cv, NULL)) {
    PHY_TX_ERROR("PHY ID: %d - Encoding/transmission conditional variable init failed.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Everything went well.
  return 0;
}

void phy_transmission_init_context(phy_transmission_t* const phy_transmission_ctx, LayerCommunicator_handle handle, srslte_rf_t *rf, transceiver_args_t *args) {
  phy_transmission_ctx->phy_comm_handle             = handle;
  phy_transmission_ctx->rf                          = rf;
  phy_transmission_ctx->rnti                        = args->rnti;
  phy_transmission_ctx->use_std_carrier_sep         = args->use_std_carrier_sep;
  phy_transmission_ctx->is_lbt_enabled              = args->lbt_threshold < 100.0?true:false;
  phy_transmission_ctx->send_tx_stats_to_mac        = args->send_tx_stats_to_mac;
  phy_transmission_ctx->add_tx_timestamp            = args->add_tx_timestamp;
  phy_transmission_ctx->initial_subframe_index      = args->initial_subframe_index;
  phy_transmission_ctx->rf_amp                      = args->rf_amp;
  phy_transmission_ctx->trx_filter_idx              = args->trx_filter_idx;
  phy_transmission_ctx->initial_tx_gain             = args->initial_tx_gain;
  phy_transmission_ctx->nof_prb                     = args->nof_prb;
  phy_transmission_ctx->default_tx_channel          = args->default_tx_channel;
  phy_transmission_ctx->radio_id                    = args->radio_id;
  phy_transmission_ctx->nof_ports                   = args->nof_ports;
  phy_transmission_ctx->default_tx_bandwidth        = helpers_get_bw_from_nprb(args->nof_prb);      // Get bandwidth from number of physical resource blocks.
  phy_transmission_ctx->bw_idx                      = helpers_get_bw_index_from_prb(args->nof_prb); // Convert from number of Resource Blocks to BW Index.
  phy_transmission_ctx->last_nof_subframes_to_tx    = 0;
  phy_transmission_ctx->last_mcs                    = 0;
  phy_transmission_ctx->number_of_tx_offset_samples = 0;
  phy_transmission_ctx->sf_n_re                     = 0;
  phy_transmission_ctx->sf_n_samples                = 0;
  phy_transmission_ctx->sf_buffer_eb                = NULL;
  phy_transmission_ctx->output_buffer               = NULL;
  phy_transmission_ctx->subframe_ofdm_symbols       = NULL;
  phy_transmission_ctx->competition_center_freq     = args->competition_center_frequency;
  phy_transmission_ctx->competition_bw              = args->competition_bw;
  phy_transmission_ctx->env_update                  = false;
  phy_transmission_ctx->competition_freq_updated    = false;
  phy_transmission_ctx->decode_pdcch                = args->decode_pdcch;
  phy_transmission_ctx->node_id                     = args->node_id;
  phy_transmission_ctx->intf_id                     = args->intf_id;
  phy_transmission_ctx->phy_filtering               = args->phy_filtering;
  phy_transmission_ctx->use_scatter_sync_seq        = args->use_scatter_sync_seq;
  phy_transmission_ctx->pss_len                     = args->pss_len;
  phy_transmission_ctx->pss_boost_factor            = args->pss_boost_factor;
  phy_transmission_ctx->enable_eob_pss              = args->enable_eob_pss;
}

// Free all the resources used by the PHY transmission module.
int phy_transmission_stop_thread(uint32_t phy_id) {
  // Stop encoding/transmission thread.
  phy_tx_threads[phy_id]->run_tx_encoding_thread = false;
  // Notify encoding/transmission condition variable.
  pthread_cond_signal(&phy_tx_threads[phy_id]->tx_basic_control_cv);
  // Destroy PHY encoding/transmission thread.
  pthread_attr_destroy(&phy_tx_threads[phy_id]->tx_encoding_thread_attr);
  int rc = pthread_join(phy_tx_threads[phy_id]->tx_encoding_thread_id, NULL);
  if(rc) {
    PHY_TX_ERROR("PHY ID: %d - Return code from PHY encoding/transmission pthread_join() is %d\n", phy_id, rc);
    return -1;
  }
  // Destroy mutexes.
  pthread_mutex_destroy(&phy_tx_threads[phy_id]->tx_basic_control_mutex);
  pthread_mutex_destroy(&phy_tx_threads[phy_id]->tx_env_update_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&phy_tx_threads[phy_id]->tx_basic_control_cv) != 0) {
    PHY_TX_ERROR("PHY ID: %d - Encoding/transmission conditional variable destruction failed.\n",phy_id);
    return -1;
  }
#if(ENABLE_PHY_TX_FILTERING==1)
  // Free FIR Filter kernel.
  trx_filter_free_simd_kernel_mm256();
#endif
  // Free all base station structures.
  phy_transmission_free_base(phy_tx_threads[phy_id]);
  PHY_TX_INFO("PHY ID: %d - phy_transmission_free_base done!\n",phy_id);
  // Free memory used for transmission buffers.
  phy_transmission_free_buffers(phy_tx_threads[phy_id]);
  PHY_TX_INFO("PHY ID: %d - phy_transmission_free_buffers done!\n",phy_id);
  // Delete circular buffer.
  tx_cb_free(&phy_tx_threads[phy_id]->tx_basic_control_handle);
  // Free memory used to store PHY Tx context object.
  if(phy_tx_threads[phy_id]) {
    free(phy_tx_threads[phy_id]);
    phy_tx_threads[phy_id] = NULL;
  }
  // Everything went well.
  return 0;
}

// Initialize struture with Cell parameters.
void phy_transmission_init_cell_parameters(phy_transmission_t* const phy_transmission_ctx) {
  phy_transmission_ctx->cell_enb.nof_prb          = phy_transmission_ctx->nof_prb;    // nof_prb
  phy_transmission_ctx->cell_enb.nof_ports        = phy_transmission_ctx->nof_ports;  // nof_ports
  phy_transmission_ctx->cell_enb.bw_idx           = 0;                                // bw idx
  phy_transmission_ctx->cell_enb.id               = phy_transmission_ctx->radio_id;   // cell_id
  phy_transmission_ctx->cell_enb.cp               = SRSLTE_CP_NORM;                   // cyclic prefix
  phy_transmission_ctx->cell_enb.phich_length     = SRSLTE_PHICH_NORM;                // PHICH length
  phy_transmission_ctx->cell_enb.phich_resources  = SRSLTE_PHICH_R_1;                 // PHICH resources
}

// Initialize structure with last configured Tx basic control.
void phy_transmission_init_last_basic_control(phy_transmission_t* const phy_transmission_ctx) {
  phy_transmission_ctx->last_tx_basic_control.trx_flag     = PHY_TX_ST;
  phy_transmission_ctx->last_tx_basic_control.seq_number   = 0;
  phy_transmission_ctx->last_tx_basic_control.send_to      = phy_transmission_ctx->node_id;
  phy_transmission_ctx->last_tx_basic_control.intf_id      = phy_transmission_ctx->intf_id;
  phy_transmission_ctx->last_tx_basic_control.bw_idx       = phy_transmission_ctx->bw_idx;
  phy_transmission_ctx->last_tx_basic_control.ch           = phy_transmission_ctx->default_tx_channel + phy_transmission_ctx->phy_id;
  phy_transmission_ctx->last_tx_basic_control.frame        = 0;
  phy_transmission_ctx->last_tx_basic_control.slot         = 0;
  phy_transmission_ctx->last_tx_basic_control.timestamp    = 0;
  phy_transmission_ctx->last_tx_basic_control.mcs          = 0;
  phy_transmission_ctx->last_tx_basic_control.gain         = phy_transmission_ctx->initial_tx_gain;
  phy_transmission_ctx->last_tx_basic_control.rf_boost     = phy_transmission_ctx->rf_amp;
  phy_transmission_ctx->last_tx_basic_control.length       = 1;
}

static inline int phy_transmission_change_bw(phy_transmission_t* const phy_transmission_ctx, basic_ctrl_t* const bc) {
  int ret = 0;
  // Lock this section of the code so that PHY Tx threads do not interfere with each other when changing the USRP parameters.
  pthread_mutex_lock(&phy_tx_bw_change_mutex);
  // Check if we are applying the changes to the correct PHY ID.
  if(phy_transmission_ctx->phy_id != bc->phy_id) {
    PHY_TX_ERROR("PHY IDs don't match during BW change: contex PHY ID: %d - basic control PHY ID: %d\n", phy_transmission_ctx->phy_id, bc->phy_id);
    ret = -1;
    goto exit_phy_tx_change_bw;
  }
  // Set eNodeB PRB based on BW index.
  phy_transmission_ctx->cell_enb.nof_prb = helpers_get_prb_from_bw_index(bc->bw_idx);
  // Free all related Tx structures.
  phy_transmission_free_base(phy_transmission_ctx);
  // Free the transmission buffers.
  phy_transmission_free_buffers(phy_transmission_ctx);
  // Update paramters for new PHY BW.
  phy_transmission_ctx->nof_prb                     = phy_transmission_ctx->cell_enb.nof_prb;
  phy_transmission_ctx->default_tx_bandwidth        = helpers_get_bw_from_nprb(phy_transmission_ctx->nof_prb);
  phy_transmission_ctx->bw_idx                      = helpers_get_bw_index_from_prb(phy_transmission_ctx->nof_prb);
  phy_transmission_ctx->sf_n_re                     = 0;
  phy_transmission_ctx->sf_n_samples                = 0;
  phy_transmission_ctx->sf_buffer_eb                = NULL;
  phy_transmission_ctx->output_buffer               = NULL;
  phy_transmission_ctx->subframe_ofdm_symbols       = NULL;
  // Set new Tx sample rate.
  if(phy_transmission_set_tx_sample_rate(phy_transmission_ctx) < 0) {
    PHY_TX_ERROR("Error setting Tx sample rate.\n", 0);
    ret = -1;
    goto exit_phy_tx_change_bw;
  }
  // Allocate memory for transmission buffers.
  if(phy_transmission_init_buffers(phy_transmission_ctx) < 0) {
    PHY_TX_ERROR("Error initializing Tx buffers.\n", 0);
    ret = -1;
    goto exit_phy_tx_change_bw;
  }
  // Initialize base station structures.
  if(phy_transmission_base_init(phy_transmission_ctx) < 0) {
    PHY_TX_ERROR("Error initializing Tx structs.\n", 0);
    ret = -1;
    goto exit_phy_tx_change_bw;
  }

exit_phy_tx_change_bw:
  // Unlock the mutex so that other Tx thread can is able to execute.
  pthread_mutex_unlock(&phy_tx_bw_change_mutex);
  // Everything went well.
  return ret;
}

// This function is used to check and change configuration parameters related to transmission.
int phy_transmission_change_parameters(phy_transmission_t* const phy_transmission_ctx, basic_ctrl_t* const bc) {
  uint32_t bw_idx, req_mcs;
  float tx_bandwidth;

  // This is a basic sanity test, can be removed in the future as both MUST be the same.
  if(phy_transmission_ctx->phy_id != bc->phy_id) {
    PHY_TX_DEBUG("PHY ID: %d - Context PHY ID: %d different from basic ontrol PHY ID: %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->phy_id, bc->phy_id);
    return -1;
  }
  // Bandwidth.
  tx_bandwidth = helpers_get_bandwidth(bc->bw_idx, &bw_idx);
  if(tx_bandwidth < 0.0 || bw_idx >= 100) {
    PHY_TX_ERROR("PHY ID: %d - Invalid Tx BW Index: %d\n", phy_transmission_ctx->phy_id, bc->bw_idx);
    return -1;
  }
  // Check MCS index range.
  if(bc->mcs > 31) {
    PHY_TX_ERROR("PHY ID: %d - Invalid MCS: %d!\n", phy_transmission_ctx->phy_id, bc->mcs);
    return -1;
  }
  // Check if channel number is outside the possible range.
  if(bc->ch > MAX_NUM_OF_CHANNELS) {
    PHY_TX_ERROR("PHY ID: %d - Invalid Channel: %d\n", phy_transmission_ctx->phy_id, bc->ch);
    return -1;
  }
  // Check send_to field range.
  if(bc->send_to >= MAXIMUM_NUMBER_OF_RADIOS) {
    PHY_TX_ERROR("PHY ID: %d - Invalid send_to field: %d, it must be less than or equal to %d.\n", phy_transmission_ctx->phy_id, bc->send_to, MAXIMUM_NUMBER_OF_RADIOS);
    return -1;
  }
  // Check Interface ID field range.
  if(bc->intf_id >= MAX_NUM_CONCURRENT_PHYS) {
    PHY_TX_ERROR("PHY ID: %d - Invalid interface ID field: %d, it must be less than or equal to %d.\n", phy_transmission_ctx->phy_id, bc->intf_id, MAX_NUM_CONCURRENT_PHYS);
    return -1;
  }
  // Check TB size.
  if(phy_transmission_validate_tb_size(bc, bw_idx, phy_transmission_ctx->phy_id) < 0) {
    PHY_TX_ERROR("PHY ID: %d - Invalid TB size.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Check if timestamp is present when timed commands are enabled.
#if(TX_TIMED_COMMAND_ENABLED==1)
  if(bc->timestamp == 0) {
    PHY_TX_ERROR("PHY ID: %d - Timestamp field in basic control was empty....\n", phy_transmission_ctx->phy_id);
    return -1;
  }

  // Only use change-timed parameters if last BW index is equal to the current one, otherwise the new parameters are set in a non-timed way below.
  if(phy_transmission_ctx->last_tx_basic_control.bw_idx == bc->bw_idx) {

    #if(ENBALE_TX_PROFILLING==1)
    // Measure time it takes to execute a timed-command scheduling.
    uint64_t change_timed_params_timestamp_start = helpers_get_host_time_now();
    #endif // ENBALE_TX_PROFILLING

    if(phy_transmission_change_timed_parameters(phy_transmission_ctx, bc) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Error setting timed Tx parameter configuration.\n", phy_transmission_ctx->phy_id);
      return -1;
    }

    #if(ENBALE_TX_PROFILLING==1)
    // Measure time it takes to execute a timed-command scheduling.
    uint64_t change_timed_params_timestamp_end = helpers_get_host_time_now();
    int change_timed_params_diff = (int)(change_timed_params_timestamp_end - change_timed_params_timestamp_start);
    if(change_timed_params_diff > 2000) {
      PHY_TX_ERROR("----------> PHY ID: %d - Change timed parameters diff: %d\n", phy_transmission_ctx->phy_id, change_timed_params_diff);
    }
    #endif // ENBALE_TX_PROFILLING

  }
#else
  // Only call non-timed parameters function if last BW index is equal to the current one, otherwise this function is always called below.
  if(phy_transmission_ctx->last_tx_basic_control.bw_idx == bc->bw_idx) {
    if(phy_transmission_change_non_timed_parameters(phy_transmission_ctx, bc) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Error changing non-timed parameters.\n", phy_transmission_ctx->phy_id);
      return -1;
    }
  }
#endif

  // All parameters and sampling rate are changed according to the new BW index.
  if(phy_transmission_ctx->last_tx_basic_control.bw_idx != bc->bw_idx) {
    // Change all related BW parameters.
    if(phy_transmission_change_bw(phy_transmission_ctx, bc) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Error changing bandwidth.\n", phy_transmission_ctx->phy_id);
      return -1;
    }
    // Execute non-timed parameter configuration as when changing BW timed ones doesn't work.
    if(phy_transmission_change_non_timed_parameters(phy_transmission_ctx, bc) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Error changing non-timed parameters.\n", phy_transmission_ctx->phy_id);
      return -1;
    }
  }
  // Change RF boost only if the parameter has changed.
  if(bc->rf_boost > 0.0 && phy_transmission_ctx->last_tx_basic_control.rf_boost != bc->rf_boost) {
    phy_transmission_ctx->rf_amp = bc->rf_boost;
    phy_transmission_ctx->last_tx_basic_control.rf_boost = bc->rf_boost;
    PHY_TX_DEBUG_TIME("PHY ID: %d - RF boost set to: %1.2f\n", phy_transmission_ctx->phy_id, bc->rf_boost);
  }
  // Change MCS. For 1.4 MHz we can not set MCS greater than 28 for the very first subframe as it carries PSS/SSS and does not have enough "room" for FEC bits.
  req_mcs = bc->mcs;
  if(bc->bw_idx == BW_IDX_OneDotFour && bc->mcs > 28) {
    // Maximum MCS for 1st subframe of 1.4 MHz PHY BW is 28.
    req_mcs = 28;
  }
  // Update MCS.
  phy_transmission_change_allocation(phy_transmission_ctx, req_mcs, bc->bw_idx);
  // Everything went well.
  return 0;
}

int phy_transmission_change_timed_parameters(phy_transmission_t* const phy_transmission_ctx, basic_ctrl_t* const bc) {
  double actual_tx_freq, tx_channel_center_freq, lo_offset;
  float tx_gain, tx_bandwidth;

  // Retrieve environment update flag status.
  bool env_update_flag = phy_transmission_get_env_update(phy_transmission_ctx);
  // Change Center frequency only if one of the parameters have changed.
  if(phy_transmission_ctx->last_tx_basic_control.ch != bc->ch || phy_transmission_ctx->last_tx_basic_control.bw_idx != bc->bw_idx || env_update_flag) {
    // Retrieve PHY Tx bandwidth.
    tx_bandwidth = helpers_get_bandwidth_float(bc->bw_idx);
    // Calculate central Tx frequency for the channels.
    tx_channel_center_freq = phy_transmission_calculate_channel_center_frequency(phy_transmission_ctx, tx_bandwidth, bc->ch);
#if(ENABLE_HW_RF_MONITOR==1)
    // Always apply offset when HW RF Monitor is enabled.
    lo_offset = (double)PHY_TX_LO_OFFSET;
    // Change the RF front-end center frequency only if there is an environment update
    if(env_update_flag){
      bool competition_center_freq_updated = phy_transmission_get_env_update_comp_freq(phy_transmission_ctx);
      // Check if competition center frequency has been changed.
      if(competition_center_freq_updated) {
        // Set RF front-end central Tx frequency.
        srslte_rf_set_tx_freq2(phy_transmission_ctx->rf, phy_transmission_ctx->competition_center_freq, lo_offset, phy_transmission_ctx->phy_id); // Prasanthi 11/20/18
        // Reset competition center freq. change flag.
        phy_transmission_set_env_update_comp_freq(phy_transmission_ctx, false);
      }
    }
    if(phy_transmission_ctx->last_tx_basic_control.gain != bc->gain) {
      actual_tx_freq = srslte_rf_set_tx_channel_freq_and_gain_cmd(phy_transmission_ctx->rf, tx_channel_center_freq, (float)bc->gain, bc->timestamp, TX_TIME_ADVANCE_FOR_COMMANDS, phy_transmission_ctx->phy_id);      // Prasanthi 11/20/18
      // Update last Tx basic control structure.
      phy_transmission_ctx->last_tx_basic_control.gain = bc->gain;
    } else {
      actual_tx_freq = srslte_rf_set_tx_channel_freq_cmd(phy_transmission_ctx->rf, tx_channel_center_freq, bc->timestamp, TX_TIME_ADVANCE_FOR_COMMANDS, phy_transmission_ctx->phy_id);         // Prasanthi 11/20/18
    }
    if(actual_tx_freq < 0.0) {
      PHY_TX_ERROR("Error setting Tx frequency or gain with timed command. Returning with error: %f\n", actual_tx_freq);
      return -10;
    }
#else // ENABLE_HW_RF_MONITOR
    // Set frequency offset for transmission.
    lo_offset = phy_transmission_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_TX_LO_OFFSET;
    if(phy_transmission_ctx->last_tx_basic_control.gain != bc->gain) {
      actual_tx_freq = srslte_rf_set_tx_freq_and_gain_cmd(phy_transmission_ctx->rf, tx_channel_center_freq, lo_offset, (float)bc->gain, bc->timestamp, TX_TIME_ADVANCE_FOR_COMMANDS, phy_transmission_ctx->phy_id);
      // Update last Tx basic control structure.
      phy_transmission_ctx->last_tx_basic_control.gain = bc->gain;
    } else {
      actual_tx_freq = srslte_rf_set_tx_freq_cmd(phy_transmission_ctx->rf, tx_channel_center_freq, lo_offset, bc->timestamp, TX_TIME_ADVANCE_FOR_COMMANDS, phy_transmission_ctx->phy_id);
    }
    if(actual_tx_freq < 0.0) {
      PHY_TX_ERROR("Error setting Tx frequency or gain with timed command. Returning with error: %f\n",actual_tx_freq);
      return -10;
    }
#endif // ENABLE_HW_RF_MONITOR
    // Check if actual frequency is inside a range of +/- 10 Hz.
    if(actual_tx_freq < (tx_channel_center_freq - 10.0) || actual_tx_freq > (tx_channel_center_freq + 10.0)) {
      PHY_TX_ERROR("PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]................\n", phy_transmission_ctx->phy_id, tx_channel_center_freq, actual_tx_freq);
      return -1;
    }
    // Set environment update flag to false only if was true at the start of this section of code.
    if(env_update_flag) {
      phy_transmission_set_env_update(phy_transmission_ctx, false);
    }
    // Set transmission context with the current channel center frequency.
    phy_transmission_ctx->tx_channel_center_frequency = actual_tx_freq;
    // Update last tx basic control structure.
    phy_transmission_ctx->last_tx_basic_control.ch = bc->ch;
    PHY_TX_DEBUG_TIME("PHY ID: %d - Tx ---> BW[%d]: %1.1f [MHz] - Channel: %d - Set freq to: %.2f [MHz] - Offset: %.2f [MHz]\n", phy_transmission_ctx->phy_id, bc->bw_idx, (tx_bandwidth/1000000.0), bc->ch, (actual_tx_freq/1000000.0),(lo_offset/1000000.0));
  }
  // Change Tx gain only if the parameter has changed.
  if(phy_transmission_ctx->last_tx_basic_control.gain != bc->gain) {
    // Set Tx gain.
    tx_gain = srslte_rf_set_tx_gain_cmd(phy_transmission_ctx->rf, (float)bc->gain, bc->timestamp, TX_TIME_ADVANCE_FOR_COMMANDS, phy_transmission_ctx->phy_id);
    if(tx_gain < 0.0) {
      PHY_TX_ERROR("PHY ID: %d - Error setting Tx gain with timed command. Returning with error: %f\n", phy_transmission_ctx->phy_id, tx_gain);
      return -10;
    }
    // Update last Tx basic control structure.
    phy_transmission_ctx->last_tx_basic_control.gain = bc->gain;
    PHY_TX_DEBUG_TIME("PHY ID: %d - Tx gain set to: %.1f [dB]\n", phy_transmission_ctx->phy_id, tx_gain);
  }
  // Everything went well.
  return 0;
}

int phy_transmission_change_non_timed_parameters(phy_transmission_t* const phy_transmission_ctx, basic_ctrl_t* const bc) {
  double actual_tx_freq, tx_channel_center_freq, lo_offset;
  float tx_gain, tx_bandwidth;

  // Retrieve environment update flag status.
  bool env_update_flag = phy_transmission_get_env_update(phy_transmission_ctx);
  // Change Center frequency only if one of the parameters have changed.
  if(phy_transmission_ctx->last_tx_basic_control.ch != bc->ch || phy_transmission_ctx->last_tx_basic_control.bw_idx != bc->bw_idx || env_update_flag) {
    // Retrieve PHY Tx bandwidth.
    tx_bandwidth = helpers_get_bandwidth_float(bc->bw_idx);
    // Calculate central Tx frequency for the channels.
    tx_channel_center_freq = phy_transmission_calculate_channel_center_frequency(phy_transmission_ctx, tx_bandwidth, bc->ch);
#if(ENABLE_HW_RF_MONITOR==1)
    // Set frequency offset for transmission.
    lo_offset = (double)PHY_TX_LO_OFFSET;
    // Change the RF front-end center frequency only if there is an environment update
    if(env_update_flag){
      bool competition_center_freq_updated = phy_transmission_get_env_update_comp_freq(phy_transmission_ctx);
      // Check if competition center frequency has been changed.
      if(competition_center_freq_updated) {
        srslte_rf_set_tx_freq2(phy_transmission_ctx->rf, phy_transmission_ctx->competition_center_freq, lo_offset, phy_transmission_ctx->phy_id); // Prasanthi 11/20/18
        // Reset competition center freq. change flag.
        phy_transmission_set_env_update_comp_freq(phy_transmission_ctx, false);
      }
    }
    // Update DDC channel frequency.
    actual_tx_freq = srslte_rf_set_tx_channel_freq(phy_transmission_ctx->rf, tx_channel_center_freq, phy_transmission_ctx->phy_id); // Prasanthi 11/20/18
#else // ENABLE_HW_RF_MONITOR
    // Set frequency offset for transmission.
    lo_offset = phy_transmission_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_TX_LO_OFFSET;
    actual_tx_freq = srslte_rf_set_tx_freq2(phy_transmission_ctx->rf, tx_channel_center_freq, lo_offset, phy_transmission_ctx->phy_id);
#endif // ENABLE_HW_RF_MONITOR
    // Check if actual frequency is inside a range of +/- 10 Hz.
    if(actual_tx_freq < (tx_channel_center_freq - 10.0) || actual_tx_freq > (tx_channel_center_freq + 10.0)) {
      PHY_TX_ERROR("PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]................\n", phy_transmission_ctx->phy_id, tx_channel_center_freq, actual_tx_freq);
      return -1;
    }
    // Set environment update flag to false only if was true at the start of this section of code.
    if(env_update_flag) {
      phy_transmission_set_env_update(phy_transmission_ctx, false);
    }
    // Set transmission context with the current channel center frequency.
    phy_transmission_ctx->tx_channel_center_frequency = actual_tx_freq;
    // Update last tx basic control structure.
    phy_transmission_ctx->last_tx_basic_control.ch = bc->ch;
    PHY_TX_DEBUG_TIME("PHY ID: %d - Tx ---> BW[%d]: %1.1f [MHz] - Channel: %d - Set freq to: %.2f [MHz] - Offset: %.2f [MHz]\n", phy_transmission_ctx->phy_id, bc->bw_idx, (tx_bandwidth/1000000.0), bc->ch, (actual_tx_freq/1000000.0),(lo_offset/1000000.0));
  }
  // Change Tx gain only if the parameter has changed.
  if(phy_transmission_ctx->last_tx_basic_control.gain != bc->gain) {
    // Set Tx gain.
    tx_gain = srslte_rf_set_tx_gain(phy_transmission_ctx->rf, (float)bc->gain, phy_transmission_ctx->phy_id);
    if(tx_gain < 0) {
      PHY_TX_ERROR("PHY ID: %d - Error setting Tx gain: %1.2f [dB]................\n", phy_transmission_ctx->phy_id, (float)bc->gain);
      return -1;
    }
    // Update last Tx basic control structure.
    phy_transmission_ctx->last_tx_basic_control.gain = bc->gain;
    PHY_TX_DEBUG_TIME("PHY ID: %d - Tx gain set to: %.1f [dB]\n", phy_transmission_ctx->phy_id, tx_gain);
  }
  // Everything went well.
  return 0;
}

int phy_transmission_validate_tb_size(basic_ctrl_t *bc, uint32_t bw_idx, uint32_t phy_id) {
  // Change MCS. For 1.4 MHz we can not set MCS greater than 28 for the very first subframe.
  if(bc->bw_idx == BW_IDX_OneDotFour && bc->mcs > 28) {
    // Check size.
    int length = bc->length - phy_transmission_get_tb_size(bw_idx, 28);
    if(length > 0 && length % phy_transmission_get_tb_size(bw_idx, bc->mcs) != 0) {
      PHY_TX_ERROR("PHY ID: %d - Data length set by MAC to subsequent subframes is invalid. Length field in Basic control command: %d - expected size: %d\n", phy_id, bc->length, phy_transmission_get_tb_size(bw_idx, bc->mcs));
      return -1;
    }
    // Check if expected TB size is not bigger than length field in basic control message.
    if(length < 0) {
      PHY_TX_ERROR("PHY ID: %d - Data length set by MAC is invalid (negative). Length field in Basic control command: %d - expected size: %d\n", phy_id, bc->length, phy_transmission_get_tb_size(bw_idx, bc->mcs));
      return -1;
    }
  } else {
    // Check size.
    if(bc->length % phy_transmission_get_tb_size(bw_idx, bc->mcs) != 0) {
      PHY_TX_ERROR("PHY ID: %d - Data length set by MAC is invalid. Length field in Basic control command: %d - expected size: %d\n", phy_id,bc->length, phy_transmission_get_tb_size(bw_idx, bc->mcs));
      return -1;
    }
  }
  // Everything went well.
  return 0;
}

void phy_transmission_change_allocation(phy_transmission_t* const phy_transmission_ctx, uint32_t req_mcs, uint32_t req_bw_idx) {
  // Change MCS only if MCS or BW have changed.
  if(phy_transmission_ctx->last_tx_basic_control.mcs != req_mcs || phy_transmission_ctx->last_tx_basic_control.bw_idx != req_bw_idx) {
    // Update allocation with number of resource blocks and MCS.
    phy_transmission_update_radl(phy_transmission_ctx, req_mcs, phy_transmission_ctx->cell_enb.nof_prb);
    // Update last Tx basic control structure.
    phy_transmission_ctx->last_tx_basic_control.mcs = req_mcs;
    phy_transmission_ctx->last_tx_basic_control.bw_idx = req_bw_idx;
    PHY_TX_DEBUG_TIME("PHY ID: %d - MCS set to: %d - Tx BW index set to: %d\n", phy_transmission_ctx->phy_id, req_mcs, req_bw_idx);
  }
}

uint32_t phy_transmission_calculate_nof_subframes(uint32_t mcs, uint32_t bw_idx, uint32_t length) {
  uint32_t nof_subframes = 0;
  // Verify if BW is 1.4 MHz and MCS greater than 28.
  if((bw_idx+1) == BW_IDX_OneDotFour && mcs > 28) {
    length = length - phy_transmission_get_tb_size(bw_idx, 28);
    nof_subframes = 1;
    if(length > 0) {
      nof_subframes = nof_subframes + (length/phy_transmission_get_tb_size(bw_idx, mcs));
    }
  } else {
    // Calculate number of slots to be transmitted.
    nof_subframes = (length/phy_transmission_get_tb_size(bw_idx, mcs));
  }
  return nof_subframes;
}

void *phy_transmission_work(void *h) {

  phy_transmission_t* phy_transmission_ctx = (phy_transmission_t*)h;
  phy_stat_t phy_tx_stat;
  srslte_rf_t *rf = phy_transmission_ctx->rf;
  int sf_idx, subframe_cnt, tx_data_offset, number_of_additional_samples, subframe_buffer_offset, ret = 0, change_param_status = 0;
  uint32_t bw_idx, nof_subframes_to_tx, mcs_local, nof_zero_padding_samples;
  bool start_of_burst, end_of_burst;
  time_t full_secs = 0;
  double frac_secs = 0.0, coding_time = 0.0;
  bool has_time_spec = false;
  struct timespec start_data_tx;
  basic_ctrl_t bc;
  lbt_stats_t lbt_stats;
  uint64_t number_of_dropped_packets = 0, fpga_time = 0;
  uint32_t filter_zero_padding_length = 0;
  srslte_dci_msg_t dci_msg;

#if(ENBALE_TX_PROFILLING==1)
  uint64_t debugging_timestamp_end, uhd_transfer_start, uhd_transfer_end, change_params_timestamp_end, coding_end_timestamp;
  int ctrl_transfer_diff, time_advance_diff, uhd_transfer_diff, coding_diff;
#endif

#if(ENABLE_PHY_TX_FILTERING==1)
  // Retrieve filter length only if it is enabled.
  uint32_t trx_filter_length = 0;
  if(phy_transmission_ctx->trx_filter_idx > 0) {
    trx_filter_length = trx_filter_get_filter_length() - 1;
  }
#endif

  // Set priority to Tx thread.
  uhd_set_thread_priority(1.0, true);

  /****************************** PHY Transmission loop - BEGIN ******************************/
  PHY_TX_DEBUG("PHY ID: %d - Entering PHY Encoding/transmission thread loop...\n", phy_transmission_ctx->phy_id);
  while(phy_transmission_timedwait_and_pop_tx_basic_control_from_container(phy_transmission_ctx, &bc) && phy_transmission_ctx->run_tx_encoding_thread) {

#if(ENBALE_TX_PROFILLING==1)
    // Measure the time it takes for a basic control to come from the main thread to Tx thread. The timestamp is set in trx.c.
    debugging_timestamp_end = helpers_get_host_time_now();
    ctrl_transfer_diff = (int)(debugging_timestamp_end - bc.debugging_timestamp_start);
    if(ctrl_transfer_diff > 1000) {
      PHY_TX_ERROR("----------> PHY ID: %d - Message transfer time diff: %d\n", phy_transmission_ctx->phy_id, ctrl_transfer_diff);
    }
    // Measure time difference between Tx timestamp and actual time.
    time_advance_diff = (int)(bc.timestamp - debugging_timestamp_end);
    if(time_advance_diff < 1000) {
      PHY_TX_ERROR("----------> PHY ID: %d - Message timestamp diff: %d\n", phy_transmission_ctx->phy_id, time_advance_diff);
    }
#endif

    // Timestamp start of transmission procedure.
    clock_gettime(CLOCK_REALTIME, &start_data_tx);

    // Check if PHY IDs match.
    if(phy_transmission_ctx->phy_id != bc.phy_id) {
      PHY_TX_ERROR("PHY ID: %d - Context PHY ID: %d is different from basic control PHY ID: %d. Dropping MAC message.\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->phy_id, bc.phy_id);
      number_of_dropped_packets++;
      continue;
    }

#ifndef ENABLE_CH_EMULATOR
    // Create watchdog timer for transmission thread. Always wait for some seconds.
    if(timer_set(&phy_transmission_ctx->tx_thread_timer_id, 2) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Not possible to set the watchdog timer for transmission thread.\n", phy_transmission_ctx->phy_id);
    }
#endif

    // Change transmission parameters according to received basic control message.
    change_param_status = phy_transmission_change_parameters(phy_transmission_ctx, &bc);

#if(ENBALE_TX_PROFILLING==1)
    // Measure time difference between Tx timestamp and actual time.
    change_params_timestamp_end = helpers_get_host_time_now();
    int change_params_diff = (int)(bc.timestamp - change_params_timestamp_end);
    if(change_params_diff < 1000) {
      PHY_TX_ERROR("----------> PHY ID: %d - Change parameters diff: %d\n", phy_transmission_ctx->phy_id, change_params_diff);
    }
#endif

    if(change_param_status >= 0) {
      // Retrieve BW index.
      bw_idx = helpers_get_bw_index(bc.bw_idx);

      // Calculate number of slots to be transmitted.
      nof_subframes_to_tx = phy_transmission_calculate_nof_subframes(bc.mcs, bw_idx, bc.length);
      if(nof_subframes_to_tx > MAX_NOF_TBS) {
        PHY_TX_ERROR("Invalid number of TBs: %d. It has to be less than %d TBs. Dropping MAC message.\n", nof_subframes_to_tx, MAX_NOF_TBS);
        number_of_dropped_packets++;
        continue;
      }
      PHY_TX_DEBUG("PHY ID: %d - Number of slots to be transmitted: %d\n", phy_transmission_ctx->phy_id, nof_subframes_to_tx);

      // Check if there is time specificication.
      has_time_spec = false;
      if(bc.timestamp != 0) {
        has_time_spec = true;
        helpers_convert_host_timestamp_into_uhd_timestamp_us(bc.timestamp, &full_secs, &frac_secs);
        PHY_TX_DEBUG("PHY ID: %d - Send at timestamp: %" PRIu64 " - FPGA time: %" PRIu64 "\n", phy_transmission_ctx->phy_id, bc.timestamp, (fpga_time = helpers_get_fpga_timestamp_us(rf)));
        PHY_TX_DEBUG("PHY ID: %d - Time difference: %d\n", phy_transmission_ctx->phy_id, (int)(bc.timestamp-fpga_time));
      }

      // Reset all necessary counters before transmitting.
      sf_idx                    = phy_transmission_ctx->initial_subframe_index;
      tx_data_offset            = 0;
      subframe_cnt              = 0;
      start_of_burst            = true;
      end_of_burst              = false;

      PHY_TX_DEBUG("PHY ID: %d - Entering Transmission (Tx) loop...\n",phy_transmission_ctx->phy_id);
      while(subframe_cnt < nof_subframes_to_tx && phy_transmission_ctx->run_tx_encoding_thread) {

        bzero(phy_transmission_ctx->sf_buffer_eb, sizeof(cf_t) * phy_transmission_ctx->sf_n_re);

        // Increase subframe counter number.
        subframe_cnt++;

        // Add SCH/PSS/SSS to the very first subframe only.
        if(sf_idx == 0 || sf_idx == 5) {
          // Map PSS sequence into the resource grid.
          srslte_pss_put_slot_scatter(phy_transmission_ctx->pss_signal, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp, phy_transmission_ctx->pss_len);
          // If decode PDCCH/PCFICH is enabled, then we map SSS sequence carrying number of transmitted slots, otherwise, we map SCH sequences carrying SRN ID/Radio Interface ID and MCS/number of transmitted slots.
          if(phy_transmission_ctx->decode_pdcch) {
            // Check if current number of subframes to transmit is different from last one.
            if(phy_transmission_ctx->last_nof_subframes_to_tx != nof_subframes_to_tx) {
              // Generate SSS sequence to carry number of slots.
              srslte_sss_generate_nof_packets_signal(phy_transmission_ctx->sss_signal, sf_idx, nof_subframes_to_tx);
              // Update variable keeping last configured number of transmitted subframes.
              phy_transmission_ctx->last_nof_subframes_to_tx = nof_subframes_to_tx;
            }
            // Insert SSS sequence into resource grid.
            srslte_sss_put_slot(phy_transmission_ctx->sss_signal, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp);
          } else {
            // If PHY filtering is enable then we encode SRN ID and Radio Interface ID.
            if(phy_transmission_ctx->phy_filtering) {
              // Check if send_to or interface ID values are diferent and then, generate new signal.
              if(phy_transmission_ctx->last_tx_basic_control.send_to != bc.send_to || phy_transmission_ctx->last_tx_basic_control.intf_id != bc.intf_id) {
                // Generate SCH sequence carrying SRN ID plus Interface ID.
                srslte_sch_generate_from_pair(phy_transmission_ctx->sch_signal0, true, bc.send_to, bc.intf_id);
                // Update last configured values.
                phy_transmission_ctx->last_tx_basic_control.send_to = bc.send_to;
                phy_transmission_ctx->last_tx_basic_control.intf_id = bc.intf_id;
              }
              // Map SCH sequence into resource grid.
              srslte_sch_put_slot_generic(phy_transmission_ctx->sch_signal0, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp, 6);
            }

            // Check if current number of subframes to transmit or MCS are different from the last ones.
            if(phy_transmission_ctx->last_nof_subframes_to_tx != nof_subframes_to_tx || phy_transmission_ctx->last_mcs != bc.mcs) {
              // Generate SCH sequence carrying MCS plus number of transmitted slots.
              srslte_sch_generate_from_pair(phy_transmission_ctx->sch_signal1, false, bc.mcs, nof_subframes_to_tx);
              // Update last configured values.
              phy_transmission_ctx->last_mcs = bc.mcs;
              phy_transmission_ctx->last_nof_subframes_to_tx = nof_subframes_to_tx;
            }
            // Map SCH sequence into resource grid. The same position as SSS, i.e., the 6th OFDM symbol.
            srslte_sch_put_slot_generic(phy_transmission_ctx->sch_signal1, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp, 2);

            // If filtering is not enabled, then use the first, i.e., the 2nd OFDM symbol, to carry the redundant SCH signal so that the probability decoding of SCH information is higher due to combining.
            if(!phy_transmission_ctx->phy_filtering) {
              // Map redundant SCH signal into the front of the first subframe (2nd OFDM symbol).
              srslte_sch_put_slot_generic(phy_transmission_ctx->sch_signal1, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp, 6);
            }
          }
        }

        // Subframe index 7 now means it is the last subframe in a sequenece of subframes, i.e., a MAC slot.
        if(phy_transmission_ctx->enable_eob_pss && sf_idx == 7) {
          // Map PSS sequence into the resource grid of the last transmitted subframe, indicating end of transmission.
          srslte_pss_put_slot_scatter(phy_transmission_ctx->pss_signal_end, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->cell_enb.cp, phy_transmission_ctx->pss_len);
        }

        // Add reference signals (RS) so that we can estimate the channel.
        srslte_refsignal_cs_put_sf(phy_transmission_ctx->cell_enb, 0, phy_transmission_ctx->est.csr_signal.pilots[0][sf_idx], phy_transmission_ctx->sf_buffer_eb);

        // Change MCS of subsequent subframes to the highest possible value.
        if(subframe_cnt == 2 && bc.bw_idx == BW_IDX_OneDotFour && bc.mcs > 28) {
          phy_transmission_change_allocation(phy_transmission_ctx, bc.mcs, bc.bw_idx);
        }
        PHY_TX_DEBUG("PHY ID: %d - subframe_cnt: %d - MCS set to: %d\n", phy_transmission_ctx->phy_id, subframe_cnt, phy_transmission_ctx->ra_dl.mcs_idx);

        // If decode PDCCH/PCFICH is enabled, then we map those signals into the resource grid.
        if(phy_transmission_ctx->decode_pdcch) {
          // Encode PCFICH.
          srslte_pcfich_encode(&phy_transmission_ctx->pcfich, DEFAULT_CFI, phy_transmission_ctx->sf_symbols, sf_idx);

          // Encode PDCCH with control for user data decoding.
          PHY_TX_DEBUG("PHY ID: %d - Putting DCI to location: n = %d, L = %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->locations[sf_idx][0].ncce, phy_transmission_ctx->locations[sf_idx][0].L);
          srslte_dci_msg_pack_pdsch(&phy_transmission_ctx->ra_dl, SRSLTE_DCI_FORMAT1, &dci_msg, phy_transmission_ctx->cell_enb.nof_prb, false);
          if(srslte_pdcch_encode(&phy_transmission_ctx->pdcch, &dci_msg, phy_transmission_ctx->locations[sf_idx][0], phy_transmission_ctx->rnti, phy_transmission_ctx->sf_symbols, sf_idx, DEFAULT_CFI)) {
            PHY_TX_ERROR("PHY ID: %d - Error encoding DCI message. Dropping MAC message.\n", phy_transmission_ctx->phy_id);
            number_of_dropped_packets++;
            break;
          }
        }

        // Configure PDSCH to transmit the requested allocation.
        srslte_ra_dl_dci_to_grant_scatter(&phy_transmission_ctx->ra_dl, phy_transmission_ctx->cell_enb.nof_prb, phy_transmission_ctx->rnti, &phy_transmission_ctx->grant);
        if(srslte_pdsch_cfg_scatter(&phy_transmission_ctx->pdsch_cfg, !phy_transmission_ctx->phy_filtering, phy_transmission_ctx->pss_len, phy_transmission_ctx->cell_enb, &phy_transmission_ctx->grant, DEFAULT_CFI, sf_idx, 0)) {
          PHY_TX_ERROR("PHY ID: %d - Error configuring PDSCH. Dropping MAC message.\n", phy_transmission_ctx->phy_id);
          number_of_dropped_packets++;
          break;
        }

        // Add Tx timestamp to data.
  #if(ENABLE_TX_TO_RX_TIME_DIFF==1)
        uint64_t tx_timestamp;
        struct timespec tx_timestamp_struct;
        if(phy_transmission_ctx->add_tx_timestamp) {
          clock_gettime(CLOCK_REALTIME, &tx_timestamp_struct);
          tx_timestamp = helpers_convert_host_timestamp(&tx_timestamp_struct);
          memcpy((void*)(bc.data+tx_data_offset), (void*)&tx_timestamp, sizeof(uint64_t));
        }
  #endif

        // Encode PDSCH.
        if(srslte_pdsch_encode_scatter(&phy_transmission_ctx->pdsch, &phy_transmission_ctx->pdsch_cfg, &phy_transmission_ctx->softbuffer, (bc.data+tx_data_offset), phy_transmission_ctx->sf_symbols)) {
          PHY_TX_ERROR("PHY ID: %d - Error encoding PDSCH. Dropping MAC message.\n",phy_transmission_ctx->phy_id);
          number_of_dropped_packets++;
          break;
        }

        // Increment data offset when there is more than 1 slot to be transmitted.
        mcs_local = bc.mcs;
        if(subframe_cnt == 1 && bc.bw_idx == BW_IDX_OneDotFour && bc.mcs > 28) {
          mcs_local = 28;
        }
        tx_data_offset = tx_data_offset + phy_transmission_get_tb_size(bw_idx, mcs_local);

        //PHY_TX_DEBUG("mcs_local: %d - tx_data_offset: %d - TB size: %d - MCS: %d - PRB: %d\n",mcs_local,tx_data_offset,phy_transmission_get_tb_size(bw_idx, mcs_local),mcs_local,phy_transmission_handle->cell_enb.nof_prb);

        // Check if it is necessary to add zeros before the subframe.
        number_of_additional_samples = 0;
        subframe_buffer_offset = FIX_TX_OFFSET_SAMPLES;
        if(start_of_burst) {
          number_of_additional_samples = phy_transmission_ctx->number_of_tx_offset_samples;
          subframe_buffer_offset = 0;
          PHY_TX_DEBUG("PHY ID: %d - Adding %d zero samples before start of subframe.\n",phy_transmission_ctx->phy_id, number_of_additional_samples);
        }

  #if(ENABLE_PHY_TX_FILTERING==1)
        // Apply filter with zero padding so that we have a kind of f-OFDM implementation.
        if(phy_transmission_ctx->trx_filter_idx > 0) {
          // Transform to OFDM symbols.
          srslte_ofdm_tx_sf(&phy_transmission_ctx->ifft, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->subframe_ofdm_symbols);
          filter_zero_padding_length = 0;
          if(subframe_cnt == nof_subframes_to_tx) {
            filter_zero_padding_length = trx_filter_length;
          }
          //struct timespec start_filter;
          //clock_gettime(CLOCK_REALTIME, &start_filter);
          trx_filter_run_fir_tx_filter_sse_mm256_complex3(phy_transmission_ctx->subframe_ofdm_symbols, (phy_transmission_ctx->sf_n_samples+filter_zero_padding_length), phy_transmission_ctx->output_buffer+FIX_TX_OFFSET_SAMPLES, subframe_cnt, nof_subframes_to_tx);
          //PHY_PROFILLING_AVG3("Avg. filtering time: %f [ms] - min: %f - max: %f - max counter %d - diff >= 1.5ms: %d - total counter: %d - perc: %f\n", helpers_profiling_diff_time(&start_filter), 1.5, 1000);
        } else {
  #endif
          // Case filtering is not enabled, then only OFDM generation is performed.
          filter_zero_padding_length = 0;
          // Transform to OFDM symbols.
          srslte_ofdm_tx_sf(&phy_transmission_ctx->ifft, phy_transmission_ctx->sf_buffer_eb, phy_transmission_ctx->output_buffer+FIX_TX_OFFSET_SAMPLES);
  #if(ENABLE_PHY_TX_FILTERING==1)
        }
  #endif

        // If last subframe to be transmitted then we flag that it is the end of burst.
        nof_zero_padding_samples = 0;
        if(subframe_cnt == nof_subframes_to_tx) {
          // If this is the last subframe of a frame, then, set the end of burst flag to true.
          end_of_burst = true;
          // If this is the last subframe of a frame then we add some zeros at the end of this subframe.
          nof_zero_padding_samples = NOF_PADDING_ZEROS;
        }
        float norm_factor = (float) phy_transmission_ctx->cell_enb.nof_prb/15/sqrtf(phy_transmission_ctx->pdsch_cfg.grant.nof_prb);
        srslte_vec_sc_prod_cfc(phy_transmission_ctx->output_buffer+FIX_TX_OFFSET_SAMPLES, (phy_transmission_ctx->rf_amp*norm_factor), phy_transmission_ctx->output_buffer+FIX_TX_OFFSET_SAMPLES, SRSLTE_SF_LEN_PRB(phy_transmission_ctx->cell_enb.nof_prb)+filter_zero_padding_length);

#if(ENBALE_TX_PROFILLING==1)
        uhd_transfer_start = helpers_get_host_time_now();
#endif

#ifndef ENABLE_CH_EMULATOR
        // Check if transmit_at timestamp field is still in the future so that subframe can be transmitted in time, otherwise it is dropped.
        if(start_of_burst) {
          int tdiff = (int)(bc.timestamp - helpers_get_host_time_now());
          if(tdiff <= 0) {
            PHY_TX_ERROR("PHY ID: %d - Transmit_at field was in the past. Time difference is: %d [us]. Dropping MAC message.\n", phy_transmission_ctx->phy_id, tdiff);
            number_of_dropped_packets++;
            break;
          }
        }
#endif

        ret = srslte_rf_send_timed3(rf, (phy_transmission_ctx->output_buffer+subframe_buffer_offset), (phy_transmission_ctx->sf_n_samples+number_of_additional_samples+nof_zero_padding_samples+filter_zero_padding_length), full_secs, frac_secs, has_time_spec, true, start_of_burst, end_of_burst, phy_transmission_ctx->is_lbt_enabled, (void*)&lbt_stats, phy_transmission_ctx->phy_id);
        // Set SOB to false after transferring the very first subframe.
        start_of_burst = false;

#if(ENBALE_TX_PROFILLING==1)
        uhd_transfer_end = helpers_get_host_time_now();
        uhd_transfer_diff = (int)(uhd_transfer_end - uhd_transfer_start);
        if(uhd_transfer_diff > 4000) {
          PHY_TX_ERROR("----------> PHY ID: %d - USRP transfer time diff: %d\n", phy_transmission_ctx->phy_id, uhd_transfer_diff);
        }
#endif

#if(WRITE_TX_SUBFRAME_INTO_FILE==1)
        static uint32_t dump_cnt[2] = {0, 0};
        char output_file_name[200];
        sprintf(output_file_name,"check_subframe_sf_%d_cnt_%d_phy_id_%d.dat", subframe_cnt, dump_cnt[phy_transmission_ctx->phy_id], phy_transmission_ctx->phy_id);
        srslte_filesink_t file_sink;
        if(dump_cnt[phy_transmission_ctx->phy_id] < 5 && subframe_cnt == 1) {
          filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
          // Write samples into file.
          filesink_write(&file_sink, (phy_transmission_ctx->output_buffer+FIX_TX_OFFSET_SAMPLES), (phy_transmission_ctx->sf_n_samples+filter_zero_padding_length));
          // Close file.
          filesink_free(&file_sink);
          dump_cnt[phy_transmission_ctx->phy_id]++;
          PHY_TX_PRINT("PHY ID: %d - File dumped: %d.\n", phy_transmission_ctx->phy_id, dump_cnt[phy_transmission_ctx->phy_id]);
        }
#endif

        // Add SCH/PSS/SSS only to the very first subframe in a COT.
        // Here we use subframe (initial_subframe_index + 1) to send the remaning data, however, it can be any subframe different from 0 and 5.
        sf_idx = phy_transmission_ctx->initial_subframe_index + 1;

        // If this is the last subframe then set the last one to be equal to 7 so that PSS is added to it for EOB signalling.
        if(phy_transmission_ctx->enable_eob_pss && subframe_cnt == (nof_subframes_to_tx-1)) {
          sf_idx++;
        }
      }

      // Check if transmission of Tx stats to MAc is enabled.
      if(phy_transmission_ctx->send_tx_stats_to_mac) {
        // Calculate coding time.
        coding_time = helpers_profiling_diff_time(&start_data_tx);
        // Create a PHY Tx Stat struture to inform upper layers transmission was successful.
        // Set common values to the Tx Stats Structure.
        phy_tx_stat.seq_number                    = bc.seq_number;                     // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
        phy_tx_stat.host_timestamp                = helpers_get_host_time_now();   // Host PC time value when (ch,slot) PHY data are demodulated.
        phy_tx_stat.ch                            = bc.ch;				                              // Channel number which in turn is translated to a central frequency. Range: [0, 59]
        phy_tx_stat.mcs                           = bc.mcs;                                   // Set MCS to unspecified number. If this number is receiber by upper layer it means nothing was received and status MUST be checked.
        phy_tx_stat.num_cb_total                  = nof_subframes_to_tx;             // Number of slots requested to be transmitted.
        phy_tx_stat.num_cb_err                    = number_of_dropped_packets;         // Number of slots dropped for the current request from MAC.
        phy_tx_stat.stat.tx_stat.power            = bc.gain;                   // Gain used for transmission.
        phy_tx_stat.stat.tx_stat.channel_free_cnt = lbt_stats.channel_free_cnt;
        phy_tx_stat.stat.tx_stat.channel_busy_cnt = lbt_stats.channel_busy_cnt;
        phy_tx_stat.stat.tx_stat.free_energy      = lbt_stats.free_energy;
        phy_tx_stat.stat.tx_stat.busy_energy      = lbt_stats.busy_energy;
        phy_tx_stat.stat.tx_stat.coding_time      = coding_time;
        phy_tx_stat.stat.tx_stat.rf_boost         = phy_transmission_ctx->rf_amp;     // RF boost is applied to slot before its actual transmission.
        // Send PHY transmission (Tx) statistics.
        phy_transmission_send_tx_statistics(phy_transmission_ctx, &phy_tx_stat, ret);
      }

      //helpers_measure_packets_per_second("Tx");

      // Print Tx statistics on screen.
      PHY_TX_INFO_TIME("[Tx STATS]: PHY ID: %d - Tx slots: %d - PRB: %d - Channel: %d - Freq: %.2f [MHz] - MCS: %d - Tx Gain: %d - CID: %d - Coding time: %f [ms]\n", phy_transmission_ctx->phy_id, nof_subframes_to_tx, phy_transmission_ctx->cell_enb.nof_prb, bc.ch, phy_transmission_ctx->tx_channel_center_frequency/1000000.0, bc.mcs, bc.gain, phy_transmission_ctx->cell_enb.id, helpers_profiling_diff_time(&start_data_tx));

      //PHY_PROFILLING_AVG6("PHY ID: %d - Avg. coding time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 0.5 [ms]: %d - total counter: %d - perc: %f\n", phy_transmission_ctx->phy_id, helpers_profiling_diff_time(&start_data_tx), 0.5, 1000);
    } else {
      PHY_TX_ERROR("PHY ID: %d - Change of Tx parameters returned an error. Dropping MAC message.\n", phy_transmission_ctx->phy_id);
      number_of_dropped_packets++;
    }

#ifndef ENABLE_CH_EMULATOR
    // Disarm watchdog timer for transmission thread.
    if(timer_disarm(&phy_transmission_ctx->tx_thread_timer_id) < 0) {
      PHY_TX_ERROR("PHY ID: %d - Not possible to disarm the watchdog timer for transmission thread.\n", phy_transmission_ctx->phy_id);
    }
#endif

#if(ENBALE_TX_PROFILLING==1)
     coding_end_timestamp = helpers_get_host_time_now();
     coding_diff = (int)(coding_end_timestamp - debugging_timestamp_end);
     if(coding_diff > 6000) {
       PHY_TX_ERROR("----------> PHY ID: %d - Coding time diff: %d\n", phy_transmission_ctx->phy_id, coding_diff);
     }
#endif

  }
  /****************************** PHY Transmission loop - END ******************************/

  PHY_TX_PRINT("PHY ID: %d - Leaving PHY Encoding/transmission thread.\n", phy_transmission_ctx->phy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void phy_transmission_send_tx_statistics(phy_transmission_t* const phy_transmission_ctx, phy_stat_t *phy_tx_stat, int ret) {
  // Set common values to the Tx Stats Structure.
  phy_tx_stat->status = (ret > 0) ? PHY_SUCCESS : PHY_LBT_TIMEOUT; // Layer receceiving this message MUST check the statistics it is carrying for real status of the current request.
  phy_tx_stat->frame  = 0;                                         // Frame number.
  phy_tx_stat->slot   = 0;			                                   // Time slot number. Range: [0, 2000]

  // Send Tx stats. There is a mutex on this function which prevents RX from sending statistics to PHY at the same time.
  if(communicator_send_phy_stat_message(phy_transmission_ctx->phy_comm_handle, TX_STAT, phy_tx_stat, NULL) < 0) {
    PHY_TX_ERROR("Tx statistics message not sent due to an error with the communicator module.\n",0);
  }
}

unsigned int phy_transmission_reverse(register unsigned int x) {
  x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
  x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
  x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
  x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
  return((x >> 16) | (x << 16));
}

uint32_t phy_transmission_prbset_to_bitmask(uint32_t nof_prb) {
  uint32_t mask = 0;
  int nb = (int) ceilf((float) nof_prb / srslte_ra_type0_P(nof_prb));
  for(int i = 0; i < nb; i++) {
    if(i >= 0 && i < nb) {
      mask = mask | (0x1<<i);
    }
  }
  return phy_transmission_reverse(mask)>>(32-nb);
}

int phy_transmission_update_radl(phy_transmission_t* const phy_transmission_ctx, uint32_t mcs, uint32_t nof_prb) {
  bzero(&phy_transmission_ctx->ra_dl, sizeof(srslte_ra_dl_dci_t));
  phy_transmission_ctx->ra_dl.harq_process            = 0;
  phy_transmission_ctx->ra_dl.mcs_idx                 = mcs;
  phy_transmission_ctx->ra_dl.ndi                     = 0;
  phy_transmission_ctx->ra_dl.rv_idx                  = 0;
  phy_transmission_ctx->ra_dl.alloc_type              = SRSLTE_RA_ALLOC_TYPE0;
  phy_transmission_ctx->ra_dl.type0_alloc.rbg_bitmask = phy_transmission_prbset_to_bitmask(nof_prb);
  // Everything went well.
  return 0;
}

int phy_transmission_init_buffers(phy_transmission_t* const phy_transmission_ctx) {

  // calculate number of resource elements and IQ samples.
  phy_transmission_ctx->sf_n_re = 2 * SRSLTE_CP_NORM_NSYMB * phy_transmission_ctx->nof_prb * SRSLTE_NRE;
  phy_transmission_ctx->sf_n_samples = 2 * SRSLTE_SLOT_LEN(srslte_symbol_sz(phy_transmission_ctx->nof_prb));

  PHY_TX_PRINT("PHY ID: %d - sf_n_re: %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->sf_n_re);
  PHY_TX_PRINT("PHY ID: %d - sf_n_samples: %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->sf_n_samples);

  // Retrieve filter length.
  uint32_t trx_filter_length = 0;

#if(ENABLE_PHY_TX_FILTERING==1)
  if(phy_transmission_ctx->trx_filter_idx > 0) {
    trx_filter_length = trx_filter_get_filter_length() - 1;
  }
#endif

  // Retrieve device name.
  const char *devname = srslte_rf_name(phy_transmission_ctx->rf);
  PHY_TX_DEBUG("PHY ID: %d - Device name: %s\n", phy_transmission_ctx->phy_id, devname);

  // Decide the number of samples in a subframe.
  int number_of_subframe_samples = phy_transmission_ctx->sf_n_samples;
  phy_transmission_ctx->number_of_tx_offset_samples = 0;
  if(strcmp(devname,DEVNAME_X300) == 0 && FIX_TX_OFFSET_SAMPLES > 0) {
    number_of_subframe_samples = phy_transmission_ctx->sf_n_samples + FIX_TX_OFFSET_SAMPLES;
    phy_transmission_ctx->number_of_tx_offset_samples = FIX_TX_OFFSET_SAMPLES;
    PHY_TX_DEBUG("PHY ID: %d - HW: %s and Tx Offset: %d - zero padding: %d\n", phy_transmission_ctx->phy_id,devname,FIX_TX_OFFSET_SAMPLES,NOF_PADDING_ZEROS);
  }

  // Increase the number of samples so that there is room for padding zeros.
  if(NOF_PADDING_ZEROS > 0) {
    number_of_subframe_samples = number_of_subframe_samples + NOF_PADDING_ZEROS;
  }

  // init memory.
  phy_transmission_ctx->subframe_ofdm_symbols = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*(phy_transmission_ctx->sf_n_samples+trx_filter_length));
  if(!phy_transmission_ctx->subframe_ofdm_symbols) {
    PHY_TX_ERROR("PHY ID: %d - Error allocating memory to subframe_ofdm_symbols\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  // Set allocated memory to 0.
  bzero(phy_transmission_ctx->subframe_ofdm_symbols, sizeof(cf_t)*(phy_transmission_ctx->sf_n_samples+trx_filter_length));
  PHY_TX_PRINT("PHY ID: %d - subframe_ofdm_symbols allocated and zeroed\n",phy_transmission_ctx->phy_id);

  // Init output buffer memory.
  phy_transmission_ctx->output_buffer = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*(number_of_subframe_samples+trx_filter_length));
  if(!phy_transmission_ctx->output_buffer) {
    PHY_TX_ERROR("PHY ID: %d - Error allocating memory to output_buffer\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  // Set allocated memory to 0.
  // TODO: when online bandwidth change is implemented this setting of memory to 0 will have an impact on the time to change the bandwidth. That should be taken into account.
  bzero(phy_transmission_ctx->output_buffer, sizeof(cf_t)*(number_of_subframe_samples+trx_filter_length));
  PHY_TX_PRINT("PHY ID: %d - output_buffer allocated and zeroed\n",phy_transmission_ctx->phy_id);

  // init memory.
  phy_transmission_ctx->sf_buffer_eb = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*phy_transmission_ctx->sf_n_re);
  if(!phy_transmission_ctx->sf_buffer_eb) {
    PHY_TX_ERROR("PHY ID: %d - Error allocating memory to sf_buffer_eb\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  PHY_TX_PRINT("PHY ID: %d - sf_buffer_eb allocated and zeroed\n",phy_transmission_ctx->phy_id);
  // Everything went well.
  return 0;
}

void phy_transmission_free_buffers(phy_transmission_t* const phy_transmission_ctx) {
  if(phy_transmission_ctx->sf_buffer_eb) {
    free(phy_transmission_ctx->sf_buffer_eb);
    phy_transmission_ctx->sf_buffer_eb = NULL;
  }
  if(phy_transmission_ctx->output_buffer) {
    free(phy_transmission_ctx->output_buffer);
    phy_transmission_ctx->output_buffer = NULL;
  }
  if(phy_transmission_ctx->subframe_ofdm_symbols) {
    free(phy_transmission_ctx->subframe_ofdm_symbols);
    phy_transmission_ctx->subframe_ofdm_symbols = NULL;
  }
  PHY_TX_PRINT("PHY ID: %d - phy_transmission_free_buffers DONE!\n",phy_transmission_ctx->phy_id);
}

int phy_transmission_base_init(phy_transmission_t* const phy_transmission_ctx) {

#if(ENABLE_PHY_TX_FILTERING==1)
  // Create filter kernel.
  if(phy_transmission_ctx->trx_filter_idx > 0) {
   trx_filter_create_tx_simd_kernel_mm256(helpers_get_bw_index(phy_transmission_ctx->bw_idx));
  }
#endif
  // create ifft object.
  if(srslte_ofdm_tx_init(&phy_transmission_ctx->ifft, phy_transmission_ctx->cell_enb.cp, phy_transmission_ctx->cell_enb.nof_prb)) {
    PHY_TX_ERROR("PHY ID: %d - Error creating iFFT object\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  // Set normalization to true in IFFT object.
  srslte_ofdm_set_normalize(&phy_transmission_ctx->ifft, true);
  // Initialize control channels only if enabled.
  if(phy_transmission_ctx->decode_pdcch) {
    // Initialize registers.
    if(srslte_regs_init(&phy_transmission_ctx->regs, phy_transmission_ctx->cell_enb)) {
      PHY_TX_ERROR("PHY ID: %d - Error initiating regs\n",phy_transmission_ctx->phy_id);
      return -1;
    }
    // Initialize PCFICH object.
    if(srslte_pcfich_init(&phy_transmission_ctx->pcfich, &phy_transmission_ctx->regs, phy_transmission_ctx->cell_enb)) {
      PHY_TX_ERROR("PHY ID: %d - Error creating PCFICH object\n",phy_transmission_ctx->phy_id);
      return -1;
    }
    // Initialize CFI register object.
    if(srslte_regs_set_cfi(&phy_transmission_ctx->regs, DEFAULT_CFI)) {
      PHY_TX_ERROR("PHY ID: %d - Error setting CFI\n",phy_transmission_ctx->phy_id);
      return -1;
    }
    // Initialize PDCCH object.
    if(srslte_pdcch_init(&phy_transmission_ctx->pdcch, &phy_transmission_ctx->regs, phy_transmission_ctx->cell_enb)) {
      PHY_TX_ERROR("PHY ID: %d - Error creating PDCCH object\n",phy_transmission_ctx->phy_id);
      return -1;
    }
    // Initiate valid DCI locations.
    for(int i = 0; i < SRSLTE_NSUBFRAMES_X_FRAME; i++) {
      srslte_pdcch_ue_locations(&phy_transmission_ctx->pdcch, phy_transmission_ctx->locations[i], 30, i, DEFAULT_CFI, phy_transmission_ctx->rnti);
    }
  }
  // Initialize PDSCH object.
  if(srslte_pdsch_init_generic(&phy_transmission_ctx->pdsch, phy_transmission_ctx->cell_enb, phy_transmission_ctx->phy_id, !phy_transmission_ctx->phy_filtering)) {
    PHY_TX_ERROR("PHY ID: %d - Error creating PDSCH object\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  // Set RNTI for PDSCH object.
  srslte_pdsch_set_rnti(&phy_transmission_ctx->pdsch, phy_transmission_ctx->rnti);
  // Initialize softbuffer object.
  if(srslte_softbuffer_tx_init_scatter(&phy_transmission_ctx->softbuffer, phy_transmission_ctx->cell_enb.nof_prb)) {
    PHY_TX_ERROR("PHY ID: %d - Error initiating soft buffer\n",phy_transmission_ctx->phy_id);
    return -1;
  }
  // Reset softbuffer.
  srslte_softbuffer_tx_reset(&phy_transmission_ctx->softbuffer);
  // Allocate memory for PSS sync.
  phy_transmission_ctx->pss_signal = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*phy_transmission_ctx->pss_len);
  // Check if memory allocation was correctly done.
  if(phy_transmission_ctx->pss_signal == NULL) {
    PHY_TX_ERROR("PHY ID: %d - Error allocating memory for PSS sync.\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Generate Scatter PSS sequence for synchronization.
  srslte_pss_generate_scatter(phy_transmission_ctx->pss_signal, phy_transmission_ctx->cell_enb.id % 3, phy_transmission_ctx->pss_len);
  // Boost Scatter PSS sequence for synchronization.
  if(phy_transmission_ctx->pss_boost_factor > 1.0) {
    srslte_vec_sc_prod_cfc(phy_transmission_ctx->pss_signal, phy_transmission_ctx->pss_boost_factor, phy_transmission_ctx->pss_signal, phy_transmission_ctx->pss_len);
  }
  if(phy_transmission_ctx->enable_eob_pss) {
    // Allocate memory for PSS sync.
    phy_transmission_ctx->pss_signal_end = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*phy_transmission_ctx->pss_len);
    // Check if memory allocation was correctly done.
    if(phy_transmission_ctx->pss_signal_end == NULL) {
      PHY_TX_ERROR("PHY ID: %d - Error allocating memory for PSS END.\n", phy_transmission_ctx->phy_id);
      return -1;
    }
    // Generate Scatter PSS sequence for end of transmission.
    srslte_pss_generate_scatter(phy_transmission_ctx->pss_signal_end, (phy_transmission_ctx->cell_enb.id % 3) + 1, phy_transmission_ctx->pss_len);
    // Boost Scatter PSS sequence for end of transmission.
    if(phy_transmission_ctx->pss_boost_factor > 1.0) {
      srslte_vec_sc_prod_cfc(phy_transmission_ctx->pss_signal_end, phy_transmission_ctx->pss_boost_factor, phy_transmission_ctx->pss_signal_end, phy_transmission_ctx->pss_len);
    }
  }
  // Generate SCH sequence carrying SRN ID plus Interface ID set to 0.
  srslte_sch_generate_from_pair(phy_transmission_ctx->sch_signal0, true, phy_transmission_ctx->last_tx_basic_control.send_to, phy_transmission_ctx->last_tx_basic_control.intf_id);
  // Generate SCH sequence carrying MCS plus number of transmitted slots set to 0.
  srslte_sch_generate_from_pair(phy_transmission_ctx->sch_signal1, false, phy_transmission_ctx->last_mcs, phy_transmission_ctx->last_nof_subframes_to_tx);
  // Generate CRS signals.
  if(srslte_chest_dl_init(&phy_transmission_ctx->est, phy_transmission_ctx->cell_enb)) {
    PHY_TX_ERROR("PHY ID: %d - Error initializing equalizer\n", phy_transmission_ctx->phy_id);
    return -1;
  }
  // Initialize slot (subframe).
  for(int i = 0; i < SRSLTE_MAX_PORTS; i++) { // now there's only 1 port
    phy_transmission_ctx->sf_symbols[i] = phy_transmission_ctx->sf_buffer_eb;
  }
  // Everything went well.
  return 0;
}

void phy_transmission_free_base(phy_transmission_t* const phy_transmission_ctx) {
  srslte_softbuffer_tx_free_scatter(&phy_transmission_ctx->softbuffer);
  srslte_pdsch_free(&phy_transmission_ctx->pdsch);
  srslte_chest_dl_free(&phy_transmission_ctx->est);
  srslte_ofdm_tx_free(&phy_transmission_ctx->ifft);
  if(phy_transmission_ctx->decode_pdcch) {
    srslte_pcfich_free(&phy_transmission_ctx->pcfich);
    srslte_pdcch_free(&phy_transmission_ctx->pdcch);
    srslte_regs_free(&phy_transmission_ctx->regs);
  }
  if(phy_transmission_ctx->pss_signal) {
    free(phy_transmission_ctx->pss_signal);
    phy_transmission_ctx->pss_signal = NULL;
  }
  if(phy_transmission_ctx->enable_eob_pss) {
    if(phy_transmission_ctx->pss_signal_end) {
      free(phy_transmission_ctx->pss_signal_end);
      phy_transmission_ctx->pss_signal_end = NULL;
    }
  }
}

int phy_transmission_set_tx_sample_rate(phy_transmission_t* const phy_transmission_ctx) {
  int srate = -1;
  float srate_rf = 0.0;
  if(phy_transmission_ctx->use_std_carrier_sep) {
    srate = srslte_sampling_freq_hz(phy_transmission_ctx->nof_prb);
  } else {
    srate = helpers_non_std_sampling_freq_hz(phy_transmission_ctx->nof_prb);
    PHY_TX_PRINT("PHY ID: %d - Setting a non-standard sampling rate: %1.2f [MHz]\n", phy_transmission_ctx->phy_id, srate/1000000.0);
  }
  if(srate != -1) {
    srate_rf = srslte_rf_set_tx_srate(phy_transmission_ctx->rf, (double)srate, phy_transmission_ctx->phy_id);
    if(srate_rf != srate) {
      PHY_TX_ERROR("PHY ID: %d - Could not set Tx sampling rate\n",phy_transmission_ctx->phy_id);
      return -1;
    }
    PHY_TX_PRINT("PHY ID: %d - Set Tx sampling rate to: %.2f [MHz]\n", phy_transmission_ctx->phy_id, srate_rf/1000000.0);

    srslte_rf_set_fir_taps(phy_transmission_ctx->rf, phy_transmission_ctx->nof_prb, phy_transmission_ctx->phy_id);
  } else {
    PHY_TX_ERROR("PHY ID: %d - Invalid number of PRB (Tx): %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->nof_prb);
    return -1;
  }
  // Everything went well.
  return 0;
}

int phy_transmission_set_initial_tx_freq_and_gain(phy_transmission_t* const phy_transmission_ctx) {
  double tx_channel_center_freq, lo_offset, actual_tx_freq;

  // Set default Tx gain.
  float current_tx_gain = srslte_rf_set_tx_gain(phy_transmission_ctx->rf, phy_transmission_ctx->initial_tx_gain, phy_transmission_ctx->phy_id);
  // Calculate the default central Tx frequency.
  tx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_transmission_ctx->competition_center_freq, phy_transmission_ctx->competition_bw, phy_transmission_ctx->default_tx_bandwidth, (phy_transmission_ctx->default_tx_channel + phy_transmission_ctx->phy_id));
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  lo_offset = (double)PHY_TX_LO_OFFSET;
  // Set channel center frequency for transmission.
  double actual_comp_freq = srslte_rf_set_tx_freq2(phy_transmission_ctx->rf, phy_transmission_ctx->competition_center_freq, lo_offset, phy_transmission_ctx->phy_id); // Prasanthi 11/20/18
  // Check if actual competition frequency is inside a range of +/- 10 Hz.
  if(actual_comp_freq < (phy_transmission_ctx->competition_center_freq - 10.0) || actual_comp_freq > (phy_transmission_ctx->competition_center_freq + 10.0)) {
     PHY_TX_ERROR("[Initialization] PHY ID: %d - Requested competition freq.: %1.2f [MHz] - Actual competition freq.: %1.2f [MHz] - Competition BW: %1.2f [MHz] - PHY BW: %1.2f [MHz] - Channel: %d\n", phy_transmission_ctx->phy_id, phy_transmission_ctx->competition_center_freq/1000000.0, actual_comp_freq/1000000.0, phy_transmission_ctx->competition_bw/1000000.0, phy_transmission_ctx->default_tx_bandwidth/1000000.0, (phy_transmission_ctx->default_tx_channel + phy_transmission_ctx->phy_id));
     return -1;
  }
  // Set DDC channel frequency.
  actual_tx_freq = srslte_rf_set_tx_channel_freq(phy_transmission_ctx->rf, tx_channel_center_freq, phy_transmission_ctx->phy_id);  // Prasanthi 11/20/18
#else
  // Set frequency offset for transmission.
  lo_offset = phy_transmission_ctx->rf->num_of_channels == 1 ? 0.0:(double)PHY_TX_LO_OFFSET;
  // Set channel center frequency for transmission.
  actual_tx_freq = srslte_rf_set_tx_freq2(phy_transmission_ctx->rf, tx_channel_center_freq, lo_offset, phy_transmission_ctx->phy_id);
#endif
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(actual_tx_freq < (tx_channel_center_freq - 10.0) || actual_tx_freq > (tx_channel_center_freq + 10.0)) {
    PHY_TX_ERROR("[Initialization] PHY ID: %d - Requested freq.: %1.2f [MHz] - Actual freq.: %1.2f [MHz]\n", phy_transmission_ctx->phy_id, tx_channel_center_freq/1000000.0, actual_tx_freq/1000000.0);
    return -1;
  }
  // Update central frequency.
  phy_transmission_ctx->tx_channel_center_frequency = actual_tx_freq;
  PHY_TX_PRINT("PHY ID: %d - Set Tx gain to: %.1f dB\n", phy_transmission_ctx->phy_id, current_tx_gain);
  PHY_TX_PRINT("PHY ID: %d - Set initial Tx freq to: %.2f [MHz] with offset of: %.2f [MHz]\n", phy_transmission_ctx->phy_id, (actual_tx_freq/1000000.0),(lo_offset/1000000.0));
  // Everything went well.
  return 0;
}

// Functions to transfer basic control message from main thread to transmission thread.
void phy_transmission_push_tx_basic_control_into_container(basic_ctrl_t* const basic_ctrl) {
 // Lock mutex so that we can push basic control to container.
 pthread_mutex_lock(&phy_tx_threads[basic_ctrl->phy_id]->tx_basic_control_mutex);
 // Push basic control into container.
 tx_cb_push_back(phy_tx_threads[basic_ctrl->phy_id]->tx_basic_control_handle, basic_ctrl);
 // Unlock mutex so that function can do other things.
 pthread_mutex_unlock(&phy_tx_threads[basic_ctrl->phy_id]->tx_basic_control_mutex);
 // Notify other thread that basic control was pushed into container.
 pthread_cond_signal(&phy_tx_threads[basic_ctrl->phy_id]->tx_basic_control_cv);
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool phy_transmission_timedwait_and_pop_tx_basic_control_from_container(phy_transmission_t* const phy_transmission_ctx, basic_ctrl_t* const basic_ctrl) {
  bool ret = true, is_cb_empty = true;
  struct timespec timeout;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_basic_control_mutex);
  // Wait for conditional variable only if container is empty.
  if(tx_cb_empty(phy_transmission_ctx->tx_basic_control_handle)) {
    do {
      // Timeout in 0.1 ms.
      helpers_get_timeout(10000, &timeout);
      // Timed wait for conditional variable to be true.
      pthread_cond_timedwait(&phy_transmission_ctx->tx_basic_control_cv, &phy_transmission_ctx->tx_basic_control_mutex, &timeout);
      // Check status of the circular buffer again.
      is_cb_empty = tx_cb_empty(phy_transmission_ctx->tx_basic_control_handle);
      // Check if the threads are still running, if not, then leave with false.
      if(!phy_transmission_ctx->run_tx_encoding_thread) {
        ret = false;
      }
    } while(is_cb_empty && ret);
  }

  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve mapped element from container.
    tx_cb_front(phy_transmission_ctx->tx_basic_control_handle, basic_ctrl);
    tx_cb_pop_front(phy_transmission_ctx->tx_basic_control_handle);
  }
  // Unlock mutex.
  pthread_mutex_unlock(& phy_transmission_ctx->tx_basic_control_mutex);
  return ret;
}

void phy_transmission_set_competiton_center_freq(phy_transmission_t* const phy_transmission_ctx, double competition_center_freq) {
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  phy_transmission_ctx->competition_center_freq = competition_center_freq;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
}

double phy_transmission_get_competiton_center_freq(phy_transmission_t* const phy_transmission_ctx) {
  double competition_center_freq;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  competition_center_freq = phy_transmission_ctx->competition_center_freq;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
  return competition_center_freq;
}

void phy_transmission_set_competiton_bw(phy_transmission_t* const phy_transmission_ctx, double competition_bw) {
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  phy_transmission_ctx->competition_bw = competition_bw;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
}

double phy_transmission_get_competiton_bw(phy_transmission_t* const phy_transmission_ctx) {
  double competition_bw;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  competition_bw = phy_transmission_ctx->competition_bw;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
  return competition_bw;
}

void phy_transmission_set_env_update(phy_transmission_t* const phy_transmission_ctx, bool env_update) {
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  phy_transmission_ctx->env_update = env_update;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
}

bool phy_transmission_get_env_update(phy_transmission_t* const phy_transmission_ctx) {
  bool env_update;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  env_update = phy_transmission_ctx->env_update;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
  return env_update;
}

void phy_transmission_set_env_update_comp_freq(phy_transmission_t* const phy_transmission_ctx, bool competition_freq_updated) {
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  phy_transmission_ctx->competition_freq_updated = competition_freq_updated;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
}

bool phy_transmission_get_env_update_comp_freq(phy_transmission_t* const phy_transmission_ctx) {
  bool competition_freq_updated;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  competition_freq_updated = phy_transmission_ctx->competition_freq_updated;
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
  return competition_freq_updated;
}

double phy_transmission_calculate_channel_center_frequency(phy_transmission_t* const phy_transmission_ctx, float tx_bandwidth, uint32_t tx_channel) {
  double tx_channel_center_freq;
  // Lock mutex.
  pthread_mutex_lock(&phy_transmission_ctx->tx_env_update_mutex);
  // Calculate central Tx frequency for the channels.
  tx_channel_center_freq = helpers_calculate_channel_center_frequency(phy_transmission_ctx->competition_center_freq, phy_transmission_ctx->competition_bw, tx_bandwidth, tx_channel);
  // Unlock mutex.
  pthread_mutex_unlock(&phy_transmission_ctx->tx_env_update_mutex);
  return tx_channel_center_freq;
}

void phy_transmission_update_environment(uint32_t phy_id, environment_t* const env_update) {
  bool parameter_updated = false;
  // Change center frequency if different from previous one.
  if(env_update->competition_center_freq != phy_transmission_get_competiton_center_freq(phy_tx_threads[phy_id])) {
    if(env_update->competition_center_freq > 0) {
      double last_value = phy_transmission_get_competiton_center_freq(phy_tx_threads[phy_id]);
      // Set new competition center frequency.
      phy_transmission_set_competiton_center_freq(phy_tx_threads[phy_id], env_update->competition_center_freq);
      parameter_updated = true;
      // Set flag used to inform change_parameters function that competiton center frequency has been changed.
      phy_transmission_set_env_update_comp_freq(phy_tx_threads[phy_id], parameter_updated);
      PHY_TX_PRINT("PHY ID: %d - Competition Center Frequency changed from: %1.2f [MHz] to: %1.2f [MHz]\n", phy_id, last_value/1000000.0, env_update->competition_center_freq/1000000.0);
    } else {
      PHY_TX_ERROR("The json file contained the following center frequency: %d\n", env_update->competition_center_freq);
    }
  }
  // Change scenario BW if different from previous one.
  if(env_update->competition_bw == 10000000 || (env_update->competition_bw != phy_transmission_get_competiton_bw(phy_tx_threads[phy_id]))) {
    if(env_update->competition_bw > 0) {
      double last_value = phy_transmission_get_competiton_bw(phy_tx_threads[phy_id]);
      // Set new competition BW.
      phy_transmission_set_competiton_bw(phy_tx_threads[phy_id], env_update->competition_bw);
      parameter_updated = true;
      PHY_TX_PRINT("PHY ID: %d - Competition BW changed from: %1.2f [MHz] to: %1.2f [MHz]\n", phy_id, last_value/1000000.0, env_update->competition_bw/1000000.0);
    } else {
      PHY_TX_ERROR("The json file contained the following competiton bw: %d\n", env_update->competition_bw);
    }
  }
  // Update flag indicating at least one of the environment parameters has been changed.
  phy_transmission_set_env_update(phy_tx_threads[phy_id], parameter_updated);
}

timer_t* phy_transmission_get_timer_id(uint32_t phy_id) {
  return &phy_tx_threads[phy_id]->tx_thread_timer_id;
}

// Retrieve Transport Block Size size.
uint32_t phy_transmission_get_tb_size(uint32_t bw_idx, uint32_t mcs) {
  return srslte_ra_get_tb_size_scatter(bw_idx, mcs);
}
