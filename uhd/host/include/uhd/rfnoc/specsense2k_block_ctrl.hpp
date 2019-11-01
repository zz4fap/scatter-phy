//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_specsense2k_block_ctrl_HPP
#define INCLUDED_LIBUHD_RFNOC_specsense2k_block_ctrl_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the Spectrum Sensing RFNoC block. * 
 *
 * This block consists of a Xilinx FFT core followed by an averaging block. 
 * This block requires packets to be the same size as the FFT length.
 * It will perform one FFT operation per incoming packet, treating it
 * as a vector of samples.
 * Both the FFT core and the averaging block can be configured using AXI
 * config bus. Configurable parameters - FFT size, avg size, fft 
 * direction, scaling schedule
 */
class UHD_RFNOC_API specsense2k_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(specsense2k_block_ctrl)

private:
    //! FFT ctrl word
    virtual void set_fft_ctrl_word(int fft_size, int fft_direction, int scaling_sch) = 0;

    //! AVG ctrl word
    virtual void set_avg_ctrl_word(int fft_size, int avg_size) = 0;

    //! Configure the FFT_size    
    virtual int set_fft_size(const int fft_size) = 0;

    //! Configure the averaging size
    virtual int set_avg_size(const int avg_size) = 0;
  
    //! Configure the fft direction 
    virtual int set_fft_direction(const int fft_direction) = 0;

    //! Configure the scaling schedule
    virtual int set_fft_scaling_sch(const int scaling_sch) = 0;

 

}; /* class specsense2k_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_specsense2k_block_ctrl_HPP */
// vim: sw=4 et:
