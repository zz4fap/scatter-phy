#include "communicator_wrapper.h"

using namespace SafeQueue;

// ************ Definition of global variables ************
static communicator_t* communicator_handle = NULL;

// ************ Definition of functions ************
int communicator_initialization(char* module_name, char* target1_name, char* target2_name, LayerCommunicator_handle* const handle, uint32_t nof_radios, char* const env_update_pathname) {
  // Allocate memory for a communicator object;
  communicator_handle = (communicator_t*)communicator_vec_malloc(sizeof(communicator_t));
  if(communicator_handle == NULL) {
    std::cerr << "[COMM ERROR] Error when allocating memory for communicator." << std::endl;
    return -1;
  }
  // Set circular buffer counter to zero.
  communicator_handle->user_data_buffer_cnt = 0;
  // Set the number of radios being used.
  communicator_handle->nof_radios           = nof_radios;
  // Allocate memory for environment path name.
  communicator_handle->env_pathname         = (char*)communicator_vec_malloc(sizeof(char)*500);
  if(communicator_handle->env_pathname == NULL) {
    std::cerr << "[COMM ERROR] Error when allocating memory for env_pathname." << std::endl;
    return -1;
  }
  // Copy environment path name to context.
  if(env_update_pathname != NULL) {
    strcpy(communicator_handle->env_pathname, env_update_pathname);
  } else {
    strcpy(communicator_handle->env_pathname, DEFAULT_ENV_PATHNAME);
  }
  std::cout << "[COMM INFO] Environment path set to " << communicator_handle->env_pathname << std::endl;
  // Make sure user data buffer is all set to NULL.
  communicator_set_user_data_buffer_to_null();
  // Check if needs to free the buffer.
  communicator_free_user_data_buffer();
  // Allocate memory to store user data sent by upper layers.
  if(communicator_allocate_user_data_buffer() < 0) {
    return -1;
  }
  // Instantiate communicator module so that we can receive/transmit commands and data.
  if(communicator_make(module_name, target1_name, target2_name, handle) < 0) {
    return -1;
  }
  // Everyhting went well.
  return 0;
}

int communicator_uninitialization(LayerCommunicator_handle* const handle) {
  // Free user data vector.
  communicator_free_user_data_buffer();
  // After use, communicator handle MUST be freed.
  communicator_free(handle);
  // Free memory used to store environment pathname string.
  if(communicator_handle->env_pathname) {
    free(communicator_handle->env_pathname);
    communicator_handle->env_pathname = NULL;
  }
  // Free memory used to store communicator object.
  if(communicator_handle) {
    free(communicator_handle);
    communicator_handle = NULL;
  }
  // Everyhting went well.
  return 0;
}

int communicator_make(char* module_name, char* target1_name, char* target2_name, LayerCommunicator_handle* handle) {
  communicator::MODULE m = communicator::MODULE_UNKNOWN;
  communicator::MODULE t1 = communicator::MODULE_UNKNOWN;
  communicator::MODULE t2 = communicator::MODULE_UNKNOWN;

  std::string m_str(module_name);
  bool success = true;
  success = success && MODULE_Parse(m_str, &m);

  if(target1_name != NULL && target2_name == NULL) {
    std::string t1_str(target1_name);
    success = success && MODULE_Parse(t1_str, &t1);
    if(success && m != communicator::MODULE_UNKNOWN && t1 != communicator::MODULE_UNKNOWN) {
      *handle = new layer_communicator_t;
      (*handle)->layer_communicator_cpp = new communicator::DefaultLayerCommunicator(m, {t1});
    } else {
      std::cerr << "[COMM ERROR] This should not have happened, invalid modules for communicator_make" << std::endl;
    }
  } else if(target1_name != NULL && target2_name != NULL) {
    std::string t1_str(target1_name);
    success = success && MODULE_Parse(t1_str, &t1);
    std::string t2_str(target2_name);
    success = success && MODULE_Parse(t2_str, &t2);
    if(success && m != communicator::MODULE_UNKNOWN && t1 != communicator::MODULE_UNKNOWN && t2 != communicator::MODULE_UNKNOWN) {
      *handle = new layer_communicator_t;
      (*handle)->layer_communicator_cpp = new communicator::DefaultLayerCommunicator(m, {t1, t2});
    } else {
      std::cerr << "[COMM ERROR] This should not have happened, invalid modules for communicator_make" << std::endl;
    }
  } else {
    std::cerr << "[COMM ERROR] Invalid number of target names." << std::endl;
    return -1;
  }
  // Everything went well.
  return 0;
}

int communicator_send_phy_stat_message(LayerCommunicator_handle handle, stat_e type, phy_stat_t *phy_stats, message_handle *msg_handle) {
  return communicator_send_phy_stat_msg_dest(handle, handle->layer_communicator_cpp->getDestinationModule(), type, phy_stats, msg_handle);
}

