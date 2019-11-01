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

template<typename samp_type> void recv_to_file(
    uhd::rx_streamer::sptr rx_stream,
    const std::string &file,
    const size_t samps_per_buff,
    const double rx_rate,
    const unsigned long long num_requested_samples,
    double time_requested = 0.0,
    bool bw_summary = false,
    bool stats = false,
    bool enable_size_map = false,
    bool continue_on_bad_packet = false
)
{
	unsigned long long num_total_samps = 0;

	uhd::rx_metadata_t md;
	std::vector<samp_type> buff(samps_per_buff);
	std::ofstream outfile;
	if (not file.empty())
	{
		outfile.open(file.c_str(), std::ofstream::binary);
	}
	bool overflow_message = true;

	//setup streaming
	uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)?
	                             uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
	                             uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
	                            );
	stream_cmd.num_samps = size_t(num_requested_samples);
	stream_cmd.stream_now = true;
	stream_cmd.time_spec = uhd::time_spec_t();
	std::cout << "Issueing stream cmd" << std::endl;
	rx_stream->issue_stream_cmd(stream_cmd);
	std::cout << "Done" << std::endl;

	boost::system_time start = boost::get_system_time();
	unsigned long long ticks_requested = (long)(time_requested * (double)boost::posix_time::time_duration::ticks_per_second());
	boost::posix_time::time_duration ticks_diff;
	boost::system_time last_update = start;
	unsigned long long last_update_samps = 0;

	typedef std::map<size_t,size_t> SizeMap;
	SizeMap mapSizes;

	while(not stop_signal_called and (num_requested_samples != num_total_samps or num_requested_samples == 0))
	{
		boost::system_time now = boost::get_system_time();

		size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, enable_size_map);

		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
		{
			std::cout << boost::format("Timeout while streaming") << std::endl;
			break;
		}
		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW)
		{
			if (overflow_message)
			{
				overflow_message = false;
				std::cerr << boost::format(
				              "Got an overflow indication. Please consider the following:\n"
				              "  Your write medium must sustain a rate of %fMB/s.\n"
				              "  Dropped samples will not be written to the file.\n"
				              "  Please modify this example for your purposes.\n"
				              "  This message will not appear again.\n"
				          ) % (rx_rate*sizeof(samp_type)/1e6);
			}
			continue;
		}
		if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)
		{
			std::string error = str(boost::format("Receiver error: %s") % md.strerror());
			if (continue_on_bad_packet)
			{
				std::cerr << error << std::endl;
				continue;
			}
			else
				throw std::runtime_error(error);
		}

		if (enable_size_map)
		{
			SizeMap::iterator it = mapSizes.find(num_rx_samps);
			if (it == mapSizes.end())
				mapSizes[num_rx_samps] = 0;
			mapSizes[num_rx_samps] += 1;
		}

		num_total_samps += num_rx_samps;

		if (outfile.is_open())
		{
			outfile.write((const char*)&buff.front(), num_rx_samps*sizeof(samp_type));
		}

		if (bw_summary)
		{
			last_update_samps += num_rx_samps;
			boost::posix_time::time_duration update_diff = now - last_update;
			if (update_diff.ticks() > boost::posix_time::time_duration::ticks_per_second())
			{
				double t = (double)update_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
				double r = (double)last_update_samps / t;
				std::cout << boost::format("\t%f Msps") % (r/1e6) << std::endl;
				last_update_samps = 0;
				last_update = now;
			}
		}

		ticks_diff = now - start;
		if (ticks_requested > 0)
		{
			if ((unsigned long long)ticks_diff.ticks() > ticks_requested)
				break;
		}
	}

	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	std::cout << "Issueing stop stream cmd" << std::endl;
	rx_stream->issue_stream_cmd(stream_cmd);
	std::cout << "Done" << std::endl;

	// Run recv until nothing is left
	int num_post_samps = 0;
	do
	{
		num_post_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0);
	}
	while(num_post_samps and md.error_code == uhd::rx_metadata_t::ERROR_CODE_NONE);

	if (outfile.is_open())
		outfile.close();

	if (stats)
	{
		std::cout << std::endl;

		double t = (double)ticks_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
		std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps % t << std::endl;
		double r = (double)num_total_samps / t;
		std::cout << boost::format("%f Msps") % (r/1e6) << std::endl;

		if (enable_size_map)
		{
			std::cout << std::endl;
			std::cout << "Packet size map (bytes: count)" << std::endl;
			for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
				std::cout << it->first << ":\t" << it->second << std::endl;
		}
	}
}

