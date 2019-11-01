

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
FPGA_TOP_DIR = /root/uhd/fpga-src
BASE_DIR = $(FPGA_TOP_DIR)/usrp3/top

# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
RFNOC_SCATTER_DIR = $(abspath ../../)

# Include makefiles and sources for all IP components
# *after* defining the LIB_IP_DIR
include $(RFNOC_SCATTER_DIR)/ip/axi_fft_2k/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_AXI_FFT_2K_SRCS) \
)


#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
SIM_TOP = noc_block_specsense2k_tb

# Add test bench, user design under test, and
# additional user created files
include $(RFNOC_SCATTER_DIR)/fpga-src/Makefile.inc

SIM_SRCS = \
$(abspath noc_block_specsense2k_tb.sv) \

MODELSIM_USER_DO =

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
