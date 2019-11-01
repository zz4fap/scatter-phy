#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>

#include "srslte/intf/intf.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

void print_received_basic_control(basic_ctrl_t *basic_ctrl);

void generateData(uint32_t numOfBytes, uchar *data);

void createBasicControl(basic_ctrl_t *basic_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data);

inline uint64_t get_host_time_now();

int main(int argc, char *argv[]) {

  LayerCommunicator_handle handle;
  char module_name[] = "MODULE_MAC";
  char target_name[] = "MODULE_PHY";
  basic_ctrl_t basic_ctrl;

  // Create communicator.
  communicator_make(module_name, target_name, &handle);

  // Create some data.
  int numOfBytes = 85; // Number of bytes for 5 MHz and MCS 0.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  sleep(1);

  uint64_t timestamp = 0;

  createBasicControl(&basic_ctrl, PHY_TX_ST, 66, BW_IDX_Five, 0, timestamp, 0, 0, numOfBytes, data);

  uint64_t milliseconds_in_the_future = 2;

  for(int i = 0; i < 10000; i++) {
    // Retrieve current time from host PC.
    basic_ctrl.timestamp = get_host_time_now() + milliseconds_in_the_future*1000;

    //printf("send at timestamp: %" PRIu64 "\n",basic_ctrl.timestamp);

    communicator_send_basic_control(handle, &basic_ctrl);

    usleep(milliseconds_in_the_future*1000);
  }

  sleep(2);

  // Free communicator related objects.
  communicator_free(&handle);

  return 0;
}

inline uint64_t get_host_time_now() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
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
    data[i] = i;
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
