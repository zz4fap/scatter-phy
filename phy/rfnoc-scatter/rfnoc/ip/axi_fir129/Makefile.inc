# 
# Copyright 2017 EJ Kreinar
# 

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_AXI_FIR129_SRCS = $(IP_BUILD_DIR)/axi_fir129/axi_fir129.xci

LIB_IP_AXI_FIR129_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_fir129/, \
axi_fir129.xci.out \
synth/axi_fir129.vhd \
) 

$(LIB_IP_AXI_FIR129_SRCS) $(LIB_IP_AXI_FIR129_OUTS) : $(RFNOC_SCATTER_DIR)/ip/axi_fir129/axi_fir129.xci
	$(call BUILD_VIVADO_IP,axi_fir129,$(ARCH),$(PART_ID),$(RFNOC_SCATTER_DIR)/ip,$(IP_BUILD_DIR),0)