// PHY sends RX statistics to other layers along with received data if available.
// ENUM RX_STAT, TX_STAT
int communicator_send_phy_stat_msg_dest(LayerCommunicator_handle handle, int destination, stat_e type, phy_stat_t *phy_stats, message_handle *msg_handle) {

  // Check if handle is not NULL.
  if(handle==NULL || handle->layer_communicator_cpp==NULL) {
    std::cout << "[COMM ERROR] Communicator Handle is NULL." << std::endl;
    return -1;
  }
  // Check if PHY stats is not NULL.
  if(phy_stats==NULL) {
    std::cout << "[COMM ERROR] PHY stats is NULL." << std::endl;
    return -1;
  }

  // Create a Internal Message.
  std::shared_ptr<communicator::Internal> internal = std::make_shared<communicator::Internal>();

  // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  internal->set_transaction_index(phy_stats->seq_number); // Transaction index is the same as sequence number.

  // Create PHY stat message.
  communicator::Phy_stat* stat = new communicator::Phy_stat();

  // Set statistics common for both RX and TX.
  stat->set_phy_id(phy_stats->phy_id);
  stat->set_host_timestamp(phy_stats->host_timestamp);
  stat->set_fpga_timestamp(phy_stats->fpga_timestamp);
	stat->set_frame(phy_stats->frame);
	stat->set_ch(phy_stats->ch);
	stat->set_slot(phy_stats->slot);
	stat->set_mcs(phy_stats->mcs);
	stat->set_num_cb_total(phy_stats->num_cb_total);
	stat->set_num_cb_err(phy_stats->num_cb_err);
  stat->set_wrong_decoding_counter(phy_stats->wrong_decoding_counter);
  // Set the corresponding statistics for RX or TX.
  if(type == RX_STAT) {
    // Create a Receive_r message.
    communicator::Receive_r *receive_r = new communicator::Receive_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    receive_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat RX message
    communicator::Phy_rx_stat *stat_rx = new communicator::Phy_rx_stat();
    stat_rx->set_nof_slots_in_frame(phy_stats->stat.rx_stat.nof_slots_in_frame);
    stat_rx->set_slot_counter(phy_stats->stat.rx_stat.slot_counter);
    stat_rx->set_gain(phy_stats->stat.rx_stat.gain);
    stat_rx->set_cqi(phy_stats->stat.rx_stat.cqi);
    stat_rx->set_rssi(phy_stats->stat.rx_stat.rssi);
    stat_rx->set_rsrp(phy_stats->stat.rx_stat.rsrp);
    stat_rx->set_rsrq(phy_stats->stat.rx_stat.rsrq);
    stat_rx->set_sinr(phy_stats->stat.rx_stat.sinr);
    stat_rx->set_cfo(phy_stats->stat.rx_stat.cfo);
    stat_rx->set_detection_errors(phy_stats->stat.rx_stat.detection_errors);
    stat_rx->set_decoding_errors(phy_stats->stat.rx_stat.decoding_errors);
    stat_rx->set_peak_value(phy_stats->stat.rx_stat.peak_value);
    stat_rx->set_noise(phy_stats->stat.rx_stat.noise);
    stat_rx->set_decoded_cfi(phy_stats->stat.rx_stat.decoded_cfi);
    stat_rx->set_found_dci(phy_stats->stat.rx_stat.found_dci);
    stat_rx->set_last_noi(phy_stats->stat.rx_stat.last_noi);
    stat_rx->set_total_packets_synchronized(phy_stats->stat.rx_stat.total_packets_synchronized);
    stat_rx->set_decoding_time(phy_stats->stat.rx_stat.decoding_time);
    stat_rx->set_synch_plus_decoding_time(phy_stats->stat.rx_stat.synch_plus_decoding_time);
    stat_rx->set_length(phy_stats->stat.rx_stat.length);
    if(phy_stats->stat.rx_stat.data != NULL) {
      try {
        // Check if length is greater than 0.
        if(phy_stats->stat.rx_stat.length <= 0) {
          std::cout << "[COMM ERROR] Data length is incorrect: " << phy_stats->stat.rx_stat.length << " !" << std::endl;
          return -1;
        }
        // Instantiate string with data.
        std::string data_str((char*)phy_stats->stat.rx_stat.data, phy_stats->stat.rx_stat.length);
        receive_r->set_data(data_str);
      } catch(const std::length_error &e) {
        std::cout << "[COMM ERROR] Exception Caught " << e.what() << std::endl;
        std::cout << "[COMM ERROR] Exception Type " << typeid(e).name() << std::endl;
        return -1;
      }
    } else {
      if(phy_stats->status == PHY_SUCCESS) {
        std::cout << "[COMM WARNING] PHY status is success but received data is NULL..." << std::endl;
		  }
    }
    // Add PHY RX Stats to PHY Stat.
    stat->set_allocated_rx_stat(stat_rx);
    // Add PHY stat to Send_r message.
    receive_r->set_allocated_stat(stat);	// receive_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_receiver(receive_r); // internal has ownership over the receive_r pointer
  } else if(type == TX_STAT) {
    // Create a Send_r message.
    communicator::Send_r *send_r = new communicator::Send_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    send_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat TX message
    communicator::Phy_tx_stat* stat_tx = new communicator::Phy_tx_stat();
    stat_tx->set_power(phy_stats->stat.tx_stat.power);
    stat_tx->set_channel_free_cnt(phy_stats->stat.tx_stat.channel_free_cnt);
    stat_tx->set_channel_busy_cnt(phy_stats->stat.tx_stat.channel_busy_cnt);
    stat_tx->set_free_energy(phy_stats->stat.tx_stat.free_energy);
    stat_tx->set_busy_energy(phy_stats->stat.tx_stat.busy_energy);
    stat_tx->set_total_dropped_slots(phy_stats->stat.tx_stat.total_dropped_slots);
    stat_tx->set_coding_time(phy_stats->stat.tx_stat.coding_time);
    stat_tx->set_rf_boost(phy_stats->stat.tx_stat.rf_boost);
    stat->set_allocated_tx_stat(stat_tx);
    // Add PHY stat to Send_r message.
    send_r->set_allocated_phy_stat(stat);	// send_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_sendr(send_r); // internal has ownership over the send_r pointer
  } else if(type == SENSING_STAT) {
    // Create a Receive_r message.
    communicator::Receive_r *receive_r = new communicator::Receive_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    receive_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat RX message
    communicator::Phy_sensing_stat *stat_sensing = new communicator::Phy_sensing_stat();
    stat_sensing->set_frequency(phy_stats->stat.sensing_stat.frequency);
    stat_sensing->set_sample_rate(phy_stats->stat.sensing_stat.sample_rate);
    stat_sensing->set_gain(phy_stats->stat.sensing_stat.gain);
    stat_sensing->set_rssi(phy_stats->stat.sensing_stat.rssi);
    stat_sensing->set_length(phy_stats->stat.sensing_stat.length);
    if(phy_stats->stat.sensing_stat.data != NULL) {
      try {
        // Check if length is greater than 0.
        if(phy_stats->stat.sensing_stat.length <= 0) {
          std::cout << "[COMM ERROR] Data length is incorrect: " << phy_stats->stat.sensing_stat.length << " !" << std::endl;
          return -1;
        }
        // Instantiate string with data.
        std::string data_str((char*)phy_stats->stat.sensing_stat.data, phy_stats->stat.sensing_stat.length);
        receive_r->set_data(data_str);
      } catch(const std::length_error &e) {
        std::cout << "[COMM ERROR] Exception Caught " << e.what() << std::endl;
        std::cout << "[COMM ERROR] Exception Type " << typeid(e).name() << std::endl;
        return -1;
      }
    } else {
      if(phy_stats->status == PHY_SUCCESS) {
        std::cout << "[COMM ERROR] PHY status is success but sensing data is NULL..." << std::endl;
		  }
    }
    // Add PHY Sensing Stats to PHY Stat.
    stat->set_allocated_sensing_stat(stat_sensing);
    // Add PHY stat to Send_r message.
    receive_r->set_allocated_stat(stat);	// receive_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_receiver(receive_r); // internal has ownership over the receive_r pointer
  } else {
    std::cout << "[COMM ERROR] Undefined type of statistics." << std::endl;
    return -1;
  }

  // Create a Message object what will be sent upwards.
  communicator::Message msg(communicator::MODULE_PHY, (communicator::MODULE)destination, internal);

  // Put message in the sensing queue.
  handle->layer_communicator_cpp->send(msg);

#if(CHECK_COMM_OUT_OF_SEQUENCE_RX==1)
  static uint8_t expected_rx_byte = 1;
  std::string data_str = internal->receiver().data().c_str();
  if(data_str[10] != expected_rx_byte) {
    std::cout << "[COMM] Out of sequence message Rx: Received: " << int(data_str[10]) << " - Expected: " << int(expected_rx_byte) << std::endl;
    expected_rx_byte = data_str[10];
  }
  expected_rx_byte = (expected_rx_byte + 1);
  if(expected_rx_byte == 101) {
    expected_rx_byte = 1;
  }
#endif

  // Add a Message object only if different from NULL.
  if(msg_handle != NULL) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
  // Everything went well.
  return 0;
}

