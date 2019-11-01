##################################################
# RFNoC Sources
##################################################

RFNOC_SRCS += $(addprefix $(RFNOC_SCATTER_DIR)/fpga-src/, \
McFsource/cvita_hdr_encoder.v \
McFsource/McFsource.v \
McFsource/noc_block_McFsource.v \
specsense16k/pkt_avg.v \
specsense16k/noc_block_specsense16k.v \
specsense2k/pkt_avg.v \
specsense2k/noc_block_specsense2k.v \
fir129/noc_block_fir129.v \
)
