import select
import socket
import time
import _thread
import threading
import getopt
import queue
import random
from time import sleep
import signal, os

import sys
sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

# ********************************* Definitions *********************************
# Number of bits (TB) that can be transmitted in one suframe, i.e., 1 ms. These number are for a bandwidth of 5 MHz, i.e., 25 RBs.
NUM_BYTES_PER_SUBFRAME_VS_MCS_1MHz4 = [21,29,35,45,56,69,83,97,111,129,129,141,165,185,213,237,245,245,269,293,325,357,381,413,445,477,493,517,605,525,525,525]
NUM_BYTES_PER_SUBFRAME_VS_MCS_3MHz  = [51,69,85,113,141,173,201,237,277,309,309,349,389,437,509,557,597,597,645,701,791,839,919,983,1047,1111,1191,1239,1447,1479,1495,1511]
NUM_BYTES_PER_SUBFRAME_VS_MCS_5MHz  = [91,119,145,189,237,293,341,413,461,533,533,581,653,757,855,951,1015,1015,1063,1207,1303,1415,1511,1668,1788,1860,2004,2100,2417,2449,2481,2513]
NUM_BYTES_PER_SUBFRAME_VS_MCS_10MHz = [185,241,293,381,485,581,685,823,935,1063,1063,1159,1319,1527,1716,1884,2028,2028,2172,2449,2641,2833,3057,3382,3622,3782,4059,4203,4904,4960,4960,5016]
NUM_BYTES_PER_SUBFRAME_VS_MCS_15MHz = [277,365,453,589,717,903,1031,1223,1431,1572,1572,1740,2028,2268,2545,2865,3057,3057,3262,3662,3915,4395,4680,5072,5413,5861,6053,6234,7327,7567,7647,7708]
NUM_BYTES_PER_SUBFRAME_VS_MCS_20MHz = [373,485,613,765,967,1175,1383,1644,1884,2124,2124,2353,2641,3057,3382,3782,4107,4107,4395,4904,5240,5861,6234,6810,7327,7647,8236,8505,10035,10147,10259,10371]

# BW Indexes.
BW_UNKNOWN              = 0 # unknown
BW_IDX_OneDotFour	    = 1	# 1.4 MHz
BW_IDX_Three	   		= 2	# 3 MHz
BW_IDX_Five	   		    = 3	# 5 MHz
BW_IDX_Ten		   		= 4	# 10 MHz
BW_IDX_Fifteen          = 5 # 15 MHz
BW_IDX_Twenty           = 6 # 20 MHz

PRB_VECTOR = [6, 15, 25, 50]

BW_IDX_VECTOR = [BW_IDX_OneDotFour, BW_IDX_Three, BW_IDX_Five, BW_IDX_Ten]

NOF_SLOTS_VECTOR = [1, 10, 25]

MCS_VECTOR = [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 31]

tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def getTransportBlockSize(index, mcs):
    if index == BW_IDX_OneDotFour:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_1MHz4[mcs]
    elif index == BW_IDX_Three:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_3MHz[mcs]
    elif index == BW_IDX_Five:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_5MHz[mcs]
    elif index == BW_IDX_Ten:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_10MHz[mcs]
    elif index == BW_IDX_Fifteen:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_15MHz[mcs]
    elif index == BW_IDX_Twenty:
        tb_size = NUM_BYTES_PER_SUBFRAME_VS_MCS_20MHz[mcs]
    else:
        tb_size = -1;

    return tb_size

if __name__ == '__main__':

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # Number of milliseconds between successive transmissions.
    guard_time = 2 # given in number of milliseconds.
    # Number of PHYs.
    nof_phys = 2

    for prb_idx in range(0,4):

        prb = PRB_VECTOR[prb_idx]

        bw_idx = BW_IDX_VECTOR[prb_idx]

        for mcs_idx in range(0,len(MCS_VECTOR)):

            mcs = MCS_VECTOR[mcs_idx]

            tb_size = getTransportBlockSize(bw_idx, mcs)

            streaming_tput = nof_phys*((tb_size*8)/0.001)

            for nof_slots_idx in range(0,3):

                nof_slots = NOF_SLOTS_VECTOR[nof_slots_idx]

                dc_percentage = (nof_slots)/(nof_slots+guard_time)

                dtx_tput = dc_percentage*streaming_tput

                print("%d\t%d\t%d\t%d\t%f\t%f\t%f" % (prb,mcs,tb_size,nof_slots,dc_percentage,streaming_tput,dtx_tput))
