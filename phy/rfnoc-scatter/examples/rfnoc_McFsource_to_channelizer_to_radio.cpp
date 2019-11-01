//
// Copyright 2014-2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/types/tune_request.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/McFsource_block_ctrl.hpp>
#include <uhd/rfnoc/channelizer_block_ctrl.hpp>
#include <uhd/rfnoc/duc_block_ctrl.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int)
{
	stop_signal_called = true;
}


int UHD_SAFE_MAIN(int argc, char *argv[])
{
	uhd::set_thread_priority_safe();

	//variables to be set by po
	std::string args, subdev, ref, wirefmt, radio_args;
	size_t spb, radio_chan, sym_rate, sym_len;
	double rate, txfreq, txgain, total_time, setup_time;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "help message")
	("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
	("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
	("progress", "periodically display short-term bandwidth")

	("args", po::value<std::string>(&args)->default_value("master_clock_rate=184.32e6,skip_ddc,skip_duc,skip_sram"), "USRP device address args")
	("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")

	("radio-chan", po::value<size_t>(&radio_chan)->default_value(0), "Radio channel")
	("rate", po::value<double>(&rate)->default_value(184320000), "Rate of the radio block")
	("txfreq", po::value<double>(&txfreq)->default_value(5000000000), "TX RF center frequency in Hz")
	("txgain", po::value<double>(&txgain)->default_value(0), "Txgain for the RF chain")
	("sym_rate", po::value<size_t>(&sym_rate)->default_value(15000), "LTE symbol rate")
	("sym_len", po::value<size_t>(&sym_len)->default_value(768), "LTE symbol length in bins (e.g.  768 bins = 96 bins per vChannel * 8 vChannels)")

	("radio-args", po::value<std::string>(&radio_args), "Radio channel")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help"))
	{
		std::cout << boost::format("UHD/RFNoC McF samples to radio %s") % desc << std::endl;
		return ~0;
	}

	/************************************************************************
	 * Create device and block controls
	 ***********************************************************************/
	std::cout << std::endl;
	std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
	uhd::device3::sptr usrp = uhd::device3::make(args);

        // Initialize RFNoC blocks
	uhd::rfnoc::block_id_t McFsource_ctrl_id = uhd::rfnoc::block_id_t(0, "McFsource", 0);
	uhd::rfnoc::block_id_t channelizer_ctrl_id = uhd::rfnoc::block_id_t(0, "channelizer", 0);
	uhd::rfnoc::block_id_t duc_ctrl_id = uhd::rfnoc::block_id_t(0, "DUC", 0);
	uhd::rfnoc::block_id_t radio_ctrl_id = uhd::rfnoc::block_id_t(0, "Radio", 0);

	// These next lines will fail if the they are not actually available
	uhd::rfnoc::McFsource_block_ctrl::sptr McFsource_ctrl = usrp->get_block_ctrl< uhd::rfnoc::McFsource_block_ctrl >(McFsource_ctrl_id);
	uhd::rfnoc::channelizer_block_ctrl::sptr channelizer_ctrl = usrp->get_block_ctrl< uhd::rfnoc::channelizer_block_ctrl >(channelizer_ctrl_id);
	uhd::rfnoc::duc_block_ctrl::sptr duc_ctrl = usrp->get_block_ctrl< uhd::rfnoc::duc_block_ctrl >(duc_ctrl_id);
	uhd::rfnoc::radio_ctrl::sptr radio_ctrl = usrp->get_block_ctrl< uhd::rfnoc::radio_ctrl >(radio_ctrl_id);

	/************************************************************************
	 * Set up McFsource
	 ***********************************************************************/
	size_t sample_len_1ms = 1e-3 * sym_rate*sym_len;		// McF Block RAM samples per 1msec interval
	size_t clk_div        = 1e-3 * (rate / sample_len_1ms);		// clock divider for McF data source

	McFsource_ctrl->set_spp(sym_len);
	McFsource_ctrl->set_sample_len_1ms(sample_len_1ms);
	McFsource_ctrl->set_clk_div(clk_div);

	/************************************************************************
	 * Set up channelizer
	 ***********************************************************************/
	size_t vChannels = 8;						// Number of virtual channels

	channelizer_ctrl->set_fft_size(vChannels);
	std::cout << boost::format("Setting channelizer fft size: %f ...") % vChannels << std::endl;

	/************************************************************************
	 * Set up DUC
	 ***********************************************************************/
	size_t channelizer_in_rate = 1e3  * sample_len_1ms;			// Channelizer input  data rate
	size_t channelizer_out_rate = channelizer_in_rate / vChannels;		// Channelizer output data rate

	duc_ctrl->set_args(uhd::device_addr_t(str(boost::format("output_rate=%lf,input_rate=%lf") % rate % channelizer_out_rate)));

	/************************************************************************
	 * Set up Tx radio
	 ***********************************************************************/
	radio_ctrl->set_args(radio_args);
	radio_ctrl->set_arg("spp", 16);
	if (vm.count("rate"))
	{
		std::cout << boost::format("Setting Rate: %f Msps...") % (rate/1e6) << std::endl;
		radio_ctrl->set_rate(rate);
	}
	if (vm.count("txfreq"))
	{
		std::cout << boost::format("Setting TX Freq: %f MHz...") % (txfreq/1e6) << std::endl;
		uhd::tune_request_t tune_request(txfreq);
		radio_ctrl->set_tx_frequency(txfreq, radio_chan);
	}
	if (vm.count("txgain"))
	{
		std::cout << boost::format("Setting TX Gain: %f dB...") % txgain << std::endl;
		radio_ctrl->set_tx_gain(txgain, radio_chan);
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds(long(setup_time*1000))); //allow for some setup time

	/************************************************************************
	 * Connect RFNOC blocks
	 ***********************************************************************/
	uhd::rfnoc::graph::sptr McF_graph = usrp->create_graph("rfnoc_McFsource_to_radio");
	usrp->clear();

	// Flowgraph  McFsource -> channelizer -> DUC -> Radio
	std::cout << "Connecting " << McFsource_ctrl_id << " ==> " << channelizer_ctrl_id << std::endl;
	McF_graph->connect(McFsource_ctrl_id, uhd::rfnoc::ANY_PORT, channelizer_ctrl_id, uhd::rfnoc::ANY_PORT);
	std::cout << "Connecting " << channelizer_ctrl_id << " ==> " << duc_ctrl_id << std::endl;
	McF_graph->connect(channelizer_ctrl_id, uhd::rfnoc::ANY_PORT, duc_ctrl_id, uhd::rfnoc::ANY_PORT);
	std::cout << "Connecting " << duc_ctrl_id << " ==> " << radio_ctrl_id << std::endl;
	McF_graph->connect(duc_ctrl_id, uhd::rfnoc::ANY_PORT, radio_ctrl_id, radio_chan);


	/************************************************************************
	 * Start Operation
	 ***********************************************************************/

	McFsource_ctrl->set_enable(true);

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl;

	while (stop_signal_called == false)
	{}

	//finished
	McFsource_ctrl->set_enable(false);

	std::cout << std::endl << "Done!" << std::endl << std::endl;

	return EXIT_SUCCESS;
}
