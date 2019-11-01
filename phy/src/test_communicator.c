#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "srslte/intf/intf.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

void print_received_basic_control(basic_ctrl_t *basic_ctrl);

int main(int argc, char *argv[]) {

  LayerCommunicator_handle handle;
  char module_name[] = "MODULE_PHY";
  char target_name[] = "MODULE_MAC";
  basic_ctrl_t basic_ctrl;
  message_handle msg_handle;

  // Create some data.
  int numOfBytes = 85; // Number of bytes for 5 MHz and MCS 0.
  uchar data[numOfBytes];
  printf("Creating %d data bytes\n",numOfBytes);
  for(int i=0; i < numOfBytes; i++) {
    data[i] = i;
  }

  communicator_make(module_name, target_name, NULL, &handle);

  phy_stat_t statistic;
  statistic.seq_number = 678;
  statistic.status = PHY_SUCCESS;
  statistic.host_timestamp = 156;
  statistic.fpga_timestamp = 12;
  statistic.frame = 89;
  statistic.ch = 8;
  statistic.slot = 1;
  statistic.mcs = 18;
  statistic.num_cb_total = 123;
  statistic.num_cb_err = 1;
  statistic.stat.rx_stat.gain = 200;
  statistic.stat.rx_stat.cqi = 23;
  statistic.stat.rx_stat.rssi = 11;
  statistic.stat.rx_stat.rsrp = 89;
  statistic.stat.rx_stat.rsrp = 56;
  statistic.stat.rx_stat.rsrq = 78;
  statistic.stat.rx_stat.length = numOfBytes;
  statistic.stat.rx_stat.data = data;

  sleep(4);

  communicator_send_phy_stat_message(handle, RX_STAT, &statistic, &msg_handle);

  printf("Sent Message is:\n");
  communicator_print_message(msg_handle);

  printf("\n\nWaiting for message...\n");
  communicator_get_high_queue_wait(handle, (void *)&basic_ctrl, &msg_handle);

  print_received_basic_control(&basic_ctrl);

  communicator_print_message(msg_handle);

  communicator_free_msg(&msg_handle);

  communicator_free(&handle);

  // It must be done here as malloc was called by the communicator wrapper.
  free(basic_ctrl.data);
  basic_ctrl.data = NULL;

  return 0;
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
