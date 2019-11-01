/*
 * Copyright 2015-2016 Ettus Research LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* C-Interface for multi_usrp */

#include <uhd/utils/static.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/specsense2k_block_ctrl.hpp>
#include <uhd/rfnoc/fir129_block_ctrl.hpp>
//#include <uhd/rfnoc/splitstream_block_ctrl.hpp>

//#include "srslte/rf/rfnoc_api/rfnoc_api.h"
#include <uhd/error.h>
#include <uhd/usrp/usrp.h>

#include <boost/thread/mutex.hpp>

#include <string.h>
#include <map>
#include <fstream>

#include <communicator/CommManager.h>

const std::vector<int> FIR_coeffs[4] = {
	{0, 0, 0, 0, -1, 1, 0, -3, 3, 1, -6, 5, 4, -13, 9, 9, -22, 12, 16, -34, 15, 28, -49, 16, 45, -68, 15, 68, -90, 11, 98, -114, 0, 137, -141, -18, 186, -169, -46, 246, -197, -88, 322, -226, -148, 417, -253, -233, 539, -278, -358, 705, -299, -551, 951, -317, -880, 1376, -330, -1584, 2381, -339, -4324, 9192, 21504},
	{0, 0, 0, 0, 0, -1, 2, -1, -1, 5, -6, 2, 6, -13, 12, -1, -15, 25, -19, -5, 32, -43, 24, 19, -59, 64, -23, -45, 95, -86, 12, 87, -142, 104, 17, -148, 197, -112, -69, 233, -258, 102, 155, -345, 321, -61, -289, 494, -381, -29, 498, -701, 434, 211, -855, 1029, -476, -615, 1621, -1757, 503, 2058, -5175, 7723, 24064},
	{0, 0, 0, 0, 0, 1, -2, 2, 0, -4, 6, -6, 0, 9, -15, 13, 0, -18, 30, -24, 0, 31, -50, 40, 0, -50, 79, -62, 0, 76, -118, 92, 0, -110, 169, -130, 0, 154, -237, 182, 0, -214, 328, -251, 0, 296, -455, 350, 0, -416, 645, -501, 0, 614, -970, 773, 0, -1017, 1694, -1449, 0, 2443, -5200, 7370, 24576},
	{0, 0, 0, 0, 0, 0, -1, 2, -4, 4, -2, -2, 7, -12, 14, -10, 0, 14, -27, 33, -28, 9, 19, -47, 64, -59, 30, 17, -69, 105, -109, 72, 0, -86, 156, -181, 142, -43, -91, 214, -279, 252, -126, -69, 273, -411, 420, -275, 0, 327, -596, 695, -553, 169, 371, -908, 1244, -1196, 648, 400, -1823, 3388, -4804, 5791, 26624}
};

/****************************************************************************
 * Helpers
 ***************************************************************************/
uhd::stream_args_t stream_args_c_to_cpp(const uhd_stream_args_t *stream_args_c)
{
    std::string otw_format(stream_args_c->otw_format);
    std::string cpu_format(stream_args_c->cpu_format);
    std::string args(stream_args_c->args);
    std::vector<size_t> channels(stream_args_c->channel_list, stream_args_c->channel_list + stream_args_c->n_channels);

    uhd::stream_args_t stream_args_cpp(cpu_format, otw_format);
    stream_args_cpp.args = args;
    stream_args_cpp.channels = channels;

    return stream_args_cpp;
}

uhd::stream_cmd_t stream_cmd_c_to_cpp(const uhd_stream_cmd_t *stream_cmd_c)
{
    uhd::stream_cmd_t stream_cmd_cpp(uhd::stream_cmd_t::stream_mode_t(stream_cmd_c->stream_mode));
    stream_cmd_cpp.num_samps   = stream_cmd_c->num_samps;
    stream_cmd_cpp.stream_now  = stream_cmd_c->stream_now;
    stream_cmd_cpp.time_spec   = uhd::time_spec_t(stream_cmd_c->time_spec_full_secs, stream_cmd_c->time_spec_frac_secs);
    return stream_cmd_cpp;
}

boost::mutex rf_monitor_zmq_mtx;
boost::mutex rf_monitor_kill_mtx;
boost::thread *rf_monitor_thread;
uhd::rx_streamer::sptr rf_mon_streamer;
double radio_tx_freq[2];
double radio_rx_freq[2];
double ddc_0_dsp_freq;

/****************************************************************************
 * Registry / Pointer Management
 ***************************************************************************/
/* Public structs */
struct uhd_usrp {
    size_t usrp_index;
    std::string last_error;
};

struct uhd_tx_streamer {
    size_t usrp_index;
    uhd::tx_streamer::sptr streamer;
    std::string last_error;
};

struct uhd_rx_streamer {
    size_t usrp_index;
    size_t streamer_index;
    uhd::rx_streamer::sptr streamer;
    std::string last_error;
};

/* Not public: We use this for our internal registry */
struct usrp_ptr {
    //bool rf_mon_on;
    //boost::mutex rf_mon_mutex;
    uhd::usrp::multi_usrp::sptr ptr;
    uhd::device3::sptr rfnoc_ptr;    
    std::vector<uhd::rfnoc::graph::sptr> rx_graphs;
    std::vector<uhd::rfnoc::graph::sptr> tx_graphs;
    std::vector<uhd::rfnoc::radio_ctrl::sptr> radio_ctrls;
    std::vector<uhd::rfnoc::block_id_t> radio_ctrl_ids;
    std::vector<uhd::rfnoc::source_block_ctrl_base::sptr> ddc_ctrls;
    std::vector<uhd::rfnoc::source_block_ctrl_base::sptr> splitstream_ctrls;
    std::vector<uhd::rfnoc::specsense2k_block_ctrl::sptr> specsense_ctrls;
    std::vector<uhd::rfnoc::sink_block_ctrl_base::sptr> dmafifo_ctrls;
    std::vector<uhd::rfnoc::sink_block_ctrl_base::sptr> duc_ctrls;
    std::vector<uhd::rfnoc::fir129_block_ctrl::sptr> fir_ctrls;
    static size_t usrp_counter;
};
size_t usrp_ptr::usrp_counter = 0;
typedef struct usrp_ptr usrp_ptr;
/* Prefer map, because the list can be discontiguous */
typedef std::map<size_t, usrp_ptr> usrp_ptrs;

UHD_SINGLETON_FCN(usrp_ptrs, get_usrp_ptrs);
/* Shortcut for accessing the underlying USRP sptr from a uhd_usrp_handle* */
#define USRP(h_ptr) (get_usrp_ptrs()[h_ptr->usrp_index].ptr)

/****************************************************************************
 * RX Streamer
 ***************************************************************************/
static boost::mutex _rx_streamer_make_mutex;
uhd_error uhd_rx_streamer_make(uhd_rx_streamer_handle* h){
    UHD_SAFE_C(
        boost::mutex::scoped_lock(_rx_streamer_make_mutex);
        (*h) = new uhd_rx_streamer;
    )
}

static boost::mutex _rx_streamer_free_mutex;
uhd_error uhd_rx_streamer_free(uhd_rx_streamer_handle* h){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_rx_streamer_free_mutex);
        delete (*h);
        (*h) = NULL;
    )
}

uhd_error uhd_rx_streamer_num_channels(uhd_rx_streamer_handle h,
                                       size_t *num_channels_out){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_channels_out = h->streamer->get_num_channels();
    )
}

uhd_error uhd_rx_streamer_max_num_samps(uhd_rx_streamer_handle h,
                                        size_t *max_num_samps_out){
    UHD_SAFE_C_SAVE_ERROR(h,
        *max_num_samps_out = h->streamer->get_max_num_samps();
    )
}

