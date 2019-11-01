#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#include "srslte/intf/intf.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define MAX_TX_SLOTS 10000

void print_received_basic_control(basic_ctrl_t *basic_ctrl);

void generateData(uint32_t numOfBytes, uchar *data);

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data);

inline uint64_t get_host_time_now_us();

inline uint64_t get_host_time_now_ms();

bool go_exit = false;

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    go_exit = true;
    printf("SIGINT received. Exiting...\n",0);
  }
}

void initialize_signal_handler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sig_int_handler);
}

int main(int argc, char *argv[]) {

  LayerCommunicator_handle handle;
  char module_name[] = "MODULE_MAC";
  char target_name[] = "MODULE_PHY";
  basic_ctrl_t basic_ctrl;

  uint32_t nof_tx_slots = 0;
  float milliseconds_in_the_future = 4.0;
  int numOfBytes = 85; // Number of bytes for 5 MHz and MCS 0.
  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  // Initialize signal handler.
  initialize_signal_handler();

  // Create communicator.
  communicator_make(module_name, target_name, NULL, &handle);

  sleep(2);

  // Create basic control message to control Tx chain.
  createBasicControl(&basic_ctrl, PHY_TX_ST, 66, BW_IDX_Five, 0, 0, 0, 3, 85, data);

  // Retrieve current time from host PC.
  basic_ctrl.timestamp = get_host_time_now_us();
  // Add some time to the current time.
  basic_ctrl.timestamp = basic_ctrl.timestamp + (uint64_t)(milliseconds_in_the_future*1000.0);

  while(!go_exit && nof_tx_slots < MAX_TX_SLOTS) {

    // Send basic control to PHY.
    communicator_send_basic_control(handle, &basic_ctrl);

    // Add some time to the current time.
    basic_ctrl.timestamp = basic_ctrl.timestamp + (uint64_t)(1100.0);

    // Count number of transmitted packets.
    nof_tx_slots++;

    // Sleep for 900 us.
    usleep(900);
  }

  sleep(2);

  // Free communicator related objects.
  communicator_free(&handle);

  return 0;
}

// This function returns timestamp with microseconds precision.
inline uint64_t get_host_time_now_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
}

// This function returns timestamp with miliseconds precision.
inline uint64_t get_host_time_now_ms() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000LL) + (uint64_t)host_timestamp.tv_nsec/1000000LL;
}

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data) {
  basic_ctrl->trx_flag = trx_flag;
  basic_ctrl->seq_number = seq_num;
  basic_ctrl->bw_idx = bw_idx;
  basic_ctrl->ch = ch;
  basic_ctrl->timestamp = timestamp;
  basic_ctrl->mcs = mcs;
  basic_ctrl->gain = gain;
  basic_ctrl->length = length;
  basic_ctrl->data = data;
}

void createRxStats(phy_stat_t *statistic, uint32_t numOfBytes, uchar *data) {
  statistic->seq_number = 678;
  statistic->status = PHY_SUCCESS;
  statistic->host_timestamp = 156;
  statistic->fpga_timestamp = 12;
  statistic->frame = 89;
  statistic->ch = 8;
  statistic->slot = 1;
  statistic->mcs = 18;
  statistic->num_cb_total = 123;
  statistic->num_cb_err = 1;
  statistic->stat.rx_stat.gain = 200;
  statistic->stat.rx_stat.cqi = 23;
  statistic->stat.rx_stat.rssi = 11;
  statistic->stat.rx_stat.rsrp = 89;
  statistic->stat.rx_stat.rsrp = 56;
  statistic->stat.rx_stat.rsrq = 78;
  statistic->stat.rx_stat.length = numOfBytes;
  statistic->stat.rx_stat.data = data;
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  printf("Creating %d data bytes\n",numOfBytes);
  for(int i = 0; i < numOfBytes; i++) {
    data[i] = (uchar)(rand() % 256);
  }
}

void print_received_basic_control(basic_ctrl_t *basic_ctrl) {
  printf("Received basic control: \n\tTRX: %d\n\tSeq number: %d\n\tBW: %d\n\tchannel: %d\n\tframe: %d\n\tslot: %d\n\tMCS: %d\n\tgain: %d\n\tlength: %d\n",basic_ctrl->trx_flag,basic_ctrl->seq_number,basic_ctrl->bw_idx,basic_ctrl->ch,basic_ctrl->frame,basic_ctrl->slot,basic_ctrl->mcs,basic_ctrl->gain,basic_ctrl->length);
  printf("Data: ");
  for(int i = 0; i < basic_ctrl->length; i++) {
    if(i < basic_ctrl->length-1) {
      printf("%d, ",basic_ctrl->data[i]);
    } else {
      printf("%d\n",basic_ctrl->data[i]);
    }
  }
}
