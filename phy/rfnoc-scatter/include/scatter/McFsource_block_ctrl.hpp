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


#ifndef INCLUDED_LIBUHD_RFNOC_SCATTER_MCFSOURCE_HPP
#define INCLUDED_LIBUHD_RFNOC_SCATTER_MCFSOURCE_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the standard copy RFNoC block.
 *
 */
class UHD_API McFsource_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(McFsource_block_ctrl)

    /*!
     * Your block configuration here
    */
    virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t) = 0;
    virtual void set_enable(const bool enable) = 0;
    virtual void set_sample_len_1ms(const int sample_len_1ms) = 0;
    virtual void set_spp(const int spp) = 0;
    virtual void set_clk_div(const int clk_div) = 0;

}; /* class McFsource_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_SCATTER_MCFSOURCE_BLOCK_CTRL_HPP */