uhd_error uhd_rx_streamer_recv( // RFNoC probably fine
    uhd_rx_streamer_handle h,
    void **buffs,
    size_t samps_per_buff,
    uhd_rx_metadata_handle *md,
    double timeout,
    bool one_packet,
    size_t *items_recvd
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //std::ofstream rx_outfile("/root/rx_output.bin", std::ofstream::binary | std::ofstream::app);
        uhd::rx_streamer::buffs_type buffs_cpp(buffs, h->streamer->get_num_channels());
        *items_recvd = h->streamer->recv(buffs_cpp, samps_per_buff, (*md)->rx_metadata_cpp, timeout, one_packet);
        //if (h->streamer_index == 1) {
        //    rx_outfile.write((const char *) *buffs, (*items_recvd) * 8);
        //}
        //rx_outfile.close();
    )
}

uhd_error uhd_rx_streamer_issue_stream_cmd( // RFNoC probably fine
    uhd_rx_streamer_handle h,
    const uhd_stream_cmd_t *stream_cmd
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->streamer->issue_stream_cmd(stream_cmd_c_to_cpp(stream_cmd));
    )
}

uhd_error uhd_rx_streamer_last_error(
    uhd_rx_streamer_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

/****************************************************************************
 * TX Streamer
 ***************************************************************************/
static boost::mutex _tx_streamer_make_mutex;
uhd_error uhd_tx_streamer_make(
    uhd_tx_streamer_handle* h
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_tx_streamer_make_mutex);
        (*h) = new uhd_tx_streamer;
    )
}

static boost::mutex _tx_streamer_free_mutex;
uhd_error uhd_tx_streamer_free(
    uhd_tx_streamer_handle* h
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_tx_streamer_free_mutex);
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_tx_streamer_num_channels(
    uhd_tx_streamer_handle h,
    size_t *num_channels_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_channels_out = h->streamer->get_num_channels();
    )
}

uhd_error uhd_tx_streamer_max_num_samps(
    uhd_tx_streamer_handle h,
    size_t *max_num_samps_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *max_num_samps_out = h->streamer->get_max_num_samps();
    )
}

uhd_error uhd_tx_streamer_send( // RFNoC probably fine
    uhd_tx_streamer_handle h,
    const void **buffs,
    size_t samps_per_buff,
    uhd_tx_metadata_handle *md,
    double timeout,
    size_t *items_sent
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //std::ofstream tx_outfile("/root/tx_input.bin", std::ofstream::binary | std::ofstream::app);
        //tx_outfile.write((const char *) *buffs, samps_per_buff * 8);
        //tx_outfile.close();
        uhd::tx_streamer::buffs_type buffs_cpp(buffs, h->streamer->get_num_channels());
        *items_sent = h->streamer->send(
            buffs_cpp,
            samps_per_buff,
            (*md)->tx_metadata_cpp,
            timeout
        );
    )
}

uhd_error uhd_tx_streamer_recv_async_msg(
    uhd_tx_streamer_handle h,
    uhd_async_metadata_handle *md,
    const double timeout,
    bool *valid
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *valid = h->streamer->recv_async_msg((*md)->async_metadata_cpp, timeout);
    )
}

uhd_error uhd_tx_streamer_last_error(
    uhd_tx_streamer_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

/****************************************************************************
 * Generate / Destroy API calls
 ***************************************************************************/
static boost::mutex _usrp_find_mutex;
uhd_error uhd_usrp_find(
    const char* args,
    uhd_string_vector_handle *strings_out
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock _lock(_usrp_find_mutex);

        uhd::device_addrs_t devs = uhd::device::find(std::string(args), uhd::device::USRP);
        (*strings_out)->string_vector_cpp.clear();
        for(const uhd::device_addr_t &dev:  devs){
            (*strings_out)->string_vector_cpp.push_back(dev.to_string());
        }
    )
}

std::shared_ptr<communicator::Internal> data_to_message(const std::vector<unsigned char>& data, uint64_t timestamp, uint64_t index)
{
    std::shared_ptr<communicator::Internal> internal = std::make_shared<communicator::Internal>();
    internal->set_transaction_index(index);
    communicator::Receive_r *receive_r = new communicator::Receive_r();
    //We do not set a status code here
    communicator::Phy_stat *stat = new communicator::Phy_stat();
    stat->set_fpga_timestamp(timestamp);
    receive_r->set_allocated_stat(stat);
    //Edit by Ruben
    const char* data_start = reinterpret_cast<const char*>(&data.front());
    std::string data_str(data_start, data.size());
    //End edit
    receive_r->set_data(data_str);
    internal->set_allocated_receiver(receive_r);
    return internal;
}

static boost::mutex _usrp_get_rf_mon_stream_mutex;
uhd_error uhd_usrp_get_rf_mon_stream( // RFNoC updated
    //uhd_usrp_handle h,
    //uhd_rx_streamer_handle h_s,
    size_t fft_size,
    size_t avg_size
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_get_rf_mon_stream_mutex);

        communicator::CommManager comm_manager(communicator::MODULE::MODULE_RF_MON, communicator::MODULE::MODULE_AI);

        //usrp_ptr &usrp = get_usrp_ptrs()[(*h)->usrp_index];
        usrp_ptr usrp = get_usrp_ptrs()[0];

        usrp.radio_ctrls.at(1)->set_arg("spp", fft_size);


        usrp.specsense_ctrls.at(0)->set_args(uhd::device_addr_t(str(boost::format("fft_size=%d,avg_size=%d") % fft_size % avg_size)));

        //CHANGES MADE AS TIMESTAMP SHOULD NOT BE IN THE DATA VECTOR
        std::vector<unsigned char> buff(fft_size * sizeof(std::complex<short>));
        uhd::rx_metadata_t md;
        //size_t num_rx_samps = 0;
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.stream_now = true;
        rf_mon_streamer->issue_stream_cmd(stream_cmd);
        
        double ts;
        size_t fft_shift_size = sizeof(std::complex<short>) * fft_size / 2;
        std::vector<unsigned char> fft_shift_buff(fft_shift_size);
        uint64_t index = 0;
        //std::ofstream rf_mon_outfile("/root/rf_mon_output.bin", std::ofstream::binary | std::ofstream::app);
        while (true) {
            //CHANGES MADE AS TIMESTAMP SHOULD NOT BE IN THE DATA VECTOR
       	    ts = clock_get_time_ns();
            
            rf_mon_streamer->recv(&buff.front(), fft_size, md);
            //ts = md.time_spec.get_real_secs();
            memcpy(&fft_shift_buff.front(), &buff.front(), fft_shift_size); 
            memcpy(&buff.front(), &buff.front() + fft_shift_size, fft_shift_size); 
            memcpy(&buff.front() + fft_shift_size, &fft_shift_buff.front(), fft_shift_size);
            //rf_mon_outfile.write((const char *) buff, fft_size * 8);

            if (rf_monitor_zmq_mtx.try_lock()) {

            		//Test by Ruben
            		uint64_t tsui = ts;// *1e9;

                std::shared_ptr<communicator::Internal> internal = data_to_message(buff, tsui, ++index);

                communicator::Message msg(communicator::MODULE_RF_MON, communicator::MODULE::MODULE_AI, internal);
                comm_manager.send(msg);
                rf_monitor_zmq_mtx.unlock();
            }

            if (rf_monitor_kill_mtx.try_lock()) {
                break;
            }
        }
        //stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
        //rf_mon_streamer->issue_stream_cmd(stream_cmd);
    )
}

static boost::mutex _usrp_rf_monitor_start_mutex;
uhd_error uhd_usrp_rf_monitor_start(
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_rf_monitor_start_mutex);
        rf_monitor_zmq_mtx.unlock();
    )
}

static boost::mutex _usrp_rf_monitor_stop_mutex;
uhd_error uhd_usrp_rf_monitor_stop(
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_rf_monitor_stop_mutex);
        rf_monitor_zmq_mtx.lock();
    )
}

static boost::mutex _rf_monitor_uninitialize_mutex;
uhd_error uhd_usrp_rf_monitor_uninitialize(
    //uhd_usrp_handle h
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_rf_monitor_uninitialize_mutex);
        rf_monitor_kill_mtx.unlock();
        rf_monitor_thread->join();
    )
}

