#include "srslte/utils/cpp_wrappers.h"

//************************** Sync Circular buffer ********************************
void sync_cb_make(sync_cb_handle* handle, uint64_t size) {
  *handle = new sync_cb_t;
  (*handle)->sync_cb_ptr = new boost::circular_buffer<short_ue_sync_t>(size);
}

// Free all allocated resources.
void sync_cb_free(sync_cb_handle* handle) {
  boost::circular_buffer<short_ue_sync_t> *ptr = ((*handle)->sync_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void sync_cb_push_back(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync) {
  handle->sync_cb_ptr->push_back(*short_ue_sync);
}

// Read element from vector.
void sync_cb_read(sync_cb_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = (*handle->sync_cb_ptr)[index];
}

void sync_cb_front(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = handle->sync_cb_ptr->front();
}

void sync_cb_pop_front(sync_cb_handle handle) {
  handle->sync_cb_ptr->pop_front();
}

bool sync_cb_empty(sync_cb_handle handle) {
  return handle->sync_cb_ptr->empty();
}

int sync_cb_size(sync_cb_handle handle) {
  return static_cast<int>(handle->sync_cb_ptr->size());
}

//************************** Sync vector ********************************
void sync_vector_make(sync_vector_handle* handle) {
  *handle = new sync_vector_t;
  (*handle)->sync_vector_ptr = new std::vector<short_ue_sync_t>;
}

// Free all allocated resources.
void sync_vector_free(sync_vector_handle* handle) {
  std::vector<short_ue_sync_t> *ptr = ((*handle)->sync_vector_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Reserve a number of positions.
void sync_vector_reserve(sync_vector_handle handle, uint64_t size) {
  handle->sync_vector_ptr->reserve(size);
}

// Push ue sync structure into vector.
void sync_vector_push(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  handle->sync_vector_ptr->push_back(*short_ue_sync);
}

// Read element from vector.
void sync_vector_read(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = (*handle->sync_vector_ptr)[index];
}

void sync_vector_pop_back(sync_vector_handle handle) {
  handle->sync_vector_ptr->pop_back();
}

bool sync_vector_empty(sync_vector_handle handle) {
  return handle->sync_vector_ptr->empty();
}

int sync_vector_size(sync_vector_handle handle) {
  return static_cast<int>(handle->sync_vector_ptr->size());
}

//************************** Tx basic control Circular buffer ********************************
void tx_cb_make(tx_cb_handle* handle, uint64_t size) {
  *handle = new tx_cb_t;
  (*handle)->tx_cb_ptr = new boost::circular_buffer<basic_ctrl_t>(size);
}

// Free all allocated resources.
void tx_cb_free(tx_cb_handle* handle) {
  boost::circular_buffer<basic_ctrl_t> *ptr = ((*handle)->tx_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void tx_cb_push_back(tx_cb_handle handle, basic_ctrl_t* const basic_ctrl) {
  handle->tx_cb_ptr->push_back(*basic_ctrl);
}

// Read element from vector.
void tx_cb_read(tx_cb_handle handle, uint64_t index, basic_ctrl_t* const basic_ctrl) {
  *basic_ctrl = (*handle->tx_cb_ptr)[index];
}

void tx_cb_front(tx_cb_handle handle, basic_ctrl_t* const basic_ctrl) {
  *basic_ctrl = handle->tx_cb_ptr->front();
}

void tx_cb_pop_front(tx_cb_handle handle) {
  handle->tx_cb_ptr->pop_front();
}

bool tx_cb_empty(tx_cb_handle handle) {
  return handle->tx_cb_ptr->empty();
}

int tx_cb_size(tx_cb_handle handle) {
  return static_cast<int>(handle->tx_cb_ptr->size());
}

//************************** Rx basic control Circular buffer ********************************
void rx_param_cb_make(rx_param_cb_handle* handle, uint64_t size) {
  *handle = new rx_param_cb_t;
  (*handle)->rx_param_cb_ptr = new boost::circular_buffer<basic_ctrl_t>(size);
}

// Free all allocated resources.
void rx_param_cb_free(rx_param_cb_handle* handle) {
  boost::circular_buffer<basic_ctrl_t> *ptr = ((*handle)->rx_param_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push basic control structure into vector.
void rx_param_cb_push_back(rx_param_cb_handle handle, basic_ctrl_t* const basic_ctrl) {
  handle->rx_param_cb_ptr->push_back(*basic_ctrl);
}

// Read element from vector.
void rx_param_cb_read(rx_param_cb_handle handle, uint64_t index, basic_ctrl_t* const basic_ctrl) {
  *basic_ctrl = (*handle->rx_param_cb_ptr)[index];
}

void rx_param_cb_front(rx_param_cb_handle handle, basic_ctrl_t* const basic_ctrl) {
  *basic_ctrl = handle->rx_param_cb_ptr->front();
}

void rx_param_cb_pop_front(rx_param_cb_handle handle) {
  handle->rx_param_cb_ptr->pop_front();
}

bool rx_param_cb_empty(rx_param_cb_handle handle) {
  return handle->rx_param_cb_ptr->empty();
}

int rx_param_cb_size(rx_param_cb_handle handle) {
  return static_cast<int>(handle->rx_param_cb_ptr->size());
}
