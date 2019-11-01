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

#ifndef INCLUDED_SCATTER_MCFSOURCE_IMPL_H
#define INCLUDED_SCATTER_MCFSOURCE_IMPL_H

#include <scatter/McFsource.h>
#include <scatter/McFsource_block_ctrl.hpp>
#include <ettus/rfnoc_block_impl.h>

namespace gr {
  namespace scatter {

    class McFsource_impl : public McFsource, public gr::ettus::rfnoc_block_impl
    {
     private:
      // Nothing to declare in this block.

     public:
      McFsource_impl(
        const gr::ettus::device3::sptr &dev,
        const ::uhd::stream_args_t &tx_stream_args,
        const ::uhd::stream_args_t &rx_stream_args,
        const int block_select,
        const int device_select
      );
      ~McFsource_impl();

    // Enable/disable McFsource block
    void set_enable(const bool enable);
    // Set McF Block RAM samples per 1msec interval
    void set_sample_len_1ms(const int sample_len_1ms);
    // Set McF Block samples per packet
    void set_spp(const int spp);
    // Set clock divider for outcoming data 
    void set_clk_div(const int clk_div);

      // Where all the action really happens
    };

  } // namespace scatter
} // namespace gr

#endif /* INCLUDED_SCATTER_MCFSOURCE_IMPL_H */

