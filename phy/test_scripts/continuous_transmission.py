import select
import socket
import time
import getopt
import queue
import random
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

# Physical Layer States.
PHY_UNKNOWN_ST          = 0
PHY_RX_ST               = 1
PHY_TX_ST               = 2

# BW Indexes.
BW_UNKNOWN              = 0 # unknown
BW_IDX_OneDotFour	    = 1	# 1.4 MHz
BW_IDX_Three	   		= 2	# 3 MHz
BW_IDX_Five	   		    = 3	# 5 MHz
BW_IDX_Ten		   		= 4	# 10 MHz

# ************ Functions ************
exit_flag = False
def handler(signum, frame):
    global exit_flag
    exit_flag = True

def getExitFlag():
    global exit_flag
    return exit_flag

def help():
    print("Usage: pyhton3 continuous_transmission.py")

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

    try:
        opts, args = getopt.getopt(argv,"hd",["help","debug"])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            help()
            sys.exit()
        elif opt in ("-d", "--debug"):
            debug = True

    return debug

def sendTxCtrlToPhy(lc, chan, mcs_idx, gain_value, slots, sequence_number, random_data):

    # Basic Control
    trx_flag    = PHY_TX_ST         # TRX Mode. 1 TX - 0 RX; 2
    seq_number  = sequence_number   # Sequence number.
    bw_idx      = BW_IDX_Five       # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    gain        = gain_value        # RX gain. We use -1 for AGC mode.
    num_slots   = slots             # Number of slots. How many data slots we expect to receive from peer.

    # Create an Internal message for TX procedure.
    internal = interf.Internal()
    # Add sequence number to internal message.
    internal.transaction_index = seq_number
    # Set basic control with proper values.
    internal.send.basic_ctrl.trx_flag     = trx_flag
    internal.send.basic_ctrl.bw_index     = bw_idx
    internal.send.basic_ctrl.ch           = channel
    internal.send.basic_ctrl.slot         = slot
    internal.send.basic_ctrl.mcs          = mcs
    internal.send.basic_ctrl.gain         = gain
    internal.send.basic_ctrl.length       = num_slots*NUM_BYTES_PER_SUBFRAME_VS_MCS[mcs]
    data = bytes()
    for j in range(num_slots):
        for i in range(NUM_BYTES_PER_SUBFRAME_VS_MCS[mcs]):
            data = data + bytes([seq_number+1])
            #data = data + bytes([random_data[i]])
            #data = data + bytes([i%256])
            #data = data + bytes([seq_number+1])
    internal.send.app_data.data = data

    # Check size of transmited data.
    if(len(internal.send.app_data.data) != internal.send.basic_ctrl.length):
        print("Length of data is diffrent of field length.")
        sys.exit(-1)

    # Print the basic control structure sent to PHY.
    printBasicControl(internal.send.basic_ctrl, internal.transaction_index, internal.send.app_data.data)

    # Send basic control to PHY.
    print("Sending basic control to PHY.")
    lc.send(Message(interf.MODULE_MAC, interf.MODULE_PHY, internal))

def receiveTxStatisticsFromPhy(lc, sequence_number, debug):

    print("Waiting for PHY TX Statistics from PHY....")
    while(True):
        try:
            #Try to get next message without waiting.
            msg = lc.get_low_queue().get_nowait()
            internal = msg.message
            seq_number = internal.transaction_index
            status = internal.sendr.result
            tx_stat_msg = internal.sendr.phy_stat
            if(sequence_number == seq_number):
                print("PHY TX Stats received from PHY layer.")
                printPhyTxStat(tx_stat_msg, seq_number, status)
                break
            else:
                print("Sequence number out of sequence...............")
                sys.exit(-1)
        except queue.Empty:
            if(getExitFlag() == True):
                print("Exit receiving function.")
                break

def generateRandomData(num_slots, mcs):
    random_data = bytes()
    for j in range(num_slots):
        for i in range(NUM_BYTES_PER_SUBFRAME_VS_MCS[mcs]):
            random_data = random_data + bytes([random.randint(0, 255)])

    return random_data

if __name__ == '__main__':

    # Parse any input option.
    debug = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    print("Create CommManager object.")
    module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    lc = LayerCommunicator(module)

    channel = 0
    mcs = 0
    gain = 30 # Good value for b200: 30 # for x310 use 200
    num_slots = 1
    seq_number = 0
    num_of_tx = -1
    random_data = generateRandomData(num_slots, mcs)
    while(True):

        # Send TX control information to PHY.
        sendTxCtrlToPhy(lc, channel, mcs, gain, num_slots, seq_number, random_data)

        # Wait for PHY TX Statistics from PHY.
        receiveTxStatisticsFromPhy(lc, seq_number, debug)

        # Increment sequence number.
        seq_number = (seq_number + 1)%100

        time.sleep(0.1)

        if(getExitFlag() == True or seq_number == num_of_tx):
            break