// This function will retrieve messages (basic_control) addressed to the PHY from the QUEUE.
// OBS.: It is a blocking call.
// MUST cast to basic_ctrl_t *
int communicator_get_high_queue_wait(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle) {

  //WAIT UNTIL THERE IS A MESSAGE IN THE HIGH QUEUE
  communicator::Message msg = handle->layer_communicator_cpp->get_high_queue().pop_wait();
  // Parse message into the corresponding structure.
  int ret = parse_received_message(msg, msg_struct);

  // Add a Message object only if diffrent from NULL.
  if(msg_handle != NULL && ret >= 0) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
  return ret;
}

// Blocking call, it waits until someone pushes a message into the QUEUE or the waiting times out.
bool communicator_get_high_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle) {
  // Wait until there is a message in the high priority QUEUE.
  communicator::Message msg;
  bool ret = handle->layer_communicator_cpp->get_high_queue().pop_wait_for(std::chrono::microseconds(timeout), msg);

  if(ret) {
    // Parse message into the corresponding structure.
    if(parse_received_message(msg, msg_struct) < 0) {
      ret = false;
    }

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL && ret == true) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  }
  return ret;
}

// Non-blocking call.
bool communicator_get_high_queue(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle) {
  bool ret;
  communicator::Message msg;
  if(handle->layer_communicator_cpp->get_high_queue().pop(msg)) {
    ret = true;
    // Parse message into the corresponding structure.
    if(parse_received_message(msg, msg_struct) < 0) {
      ret = false;
    }

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL && ret == true) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  } else {
    msg_struct = NULL;  // Make sure it returns NULL for the caller to check before using it.
    msg_handle = NULL;  // Make sure it returns NULL for the caller to check before using it.
    ret = false;
  }

  return ret;
}

