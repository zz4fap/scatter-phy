import select
import socket
import time
import getopt
import queue
import _thread
import threading
from time import sleep
import signal, os

import sys
sys.path.append('../../')
sys.path.append('../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

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
rx_exit_flag = False
def handler(signum, frame):
    global rx_exit_flag
    rx_exit_flag = True

def getExitFlag():
    global rx_exit_flag
    return rx_exit_flag

rx_stat_flag_lock = threading.Lock()
is_rx_stat_received = False
def get_is_stat_received():
    rx_stat_flag_lock.acquire()
    global is_rx_stat_received
    flag = is_rx_stat_received
    rx_stat_flag_lock.release()
    return flag

def set_is_stat_received(flag):
    rx_stat_flag_lock.acquire()
    global is_rx_stat_received
    is_rx_stat_received = flag
    rx_stat_flag_lock.release()

rxStatThreadLock = threading.Lock()
rx_sequence_number = 0
def set_seq_number(seq_num):
    # Get lock to synchronize threads
    rxStatThreadLock.acquire()
    global rx_sequence_number
    rx_sequence_number = seq_num
    # Free lock to release next thread
    rxStatThreadLock.release()

def get_seq_number():
    # Get lock to synchronize threads
    rxStatThreadLock.acquire()
    global rx_sequence_number
    seq_num = rx_sequence_number
    # Free lock to release next thread
    rxStatThreadLock.release()
    return seq_num

def help():
    print("Usage: pyhton3 continuous_receptionv2.py")

def printPhyRxStat(stats, pkt_count, seq_number, status, rx_data):
    print("***************** RX PHY Stats Packet", pkt_count, "****************")
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
    print("gain: ",stats.rx_stat.gain)
    print("cqi: ",stats.rx_stat.cqi)
    print("rssi: ",stats.rx_stat.rssi)
    print("rsrp: ",stats.rx_stat.rsrp)
    print("rsrq: ",stats.rx_stat.rsrq)
    print("sinr: ",stats.rx_stat.sinr)
    if(len(rx_data) > 0):
        print("length: ",stats.rx_stat.length)
        data = []
        for d in rx_data:
            data.append(int(d))
        print("data: ",data)
    print("*********************************************************************\n")

def printBasicControl(basic_control, seq_number):
    print("******************** Basic Control CMD Transmitted ******************")
    print("trx_flag: ",basic_control.trx_flag)
    print("seq_number: ",seq_number)
    print("bw_index: ",basic_control.bw_index)
    print("ch: ",basic_control.ch)
    print("slot: ",basic_control.slot)
    print("mcs: ",basic_control.mcs)
    print("gain: ",basic_control.gain)
    print("length: ",basic_control.length)
    print("*********************************************************************\n")

def inputOptions(argv):
    debug = False # by default debug is disabled.
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

def sendRxCtrlToPhy(lc, chan, mcs_idx, rx_gain, num_data_slots, seq_number, module, debug):

    # Basic Control
    trx_flag    = PHY_RX_ST         # TRX Mode. 1 TX - 0 RX;
    send_to     = 0
    seq_number  = seq_number        # Sequence number.
    bw_idx      = BW_IDX_Five       # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx                 # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    frame       = 0
    gain        = rx_gain                # RX gain. We use -1 for AGC mode.
    num_slots   = num_data_slots    # Number of slots. How many data slots we expect to receive from peer.

    # Create an Internal message for RX procedure.
    internal = interf.Internal()
    # Add sequence number to internal message.
    internal.transaction_index = seq_number

    # Add values to the Basic Control Message for RX procedure.
    internal.receive.basic_ctrl.trx_flag     = trx_flag
    internal.receive.basic_ctrl.send_to      = send_to
    internal.receive.basic_ctrl.bw_index     = bw_idx
    internal.receive.basic_ctrl.ch           = channel
    internal.receive.basic_ctrl.frame        = frame
    internal.receive.basic_ctrl.slot         = slot
    internal.receive.basic_ctrl.mcs          = mcs
    internal.receive.basic_ctrl.gain         = gain
    internal.receive.basic_ctrl.length       = num_slots

    # Print the basic control structure sent to PHY.
    if(debug == True): printBasicControl(internal.receive.basic_ctrl, internal.transaction_index)

    # Send basic control to PHY.
    if(debug == True): print("Sending basic control to PHY.")
    print("Sending basic control to PHY.")
    lc.send(Message(module, interf.MODULE_PHY, internal))

if __name__ == '__main__':

    # Parse any input option.
    debug, single = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # decides if single or two host PCs.
    if(single == False):
        source_module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    else:
        source_module = interf.MODULE_DEBUG1

    print("Create CommManager object.")
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    mcs_idx = 0
    rx_gain = 20
    seq_number = 0
    channel = 0
    num_slots = 1

    # Send RX control information to PHY.
    sendRxCtrlToPhy(lc, channel, mcs_idx, rx_gain, num_slots, seq_number, source_module, debug)

    sleep(2)
