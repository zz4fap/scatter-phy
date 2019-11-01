#ifndef _CPP_WRAPPERS_H_
#define _CPP_WRAPPERS_H_

#ifdef __cplusplus

#include <math.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <queue>
#include <map>
#include <vector>
#include <boost/circular_buffer.hpp>

#include "srslte/ue/ue_sync.h"
#include "../intf/intf.h"

struct sync_vector_t {
  std::vector<short_ue_sync_t>* sync_vector_ptr;
};

struct sync_cb_t {
  boost::circular_buffer<short_ue_sync_t>* sync_cb_ptr;
};

struct tx_cb_t {
  boost::circular_buffer<basic_ctrl_t>* tx_cb_ptr;
};

struct rx_param_cb_t {
  boost::circular_buffer<basic_ctrl_t>* rx_param_cb_ptr;
};

extern "C" {
#else
struct sync_vector_t;
struct sync_cb_t;
struct tx_cb_t;
struct rx_param_cb_t;
#endif

#include <inttypes.h>

#include "srslte/ue/ue_sync.h"
#include "../intf/intf.h"

typedef struct sync_vector_t* sync_vector_handle;

typedef struct sync_cb_t* sync_cb_handle;

typedef struct tx_cb_t* tx_cb_handle;

typedef struct rx_param_cb_t* rx_param_cb_handle;

//******************************************************************************
SRSLTE_API void sync_cb_make(sync_cb_handle* handle, uint64_t size);

SRSLTE_API void sync_cb_free(sync_cb_handle* handle);

SRSLTE_API void sync_cb_push_back(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_read(sync_cb_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_front(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_pop_front(sync_cb_handle handle);

SRSLTE_API bool sync_cb_empty(sync_cb_handle handle);

SRSLTE_API int sync_cb_size(sync_cb_handle handle);
//******************************************************************************

//******************************************************************************
SRSLTE_API void sync_vector_make(sync_vector_handle* handle);

SRSLTE_API void sync_vector_free(sync_vector_handle* handle);

SRSLTE_API void sync_vector_push(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_vector_read(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_vector_pop_back(sync_vector_handle handle);

SRSLTE_API bool sync_vector_empty(sync_vector_handle handle);

SRSLTE_API int sync_vector_size(sync_vector_handle handle);

SRSLTE_API void sync_vector_reserve(sync_vector_handle handle, uint64_t size);
//******************************************************************************

//******************************************************************************
SRSLTE_API void tx_cb_make(tx_cb_handle* handle, uint64_t size);

SRSLTE_API void tx_cb_free(tx_cb_handle* handle);

SRSLTE_API void tx_cb_push_back(tx_cb_handle handle, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void tx_cb_read(tx_cb_handle handle, uint64_t index, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void tx_cb_front(tx_cb_handle handle, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void tx_cb_pop_front(tx_cb_handle handle);

SRSLTE_API bool tx_cb_empty(tx_cb_handle handle);

SRSLTE_API int tx_cb_size(tx_cb_handle handle);
//******************************************************************************

//******************************************************************************
SRSLTE_API void rx_param_cb_make(rx_param_cb_handle* handle, uint64_t size);

SRSLTE_API void rx_param_cb_free(rx_param_cb_handle* handle);

SRSLTE_API void rx_param_cb_push_back(rx_param_cb_handle handle, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void rx_param_cb_read(rx_param_cb_handle handle, uint64_t index, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void rx_param_cb_front(rx_param_cb_handle handle, basic_ctrl_t* const basic_ctrl);

SRSLTE_API void rx_param_cb_pop_front(rx_param_cb_handle handle);

SRSLTE_API bool rx_param_cb_empty(rx_param_cb_handle handle);

SRSLTE_API int rx_param_cb_size(rx_param_cb_handle handle);
//******************************************************************************

#ifdef __cplusplus
}
#endif

#endif /* _CPP_WRAPPERS_H_ */
