# Specify all relevant Makefiles to include

RFNOC_SCATTER_DIR := $(OOT_DIR)
include $(abspath $(RFNOC_SCATTER_DIR)/fpga-src/Makefile.inc)
include $(abspath $(RFNOC_SCATTER_DIR)/ip/Makefile.inc)
