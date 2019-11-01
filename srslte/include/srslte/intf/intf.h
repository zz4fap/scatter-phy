#ifndef _INTF_H_
#define _INTF_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>

// MF-TDMA Design: (May be changed later according to DARPA reference design)
// Basic resource unit in the air: (ch, slot). ch -- channel number; slot -- slot number
// One scheduling period is defined as a frame. For instance, slot 0 ~ slot N-1 compose a frame which contains N slots.
// Scheduler can be updated frame by frame. Frame number can be defined from 0 to max_frame_number.
// When a (ch, slot) is scheduled, MAC send multiple TB(transportation block) to PHY, PHY will split a TB to many CB(code block) if a TB is too long.
// CB size has upper limit (in LTE it is 6144 bits). If multiple CBs are generated for one TB, except the last CB, all previous CB must have 6144 bits.
// Before MAC send data to PHY, MAC should query PHY info: current (ch, slot), target (ch, slot) abliity (how many bits can be carried under specific MCS)
// PHY CB HARQ won't be supported in the first step because of potential timing performance.
// MAC TB ARQ may be defined in MAC layer. Network and application layer may design their own ARQ scheme.

// Revision History
// 2017 Feb. 21: Drafting some interfaces/design to be exposed to AI, not MAC-PHY interface/design (maybe some overlapping and reuse in the future, not intentional).
// 2017 Feb. 21: Addition of more statistics parameters to PHY RX Statistics struct.
// 2017 Feb. 22: Removal of BLER and CRC from PHY RX Statistics struct. Addition of CRC to MAC statistics. Modification of signed integers to unsigned integers. Some code alignment.

// LTE control resource and guard band harvesting switches, if these are changed please align also the sister definitions at mac/helpers.h
#define ENABLE_CTRL_RES_HARVESTING 1
#define ENABLE_GUARD_BAND_HARVESTING 1

// Maximum alowed number of Physical Radios that can be used to create PHYS.
#define MAX_NUM_CONCURRENT_PHYS 2

typedef enum {PHY_UNKNOWN_ST=0, PHY_RX_ST=1, PHY_TX_ST=2} trx_flag_e;

typedef enum {RX_STAT=0, TX_STAT=1, SENSING_STAT=2} stat_e;

typedef enum {PHY_UNKNOWN=0, PHY_SUCCESS=100, PHY_ERROR=101, PHY_TIMEOUT=102, PHY_LBT_TIMEOUT=103} phy_status_e;

typedef enum {BW_IDX_UNKNOWN=0, BW_IDX_OneDotFour=1, BW_IDX_Three=2, BW_IDX_Five=3, BW_IDX_Ten=4, BW_IDX_Fifteen=5, BW_IDX_Twenty=6} bw_index_e;

typedef enum {MODULE_UNKNOWN=0, MODULE_PHY=1, MODULE_MAC=2, MODULE_AI=3, MODULE_RF_MON=4, MODULE_APP=5, MODULE_PHY_DEBUG_1=6, MODULE_PHY_DEBUG_2=7, MODULE_MAC_DEBUG_1=8, MODULE_MAC_DEBUG_2=9} module_index_e;

typedef unsigned char uchar;

// Extended Basic control and scheduler definition
// Extended Basic control -- extended basic control header for both tx and rx
typedef struct {
  trx_flag_e trx_flag;	              // 0 -- rx; 1 -- tx; may have more status in the future.
  uint32_t phy_id;                    // Indicate wich one of the radios should be used for this PHY.
  uint64_t seq_number;                // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  uint32_t send_to;                   // This field gives the ID of radio the we are sending a packet to.
  uint32_t intf_id;                   // Interface ID. This field specifies to which radio interface a message is intended to.
  uint32_t bw_idx;                    // Channel BW: 1.4, 3, 5 and 10 MHz.
  uint32_t ch; 			                  // To tx or rx at this channel
  uint32_t frame;                     // Frame number
  uint32_t slot; 		                  // To tx or rx at this slot
  uint64_t timestamp;                 // Time at which slot should be sent given in milliseconds.
  uint64_t debugging_timestamp_start; // Timestamp used to measure the amount of time it takes to pass msgs from main to Tx threads.
  uint32_t mcs; 		                  // Set mcs for TX; When in RX state it is automatically found by the receiver.
  int32_t gain; 		                  // tx or rx gain. dB. For rx, -1 means AGC mode.
  double rf_boost;                    // RF boost amplification
  uint32_t length;                    // During TX state, it indicates number of bytes after this header. It must be an integer times the TB size. During RX indicates number of expected slots to be received.
  uchar *data;                        // Data to be transmitted.
} basic_ctrl_t;

