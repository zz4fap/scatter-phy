

# 
# Copyright 2018 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(FPGA_TOP_DIR)/usrp3/top
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
RFNOC_SCATTER_DIR = $(abspath ../../)

$(info RFNOC_SCATTER_DIR="$(RFNOC_SCATTER_DIR)")
# Include makefiles and sources for all IP components
# *after* defining the RFNOC_SCATTER_DIR
include $(RFNOC_SCATTER_DIR)/ip/McF_block_RAM/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/axi_buffer/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/circ_buff_ram/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/exp_shifter_fifo/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/input_buff_RAM/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/pfb_mac/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/pfb_mac_0/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/pfb_taps/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/phase_offset/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/hb_fil/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/sample_delay/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/sample_ram/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/xfft_stream_var/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/exp_averager_filter/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/exp_corr_rom/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/exp_corr_mult/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_MCF_BLOCK_RAM_SRCS) \
$(LIB_IP_AXI_BUFFER_SRCS) \
$(LIB_IP_CIRC_BUFF_RAM_SRCS) \
$(LIB_IP_EXP_SHIFTER_FIFO_SRCS) \
$(LIB_IP_HB_FIL_SRCS) \
$(LIB_IP_INPUT_BUFF_RAM_SRCS) \
$(LIB_IP_PFB_MAC_SRCS) \
$(LIB_IP_PFB_MAC_0_SRCS) \
$(LIB_IP_PFB_TAPS_SRCS) \
$(LIB_IP_PHASE_OFFSET_SRCS) \
$(LIB_IP_SAMPLE_DELAY_SRCS) \
$(LIB_IP_SAMPLE_RAM_SRCS) \
$(LIB_IP_XFFT_STREAM_VAR_SRCS) \
$(LIB_IP_EXP_AVERAGER_FILTER_SRCS) \
$(LIB_IP_EXP_CORR_ROM_SRCS) \
$(LIB_IP_EXP_CORR_MULT_SRCS))

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
SIM_TOP = noc_block_scatterPHY_tb


# Add test bench, user design under test, and
# additional user created files
SIM_SRCS = \
$(abspath noc_block_scatterPHY_tb.sv) \
$(abspath ../../fpga-src/McFsource/noc_block_McFsource.v) \
$(abspath ../../fpga-src/McFsource/McFsource.v) \
$(abspath ../../fpga-src/McFsource/cvita_hdr_encoder.v) \
$(abspath ../../fpga-src/channelizer/axi_demux8_adj.v) \
$(abspath ../../fpga-src/channelizer/channelizer_top.v) \
$(abspath ../../fpga-src/channelizer/circ_buffer.v) \
$(abspath ../../fpga-src/channelizer/count_cycle_iw36_cw11.v) \
$(abspath ../../fpga-src/channelizer/count_items_iw36_cw11.v) \
$(abspath ../../fpga-src/channelizer/exp_shifter.v) \
$(abspath ../../fpga-src/channelizer/input_buffer.v) \
$(abspath ../../fpga-src/channelizer/pfb_2x.v) \
$(abspath ../../fpga-src/channelizer/noc_block_channelizer.v) \

MODELSIM_USER_DO =

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