static boost::mutex _rf_monitor_initialize_mutex;
uhd_error uhd_usrp_rf_monitor_initialize(
    uhd_usrp_handle h,
    double freq,
    double rate,
    double lo_off,
    size_t fft_size,
    size_t avg_size
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_rf_monitor_initialize_mutex);

        usrp_ptr &usrp = get_usrp_ptrs()[h->usrp_index];
        //usrp.specsense_ctrls.at(0)->set_args(uhd::device_addr_t(str(boost::format("fft_size=%d,avg_size=%d") % fft_size % avg_size)));
        usrp.ddc_ctrls.at(0)->set_args(uhd::device_addr_t(str(boost::format("input_rate=184320000.0,output_rate=%lf") % rate)));
        usrp.ddc_ctrls.at(2)->set_arg<double>("input_rate", rate);

        uhd_tune_request_t tune_request;
        uhd_tune_result_t tune_result;
        // Set comman tune request parameters.
        tune_request.target_freq = freq;
        tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
        tune_request.dsp_freq = 0.0;
        tune_request.args = (char *) "mode_n=integer";
        // If local oscillator offset is zero then default tune request is used.
        if (lo_off == 0.0) {
            tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
            tune_request.rf_freq = 0.0;
        } else { // Any value different than 0.0 means we want to give an offset in frequency.
            tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
            tune_request.rf_freq = freq + lo_off;
        }

        uhd_usrp_set_rx_freq(h, &tune_request, 1, &tune_result);
        rf_monitor_kill_mtx.lock();
        rf_monitor_thread = new boost::thread(uhd_usrp_get_rf_mon_stream, fft_size, avg_size);
    )
}

static boost::mutex _usrp_make_mutex;
uhd_error uhd_usrp_make( // RFNoC updated
    uhd_usrp_handle *h,
    const char *args
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_make_mutex);
     
        //size_t fft_size = 256, avg_size = 64;
        //double freq = 1e9, lo_off = 42e6;

        std::string fft_size_str;
        size_t usrp_count = usrp_ptr::usrp_counter;
        usrp_ptr::usrp_counter++;

        // Initialize USRP
        std::string args_str = std::string(args);
        size_t pos;
        bool enable_fir = false;
        
        if (args_str == "enable_fir") {
            args_str = "";
            enable_fir = true;
        } else {
            pos = args_str.find("enable_fir");
            if(pos != std::string::npos) {
                enable_fir = true;
                if (pos > 0) {
                    args_str.erase(pos - 1, 11);
                } else if (pos == 0) {
                    args_str.erase(pos, 10);
            }
            }
        }
        pos = args_str.find("fft_size");
        if (pos != std::string::npos) {
            size_t next_comma_pos = args_str.find(",", pos);
            if (next_comma_pos != std::string::npos) {
                fft_size_str = args_str.substr(pos, next_comma_pos - pos);
                args_str.erase(pos, next_comma_pos - pos + 1);
            } else {
                fft_size_str = args_str.substr(pos);
                if (pos > 0) {
                    args_str.erase(pos - 1);
                } else {
                    args_str.erase(pos);
                }
            }
            fft_size_str = fft_size_str.substr(9);
            //fft_size = boost::lexical_cast<unsigned int>(fft_size_str);
        }
          
        if (strstr(args, "master_clock_rate")) {
            args_str = args_str + std::string(",skip_ddc,skip_duc,skip_sram");
        } else {
            if (args_str == "") {
                args_str = std::string("master_clock_rate=184.32e6,skip_ddc,skip_duc,skip_sram");
            } else {
            args_str = args_str + std::string(",master_clock_rate=184.32e6,skip_ddc,skip_duc,skip_sram");
        }
        }
        //uhd::device_addr_t device_addr(args);
        usrp_ptr P;
        //P.ptr = uhd::usrp::multi_usrp::make(device_addr);
        P.ptr = uhd::usrp::multi_usrp::make(args_str);
        P.rfnoc_ptr = P.ptr->get_device3();
        P.rfnoc_ptr->clear(); //clear the legacy flowgraphs

        // Initialize RFNoC blocks
        P.radio_ctrl_ids.push_back(uhd::rfnoc::block_id_t(0, "Radio", 0));
        P.radio_ctrl_ids.push_back(uhd::rfnoc::block_id_t(0, "Radio", 1));
        uhd::rfnoc::block_id_t ddc_ctrl_id_0 = uhd::rfnoc::block_id_t(0, "DDC", 0);
        uhd::rfnoc::block_id_t ddc_ctrl_id_1 = uhd::rfnoc::block_id_t(0, "DDC", 1);
        uhd::rfnoc::block_id_t ddc_ctrl_id_2 = uhd::rfnoc::block_id_t(0, "DDC", 2);
        uhd::rfnoc::block_id_t duc_ctrl_id_0 = uhd::rfnoc::block_id_t(0, "DUC", 0);
        uhd::rfnoc::block_id_t duc_ctrl_id_1 = uhd::rfnoc::block_id_t(0, "DUC", 1);
        uhd::rfnoc::block_id_t dmafifo_ctrl_id = uhd::rfnoc::block_id_t(0, "DmaFIFO", 0);
        uhd::rfnoc::block_id_t splitstream_ctrl_id_0 = uhd::rfnoc::block_id_t(0, "SplitStream", 0);
        uhd::rfnoc::block_id_t specsense_ctrl_id_0 = uhd::rfnoc::block_id_t(0, "SpecSense2k", 0);
        uhd::rfnoc::block_id_t fir_ctrl_id_0 = uhd::rfnoc::block_id_t(0, "fir129", 0);
        uhd::rfnoc::block_id_t fir_ctrl_id_1 = uhd::rfnoc::block_id_t(0, "fir129", 1);
        P.radio_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::radio_ctrl>(P.radio_ctrl_ids.at(0)));
        P.radio_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::radio_ctrl>(P.radio_ctrl_ids.at(1)));
        P.ddc_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(ddc_ctrl_id_0));
        P.ddc_ctrls.at(0)->set_args(uhd::device_addr_t("input_rate=184320000.0,output_rate=46080000.0"));
        P.ddc_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(ddc_ctrl_id_1));
        P.ddc_ctrls.at(1)->set_args(uhd::device_addr_t("input_rate=184320000.0,output_rate=5760000.0"));
        P.ddc_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(ddc_ctrl_id_2));
        P.ddc_ctrls.at(2)->set_args(uhd::device_addr_t("input_rate=46080000.0,output_rate=5760000.0"));
        P.duc_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::sink_block_ctrl_base>(duc_ctrl_id_0));
        P.duc_ctrls.at(0)->set_args(uhd::device_addr_t("output_rate=184320000.0,input_rate=5760000.0"));
        P.duc_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::sink_block_ctrl_base>(duc_ctrl_id_1));
        P.duc_ctrls.at(1)->set_args(uhd::device_addr_t("output_rate=184320000.0,input_rate=5760000.0"));
        P.dmafifo_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::sink_block_ctrl_base>(dmafifo_ctrl_id));
        P.splitstream_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(splitstream_ctrl_id_0));
        P.specsense_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::specsense2k_block_ctrl>(specsense_ctrl_id_0));
        //P.specsense_ctrls.at(0)->set_args(uhd::device_addr_t(str(boost::format("fft_size=%d,avg_size=%d") % fft_size % avg_size)));
        P.fir_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::fir129_block_ctrl>(fir_ctrl_id_0));
        P.fir_ctrls.push_back(P.rfnoc_ptr->get_block_ctrl<uhd::rfnoc::fir129_block_ctrl>(fir_ctrl_id_1));
        P.radio_ctrls.at(0)->set_rate(184320000);
        P.radio_ctrls.at(1)->set_rate(184320000);

        // Initialize RFNoC graphs
        P.rx_graphs.push_back(P.rfnoc_ptr->create_graph(str(boost::format("rx_graph_%d") % 0)));
        P.rx_graphs.push_back(P.rfnoc_ptr->create_graph(str(boost::format("rx_graph_%d") % 1)));
        P.rx_graphs.push_back(P.rfnoc_ptr->create_graph(str(boost::format("rf_monitor_graph_%d") % 0)));
        P.tx_graphs.push_back(P.rfnoc_ptr->create_graph(str(boost::format("tx_graph_%d") % 0)));
        P.tx_graphs.push_back(P.rfnoc_ptr->create_graph(str(boost::format("tx_graph_%d") % 1)));

        // Connect RX graphs
        P.rx_graphs.at(0)->connect(P.radio_ctrl_ids.at(0), 0, P.ddc_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT);
        P.rx_graphs.at(1)->connect(P.radio_ctrl_ids.at(1), 0, P.ddc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);
        P.rx_graphs.at(1)->connect(P.ddc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT, P.splitstream_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);
        P.rx_graphs.at(1)->connect(P.splitstream_ctrls.at(0)->get_block_id(), 0, P.ddc_ctrls.at(2)->get_block_id(), uhd::rfnoc::ANY_PORT);
        //P.rx_graphs.at(1)->connect(P.ddc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT, P.ddc_ctrls.at(2)->get_block_id(), uhd::rfnoc::ANY_PORT);
        P.rx_graphs.at(1)->connect(P.splitstream_ctrls.at(0)->get_block_id(), 1, P.specsense_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);

        // Connect TX graphs
        if (enable_fir) {
            P.tx_graphs.at(0)->connect(P.dmafifo_ctrls.at(0)->get_block_id(), 0, P.fir_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);
            P.tx_graphs.at(0)->connect(P.fir_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT, P.duc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);
            P.tx_graphs.at(1)->connect(P.dmafifo_ctrls.at(0)->get_block_id(), 1, P.fir_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT);
            P.tx_graphs.at(1)->connect(P.fir_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT, P.duc_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT);
        } else {
            P.tx_graphs.at(0)->connect(P.dmafifo_ctrls.at(0)->get_block_id(), 0, P.duc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT);
            P.tx_graphs.at(1)->connect(P.dmafifo_ctrls.at(0)->get_block_id(), 1, P.duc_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT);
        }
        P.tx_graphs.at(0)->connect(P.duc_ctrls.at(0)->get_block_id(), uhd::rfnoc::ANY_PORT, P.radio_ctrl_ids.at(0), 0);
        P.tx_graphs.at(1)->connect(P.duc_ctrls.at(1)->get_block_id(), uhd::rfnoc::ANY_PORT, P.radio_ctrl_ids.at(1), 0);
 
        // Dump into registry
        get_usrp_ptrs()[usrp_count] = P;

        // get rf monitor streamer

        uhd::stream_args_t stream_args("sc16", "sc16"); // We should read the wire format from the blocks
        uhd::device_addr_t stream_args_args("");
        stream_args_args["spp"] = fft_size_str;
        stream_args_args["block_id"] = P.specsense_ctrls.at(0)->get_block_id().to_string();
        stream_args.args = stream_args_args;
        rf_mon_streamer = P.rfnoc_ptr->get_rx_stream(stream_args);
        //uhd::rx_streamer::sptr rf_mon_streamer_temp = P.rfnoc_ptr->get_rx_stream(stream_args);
        //P.rf_mon_streamers.push_back(&rf_mon_streamer_temp);

        // Update handle
        (*h) = new uhd_usrp;
        (*h)->usrp_index = usrp_count;

    )
}

