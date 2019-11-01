import select
import socket
import time
import getopt
import queue
from time import sleep
import signal, os

import sys
sys.path.append('../../')
sys.path.append('../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

# Number of bits (TB) that can be transmitted in one suframe, i.e., 1 ms. These number are for a bandwidth of 5 MHz, i.e., 25 RBs.
NUM_BYTES_PER_SUBFRAME_VS_MCS = [85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292];

# Supported bandwidths
BW_INDEX_UNKNOWN      = 0  # UNKNOWN
BW_INDEX_1DOT4_MHZ    = 1  # 1.4 MHz
BW_INDEX_3_MHZ	      = 2  # 3 MHz
BW_INDEX_5_MHZ	   	  = 3  # 5 MHz
BW_INDEX_10_MHZ		  = 4  # 10 MHz

# Physical Layer States.
PHY_UNKNOWN_ST          = 0
PHY_RX_ST               = 1
PHY_TX_ST               = 2

# PHY Results.
PHY_UNKNOWN             = 0
PHY_SUCCESS             = 100
PHY_ERROR               = 101
PHY_TIMEOUT             = 102

# ************ Functions ************
exit_flag = False
def handler(signum, frame):
    global exit_flag
    exit_flag = True

def getExitFlag():
    global exit_flag
    return exit_flag

def help():
    print("Usage: pyhton3 transmit_data.py")

def printPhyTxStat(stats, seq_number, status):
    print("************************ TX PHY Stats Packet ************************")
    print("seq_number: ",seq_number)
    print("status: ",status)
    print("host_timestamp: ",stats.host_timestamp)
    print("fpga_timestamp: ",stats.fpga_timestamp)
    print("frame: ",stats.frame)
    print("slot: ",stats.slot)
    print("ch: ",stats.ch)
    print("mcs: ",stats.mcs)
    print("num_cb_total: ",stats.num_cb_total)
    print("num_cb_err: ",stats.num_cb_err)
    print("power: ",stats.tx_stat.power)
    print("*********************************************************************\n")

def printBasicControl(basic_control, seq_number, tx_data):
    print("******************** Basic Control CMD Transmitted ******************")
    print("trx_flag: ",basic_control.trx_flag)
    print("seq_number: ",seq_number)
    print("bw_index: ",basic_control.bw_index)
    print("ch: ",basic_control.ch)
    print("slot: ",basic_control.slot)
    print("mcs: ",basic_control.mcs)
    print("gain: ",basic_control.gain)
    print("length: ",basic_control.length)
    if(basic_control.trx_flag == 1):
        data = []
        for d in tx_data:
            data.append(int(d))
        print("data: ",data)
    print("*********************************************************************\n")

def inputOptions(argv):
    debug = True # by default debug is enabled.
    single = False # By default we use two computers to run the tests.

    try:
        opts, args = getopt.getopt(argv,"hds",["help","debug","single"])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            help()
            sys.exit()
        elif opt in ("-d", "--debug"):
            debug = True
        elif opt in ("-s", "--single"):
            single = True

    return debug, single

def sendTxCtrlToPhy(lc, chan, mcs_idx, gain_value, slots, source_module):

    # Basic Control
    trx_flag    = TRX_FLAG_TX       # TRX Mode. 2->TX - 1->RX;
    seq_number  = 66                # Sequence number.
    bw_idx      = BW_INDEX_5_MHZ    # By default use BW: 5 MHz. Possible values: 0 - UNKNOWN, 1 - 1.4 MHz, 2 - 3 MHz, 3 - 5 MHz, 4 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    frame       = 0                 # Frame number (not used now, for future use)
    gain        = gain_value        # RX gain. We use -1 for AGC mode.
    num_slots   = slots             # Number of slots. How many data slots we expect to receive from peer.
    send_to     = 0

    # Create an Internal message for TX procedure.
    internal = interf.Internal()
    # Add sequence number to internal message.
    internal.transaction_index = seq_number
    # Set basic control with proper values.
    internal.send.basic_ctrl.trx_flag     = trx_flag
    internal.send.basic_ctrl.send_to      = send_to
    internal.send.basic_ctrl.bw_index     = bw_idx
    internal.send.basic_ctrl.ch           = channel
    internal.send.basic_ctrl.slot         = slot
    internal.send.basic_ctrl.frame        = frame
    internal.send.basic_ctrl.mcs          = mcs
    internal.send.basic_ctrl.gain         = gain
    internal.send.basic_ctrl.length       = num_slots*NUM_BYTES_PER_SUBFRAME_VS_MCS[mcs]
    data = bytes()
    for j in range(num_slots):
        for i in range(NUM_BYTES_PER_SUBFRAME_VS_MCS[mcs]):
            data = data + bytes([i%256])
    internal.send.app_data.data = data

    # Print the basic control structure sent to PHY.
    printBasicControl(internal.send.basic_ctrl, internal.transaction_index, internal.send.app_data.data)

    # Send basic control to PHY.
    print("Sending basic control to PHY.")
    lc.send(Message(source_module, interf.MODULE_PHY, internal))

def receiveTxStatisticsFromPhy(lc, debug):

    print("Waiting for PHY TX Statistics from PHY....")
    while(True):
        try:
            #Try to get next message without waiting.
            msg = lc.get_low_queue().get_nowait()
            internal = msg.message
            seq_number = internal.transaction_index
            status = internal.sendr.result
            tx_stat_msg = internal.sendr.phy_stat
            if(internal.sendr.result > 0 and internal.sendr.result == PHY_SUCCESS):
                print("PHY TX Stats received from PHY layer.")
                printPhyTxStat(tx_stat_msg, seq_number, status)
                break
        except queue.Empty:
            if(getExitFlag() == True):
                print("Exit receiving function.")
                break

if __name__ == '__main__':

    # Parse any input option.
    debug, single = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # decides if single or two host PCs.
    print("Create CommManager object.")
    if(single == False):
        source_module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    else:
        source_module = interf.MODULE_DEBUG2

    # Create a new layer Communicator.
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    while(True):
        print('')
        option = int(input("Options: \n\tTransmission: 0 \n\tExit: 1\nSelected option: "))
        if(option == 1):
            break
        channel = int(input("Please input channel(0~79):"))
        mcs = int(input("Please input MCS(0~28):"))
        gain = int(input("Please input gain (65 is good for B200; 0 is good for X300):"))
        num_slots = int(input("Please input number of data slots (1~18):")) # maximum value

        # Send TX control information to PHY.
        sendTxCtrlToPhy(lc, channel, mcs, gain, num_slots, source_module)

        # Wait for PHY TX Statistics from PHY.
        receiveTxStatisticsFromPhy(lc, debug)

        if(getExitFlag() == True):
            break