int UHD_SAFE_MAIN(int argc, char *argv[])
{
	uhd::set_thread_priority_safe();

	//variables to be set by po
	std::string args, file, format, ant, subdev, ref, wirefmt, streamargs;
	size_t total_num_samps, spb, sym_rate, sym_len;
	double rate, total_time, setup_time;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "help message")
	("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
	("format", po::value<std::string>(&format)->default_value("fc32"), "File sample format: sc16, fc32, or fc64")
	("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
	("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
	("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
	("streamargs", po::value<std::string>(&streamargs)->default_value(""), "stream args")
	("progress", "periodically display short-term bandwidth")
	("stats", "show average bandwidth on exit")
	("sizemap", "track packet size and display breakdown on exit")
	("null", "run without writing to file")
	("continue", "don't abort on a bad packet")

	("args", po::value<std::string>(&args)->default_value("master_clock_rate=184.32e6,skip_ddc,skip_duc,skip_sram"), "USRP device address args")
	("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
	("rate", po::value<double>(&rate)->default_value(184320000), "Master clock rate")
	("sym_rate", po::value<size_t>(&sym_rate)->default_value(15000), "LTE symbol rate")
	("sym_len", po::value<size_t>(&sym_len)->default_value(768), "LTE symbol length in bins (e.g.  768 bins = 96 bins per vChannel * 8 vChannels)")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help"))
	{
		std::cout << boost::format("UHD/RFNoC McF samples to file %s") % desc << std::endl;
		std::cout
		        << std::endl
		        << "This application streams data from a single channel of a USRP device to a file.\n"
		        << std::endl;
		return ~0;
	}

	bool bw_summary = vm.count("progress") > 0;
	bool stats = vm.count("stats") > 0;
	if (vm.count("null") > 0)
	{
		file = "";
	}
	bool enable_size_map = vm.count("sizemap") > 0;
	bool continue_on_bad_packet = vm.count("continue") > 0;

	if (enable_size_map)
	{
		std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;
	}

	if (format != "sc16" and format != "fc32" and format != "fc64")
	{
		std::cout << "Invalid sample format: " << format << std::endl;
		return EXIT_FAILURE;
	}

	/************************************************************************
	 * Create device and block controls
	 ***********************************************************************/
	std::cout << std::endl;
	std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
	uhd::device3::sptr usrp = uhd::device3::make(args);

	// Create handle for McFsource object
	uhd::rfnoc::block_id_t McFsource_ctrl_id = uhd::rfnoc::block_id_t(0, "McFsource", 0);

	// This next line will fail if McFsource is not actually available
	uhd::rfnoc::McFsource_block_ctrl::sptr McFsource_ctrl = usrp->get_block_ctrl< uhd::rfnoc::McFsource_block_ctrl >(McFsource_ctrl_id);

	/************************************************************************
	 * Set up McFsource
	 ***********************************************************************/
	size_t sample_len_1ms = 1e-3 * sym_rate*sym_len;		// McF Block RAM samples per 1msec interval
	size_t clk_div        = 1e-3 * (rate / sample_len_1ms);		// clock divider for McF data source
	size_t McFsource_rate = 1e3 * sample_len_1ms;			// McF data source clock rate

	McFsource_ctrl->set_spp(sym_len);
	McFsource_ctrl->set_sample_len_1ms(sample_len_1ms);
	McFsource_ctrl->set_clk_div(clk_div);

	/************************************************************************
	 * Set up streaming
	 ***********************************************************************/
	uhd::device_addr_t streamer_args(streamargs);

	streamer_args["block_id"] = McFsource_ctrl_id.to_string();
	streamer_args["block_port"] = "0";

	//create a receive streamer
	uhd::stream_args_t stream_args(format, "sc16"); // We should read the wire format from the blocks
	stream_args.args = streamer_args;
	std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	if (total_num_samps == 0)
	{
		std::signal(SIGINT, &sig_int_handler);
		std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
	}
#define recv_to_file_args() \
	(rx_stream, file, spb, McFsource_rate, total_num_samps, total_time, bw_summary, stats, enable_size_map, continue_on_bad_packet)
	//recv to file
	if (format == "fc64") recv_to_file<std::complex<double> >recv_to_file_args();
	else if (format == "fc32") recv_to_file<std::complex<float> >recv_to_file_args();
	else if (format == "sc16") recv_to_file<std::complex<short> >recv_to_file_args();
	else throw std::runtime_error("Unknown data format: " + format);

	//finished
	std::cout << std::endl << "Done!" << std::endl << std::endl;

	return EXIT_SUCCESS;
}