static boost::mutex _usrp_free_mutex;
uhd_error uhd_usrp_free(
    uhd_usrp_handle *h
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_free_mutex);

        if(!get_usrp_ptrs().count((*h)->usrp_index)){
            return UHD_ERROR_INVALID_DEVICE;
        }

        get_usrp_ptrs().erase((*h)->usrp_index);
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_usrp_last_error(
    uhd_usrp_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

static boost::mutex _usrp_get_rx_stream_mutex;
uhd_error uhd_usrp_get_rx_stream(
    uhd_usrp_handle h_u,
    uhd_stream_args_t *stream_args,
    uhd_rx_streamer_handle h_s
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_get_rx_stream_mutex);
        static size_t streamer_index = 0;

        if(!get_usrp_ptrs().count(h_u->usrp_index)){
            return UHD_ERROR_INVALID_DEVICE;
        }

        usrp_ptr &usrp = get_usrp_ptrs()[h_u->usrp_index];

        //h_s->streamer_index = usrp.rx_streamers.size();
        h_s->streamer_index = streamer_index;
        uhd::device_addr_t stream_args_args("");
        stream_args_args["block_id"] = usrp.ddc_ctrls.at(streamer_index + 1)->get_block_id().to_string();
        //stream_args_args["block_port"] = "0";
        uhd::stream_args_t stream_args_cpp = stream_args_c_to_cpp(stream_args);
        stream_args_cpp.args = stream_args_args;

        h_s->streamer = usrp.rfnoc_ptr->get_rx_stream(stream_args_cpp);
        h_s->usrp_index     = h_u->usrp_index;
        streamer_index++;
    )
}

static boost::mutex _usrp_get_tx_stream_mutex;
uhd_error uhd_usrp_get_tx_stream( // RFNoC updated
    uhd_usrp_handle h_u,
    uhd_stream_args_t *stream_args,
    uhd_tx_streamer_handle h_s
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_get_tx_stream_mutex);
        static size_t streamer_index = 0;

        if(!get_usrp_ptrs().count(h_u->usrp_index)){
            return UHD_ERROR_INVALID_DEVICE;
        }

        usrp_ptr &usrp = get_usrp_ptrs()[h_u->usrp_index];

        uhd::device_addr_t stream_args_args("");
        stream_args_args["block_id"] = usrp.dmafifo_ctrls.at(0)->get_block_id().to_string();
        stream_args_args["block_port"] = str(boost::format("%d") % streamer_index);
        uhd::stream_args_t stream_args_cpp = stream_args_c_to_cpp(stream_args);
        stream_args_cpp.args = stream_args_args;

        h_s->streamer = usrp.rfnoc_ptr->get_tx_stream(stream_args_cpp);
        h_s->usrp_index     = h_u->usrp_index;
        streamer_index++;
    )
}

/****************************************************************************
 * multi_usrp API calls
 ***************************************************************************/

#define COPY_INFO_FIELD(out, dict, field) \
    out->field = strdup(dict.get(BOOST_STRINGIZE(field)).c_str())

uhd_error uhd_usrp_get_rx_info(
    uhd_usrp_handle h,
    size_t chan,
    uhd_usrp_rx_info_t *info_out
) {
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::dict<std::string, std::string> rx_info = USRP(h)->get_usrp_rx_info(chan);

        COPY_INFO_FIELD(info_out, rx_info, mboard_id);
        COPY_INFO_FIELD(info_out, rx_info, mboard_name);
        COPY_INFO_FIELD(info_out, rx_info, mboard_serial);
        COPY_INFO_FIELD(info_out, rx_info, rx_id);
        COPY_INFO_FIELD(info_out, rx_info, rx_subdev_name);
        COPY_INFO_FIELD(info_out, rx_info, rx_subdev_spec);
        COPY_INFO_FIELD(info_out, rx_info, rx_serial);
        COPY_INFO_FIELD(info_out, rx_info, rx_antenna);
    )
}

uhd_error uhd_usrp_get_tx_info(
    uhd_usrp_handle h,
    size_t chan,
    uhd_usrp_tx_info_t *info_out
) {
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::dict<std::string, std::string> tx_info = USRP(h)->get_usrp_tx_info(chan);

        COPY_INFO_FIELD(info_out, tx_info, mboard_id);
        COPY_INFO_FIELD(info_out, tx_info, mboard_name);
        COPY_INFO_FIELD(info_out, tx_info, mboard_serial);
        COPY_INFO_FIELD(info_out, tx_info, tx_id);
        COPY_INFO_FIELD(info_out, tx_info, tx_subdev_name);
        COPY_INFO_FIELD(info_out, tx_info, tx_subdev_spec);
        COPY_INFO_FIELD(info_out, tx_info, tx_serial);
        COPY_INFO_FIELD(info_out, tx_info, tx_antenna);
    )
}

