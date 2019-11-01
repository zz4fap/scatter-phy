# 
# Copyright 2017 EJ Kreinar
# 

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_AXI_FFT_16K_SRCS = $(IP_BUILD_DIR)/axi_fft_16k/axi_fft_16k.xci

LIB_IP_AXI_FFT_16K_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_fft_16k/, \
axi_fft_16k.xci.out \
synth/axi_fft_16k.vhd \
) 

$(LIB_IP_AXI_FFT_16K_SRCS) $(LIB_IP_AXI_FFT_16K_OUTS) : $(RFNOC_SCATTER_DIR)/ip/axi_fft_16k/axi_fft_16k.xci
	$(call BUILD_VIVADO_IP,axi_fft_16k,$(ARCH),$(PART_ID),$(RFNOC_SCATTER_DIR)/ip,$(IP_BUILD_DIR),0)
