#ifndef _COMMUNICATOR_WRAPPER_H
#define _COMMUNICATOR_WRAPPER_H

// *********** Definition of COMM macros ***********
// As we can have overlapping of Tx data we create a vector with a given number of buffers.
#define NUMBER_OF_USER_DATA_BUFFERS 400000

// Maximum allowed number of transmitted TBs in a single transmission.
#define MAX_NOF_TBS 25

// Size of the current TX data.
#define USER_DATA_BUFFER_LEN MAX_NOF_TBS*2600 // 25 TB * TB size for 5 MHz PHY BW and MCS 31.

#define DEFAULT_ENV_PATHNAME "/root/radio_api/environment.json"

#ifdef __cplusplus

#include "LayerCommunicator.h"
#include "interf.pb.h"
#include <iostream>
#include <stdexcept>
#include <mutex>
#include <map>
#include <thread>
#include <condition_variable>
#include "../../phy/srslte/include/srslte/intf/intf.h"
#include "../../envupdates/Envupdates.h"

#define ENABLE_COMMUNICATOR_PRINT 0

#define CHECK_COMM_OUT_OF_SEQUENCE_RX 0

#define ENBALE_0MQ_PROFILLING 0

#define COMMUNICATOR_DEBUG(x) do { if(ENABLE_COMMUNICATOR_PRINT) {std::cout << x;} } while (0)

struct layer_communicator_t {
  communicator::DefaultLayerCommunicator *layer_communicator_cpp;
};

struct message_t {
  communicator::Message message_cpp;
};

typedef struct {
  // This is a pointer to the user data buffer.
  unsigned char *user_data_buffer[NUMBER_OF_USER_DATA_BUFFERS];
  // This counter is used to change the current buffer row so that we don't have overlapping even when Tx is fast enough.
  uint32_t user_data_buffer_cnt;
  // Hold the number of radios being used.
  uint32_t nof_radios;
  // File path name to the place where the enviroment .json file will be stored.
  char* env_pathname;
} communicator_t;

int parse_received_message(communicator::Message msg, void *msg_struct);

extern "C" {
#else
struct layer_communicator_t;
struct message_t;
#endif

typedef struct layer_communicator_t* LayerCommunicator_handle;

typedef struct message_t* message_handle;

int communicator_initialization(char* module_name, char* target1_name, char* target2_name, LayerCommunicator_handle* const handle, uint32_t nof_radios, char* const env_update_pathname);

int communicator_uninitialization(LayerCommunicator_handle* const handle);

int communicator_make(char* module_name, char* target1_name, char* target2_name, LayerCommunicator_handle* handle);

int communicator_send_phy_stat_message(LayerCommunicator_handle handle, stat_e type, phy_stat_t *phy_stats, message_handle *msg_handle);

int communicator_send_phy_stat_msg_dest(LayerCommunicator_handle handle, int destination, stat_e type, phy_stat_t *phy_stats, message_handle *msg_handle);

bool communicator_get_high_queue(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle);

int communicator_get_high_queue_wait(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle);

bool communicator_get_high_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle);

bool communicator_get_low_queue(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle);

int communicator_get_low_queue_wait(LayerCommunicator_handle handle, void *msg_struct, message_handle *msg_handle);

bool communicator_get_low_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle);

bool communicator_is_high_queue_empty(LayerCommunicator_handle handle);

bool communicator_is_low_queue_empty(LayerCommunicator_handle handle);

void communicator_print_message(message_handle msg_handle);

void communicator_free(LayerCommunicator_handle* handle);

void communicator_free_msg(message_handle *msg_handle);

int communicator_send_basic_control(LayerCommunicator_handle handle, basic_ctrl_t* const basic_ctrl);

void communicator_send_channel_emulator_config(LayerCommunicator_handle handle, channel_emulator_config_t* const chan_emu_conf);

int communicator_allocate_user_data_buffer();

void communicator_free_user_data_buffer();

void communicator_set_user_data_buffer_to_null();

void *communicator_vec_malloc(uint32_t size);

bool communicator_verify_environment_update(environment_t *env);

uint64_t communicator_get_host_time_now();

#ifdef __cplusplus
}
#endif

#endif /* _COMMUNICATOR_WRAPPER_H */