/****************************************************************************
 * Motherboard methods
 ***************************************************************************/
uhd_error uhd_usrp_set_master_clock_rate( // RFNoC needs update
    uhd_usrp_handle h,
    double rate,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_master_clock_rate(rate, mboard);
    )
}

uhd_error uhd_usrp_get_master_clock_rate( // RFNoC needs update
    uhd_usrp_handle h,
    size_t mboard,
    double *clock_rate_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *clock_rate_out = USRP(h)->get_master_clock_rate(mboard);
    )
}

uhd_error uhd_usrp_get_pp_string(
    uhd_usrp_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        strncpy(pp_string_out, USRP(h)->get_pp_string().c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_mboard_name(
    uhd_usrp_handle h,
    size_t mboard,
    char* mboard_name_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        strncpy(mboard_name_out, USRP(h)->get_mboard_name(mboard).c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_time_now(
    uhd_usrp_handle h,
    size_t mboard,
    time_t *full_secs_out,
    double *frac_secs_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp = USRP(h)->get_time_now(mboard);
        *full_secs_out = time_spec_cpp.get_full_secs();
        *frac_secs_out = time_spec_cpp.get_frac_secs();
    )
}

uhd_error uhd_usrp_get_time_last_pps(
    uhd_usrp_handle h,
    size_t mboard,
    time_t *full_secs_out,
    double *frac_secs_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp = USRP(h)->get_time_last_pps(mboard);
        *full_secs_out = time_spec_cpp.get_full_secs();
        *frac_secs_out = time_spec_cpp.get_frac_secs();
    )
}

uhd_error uhd_usrp_set_time_now(
    uhd_usrp_handle h,
    time_t full_secs,
    double frac_secs,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp(full_secs, frac_secs);
        USRP(h)->set_time_now(time_spec_cpp, mboard);
    )
}

uhd_error uhd_usrp_set_time_next_pps(
    uhd_usrp_handle h,
    time_t full_secs,
    double frac_secs,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp(full_secs, frac_secs);
        USRP(h)->set_time_next_pps(time_spec_cpp, mboard);
    )
}

uhd_error uhd_usrp_set_time_unknown_pps(
    uhd_usrp_handle h,
    time_t full_secs,
    double frac_secs
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp(full_secs, frac_secs);
        USRP(h)->set_time_unknown_pps(time_spec_cpp);
    )
}

uhd_error uhd_usrp_get_time_synchronized(
    uhd_usrp_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = USRP(h)->get_time_synchronized();
        return UHD_ERROR_NONE;
    )
}

uhd_error uhd_usrp_set_command_time(
    uhd_usrp_handle h,
    time_t full_secs,
    double frac_secs,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp(full_secs, frac_secs);
        USRP(h)->set_command_time(time_spec_cpp, mboard);
    )
}

uhd_error uhd_usrp_clear_command_time(
    uhd_usrp_handle h,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->clear_command_time(mboard);
    )
}

uhd_error uhd_usrp_set_time_source(
    uhd_usrp_handle h,
    const char* time_source,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_time_source(std::string(time_source), mboard);
    )
}

uhd_error uhd_usrp_get_time_source(
    uhd_usrp_handle h,
    size_t mboard,
    char* time_source_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        strncpy(time_source_out, USRP(h)->get_time_source(mboard).c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_time_sources(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_string_vector_handle *time_sources_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*time_sources_out)->string_vector_cpp = USRP(h)->get_time_sources(mboard);
    )
}

uhd_error uhd_usrp_set_clock_source(
    uhd_usrp_handle h,
    const char* clock_source,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_clock_source(std::string(clock_source), mboard);
    )
}

uhd_error uhd_usrp_get_clock_source(
    uhd_usrp_handle h,
    size_t mboard,
    char* clock_source_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        strncpy(clock_source_out, USRP(h)->get_clock_source(mboard).c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_clock_sources(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_string_vector_handle *clock_sources_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*clock_sources_out)->string_vector_cpp = USRP(h)->get_clock_sources(mboard);
    )
}

uhd_error uhd_usrp_set_clock_source_out(
    uhd_usrp_handle h,
    bool enb,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_clock_source_out(enb, mboard);
    )
}

uhd_error uhd_usrp_set_time_source_out(
    uhd_usrp_handle h,
    bool enb,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_time_source_out(enb, mboard);
    )
}

uhd_error uhd_usrp_get_num_mboards(
    uhd_usrp_handle h,
    size_t *num_mboards_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_mboards_out = USRP(h)->get_num_mboards();
    )
}

uhd_error uhd_usrp_get_mboard_sensor(
    uhd_usrp_handle h,
    const char* name,
    size_t mboard,
    uhd_sensor_value_handle *sensor_value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        delete (*sensor_value_out)->sensor_value_cpp;
        (*sensor_value_out)->sensor_value_cpp = new uhd::sensor_value_t(USRP(h)->get_mboard_sensor(name, mboard));
    )
}

uhd_error uhd_usrp_get_mboard_sensor_names(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_string_vector_handle *mboard_sensor_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*mboard_sensor_names_out)->string_vector_cpp = USRP(h)->get_mboard_sensor_names(mboard);
    )
}

uhd_error uhd_usrp_set_user_register(
    uhd_usrp_handle h,
    uint8_t addr,
    uint32_t data,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_user_register(addr, data, mboard);
    )
}

/****************************************************************************
 * EEPROM access methods
 ***************************************************************************/

uhd_error uhd_usrp_get_mboard_eeprom(
    uhd_usrp_handle h,
    uhd_mboard_eeprom_handle mb_eeprom,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::fs_path eeprom_path = str(boost::format("/mboards/%d/eeprom")
                                       % mboard);

        uhd::property_tree::sptr ptree = USRP(h)->get_device()->get_tree();
        mb_eeprom->mboard_eeprom_cpp = ptree->access<uhd::usrp::mboard_eeprom_t>(eeprom_path).get();
    )
}

uhd_error uhd_usrp_set_mboard_eeprom(
    uhd_usrp_handle h,
    uhd_mboard_eeprom_handle mb_eeprom,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::fs_path eeprom_path = str(boost::format("/mboards/%d/eeprom")
                                       % mboard);

        uhd::property_tree::sptr ptree = USRP(h)->get_device()->get_tree();
        ptree->access<uhd::usrp::mboard_eeprom_t>(eeprom_path).set(mb_eeprom->mboard_eeprom_cpp);
    )
}

uhd_error uhd_usrp_get_dboard_eeprom(
    uhd_usrp_handle h,
    uhd_dboard_eeprom_handle db_eeprom,
    const char* unit,
    const char* slot,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::fs_path eeprom_path = str(boost::format("/mboards/%d/dboards/%s/%s_eeprom")
                                       % mboard % slot % unit);

        uhd::property_tree::sptr ptree = USRP(h)->get_device()->get_tree();
        db_eeprom->dboard_eeprom_cpp = ptree->access<uhd::usrp::dboard_eeprom_t>(eeprom_path).get();
    )
}

uhd_error uhd_usrp_set_dboard_eeprom(
    uhd_usrp_handle h,
    uhd_dboard_eeprom_handle db_eeprom,
    const char* unit,
    const char* slot,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::fs_path eeprom_path = str(boost::format("/mboards/%d/dboards/%s/%s_eeprom")
                                       % mboard % slot % unit);

        uhd::property_tree::sptr ptree = USRP(h)->get_device()->get_tree();
        ptree->access<uhd::usrp::dboard_eeprom_t>(eeprom_path).set(db_eeprom->dboard_eeprom_cpp);
    )
}

/****************************************************************************
 * RX methods
 ***************************************************************************/

uhd_error uhd_usrp_set_rx_subdev_spec(
    uhd_usrp_handle h,
    uhd_subdev_spec_handle subdev_spec,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_subdev_spec(subdev_spec->subdev_spec_cpp, mboard);
    )
}

