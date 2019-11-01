#!/bin/bash

# Modified Transport Block size for different Physical Resource Blocks (source tbs_tables_new.c)
TBS_PRB_6=(    168  232  280  360  448  552   664   776   888  1032  1032  1128  1320  1480  1704  1896  1960  1960  2152  2344  2600  2856  3048  3304  3560  3816  3944  4136 )
TBS_PRB_7=(    200  248  328  432  536  664   792   936  1064  1224  1224  1352  1544  1768  1992  2216  2408  2408  2536  2792  3112  3304  3624  3880  4136  4456  4712  4840 )
TBS_PRB_15=(   408  552  680  904 1128 1384  1608  1896  2216  2472  2472  2792  3112  3496  4072  4456  4776  4776  5160  5608  6328  6712  7352  7864  8376  8888  9528  9912 11576 )
TBS_PRB_25=(   728  952 1160 1512 1896 2344  2728  3304  3688  4264  4264  4648  5224  6056  6840  7608  8120  8120  8504  9656 10424 11320 12088 13344 14304 14880 16032 16800 19336 )
TBS_PRB_50=(  1480 1928 2344 3048 3880 4648  5480  6584  7480  8504  8504  9272 10552 12216 13728 15072 16224 16224 17376 19592 21128 22664 24456 27056 28976 30256 32472 33624 39232 )
TBS_PRB_75=(  2216 2920 3624 4712 5736 7224  8248  9784 11448 12576 12576 13920 16224 18144 20360 22920 24456 24456 26096 29296 31320 35160 37440 40576 43304 46888 48424 49872 58616 )
TBS_PRB_100=( 2984 3880 4904 6120 7736 9400 11064 13152 15072 16992 16992 18824 21128 24456 27056 30256 32856 32856 35160 39232 41920 46888 49872 54480 58616 61176 65888 68040 80280 )

ENCODE_SUPERFRAME_EXEC="../../build/phy/srslte/examples/encode_superframe"
DECODE_SUPERFRAME_EXEC="../../build/phy/srslte/examples/decode_superframe"

# Bandwidth = 1.4 MHz
printf "PRB_SIZE = 6\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_6[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_6[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 6 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 6 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_6[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done
printf "\n"
printf "PRB_SIZE = 7\n"
printf "============\n"
printf "%*s" 14 "NOF_SUBFRAMES"
for (( i = 0 ; i < 29 ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 14 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_7[@]} ; j++ )) do
		# Encode a subframe
		$ENCODE_SUPERFRAME_EXEC -o signal.bin -p 7 -n $i -m $j > /dev/null

		# Decode a subframe
		nof_rxd_bytes=$($DECODE_SUPERFRAME_EXEC -i signal.bin -p 7 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_7[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done
printf "\n"

# Bandwidth = 3 MHz
printf "PRB_SIZE = 15\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_15[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_15[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 15 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 15 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_15[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done
printf "\n"

# Bandwidth = 5 MHz
printf "PRB_SIZE = 25\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_25[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_25[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 25 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 25 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_25[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done

# Bandwidth = 10 MHz
printf "PRB_SIZE = 50\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_50[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_50[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 50 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 50 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_50[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done
printf "\n"

# Bandwidth = 15 MHz
printf "PRB_SIZE = 75\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_75[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_75[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 75 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 75 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_75[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done
printf "\n"

# Bandwidth = 20 MHz
printf "PRB_SIZE = 100\n"
printf "============\n"
printf "%*s" 13 "NOF_SUBFRAMES"
for (( i = 0 ; i < ${#TBS_PRB_100[@]} ; i++ )) do
	printf "%*s" 6 "MCS$i"
done
printf "\n"

for (( i = 1 ; i <= 23 ; i++ )) do

	printf "%*s" 13 "$i"
	for (( j = 0 ; j < ${#TBS_PRB_100[@]} ; j++ )) do

		# Encode and decode a superframe
		nof_rxd_bytes=$($ENCODE_SUPERFRAME_EXEC -p 100 -n $i -m $j | $DECODE_SUPERFRAME_EXEC -p 100 | tr -cd '*' | wc -c)
		if [[ $((8*$nof_rxd_bytes)) -eq $(($i*${TBS_PRB_100[$j]})) ]]; then
			printf "%*s" 6 "PASS"
		else
			printf "%*s" 6 "FAIL"
		fi
	done
	printf "\n"
done