// Blocking call.
int communicator_get_low_queue_wait(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle) {

  //WAIT UNTIL THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg = handle->layer_communicator_cpp->get_low_queue().pop_wait();
  // Parse message into the corresponding structure.
  int ret = parse_received_message(msg, msg_struct);

  // Add a Message object only if diffrent from NULL.
  if(msg_handle != NULL && ret >= 0) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
  return ret;
}

// Blocking call, it waits until someone pushes a message into the QUEUE or the waiting times out.
bool communicator_get_low_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle) {
  //WAIT UNTIL THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg;
  bool ret = handle->layer_communicator_cpp->get_low_queue().pop_wait_for(std::chrono::microseconds(timeout), msg);

  if(ret) {
    // Parse message into the corresponding structure.
    if(parse_received_message(msg, msg_struct) < 0) {
      ret = false;
    }

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL && ret == true) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  }
  return ret;
}

// Non-blocking call.
bool communicator_get_low_queue(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle) {
  bool ret;
  //CHECK IF THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg;

  if(handle->layer_communicator_cpp->get_low_queue().pop(msg)) {
    ret = true;
    // Parse message into the corresponding structure.
    if(parse_received_message(msg, msg_struct) < 0) {
      ret = false;
    }

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL && ret == true) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  } else {
    msg_struct = NULL; // Make sure it returns NULL for the caller to check before using it.
    msg_handle = NULL; // Make sure it returns NULL for the caller to check before using it.
    ret = false;
  }

  return ret;
}

void communicator_print_message(message_handle msg_handle) {
  // Print the Message.
  std::cout << "[COMM PRINT] Get message " << msg_handle->message_cpp << std::endl;
}