uhd_error uhd_usrp_get_rx_subdev_spec(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_subdev_spec_handle subdev_spec_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        subdev_spec_out->subdev_spec_cpp = USRP(h)->get_rx_subdev_spec(mboard);
    )
}

uhd_error uhd_usrp_get_rx_num_channels(
    uhd_usrp_handle h,
    size_t *num_channels_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_channels_out = USRP(h)->get_rx_num_channels();
    )
}

uhd_error uhd_usrp_get_rx_subdev_name(
    uhd_usrp_handle h,
    size_t chan,
    char* rx_subdev_name_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string rx_subdev_name = USRP(h)->get_rx_subdev_name(chan);
        strncpy(rx_subdev_name_out, rx_subdev_name.c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_set_rx_rate( // RFNoC updated
    uhd_usrp_handle h,
    double rate,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //USRP(h)->set_rx_rate(rate, chan);
        get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(chan + 1)->set_arg<double>("output_rate", rate);
    )
}

uhd_error uhd_usrp_get_rx_rate( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    double *rate_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //*rate_out = USRP(h)->get_rx_rate(chan);
        *rate_out = get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(chan + 1)->get_arg<double>("output_rate");
    )
}


/******** sets DDC0 output rate - need this to set rf monitor bw for environment updates *******************
*** single phy uses uhd_usrp_set_rx_rate to set the rf monitor bw, dual phy can not use the same function */

uhd_error uhd_usrp_set_rf_mon_rate( // RFNoC updated
    uhd_usrp_handle h,
    double rate
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //USRP(h)->set_rx_rate(rate, chan);
        get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(0)->set_arg<double>("output_rate", rate);
        get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(2)->set_arg<double>("input_rate", rate);
    )
}


uhd_error uhd_usrp_get_rf_mon_rate( // RFNoC updated
    uhd_usrp_handle h,
    double *rate_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //*rate_out = USRP(h)->get_rx_rate(chan);
        *rate_out = get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(0)->get_arg<double>("output_rate");
    )
}
/**********************************************************************************************************/
uhd_error uhd_usrp_get_rx_rates(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle rates_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        rates_out->meta_range_cpp = USRP(h)->get_rx_rates(chan);
    )
}

uhd_error uhd_usrp_set_rx_freq( // RFNoC updated
    uhd_usrp_handle h,
    uhd_tune_request_t *tune_request,
    size_t chan,
    uhd_tune_result_t *tune_result
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::tune_request_t tune_request_cpp = uhd_tune_request_c_to_cpp(tune_request);
        //uhd::tune_result_t tune_result_cpp = USRP(h)->set_rx_freq(tune_request_cpp, chan);
        uhd::tune_result_t tune_result_cpp;
        double freq = tune_request_cpp.target_freq;
        double rf_freq, lo_off;
        if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            rf_freq = freq;
        } else {
            rf_freq = tune_request_cpp.rf_freq;
        }
        if (tune_request_cpp.dsp_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            lo_off = rf_freq - freq;
        } else {
            lo_off = tune_request_cpp.dsp_freq;
            if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
                rf_freq = freq + lo_off;
            }
        }

        get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_rx_frequency(rf_freq, 0);
        double actual_rf_freq = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_rx_frequency(0);
        radio_rx_freq[chan] = actual_rf_freq;
        get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at((chan == 0)?1:0)->set_arg<double>("freq", lo_off + actual_rf_freq - rf_freq);
        double actual_dsp_freq = get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at((chan == 0)?1:0)->get_arg<double>("freq");
        if (chan == 1) {
            ddc_0_dsp_freq = actual_dsp_freq;
        }

        tune_result_cpp.clipped_rf_freq = rf_freq;
        tune_result_cpp.actual_rf_freq = actual_rf_freq;
        tune_result_cpp.actual_dsp_freq = actual_dsp_freq;
        if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            tune_result_cpp.target_rf_freq = freq + actual_rf_freq - rf_freq;
        } else {
            tune_result_cpp.target_rf_freq = rf_freq;
        }
        if (tune_request_cpp.dsp_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            tune_result_cpp.target_dsp_freq = lo_off + actual_rf_freq - rf_freq;
        } else {
            tune_result_cpp.target_dsp_freq = lo_off;
        }
        uhd_tune_result_cpp_to_c(tune_result_cpp, tune_result);
    )
}


uhd_error uhd_usrp_get_rx_freq( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    double *freq_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //*freq_out = USRP(h)->get_rx_freq(chan);
        *freq_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_rx_frequency(0) - get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at((chan == 0)?1:0)->get_arg<double>("freq");
    )
}

/* SPECIFIC TO SCATTER **************************************************************************/

uhd_error uhd_usrp_set_rx_channel_freq( // Does not change RF center frequency, only changes DDC frequency to switch channels within a competition band
    uhd_usrp_handle h,
    size_t chan,
    double freq
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //std::cout << "\n\n***** Setting RX frequency for channel " << chan << " to " << boost::format("%7.2lf") % (freq / 1e6) << std::endl;

        //set frequency of DDC_1 for chan=0, DDC_2 for chan=1
        if (chan == 0) {
            get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(1)->set_arg<double>("freq", radio_rx_freq[0] - freq);
        } else if (chan == 1) {
            get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(2)->set_arg<double>("freq", radio_rx_freq[1] - freq - ddc_0_dsp_freq);
        }

    )
}

uhd_error uhd_usrp_set_tx_channel_freq( // Does not change RF center frequency, only changes DUC frequency to switch channels within a competition band
    uhd_usrp_handle h,
    size_t chan,
    double freq
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //std::cout << "\n\n***** Setting TX frequency for channel " << chan << " to " << boost::format("%7.2lf") % (freq / 1e6) << std::endl;
        get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->set_arg<double>("freq", freq - radio_tx_freq[chan]);

    )
}

uhd_error uhd_usrp_get_rx_channel_freq( 
    uhd_usrp_handle h,    
    size_t chan,
    double *freq
){
    UHD_SAFE_C_SAVE_ERROR(h,

        //get frequency of DDC_1 for chan=0, DDC_2 for chan=1
        if(chan == 0)
          *freq = radio_rx_freq[0] - get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(1)->get_arg<double>("freq");
        else if(chan == 1)
          *freq = radio_rx_freq[1] - ddc_0_dsp_freq - get_usrp_ptrs()[h->usrp_index].ddc_ctrls.at(2)->get_arg<double>("freq");

    )
}

uhd_error uhd_usrp_get_tx_channel_freq( 
    uhd_usrp_handle h,
    size_t chan,
    double *freq
){
    UHD_SAFE_C_SAVE_ERROR(h,
        
        *freq = radio_tx_freq[chan] + get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->get_arg<double>("freq");

    )
}

/*********** DONE SPECIFIC TO SCATTER ***********************************************************/


uhd_error uhd_usrp_get_rx_freq_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle freq_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        freq_range_out->meta_range_cpp = USRP(h)->get_rx_freq_range(chan);
    )
}

uhd_error uhd_usrp_get_fe_rx_freq_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle freq_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        freq_range_out->meta_range_cpp = USRP(h)->get_fe_rx_freq_range(chan);
    )
}

UHD_API uhd_error uhd_usrp_get_rx_lo_names(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *rx_lo_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*rx_lo_names_out)->string_vector_cpp = USRP(h)->get_rx_lo_names(chan);
    )
}

UHD_API uhd_error uhd_usrp_set_rx_lo_source(
    uhd_usrp_handle h,
    const char* src,
    const char* name,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_lo_source(src, name, chan);
    )
}

UHD_API uhd_error uhd_usrp_get_rx_lo_source(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    char* rx_lo_source_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        strncpy(rx_lo_source_out, USRP(h)->get_rx_lo_source(name, chan).c_str(), strbuffer_len);
    )
}

UHD_API uhd_error uhd_usrp_get_rx_lo_sources(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    uhd_string_vector_handle *rx_lo_sources_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*rx_lo_sources_out)->string_vector_cpp = USRP(h)->get_rx_lo_sources(name, chan);
    )
}

