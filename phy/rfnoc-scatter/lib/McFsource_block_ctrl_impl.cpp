/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#include <scatter/McFsource_block_ctrl.hpp>
#include <uhd/convert.hpp>

using namespace uhd::rfnoc;

static const boost::uint32_t SR_ENABLE         = 129;
static const boost::uint32_t SR_SAMPLE_LEN_1MS = 130;
static const boost::uint32_t SR_SPP            = 131;
static const boost::uint32_t SR_CLK_DIV        = 132;


class McFsource_block_ctrl_impl : public McFsource_block_ctrl
{
public:

    UHD_RFNOC_BLOCK_CONSTRUCTOR(McFsource_block_ctrl)
    {
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t)
    {
        UHD_LOGGER_TRACE(unique_id()) << "issue_stream_cmd()" << std::endl;
        if (not stream_cmd.stream_now) {
            throw uhd::not_implemented_error(
                "McFsource does not support timed commands.");
        }
        switch (stream_cmd.stream_mode) {
            case uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
                sr_write(SR_ENABLE, true);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS:
                sr_write(SR_ENABLE, false);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:
                throw uhd::not_implemented_error(
                    "McFsource does not support streaming modes other than CONTINUOUS");

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    // Enable/disable McFsource block
    void set_enable(const bool enable)
    {
        sr_write(SR_ENABLE, enable);
    }

    // Set McF Block RAM packet len
    void set_sample_len_1ms(const int sample_len_1ms)
    {
        sr_write(SR_SAMPLE_LEN_1MS, uint32_t(sample_len_1ms));
    }

    // Set McF Block samples per packet
    void set_spp(const int spp)
    {
        sr_write(SR_SPP, uint32_t(spp));
    }

    // Set clock divider for outcoming data 
    void set_clk_div(const int clk_div)
    {
        sr_write(SR_CLK_DIV, uint32_t(clk_div));
    }

private:

};

UHD_RFNOC_BLOCK_REGISTER(McFsource_block_ctrl,"McFsource");
