#
# Copyright 2018 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_MCF_BLOCK_RAM_SRCS = $(IP_BUILD_DIR)/McF_block_RAM/McF_block_RAM.xci

LIB_IP_MCF_BLOCK_RAM_OUTS = $(addprefix $(IP_BUILD_DIR)/McF_block_RAM/, \
    McF_block_RAM.xci.out \
    synth/McF_block_RAM.vhd \
) 

$(LIB_IP_MCF_BLOCK_RAM_SRCS) $(LIB_IP_MCF_BLOCK_RAM_OUTS) : $(RFNOC_SCATTER_DIR)/ip/McF_block_RAM/McF_block_RAM.xci
	$(call BUILD_VIVADO_IP,McF_block_RAM,$(ARCH),$(PART_ID),$(RFNOC_SCATTER_DIR)/ip,$(IP_BUILD_DIR),0)