UHD_API uhd_error uhd_usrp_set_rx_lo_export_enabled(
    uhd_usrp_handle h,
    bool enabled,
    const char* name,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_lo_export_enabled(enabled, name, chan);
    )
}

UHD_API uhd_error uhd_usrp_get_rx_lo_export_enabled(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    bool* result_out
) {
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = USRP(h)->get_rx_lo_export_enabled(name, chan);
    )
}

UHD_API uhd_error uhd_usrp_set_rx_lo_freq(
    uhd_usrp_handle h,
    double freq,
    const char* name,
    size_t chan,
    double* coerced_freq_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *coerced_freq_out = USRP(h)->set_rx_lo_freq(freq, name, chan);
    )
}

UHD_API uhd_error uhd_usrp_get_rx_lo_freq(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    double* rx_lo_freq_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *rx_lo_freq_out = USRP(h)->get_rx_lo_freq(name, chan);
    )
}

uhd_error uhd_usrp_set_rx_gain( // RFNoC updated
    uhd_usrp_handle h,
    double gain,
    size_t chan,
    const char *gain_name
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string name(gain_name);
        if(name.empty()){
            USRP(h)->set_rx_gain(gain, chan);
            //get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_rx_gain(gain, chan);
        }
        else{
            USRP(h)->set_rx_gain(gain, name, chan);
            //get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_rx_gain(gain, name, chan);
        }
    )
}

uhd_error uhd_usrp_set_normalized_rx_gain(
    uhd_usrp_handle h,
    double gain,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_normalized_rx_gain(gain, chan);
    )
}

uhd_error uhd_usrp_set_rx_agc(
    uhd_usrp_handle h,
    bool enable,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_agc(enable, chan);
    )
}

uhd_error uhd_usrp_get_rx_gain( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    const char *gain_name,
    double *gain_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string name(gain_name);
        if(name.empty()){
            *gain_out = USRP(h)->get_rx_gain(chan);
            //*gain_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_rx_gain(chan);
        }
        else{
            *gain_out = USRP(h)->get_rx_gain(name, chan);
            //*gain_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_rx_gain(name, chan);
        }
    )
}

uhd_error uhd_usrp_get_normalized_rx_gain(
    uhd_usrp_handle h,
    size_t chan,
    double *gain_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *gain_out = USRP(h)->get_normalized_rx_gain(chan);
    )
}

uhd_error uhd_usrp_get_rx_gain_range(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    uhd_meta_range_handle gain_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        gain_range_out->meta_range_cpp = USRP(h)->get_rx_gain_range(name, chan);
    )
}

uhd_error uhd_usrp_get_rx_gain_names(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *gain_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*gain_names_out)->string_vector_cpp = USRP(h)->get_rx_gain_names(chan);
    )
}

uhd_error uhd_usrp_set_rx_antenna(
    uhd_usrp_handle h,
    const char* ant,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_antenna(std::string(ant), chan);
    )
}

uhd_error uhd_usrp_get_rx_antenna(
    uhd_usrp_handle h,
    size_t chan,
    char* ant_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string rx_antenna = USRP(h)->get_rx_antenna(chan);
        strncpy(ant_out, rx_antenna.c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_rx_antennas(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *antennas_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*antennas_out)->string_vector_cpp = USRP(h)->get_rx_antennas(chan);
    )
}

uhd_error uhd_usrp_set_rx_bandwidth(
    uhd_usrp_handle h,
    double bandwidth,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_bandwidth(bandwidth, chan);
    )
}

uhd_error uhd_usrp_get_rx_bandwidth(
    uhd_usrp_handle h,
    size_t chan,
    double *bandwidth_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *bandwidth_out = USRP(h)->get_rx_bandwidth(chan);
    )
}

uhd_error uhd_usrp_get_rx_bandwidth_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle bandwidth_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        bandwidth_range_out->meta_range_cpp = USRP(h)->get_rx_bandwidth_range(chan);
    )
}

uhd_error uhd_usrp_get_rx_sensor(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    uhd_sensor_value_handle *sensor_value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        delete (*sensor_value_out)->sensor_value_cpp;
        (*sensor_value_out)->sensor_value_cpp = new uhd::sensor_value_t(USRP(h)->get_rx_sensor(name, chan));
    )
}

uhd_error uhd_usrp_get_rx_sensor_names(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *sensor_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*sensor_names_out)->string_vector_cpp = USRP(h)->get_rx_sensor_names(chan);
    )
}

uhd_error uhd_usrp_set_rx_dc_offset_enabled(
    uhd_usrp_handle h,
    bool enb,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_dc_offset(enb, chan);
    )
}

uhd_error uhd_usrp_set_rx_iq_balance_enabled(
    uhd_usrp_handle h,
    bool enb,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_rx_iq_balance(enb, chan);
    )
}

/****************************************************************************
 * TX methods
 ***************************************************************************/

uhd_error uhd_usrp_set_tx_subdev_spec(
    uhd_usrp_handle h,
    uhd_subdev_spec_handle subdev_spec,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_tx_subdev_spec(subdev_spec->subdev_spec_cpp, mboard);
    )
}

uhd_error uhd_usrp_get_tx_subdev_spec(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_subdev_spec_handle subdev_spec_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        subdev_spec_out->subdev_spec_cpp = USRP(h)->get_tx_subdev_spec(mboard);
    )
}


uhd_error uhd_usrp_get_tx_num_channels(
    uhd_usrp_handle h,
    size_t *num_channels_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_channels_out = USRP(h)->get_tx_num_channels();
    )
}

uhd_error uhd_usrp_get_tx_subdev_name(
    uhd_usrp_handle h,
    size_t chan,
    char* tx_subdev_name_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string tx_subdev_name = USRP(h)->get_tx_subdev_name(chan);
        strncpy(tx_subdev_name_out, tx_subdev_name.c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_set_fir_taps(
    uhd_usrp_handle h,
    size_t nof_prb,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        // map physical resource block to bandwidth index
        uint8_t idx;
        switch (nof_prb) {
            case 6:
                idx = 0;
                break;
            case 7:
                idx = 1;
                break;
            case 15:
                idx = 2;
                break;
            case 25:
                idx = 3;
                break;
            default:
                idx = 1;
        }
        get_usrp_ptrs()[h->usrp_index].fir_ctrls.at(chan)->set_taps(FIR_coeffs[idx]);
    )
}


uhd_error uhd_usrp_set_tx_rate( // RFNoC updated
    uhd_usrp_handle h,
    double rate,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //USRP(h)->set_tx_rate(rate, chan);
        get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->set_arg<double>("input_rate", rate);
    )
}

uhd_error uhd_usrp_get_tx_rate( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    double *rate_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //*rate_out = USRP(h)->get_tx_rate(chan);
        *rate_out = get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->get_arg<double>("input_rate");
    )
}

uhd_error uhd_usrp_get_tx_rates(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle rates_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        rates_out->meta_range_cpp = USRP(h)->get_tx_rates(chan);
    )
}

