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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "McFsource_impl.h"
namespace gr {
  namespace scatter {
    McFsource::sptr
    McFsource::make(
        const gr::ettus::device3::sptr &dev,
        const ::uhd::stream_args_t &tx_stream_args,
        const ::uhd::stream_args_t &rx_stream_args,
        const int block_select,
        const int device_select
    )
    {
      return gnuradio::get_initial_sptr(
        new McFsource_impl(
            dev,
            tx_stream_args,
            rx_stream_args,
            block_select,
            device_select
        )
      );
    }

    /*
     * The private constructor
     */
    McFsource_impl::McFsource_impl(
         const gr::ettus::device3::sptr &dev,
         const ::uhd::stream_args_t &tx_stream_args,
         const ::uhd::stream_args_t &rx_stream_args,
         const int block_select,
         const int device_select
    )
      : gr::ettus::rfnoc_block("McFsource"),
        gr::ettus::rfnoc_block_impl(
            dev,
            gr::ettus::rfnoc_block_impl::make_block_id("McFsource",  block_select, device_select),
            tx_stream_args, rx_stream_args
            )
    {}

    /*
     * Our virtual destructor.
     */
    McFsource_impl::~McFsource_impl()
    {
    }

    // Enable/disable McFsource block
    void McFsource_impl::set_enable(const bool enable)
    {
        get_block_ctrl_throw< ::uhd::rfnoc::McFsource_block_ctrl>()-> set_enable(enable);
    }
    // Set McF Block RAM samples per 1msec interval
    void McFsource_impl::set_sample_len_1ms(const int sample_len_1ms)
    {
        get_block_ctrl_throw< ::uhd::rfnoc::McFsource_block_ctrl>()-> set_sample_len_1ms(sample_len_1ms);
    }
    // Set McF Block samples per packet
    void McFsource_impl::set_spp(const int spp)
    {
        get_block_ctrl_throw< ::uhd::rfnoc::McFsource_block_ctrl>()-> set_spp(spp);
    }
    // Set clock divider for outcoming data
    void McFsource_impl::set_clk_div(const int clk_div)
    {
        get_block_ctrl_throw< ::uhd::rfnoc::McFsource_block_ctrl>()-> set_clk_div(clk_div);
    }

  } /* namespace scatter */
} /* namespace gr */

