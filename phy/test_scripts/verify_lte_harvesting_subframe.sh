#!/bin/bash

# Modified Transport Block size for different Physical Resource Blocks (source tbs_tables_new.c)
TBS_PRB_6=(    168  232  280  360  448  552   664   776   888  1032  1032  1128  1320  1480  1704  1896  1960  1960  2152  2344  2600  2856  3048  3304  3560  3816  3944  4136  4840 )
TBS_PRB_15=(   408  552  680  904 1128 1384  1608  1896  2216  2472  2472  2792  3112  3496  4072  4456  4776  4776  5160  5608  6328  6712  7352  7864  8376  8888  9528  9912 11576 )
TBS_PRB_25=(   728  952 1160 1512 1896 2344  2728  3304  3688  4264  4264  4648  5224  6056  6840  7608  8120  8120  8504  9656 10424 11320 12088 13344 14304 14880 16032 16800 19336 )
TBS_PRB_50=(  1480 1928 2344 3048 3880 4648  5480  6584  7480  8504  8504  9272 10552 12216 13728 15072 16224 16224 17376 19592 21128 22664 24456 27056 28976 30256 32472 33624 39232 )
TBS_PRB_75=(  2216 2920 3624 4712 5736 7224  8248  9784 11448 12576 12576 13920 16224 18144 20360 22920 24456 24456 26096 29296 31320 35160 37440 40576 43304 46888 48424 49872 58616 )
TBS_PRB_100=( 2984 3880 4904 6120 7736 9400 11064 13152 15072 16992 16992 18824 21128 24456 27056 30256 32856 32856 35160 39232 41920 46888 49872 54480 58616 61176 65888 68040 80280 )

ENCODE_SUBFRAME_EXEC="../../build/phy/srslte/examples/encode_subframe"
DECODE_SUBFRAME_EXEC="../../build/phy/srslte/examples/decode_subframe"

# Print header
printf "%*s" 6 "PRB"
for (( i = 0 ; i < 29 ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

# Bandwidth = 1.4 MHz
printf "%*s" 6 "6"
for (( i = 0 ; i < ${#TBS_PRB_6[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 6 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 6 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_6[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"

# Bandwidth = 3 MHz
printf "%*s" 6 "15"
for (( i = 0 ; i < ${#TBS_PRB_15[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 15 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 15 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_15[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"

# Bandwidth = 5 MHz
printf "%*s" 6 "25"
for (( i = 0 ; i < ${#TBS_PRB_25[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 25 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 25 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_25[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"

# Bandwidth = 10 MHz
printf "%*s" 6 "50"
for (( i = 0 ; i < ${#TBS_PRB_50[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 50 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 50 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_50[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"

# Bandwidth = 15 MHz
printf "%*s" 6 "75"
for (( i = 0 ; i < ${#TBS_PRB_75[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 75 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 75 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_75[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"

# Bandwidth = 20 MHz
printf "%*s" 6 "100"
for (( i = 0 ; i < ${#TBS_PRB_100[@]} ; i++ )) do

	# Encode a subframe
	$ENCODE_SUBFRAME_EXEC -o signal.bin -p 100 -m $i > /dev/null

	# Decode a subframe
	nof_rxd_bytes=$($DECODE_SUBFRAME_EXEC -i signal.bin -p 100 | tr -cd '*' | wc -c)
	if [[ $((8*$nof_rxd_bytes)) -eq ${TBS_PRB_100[$i]} ]]; then
		printf "%*s" 6 "PASS"
	else
		printf "%*s" 6 "FAIL"
	fi
done
printf "\n"