uhd_error uhd_usrp_set_tx_freq( // RFNoC updated
    uhd_usrp_handle h,
    uhd_tune_request_t *tune_request,
    size_t chan,
    uhd_tune_result_t *tune_result
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::tune_request_t tune_request_cpp = uhd_tune_request_c_to_cpp(tune_request);
        //uhd::tune_result_t tune_result_cpp = USRP(h)->set_tx_freq(tune_request_cpp, chan);
        uhd::tune_result_t tune_result_cpp;
        double freq = tune_request_cpp.target_freq;
        double rf_freq, lo_off;
        if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            rf_freq = freq;
        } else {
            rf_freq = tune_request_cpp.rf_freq;
        }
        if (tune_request_cpp.dsp_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            lo_off = rf_freq - freq;
        } else {
            lo_off = tune_request_cpp.dsp_freq;
            if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
                rf_freq = freq + lo_off;
            }
        }

        get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_tx_frequency(rf_freq, 0);
        double actual_rf_freq = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_tx_frequency(0);
        radio_tx_freq[chan] = actual_rf_freq;
        get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->set_arg<double>("freq", rf_freq - actual_rf_freq - lo_off);
        double actual_dsp_freq = get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->get_arg<double>("freq");

        tune_result_cpp.clipped_rf_freq = rf_freq;
        tune_result_cpp.actual_rf_freq = actual_rf_freq;
        tune_result_cpp.actual_dsp_freq = actual_dsp_freq;
        if (tune_request_cpp.rf_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            tune_result_cpp.target_rf_freq = freq + actual_rf_freq - rf_freq;
        } else {
            tune_result_cpp.target_rf_freq = rf_freq;
        }
        if (tune_request_cpp.dsp_freq_policy == uhd::tune_request_t::POLICY_AUTO) {
            tune_result_cpp.target_dsp_freq = lo_off + actual_rf_freq - rf_freq;
        } else {
            tune_result_cpp.target_dsp_freq = lo_off;
        }
        uhd_tune_result_cpp_to_c(tune_result_cpp, tune_result);
    )
}

uhd_error uhd_usrp_get_tx_freq( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    double *freq_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        //*freq_out = USRP(h)->get_tx_freq(chan);
        *freq_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_tx_frequency(0) + get_usrp_ptrs()[h->usrp_index].duc_ctrls.at(chan)->get_arg<double>("freq");
    )
}

uhd_error uhd_usrp_get_tx_freq_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle freq_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        freq_range_out->meta_range_cpp = USRP(h)->get_tx_freq_range(chan);
    )
}

uhd_error uhd_usrp_get_fe_tx_freq_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle freq_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        freq_range_out->meta_range_cpp = USRP(h)->get_fe_tx_freq_range(chan);
    )
}

uhd_error uhd_usrp_set_tx_gain( // RFNoC updated
    uhd_usrp_handle h,
    double gain,
    size_t chan,
    const char *gain_name
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string name(gain_name);
        if(name.empty()){
            USRP(h)->set_tx_gain(gain, chan);
            //get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_tx_gain(gain, chan);
        }
        else{
            USRP(h)->set_tx_gain(gain, name, chan);
            //get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->set_tx_gain(gain, name, chan);
        }
    )
}

uhd_error uhd_usrp_set_normalized_tx_gain(
    uhd_usrp_handle h,
    double gain,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_normalized_tx_gain(gain, chan);
    )
}

uhd_error uhd_usrp_get_tx_gain( // RFNoC updated
    uhd_usrp_handle h,
    size_t chan,
    const char *gain_name,
    double *gain_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string name(gain_name);
        if(name.empty()){
            *gain_out = USRP(h)->get_tx_gain(chan);
            //*gain_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_tx_gain(chan);
        }
        else{
            *gain_out = USRP(h)->get_tx_gain(name, chan);
            //*gain_out = get_usrp_ptrs()[h->usrp_index].radio_ctrls.at(chan)->get_tx_gain(name, chan);
        }
    )
}

uhd_error uhd_usrp_get_normalized_tx_gain(
    uhd_usrp_handle h,
    size_t chan,
    double *gain_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *gain_out = USRP(h)->get_normalized_tx_gain(chan);
    )
}

uhd_error uhd_usrp_get_tx_gain_range(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    uhd_meta_range_handle gain_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        gain_range_out->meta_range_cpp = USRP(h)->get_tx_gain_range(name, chan);
    )
}

uhd_error uhd_usrp_get_tx_gain_names(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *gain_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*gain_names_out)->string_vector_cpp = USRP(h)->get_tx_gain_names(chan);
    )
}

uhd_error uhd_usrp_set_tx_antenna(
    uhd_usrp_handle h,
    const char* ant,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_tx_antenna(std::string(ant), chan);
    )
}

uhd_error uhd_usrp_get_tx_antenna(
    uhd_usrp_handle h,
    size_t chan,
    char* ant_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string tx_antenna = USRP(h)->get_tx_antenna(chan);
        strncpy(ant_out, tx_antenna.c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_get_tx_antennas(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *antennas_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*antennas_out)->string_vector_cpp = USRP(h)->get_tx_antennas(chan);
    )
}

uhd_error uhd_usrp_set_tx_bandwidth(
    uhd_usrp_handle h,
    double bandwidth,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_tx_bandwidth(bandwidth, chan);
    )
}

uhd_error uhd_usrp_get_tx_bandwidth(
    uhd_usrp_handle h,
    size_t chan,
    double *bandwidth_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *bandwidth_out = USRP(h)->get_tx_bandwidth(chan);
    )
}

uhd_error uhd_usrp_get_tx_bandwidth_range(
    uhd_usrp_handle h,
    size_t chan,
    uhd_meta_range_handle bandwidth_range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        bandwidth_range_out->meta_range_cpp = USRP(h)->get_tx_bandwidth_range(chan);
    )
}

uhd_error uhd_usrp_get_tx_sensor(
    uhd_usrp_handle h,
    const char* name,
    size_t chan,
    uhd_sensor_value_handle *sensor_value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        delete (*sensor_value_out)->sensor_value_cpp;
        (*sensor_value_out)->sensor_value_cpp = new uhd::sensor_value_t(USRP(h)->get_tx_sensor(name, chan));
    )
}

uhd_error uhd_usrp_get_tx_sensor_names(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *sensor_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*sensor_names_out)->string_vector_cpp = USRP(h)->get_tx_sensor_names(chan);
    )
}

uhd_error uhd_usrp_set_tx_dc_offset_enabled(
    uhd_usrp_handle h,
    bool enb,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_tx_dc_offset(enb, chan);
    )
}

uhd_error uhd_usrp_set_tx_iq_balance_enabled(
    uhd_usrp_handle h,
    bool enb,
    size_t chan
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_tx_iq_balance(enb, chan);
    )
}

/****************************************************************************
 * GPIO methods
 ***************************************************************************/

uhd_error uhd_usrp_get_gpio_banks(
    uhd_usrp_handle h,
    size_t chan,
    uhd_string_vector_handle *gpio_banks_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*gpio_banks_out)->string_vector_cpp = USRP(h)->get_gpio_banks(chan);
    )
}

uhd_error uhd_usrp_set_gpio_attr(
    uhd_usrp_handle h,
    const char* bank,
    const char* attr,
    uint32_t value,
    uint32_t mask,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->set_gpio_attr(std::string(bank), std::string(attr),
                               value, mask, mboard);
    )
}

uhd_error uhd_usrp_get_gpio_attr(
    uhd_usrp_handle h,
    const char* bank,
    const char* attr,
    size_t mboard,
    uint32_t *attr_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *attr_out = USRP(h)->get_gpio_attr(std::string(bank), std::string(attr), mboard);
    )
}

uhd_error uhd_usrp_enumerate_registers(
    uhd_usrp_handle h,
    size_t mboard,
    uhd_string_vector_handle *registers_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*registers_out)->string_vector_cpp = USRP(h)->enumerate_registers(mboard);
    )
}

uhd_error uhd_usrp_get_register_info(
    uhd_usrp_handle h,
    const char* path,
    size_t mboard,
    uhd_usrp_register_info_t *register_info_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::usrp::multi_usrp::register_info_t register_info_cpp = USRP(h)->get_register_info(path, mboard);
        register_info_out->bitwidth = register_info_cpp.bitwidth;
        register_info_out->readable = register_info_cpp.readable;
        register_info_out->writable = register_info_cpp.writable;
    )
}

uhd_error uhd_usrp_write_register(
    uhd_usrp_handle h,
    const char* path,
    uint32_t field,
    uint64_t value,
    size_t mboard
){
    UHD_SAFE_C_SAVE_ERROR(h,
        USRP(h)->write_register(path, field, value, mboard);
    )
}

uhd_error uhd_usrp_read_register(
    uhd_usrp_handle h,
    const char* path,
    uint32_t field,
    size_t mboard,
    uint64_t *value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *value_out = USRP(h)->read_register(path, field, mboard);
    )
}