void communicator_free(LayerCommunicator_handle* handle) {
  communicator::DefaultLayerCommunicator *ptr = ((*handle)->layer_communicator_cpp);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

void communicator_free_msg(message_handle *msg_handle) {
  delete *msg_handle;
  *msg_handle = NULL;
}

int parse_received_message(communicator::Message msg, void *msg_struct) {

  if(msg_struct==NULL) {
    std::cout << "[COMM ERROR] Message struct is NULL." << std::endl;
    return -1;
  }

  // Check if message was addressed to PHY.
  if(msg.destination == communicator::MODULE::MODULE_PHY) {

    // Cast protobuf message into a Internal message one.
    std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(msg.message);

#if(ENBALE_0MQ_PROFILLING==1)
    uint64_t bc_timestamp_start = communicator_get_host_time_now();
    uint64_t bc_timestamp_end = (uint64_t)internal->receive().basic_ctrl().timestamp();
    if(bc_timestamp_end > 0) {
      int zmq_tdiff = (int)(bc_timestamp_end - bc_timestamp_start);
      if(zmq_tdiff < 2000) {
        std::cout << "[COMM ERROR] 0MQ time diff at COMM: " << zmq_tdiff << std::endl;
      }
    }
#endif

    // Switch case used to select among different messages.
    switch(internal->payload_case()) {
      case communicator::Internal::kReceive:
      {
        // Cast msg_struct to be a basic control struct.
        basic_ctrl_t *basic_ctrl = (basic_ctrl_t *)msg_struct;
        // Set sequence number.
        basic_ctrl->seq_number = internal->transaction_index();
        // Copy from Internal message values into basic control structure.
        basic_ctrl->trx_flag = (trx_flag_e)internal->receive().basic_ctrl().trx_flag();
        // Check if the flag really is RX.
        if(basic_ctrl->trx_flag != PHY_RX_ST) {
          std::cout << "[COMM ERROR] TRX Flag different from RX!!" << std::endl;
          return -1;
        }
        // Populate basic control structure.
        basic_ctrl->phy_id = internal->receive().basic_ctrl().phy_id();
        // Check if PHY ID is valid.
        if(basic_ctrl->phy_id >= communicator_handle->nof_radios) {
          std::cout << "[COMM ERROR] Invalid PHY ID: " << basic_ctrl->phy_id << ". It must be less then " << communicator_handle->nof_radios << std::endl;
          return -1;
        }
        basic_ctrl->send_to   = internal->receive().basic_ctrl().send_to();
        basic_ctrl->intf_id   = internal->receive().basic_ctrl().intf_id();
        basic_ctrl->bw_idx    = internal->receive().basic_ctrl().bw_index();
        basic_ctrl->ch        = internal->receive().basic_ctrl().ch();
        basic_ctrl->frame     = internal->receive().basic_ctrl().frame();
        basic_ctrl->slot      = internal->receive().basic_ctrl().slot();
        basic_ctrl->timestamp = internal->receive().basic_ctrl().timestamp();
        basic_ctrl->mcs       = internal->receive().basic_ctrl().mcs();
        basic_ctrl->gain      = internal->receive().basic_ctrl().gain();
        basic_ctrl->rf_boost  = internal->receive().basic_ctrl().rf_boost();
        basic_ctrl->length    = internal->receive().basic_ctrl().length();
        break;
      }
      case communicator::Internal::kSend:
      {
        // Cast msg_struct to be a basic control struct.
        basic_ctrl_t *basic_ctrl = (basic_ctrl_t *)msg_struct;
        // Set sequence number.
        basic_ctrl->seq_number  = internal->transaction_index();
        // Copy from Internal message values into basic control structure.
        basic_ctrl->trx_flag    = (trx_flag_e)internal->send().basic_ctrl().trx_flag();
        // Check if the flag really is TX.
        if(basic_ctrl->trx_flag != PHY_TX_ST) {
          std::cout << "[COMM ERROR] TRX Flag different from TX!!" << std::endl;
          return -1;
        }
        // Populate basic control structure.
        basic_ctrl->phy_id = internal->send().basic_ctrl().phy_id();
        // Check if PHY ID is valid.
        if(basic_ctrl->phy_id >= communicator_handle->nof_radios) {
          std::cout << "[COMM ERROR] Invalid PHY ID: " << basic_ctrl->phy_id << ". It must be less then " << communicator_handle->nof_radios << std::endl;
          return -1;
        }
        basic_ctrl->send_to   = internal->send().basic_ctrl().send_to();
        basic_ctrl->intf_id   = internal->send().basic_ctrl().intf_id();
        basic_ctrl->bw_idx    = internal->send().basic_ctrl().bw_index();
        basic_ctrl->ch        = internal->send().basic_ctrl().ch();
        basic_ctrl->frame     = internal->send().basic_ctrl().frame();
        basic_ctrl->slot      = internal->send().basic_ctrl().slot();
        basic_ctrl->timestamp = internal->send().basic_ctrl().timestamp();
        basic_ctrl->mcs       = internal->send().basic_ctrl().mcs();
        basic_ctrl->gain      = internal->send().basic_ctrl().gain();
        basic_ctrl->rf_boost  = internal->send().basic_ctrl().rf_boost();
        basic_ctrl->length    = internal->send().basic_ctrl().length();
        // Only do basic checking. Other checking is done by PHY.
        if(basic_ctrl->length <= 0) {
          std::cout << "[COMM ERROR] Invalid Basic control length field: " << basic_ctrl->length << std::endl;
          return -1;
        }
        // Only allocate memory for data if it is a basic_control for TX processing.
        size_t data_length = internal->send().app_data().data().length();
        // The data length field should have the same value as the basic control length.
        if(data_length != basic_ctrl->length) {
          std::cout << "[COMM ERROR] Basic control length: " << basic_ctrl->length << " does not match data length: " << data_length << std::endl;
          return -1;
        }
        // Check if the size of the frame sent by MAC fits into the allocated memory.
        if(data_length > USER_DATA_BUFFER_LEN) {
          std::cout << "[COMM ERROR] Basic control length: " << data_length << " is greater than the allocated buffers: " << USER_DATA_BUFFER_LEN << std::endl;
          return -1;
        }
        // Copy received user data into user data buffer.
        memcpy(communicator_handle->user_data_buffer[communicator_handle->user_data_buffer_cnt], (unsigned char*)internal->send().app_data().data().c_str(), data_length);
        basic_ctrl->data = communicator_handle->user_data_buffer[communicator_handle->user_data_buffer_cnt];
        // Increment the counter used in the circular user data buffer.
        communicator_handle->user_data_buffer_cnt = (communicator_handle->user_data_buffer_cnt + 1)%NUMBER_OF_USER_DATA_BUFFERS;
        break;
      }
      case communicator::Internal::kSet:
      {
        // Cast msg_struct to be a environment change struct.
        environment_t *env = (environment_t *)msg_struct;
        // Set flag to false in order to make sure if nothing is updated, then no configuration takes place.
        env->environment_updated = false;
        // Parse environment updated parameters.
        if(internal->set().environment_updated() == true) {
          // Instantiate environment update object.
          Envupdates env_updates;
          // Try to parse the json file.
          env_updates.read_envupdates(communicator_handle->env_pathname);
          // If any update, then set the PHY structure.
          if(env_updates.has_envupdates()) {
            env->environment_updated      = true;
            env->competition_center_freq  = (double)env_updates.get_envupdate().scenario_center_frequency;
            env->competition_bw           = (double)env_updates.get_envupdate().scenario_rf_bandwidth;
          } else {
            std::cout << "[COMM ERROR] Error parsing the environment.json file." << std::endl;
          }
        } else {
          std::cout << "[COMM INFO] environment_updated flag set to False, parameters won't be changed." << std::endl;
        }
        break;
      }
      case communicator::Internal::kSetr:
      case communicator::Internal::kReceiver:
      case communicator::Internal::kStats:
      case communicator::Internal::kGet:
      case communicator::Internal::kGetr:
      case communicator::Internal::kSendr:
      default:
        std::cout << "[COMM ERROR] This message type is not handled by PHY!!" << std::endl;
        break;
    }
  } else if(msg.destination == communicator::MODULE::MODULE_MAC) {
    // Cast msg_struct to be a phy stat struct.
    phy_stat_t *phy_rx_stat = (phy_stat_t *)msg_struct;
    // Cast protobuf message into a Internal message one.
    std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(msg.message);
    // Parse accordind to the specific flag.
    switch(internal->payload_case()) {
      case communicator::Internal::kReceiver:
      {
        phy_rx_stat->seq_number                               = internal->transaction_index();
        phy_rx_stat->status                                   = internal->receiver().result();
        phy_rx_stat->phy_id                                   = internal->receiver().stat().phy_id();
        phy_rx_stat->host_timestamp                           = internal->receiver().stat().host_timestamp();
        phy_rx_stat->fpga_timestamp                           = internal->receiver().stat().fpga_timestamp();
        phy_rx_stat->frame                                    = internal->receiver().stat().frame();
        phy_rx_stat->slot                                     = internal->receiver().stat().slot();
        phy_rx_stat->ch                                       = internal->receiver().stat().ch();
        phy_rx_stat->mcs                                      = internal->receiver().stat().mcs();
        phy_rx_stat->num_cb_total                             = internal->receiver().stat().num_cb_total();
        phy_rx_stat->num_cb_err                               = internal->receiver().stat().num_cb_err();
        phy_rx_stat->wrong_decoding_counter                   = internal->receiver().stat().wrong_decoding_counter();
        phy_rx_stat->stat.rx_stat.nof_slots_in_frame          = internal->receiver().stat().rx_stat().nof_slots_in_frame();
        phy_rx_stat->stat.rx_stat.slot_counter                = internal->receiver().stat().rx_stat().slot_counter();
        phy_rx_stat->stat.rx_stat.gain                        = internal->receiver().stat().rx_stat().gain();
        phy_rx_stat->stat.rx_stat.cqi                         = internal->receiver().stat().rx_stat().cqi();
        phy_rx_stat->stat.rx_stat.rssi                        = internal->receiver().stat().rx_stat().rssi();
        phy_rx_stat->stat.rx_stat.rsrp                        = internal->receiver().stat().rx_stat().rsrp();
        phy_rx_stat->stat.rx_stat.rsrq                        = internal->receiver().stat().rx_stat().rsrq();
        phy_rx_stat->stat.rx_stat.sinr                        = internal->receiver().stat().rx_stat().sinr();
        phy_rx_stat->stat.rx_stat.detection_errors            = internal->receiver().stat().rx_stat().detection_errors();
        phy_rx_stat->stat.rx_stat.decoding_errors             = internal->receiver().stat().rx_stat().decoding_errors();
        phy_rx_stat->stat.rx_stat.cfo                         = internal->receiver().stat().rx_stat().cfo();
        phy_rx_stat->stat.rx_stat.peak_value                  = internal->receiver().stat().rx_stat().peak_value();
        phy_rx_stat->stat.rx_stat.noise                       = internal->receiver().stat().rx_stat().noise();
        phy_rx_stat->stat.rx_stat.decoded_cfi                 = internal->receiver().stat().rx_stat().decoded_cfi();
        phy_rx_stat->stat.rx_stat.found_dci                   = internal->receiver().stat().rx_stat().found_dci();
        phy_rx_stat->stat.rx_stat.last_noi                    = internal->receiver().stat().rx_stat().last_noi();
        phy_rx_stat->stat.rx_stat.total_packets_synchronized  = internal->receiver().stat().rx_stat().total_packets_synchronized();
        phy_rx_stat->stat.rx_stat.decoding_time               = internal->receiver().stat().rx_stat().decoding_time();
        phy_rx_stat->stat.rx_stat.synch_plus_decoding_time    = internal->receiver().stat().rx_stat().synch_plus_decoding_time();
        phy_rx_stat->stat.rx_stat.length                      = internal->receiver().stat().rx_stat().length();
        if(phy_rx_stat->status == PHY_SUCCESS) {
          if(phy_rx_stat->stat.rx_stat.length > 0 && phy_rx_stat->stat.rx_stat.data != NULL) {
            memcpy(phy_rx_stat->stat.rx_stat.data, internal->receiver().data().c_str(), internal->receiver().data().length());
          } else {
            std::cout << "[COMM ERROR] Data length less or equal to 0!" << std::endl;
            return -1;
          }
        }
        break;
      }
      default:
        std::cout << "[COMM ERROR] This message type is not handled by MAC!!" << std::endl;
        break;
    }
  } else if(msg.destination == communicator::MODULE::MODULE_CHANNEL_EMULATOR) {
    // Cast msg_struct to be a channel_emulator_config_t struct.
    channel_emulator_config_t *channel_emulator_config = (channel_emulator_config_t *)msg_struct;
    // Cast protobuf message into a Internal message one.
    std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(msg.message);
    // Parse according to the specific flag.
    switch(internal->payload_case()) {
      case communicator::Internal::kReceive:
      {
        if(internal->receive().has_channel_emulator_config()) {
          channel_emulator_config->enable_simple_awgn_channel = internal->receive().channel_emulator_config().enable_simple_awgn_channel();
          channel_emulator_config->noise_variance             = internal->receive().channel_emulator_config().noise_variance();
          channel_emulator_config->snr                        = internal->receive().channel_emulator_config().snr();
        }
        break;
      }
      default:
        std::cout << "[COMM ERROR] This message type is not handled by the Channel Emulator!!" << std::endl;
        break;
    }
  } else {
    std::cout << "[COMM ERROR] Message not addressed to PHY or MAC." << std::endl;
    return -1;
  }
  // Everything went well.
  return 0;
}

bool communicator_verify_environment_update(environment_t *env) {
  // Set flag to false in order to make sure if nothing is updated, then no configuration takes place.
  env->environment_updated = false;
  // Instantiate environment update object.
  Envupdates env_updates;
  // Try to parse the json file.
  env_updates.read_envupdates(communicator_handle->env_pathname);
  // If any update, then set the PHY structure.
  if(env_updates.has_envupdates()) {
    env->environment_updated      = true;
    env->competition_center_freq  = (double)env_updates.get_envupdate().scenario_center_frequency;
    env->competition_bw           = (double)env_updates.get_envupdate().scenario_rf_bandwidth;
  } else {
    std::cout << "[COMM ERROR] Error parsing the environment.json file." << std::endl;
  }

  return env->environment_updated;
}

bool communicator_is_high_queue_empty(LayerCommunicator_handle handle) {
  return handle->layer_communicator_cpp->get_high_queue().empty();
}

bool communicator_is_low_queue_empty(LayerCommunicator_handle handle) {
  return handle->layer_communicator_cpp->get_low_queue().empty();
}

int communicator_send_basic_control(LayerCommunicator_handle handle, basic_ctrl_t* const basic_ctrl) {

  std::shared_ptr<communicator::Internal> message = std::make_shared<communicator::Internal>();
  message->set_transaction_index(basic_ctrl->seq_number);
  message->set_owner_module(communicator::MODULE_MAC);
  communicator::Message mes = communicator::Message(communicator::MODULE_MAC, communicator::MODULE_PHY, message);

  communicator::Basic_ctrl *ctrl = new communicator::Basic_ctrl();

  switch(basic_ctrl->trx_flag) {
    case PHY_TX_ST:
    {
      // Check if MAC frame contains some data at all.
      if(basic_ctrl->length <= 0) {
        std::cout << "[COMM ERROR] MAC Tx message with no data....." << std::endl;
        return -1;
      }

      // Check if the size of the frame sent by MAC fits into the allocated memory.
      if(basic_ctrl->length > USER_DATA_BUFFER_LEN) {
        std::cout << "[COMM ERROR] Basic control length: " << basic_ctrl->length << " is greater than the allocated buffers: " << USER_DATA_BUFFER_LEN << std::endl;
        return -1;
      }

      communicator::Send* send = new communicator::Send();
      mes.message->set_allocated_send(send);

      send->set_allocated_basic_ctrl(ctrl);

      communicator::Application_data *app_data = new communicator::Application_data();
      send->set_allocated_app_data(app_data);

      ctrl->set_trx_flag((communicator::Basic_ctrl_TRX)basic_ctrl->trx_flag);
      ctrl->set_bw_index((communicator::BW_INDEX)basic_ctrl->bw_idx);
      ctrl->set_phy_id(basic_ctrl->phy_id);
      ctrl->set_send_to(basic_ctrl->send_to);
      ctrl->set_intf_id(basic_ctrl->intf_id);
      ctrl->set_ch(basic_ctrl->ch);
      ctrl->set_timestamp(basic_ctrl->timestamp);
      ctrl->set_mcs(basic_ctrl->mcs);
      ctrl->set_rf_boost(basic_ctrl->rf_boost);
      ctrl->set_gain(basic_ctrl->gain);
      ctrl->set_length(basic_ctrl->length);

      std::string *data_str = new std::string((char*)basic_ctrl->data, basic_ctrl->length);
      app_data->set_allocated_data(data_str);

      break;
    }
    case PHY_RX_ST:
    {
      communicator::Receive* receive = new communicator::Receive();
      mes.message->set_allocated_receive(receive);

      receive->set_allocated_basic_ctrl(ctrl);

      ctrl->set_trx_flag((communicator::Basic_ctrl_TRX)basic_ctrl->trx_flag);
      ctrl->set_bw_index((communicator::BW_INDEX)basic_ctrl->bw_idx);
      ctrl->set_phy_id(basic_ctrl->phy_id);
      ctrl->set_send_to(basic_ctrl->send_to);
      ctrl->set_intf_id(basic_ctrl->intf_id);
      ctrl->set_ch(basic_ctrl->ch);
      ctrl->set_timestamp(basic_ctrl->timestamp);
      ctrl->set_mcs(basic_ctrl->mcs);
      ctrl->set_gain(basic_ctrl->gain);
      ctrl->set_rf_boost(basic_ctrl->rf_boost);
      ctrl->set_length(basic_ctrl->length);

      break;
    }
    default:
      std::cout << "[COMM ERROR] Message type not addressed..." << std::endl;
      return -1;
  }
  // Send message to PHY module.
  handle->layer_communicator_cpp->send(mes);
  // Everyhting went well.
  return 0;
}

void communicator_send_channel_emulator_config(LayerCommunicator_handle handle, channel_emulator_config_t* const chan_emu_conf) {

  std::shared_ptr<communicator::Internal> message = std::make_shared<communicator::Internal>();
  message->set_owner_module(communicator::MODULE_MAC);
  communicator::Message mes = communicator::Message(communicator::MODULE_MAC, communicator::MODULE_CHANNEL_EMULATOR, message);

  communicator::Channel_emulator_config *config = new communicator::Channel_emulator_config();

  communicator::Receive* receive = new communicator::Receive();
  mes.message->set_allocated_receive(receive);

  receive->set_allocated_channel_emulator_config(config);
  config->set_enable_simple_awgn_channel(chan_emu_conf->enable_simple_awgn_channel);
  config->set_noise_variance(chan_emu_conf->noise_variance);
  config->set_snr(chan_emu_conf->snr);

  handle->layer_communicator_cpp->send(mes);
}

// Allocate heap memory used to store user data comming from upper layers.
int communicator_allocate_user_data_buffer() {
  for(uint32_t i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    communicator_handle->user_data_buffer[i] = (unsigned char*)communicator_vec_malloc(USER_DATA_BUFFER_LEN);
    if(communicator_handle->user_data_buffer[i] == NULL) {
      std::cout << "[COMM ERROR] User data buffer malloc failed." << std::endl;
      return -1;
    }
  }
  // Everything went well.
  return 0;
}

// Deallocate heap memory used to store user data comming from upper layers.
void communicator_free_user_data_buffer() {
  for(uint32_t i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    if(communicator_handle->user_data_buffer[i] != NULL) {
      free(communicator_handle->user_data_buffer[i]);
      communicator_handle->user_data_buffer[i] = NULL;
    }
  }
}

// make sure the user data buffer is initialized with NULL.
void communicator_set_user_data_buffer_to_null() {
  for(uint32_t i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    if(communicator_handle->user_data_buffer[i] != NULL) {
      communicator_handle->user_data_buffer[i] = NULL;
    }
  }
}

// Note: We align memory to 32 bytes (for AVX compatibility)
// because in some cases volk can incorrectly detect the architecture.
// This could be inefficient for SSE or non-SIMD platforms but shouldn't
// be a huge problem.
void *communicator_vec_malloc(uint32_t size) {
  void *ptr;
  if(posix_memalign(&ptr, 32, size)) {
    return NULL;
  } else {
    return ptr;
  }
}

// Retrieve host PC time value now in microseconds.
uint64_t communicator_get_host_time_now() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)host_timestamp.tv_sec*1000000LL + (uint64_t)(host_timestamp.tv_nsec/1000LL);
}
