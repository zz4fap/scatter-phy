#
# Copyright 2018 Ettus Research
#

include $(RFNOC_SCATTER_DIR)/ip/McF_block_RAM/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/axi_fft_16k/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/axi_fft_2k/Makefile.inc
include $(RFNOC_SCATTER_DIR)/ip/axi_fir129/Makefile.inc



LIB_OOT_IP_XCI_SRCS = \
$(LIB_IP_MCF_BLOCK_RAM_SRCS) \
$(LIB_IP_AXI_FFT_16K_SRCS) \
$(LIB_IP_AXI_FFT_2K_SRCS) \
$(LIB_IP_AXI_FIR129_SRCS)

LIB_OOT_IP_SYNTH_OUTPUTS = \
$(LIB_IP_MCF_BLOCK_RAM_OUTS) \
$(LIB_IP_AXI_FFT_16K_OUTS) \
$(LIB_IP_AXI_FFT_2K_OUTS) \
$(LIB_IP_AXI_FIR129_OUTS)

LIB_IP_XCI_SRCS += $(LIB_OOT_IP_XCI_SRCS)
