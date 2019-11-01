#include "helpers.h"

void helpers_get_data_time_string(char* date_time_str) {
  struct timeval tmnow;
  struct tm *tm;
  char usec_buf[20];
  gettimeofday(&tmnow, NULL);
  tm = localtime(&tmnow.tv_sec);
  strftime(date_time_str,30,"[%d/%m/%Y %H:%M:%S", tm);
  strcat(date_time_str,".");
  sprintf(usec_buf,"%06ld]",tmnow.tv_usec);
  strcat(date_time_str,usec_buf);
}

void helpers_print_basic_control(basic_ctrl_t* bc) {
  printf("*************************** Basic Control ***************************\n"\
         "TRX mode: %s\n"\
         "Seq. number: %d\n"\
         "BW: %s\n"\
         "Channel: %d\n"\
         "Timestamp: %" PRIu64 "\n" \
         "Frame: %d\n"\
         "Slot: %d\n"\
         "MCS: %d\n"\
         "Gain: %d\n",TRX_MODE(bc->trx_flag),bc->seq_number,BW_STRING(bc->bw_idx),bc->ch,bc->timestamp,bc->frame,bc->slot,bc->mcs,bc->gain);
  if(bc->trx_flag == PHY_RX_ST) { // RX
    printf("Num of slots: %d\n",bc->length);
  } else if(bc->trx_flag == PHY_TX_ST) { // TX
    printf("Length: %d\n",bc->length);
    printf("Data: ");
    for(int i = 0; i < bc->length; i++) {
      if(i < (bc->length-1)) {
        printf("%d, ", bc->data[i]);
      } else {
        printf("%d\n", bc->data[i]);
      }
    }
  }
  printf("*********************************************************************\n");
}

void helpers_print_rx_statistics(phy_stat_t *phy_stat) {
  // Print PHY RX Stats Structure.
  printf("******************* PHY RX Statistics *******************\n"\
    "Seq. number: %d\n"\
    "Status: %d\n"\
    "Host Timestamp: %" PRIu64 "\n"\
    "FPGA Timestamp: %d\n"\
    "Channel: %d\n"\
    "MCS: %d\n"\
    "Num. Packets: %d\n"\
    "Num. Errors: %d\n"\
    "Gain: %d\n"\
    "CQI: %d\n"\
    "RSSI: %1.2f\n"\
    "RSRP: %1.2f\n"\
    "RSRQ: %1.2f\n"\
    "SINR: %1.2f\n"\
    "Length: %d\n"\
    "*********************************************************************\n"\
    ,phy_stat->seq_number,phy_stat->status,phy_stat->host_timestamp,phy_stat->fpga_timestamp,phy_stat->ch,phy_stat->mcs,phy_stat->num_cb_total,phy_stat->num_cb_err,phy_stat->stat.rx_stat.gain,phy_stat->stat.rx_stat.cqi,phy_stat->stat.rx_stat.rssi,phy_stat->stat.rx_stat.rsrp,phy_stat->stat.rx_stat.rsrq,phy_stat->stat.rx_stat.sinr,phy_stat->stat.rx_stat.length);
}

void helpers_print_tx_statistics(phy_stat_t *phy_stat) {
  // Print PHY TX Stats Structure.
  printf("******************* PHY TX Statistics *******************\n"\
    "Seq. number: %d\n"\
    "Status: %d\n"\
    "Host Timestamp: %" PRIu64 "\n"\
    "FPGA Timestamp: %d\n"\
    "Channel: %d\n"\
    "MCS: %d\n"\
    "Num. Packets: %d\n"\
    "Num. Errors: %d\n"\
    "Power: %d\n"\
    "*********************************************************************\n"\
    ,phy_stat->seq_number,phy_stat->status,phy_stat->host_timestamp,phy_stat->fpga_timestamp,phy_stat->ch,phy_stat->mcs,phy_stat->num_cb_total,phy_stat->num_cb_err,phy_stat->stat.tx_stat.power);
}

void helpers_print_subframe(cf_t *subframe, int num_of_samples, bool start_of_burst, bool end_of_burst) {
  if(start_of_burst) {
    printf("************** SOB **************\n");
  }
  for(int i = 0; i < num_of_samples; i++) {
    printf("sample: %d - (%1.3f,%1.3f)\n",i,crealf(subframe[i]),cimagf(subframe[i]));
  }
  if(end_of_burst) {
    printf("************** EOB **************\n");
  } else {
    printf("\n");
  }
}

void helpers_print_subframe_buffer(cf_t* const buffer, uint32_t length) {
  for(uint32_t i = 0; i < length; i++) {
    printf("sample %d: %1.2f,%1.2f\n",i,crealf(buffer[i]),cimagf(buffer[i]));
  }
  printf("###############################################################\n\n\n");
}
