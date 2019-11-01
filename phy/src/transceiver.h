#ifndef _TRANSCEIVER_H_
#define _TRANSCEIVER_H_

// Structure used to store parsed arguments.
typedef struct {
    bool enable_cfo_correction;
    uint16_t rnti;
    uint32_t nof_prb;
    uint32_t nof_ports;
    char *rf_args;
    double competition_center_frequency;
    float initial_rx_gain;
    float initial_tx_gain;
    int node_operation;
    unsigned long single_log_duration;
    unsigned long logging_frequency;
    unsigned long max_number_of_dumps;
    char *path_to_start_file;
    char *path_to_log_files;
    uint32_t node_id;
    uint32_t intf_id;
    srslte_datatype_t data_type;
    float sensing_rx_gain;
    uint32_t radio_id;
    bool use_std_carrier_sep;
    float initial_agc_gain;
    double competition_bw;
    uint32_t default_tx_channel;
    uint32_t default_rx_channel;
    float lbt_threshold;
    uint64_t lbt_timeout;
    uint32_t max_turbo_decoder_noi;
    bool phy_filtering;
    srslte_datatype_t iq_dump_data_type;
    float rf_monitor_rx_sample_rate;
    size_t rf_monitor_channel;
    bool lbt_use_fft_based_pwr;
    bool send_tx_stats_to_mac;
    uint32_t max_backoff_period;
    bool iq_dumping;
    bool immediate_transmission;
    bool add_tx_timestamp;
    uint32_t rf_monitor_option;
    int initial_subframe_index;
    size_t rf_monitor_fft_size;
    size_t rf_monitor_avg_size;
    bool plot_rx_info;
    float rf_amp;
    uint32_t trx_filter_idx;
    bool enable_rfnoc_tx_fir;
    uint32_t nof_phys;
    uint32_t default_phy_id;
    bool decode_pdcch;
    bool enable_avg_psr;
    float threshold;
    uint32_t max_turbo_decoder_noi_for_high_mcs;
    bool use_scatter_sync_seq;
    uint32_t pss_len;
    float pss_boost_factor;
    bool enable_second_stage_pss_detection;
    float pss_first_stage_threshold;
    float pss_second_stage_threshold;
    bool enable_eob_pss;
    char env_pathname[200];
} transceiver_args_t;

#define DEFAULT_CFI 1 // We use only one OFDM symbol for control.

#define DEVNAME_B200 "uhd_b200"

#define DEVNAME_X300 "uhd_x300"

#define MAXIMUM_NUMBER_OF_RADIOS 256 // This is related to the number of IP addresses we can get.

#define MAXIMUM_NUMBER_OF_CELLS 504

// ID of Broadcast messages sent by MAC to all nodes.
#define BROADCAST_ID 255

// Define used to enable timestamps to be added to TX data and measure time it takes to receive packet on RX side.
#define ENABLE_TX_TO_RX_TIME_DIFF 0

#endif // _TRANSCEIVER_H_