// Scheduler definition
typedef struct {
  int32_t frame_size; // How many slots in one frame/scheduling-period
  basic_ctrl_t *ctrl; // Dynamic size. Should contain frame_size elements in the target memory pointed by this pointer.
						          // In case of stream (TCP, zeroMQ), ctrl contents should follow frame_size exactly.
						          // in ctrl, we request slot number should be increasing from 0 to frame_size-1
} mf_tdma_t;

// Scheduler update command
// This cmd tells MAC&PHY: from fpga_timestamp or (frame, slot), new shceduler should be used.
typedef struct {
  uint64_t fpga_timestamp;  // FPGA internal counter for global time slot synchronization
  uint32_t frame; 			    // frame number
  uint32_t slot; 			      // slot number
  mf_tdma_t scheduler;
} mf_tdma_update_cmd_t;

// PHY TX statistics
typedef struct {
  int32_t power; 			// TX gain. dB
  uint64_t channel_free_cnt;
  uint64_t channel_busy_cnt;
  double free_energy;
  double busy_energy;
  uint64_t total_dropped_slots;
  double coding_time;
  double rf_boost;
} phy_tx_stat_t;

// PHY RX statistics
typedef struct {
  uint32_t nof_slots_in_frame;  // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
  uint32_t slot_counter;        // This field indicates the slot number inside of a MAC frame.
  int32_t gain; 		            // RX gain. dB. For receiver, -1 means AGC mode. No need to contro gain by outside.
  uint32_t cqi; 		            // Channel Quality Indicator. Range: [1, 15]
  float rssi; 			            // Received Signal Strength Indicator. Range: [–2^31, (2^31) - 1]. dBm*10. For example, value -567 means -56.7dBm.
  float rsrp;				            // Reference Signal Received Power. Range: [-1400, -400]. dBm*10. For example, value -567 means -56.7dBm.
  float rsrq;				            // Reference Signal Receive Quality. Range: [-340, -25]. dB*10. For example, value 301 means 30.1 dB.
  float sinr;                   // Signal to Interference plus Noise Ratio. Range: [–2^31, (2^31) - 1]. dB.
  uint64_t detection_errors;
  uint64_t decoding_errors;
  uint64_t filler_bits_error;                     // Specific error when there is a decoding error.
  uint64_t nof_cbs_exceeds_softbuffer_size_error; // Specific error when there is a decoding error.
  uint64_t rate_matching_error;                   // Specific error when there is a decoding error.
  uint64_t cb_crc_error;                          // Specific error when there is a decoding error.
  uint64_t tb_crc_error;                          // Specific error when there is a decoding error.
  float cfo;
  float peak_value;
  float noise;
  int32_t decoded_cfi;
  bool found_dci;
  int32_t last_noi;
  uint64_t total_packets_synchronized;
  double decoding_time;
  double synch_plus_decoding_time;
  int32_t length;   // How many bytes are after this header. It should be equal to current TB size.
  uchar *data;      // Data received by the PHY.
} phy_rx_stat_t;

typedef struct {
  float frequency;    // Central frequency used for sensing the channel.
  float sample_rate;  // Sample rate used for sensing the channel.
  float gain;         // RX gain used for sensing the channel.
  float rssi;         // RSSI of the received IQ samples.
  int32_t length;			// How many bytes are after this header. It should be equal to current TB size.
  uchar *data;        // Data received by the PHY.
} phy_sensing_stat_t;

typedef union {
  phy_tx_stat_t tx_stat;
  phy_rx_stat_t rx_stat;
  phy_sensing_stat_t sensing_stat;
} stat_t;

// Statistics
typedef struct {
  uint32_t phy_id;      // Indicate wich one of the radios was used to decode this message.
  uint64_t seq_number;  // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  uint32_t status;      // Indicates whether there were some kind of problem during TX or RX or if it was successful. We must define some error codes.
  uint64_t host_timestamp;
	uint64_t fpga_timestamp;
	uint32_t frame;
  uint32_t slot;
	uint32_t ch;
	uint32_t mcs;
	uint32_t num_cb_total;
	uint32_t num_cb_err;
	uint32_t wrong_decoding_counter;
  stat_t   stat;
} phy_stat_t;

typedef struct {
  bool environment_updated;
  double competition_bw;
  double competition_center_freq;
} environment_t;

typedef struct {
  bool enable_simple_awgn_channel;
  float noise_variance;
  float snr;
} channel_emulator_config_t;

#endif // _INTF_H_
