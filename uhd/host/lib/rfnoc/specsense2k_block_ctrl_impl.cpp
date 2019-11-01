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

#include <uhd/rfnoc/specsense2k_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>
#include <math.h>

using namespace uhd::rfnoc;

class specsense2k_block_ctrl_impl : public specsense2k_block_ctrl
{
public:

    static const uint32_t FFT_CONFIG_BUS = 129;
    static const uint32_t AVG_CONFIG_BUS = 131;
    static const uint32_t AVG_SIZE = 134;

    UHD_RFNOC_BLOCK_CONSTRUCTOR(specsense2k_block_ctrl)
    {
       UHD_LOGGER_INFO("RFNOC") << "specsense2k_block::specsense2k_block()" << std::endl;

       int default_fft_size = get_arg<int>("fft_size");
       _tree->access<int>(get_arg_path("fft_size/value"))
           .set_coercer(boost::bind(&specsense2k_block_ctrl_impl::set_fft_size, this, _1))
           .set(default_fft_size)
       ;

       int default_fft_direction = get_arg<int>("fft_direction");
       _tree->access<int>(get_arg_path("fft_direction/value"))
           .set_coercer(boost::bind(&specsense2k_block_ctrl_impl::set_fft_direction, this, _1))
           .set(default_fft_direction)
       ;

       int default_avg_size = get_arg<int>("avg_size");
       _tree->access<int>(get_arg_path("avg_size/value"))
           .set_coercer(boost::bind(&specsense2k_block_ctrl_impl::set_avg_size, this, _1))
           .set(default_avg_size)
       ;

       int default_scaling_sch = get_arg<int>("fft_scaling_sch");
       _tree->access<int>(get_arg_path("fft_scaling_sch/value"))
           .set_coercer(boost::bind(&specsense2k_block_ctrl_impl::set_fft_scaling_sch, this, _1))
           .set(default_scaling_sch)
       ;

    }

private:

    void set_fft_ctrl_word(int fft_size, int fft_direction, int scaling_sch)
    {
       uint32_t fft_ctrl_word = (scaling_sch << 9) + (fft_direction << 8) + std::log2(fft_size);
       UHD_LOGGER_INFO("RFNOC") << boost::format("FFT control word : %08x ") %fft_ctrl_word <<std::endl;
       sr_write(FFT_CONFIG_BUS, fft_ctrl_word);
    }

    void set_avg_ctrl_word(int fft_size, int avg_size)
    {
        uint32_t avg_config_word = (avg_size << 11) + fft_size; // only difference between spec_sense16k and specsense2k   avg_size << 14 for spec_sense16k
        UHD_LOGGER_INFO("RFNOC") << boost::format("AVG config word : %08x ") %avg_config_word <<std::endl;
        sr_write(AVG_CONFIG_BUS, avg_config_word);
    }

    int set_fft_size(const int fft_size)
    {
       UHD_RFNOC_BLOCK_TRACE() << "specsense2k_block::set_fft_size() : " << fft_size << std::endl;
       if(fft_size == 8 || fft_size == 16 || fft_size == 32 || fft_size == 64 || fft_size == 128 || fft_size == 256 || fft_size == 512 || fft_size == 1024 || fft_size == 2048)
       {  
         int fft_direction = get_arg<int>("fft_direction");
         int scaling_sch = get_arg<int>("fft_scaling_sch");

         set_arg("reset", 1);  // reset 1 and 0 are written

         // Averaging block needs to know the new FFT size
         int avg_size = get_arg<int>("avg_size");
         set_avg_ctrl_word(fft_size, avg_size);

         set_fft_ctrl_word(fft_size, fft_direction, scaling_sch);
    
         return fft_size;
       } 
       else
         throw uhd::value_error(str(boost::format("FFT size not valid (8/16/32/64/128/256/512/1024/2048)"))); 
    }


    int set_avg_size(const int avg_size)
    {
       UHD_RFNOC_BLOCK_TRACE() << "specsense2k_block::set_avg_size() : " << avg_size << std::endl;
       if((1 < avg_size) && ( avg_size < 257))
       {
         int fft_size = get_arg<int>("fft_size");
         int fft_direction = get_arg<int>("fft_direction");
         int scaling_sch = get_arg<int>("fft_scaling_sch");        


         set_arg("reset", 1);  // reset 1 and 0 are written

         set_avg_ctrl_word(fft_size, avg_size);        
         sr_write(AVG_SIZE, avg_size);
         set_fft_ctrl_word(fft_size, fft_direction, scaling_sch);

         //set_fft_ctrl_word(fft_size, fft_direction, scaling_sch);
         return avg_size;
       }
       else
         throw uhd::value_error(str(boost::format("Averaging size not valid (1 < avg size < 257)")));
    }


    int set_fft_direction(const int fft_direction)
    {
       UHD_RFNOC_BLOCK_TRACE() << "specsense2k_block::set_fft_direction() : " << fft_direction << std::endl;
       int scaling_sch = get_arg<int>("fft_scaling_sch");
       int fft_size = get_arg<int>("fft_size");
       if((fft_direction != 0) && (fft_direction != 1))
           throw uhd::value_error(str(boost::format("FFT direction can be either 0(inverse) or 1(forward) only")));
       else 
       {
          if(fft_direction == 0)
             UHD_LOGGER_INFO("RFNOC") << "setting up as IFFT" << std::endl;
          else
             UHD_LOGGER_INFO("RFNOC") << "setting up as FFT" << std::endl;
          set_fft_ctrl_word(fft_size, fft_direction, scaling_sch);
          return fft_direction;
       }
    }

    int set_fft_scaling_sch(const int scaling_sch)
    {
         UHD_RFNOC_BLOCK_TRACE() << "specsense2k_block::set_fft_scaling_schedule() : " << scaling_sch << std::endl;
         int fft_size = get_arg<int>("fft_size");
         int fft_direction = get_arg<int>("fft_direction");
         set_fft_ctrl_word(fft_size, fft_direction, scaling_sch);
         return scaling_sch;
    }

};

UHD_RFNOC_BLOCK_REGISTER(specsense2k_block_ctrl, "SpecSense2k");
