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
sys.path.append('../../')
sys.path.append('../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

# ********************************* Definitions *********************************
# Number of bits (TB) that can be transmitted in one suframe, i.e., 1 ms. These number are for a bandwidth of 5 MHz, i.e., 25 RBs.
NUM_BYTES_PER_SUBFRAME_VS_MCS_1MHz4 = [19,26,32,41,51,63,75,89,101,117,117,129,149,169,193,217,225,225,241,269,293,325,349,373,405,437,453,469,549]
NUM_BYTES_PER_SUBFRAME_VS_MCS_3MHz = [49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383]
NUM_BYTES_PER_SUBFRAME_VS_MCS_5MHz = [85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292]
NUM_BYTES_PER_SUBFRAME_VS_MCS_10MHz = [173,225,277,357,453,549,645,775,871,999,999,1095,1239,1431,1620,1764,1908,1908,2052,2292,2481,2673,2865,3182,3422,3542,3822,3963,4587]
NUM_BYTES_PER_SUBFRAME_VS_MCS_15MHz = [261,341,421,549,669,839,967,1143,1335,1479,1479,1620,1908,2124,2385,2673,2865,2865,3062,3422,3662,4107,4395,4736,5072,5477,5669,5861,6882]
NUM_BYTES_PER_SUBFRAME_VS_MCS_20MHz = [349,453,573,717,903,1095,1287,1527,1764,1980,1980,2196,2481,2865,3182,3542,3822,3822,4107,4587,4904,5477,5861,6378,6882,7167,7708,7972,9422]

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
BW_IDX_Fifteen          = 5 # 15 MHz
BW_IDX_Twenty           = 6 # 20 MHz

# PHY Results.
PHY_UNKNOWN             = 0
PHY_SUCCESS             = 100
PHY_ERROR               = 101
PHY_TIMEOUT             = 102

# ********************************* Functions *********************************
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

rxStatusThreadLock = threading.Lock()
rx_status = PHY_UNKNOWN
def get_rx_status():
    # Get lock to synchronize threads
    rxStatusThreadLock.acquire()
    global rx_status
    status = rx_status
    # Free lock to release next thread
    rxStatusThreadLock.release()
    return status

def set_rx_status(status):
    # Get lock to synchronize threads
    rxStatusThreadLock.acquire()
    global rx_status
    rx_status = status
    # Free lock to release next thread
    rxStatusThreadLock.release()

def help():
    print("Usage: pyhton3 throughput_optimization.py [-c radio_id] [-i] [-d] [-r nof_prb]")

def inputOptions(argv):
    debug = False # by default debug is enabled.
    profiling = False # by default profiling is disabled.
    single = False # By default we use two computers to run the tests.
    initiator = False # By default this is not the initiator node.
    send_to_id = 0 # By defaylt we set the ID of the radio where to send a packet to to 0.
    nof_prb = 25 # By dafult we set the number of resource blocks to 25, i.e., 5 MHz bandwidth.

    try:
        opts, args = getopt.getopt(argv,"hdpsic:r:",["help","debug","profile","single","initiator","cid","rb"])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            help()
            sys.exit()
        elif opt in ("-d", "--debug"):
            debug = True
        elif opt in ("-p", "--profile"):
            profiling = True
        elif opt in ("-s", "--single"):
            single = True
        elif opt in ("-i", "--initiator"):
            initiator = True
        elif opt in ("-c", "--cid"):
            send_to_id = int(arg)
        elif opt in ("-r", "--rb"):
            nof_prb = int(arg)
        else:
            help()
            sys.exit()

    return debug, profiling, single, initiator, send_to_id, nof_prb

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

def printPhyRxStat(internal, seq_number, status):
    rx_stats = internal.receiver.stat
    print("***************** RX PHY Stats Packet ****************")
    print("seq_number: ",seq_number)
    print("status: ",status)
    print("host_timestamp: ",rx_stats.host_timestamp)
    print("fpga_timestamp: ",rx_stats.fpga_timestamp)
    print("frame: ",rx_stats.frame)
    print("slot: ",rx_stats.slot)
    print("ch: ",rx_stats.ch)
    print("mcs: ",rx_stats.mcs)
    print("num_cb_total: ",rx_stats.num_cb_total)
    print("num_cb_err: ",rx_stats.num_cb_err)
    print("gain: ",rx_stats.rx_stat.gain)
    print("cqi: ",rx_stats.rx_stat.cqi)
    print("rssi: ",rx_stats.rx_stat.rssi)
    print("rsrp: ",rx_stats.rx_stat.rsrp)
    print("rsrq: ",rx_stats.rx_stat.rsrq)
    print("sinr: ",rx_stats.rx_stat.sinr)
    if(rx_stats.rx_stat.length > 0):
        if(len(internal.receiver.data) > 0):
            print("length: ",rx_stats.rx_stat.length)
            data = []
            for d in internal.receiver.data:
                data.append(int(d))
            print("data: ",data)
    print("*********************************************************************\n")

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

def getBWIndex(nof_prb):
    if nof_prb == 6:
        bw_idx = BW_IDX_OneDotFour
    elif nof_prb == 15:
        bw_idx = BW_IDX_Three
    elif nof_prb == 25:
        bw_idx = BW_IDX_Five
    elif nof_prb == 50:
        bw_idx = BW_IDX_Ten
    elif nof_prb == 75:
        bw_idx = BW_IDX_Fifteen
    elif nof_prb == 100:
        bw_idx = BW_IDX_Twenty
    else:
        print("Invalid number of resource blocks.")
        sys.exit(-1)

    return bw_idx

def sendRxCtrlToPhy(lc, trx_mode, bandwidth, chan, num_data_slots, seq_number, mcs_idx, rx_gain, module, debug):

    # Basic Control
    trx_flag    = trx_mode          # TRX Mode. 1 TX - 0 RX;
    send_to     = 1000              # send_to field does not have any meaning for the receivig basic control, then set it to invalid value.
    seq_number  = seq_number        # Sequence number.
    bw_idx      = bandwidth         # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    gain        = rx_gain           # RX gain.
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
    internal.receive.basic_ctrl.slot         = slot
    internal.receive.basic_ctrl.mcs          = mcs
    internal.receive.basic_ctrl.gain         = gain
    internal.receive.basic_ctrl.length       = num_slots

    # Print the basic control structure sent to PHY.
    if(debug == True): printBasicControl(internal.receive.basic_ctrl, internal.transaction_index)

    # Send basic control to PHY.
    if(debug == True): print("Sending basic control to PHY.")
    lc.send(Message(module, interf.MODULE_PHY, internal))

def receiveRxStatisticsFromPhyThread(lc, rx_stat_queue, debug):

    time_start = time.time()
    wait_for_timeout = 0;
    time_out_second = 1 # Wait 1 second.
    host_timestamp = 0
    while(getExitFlag() == False):
        if(lc.get_low_queue().empty() == False):
            try:
                #Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                time_start = time.time()
                wait_for_timeout = 1
                if(internal.receiver.result > 0): # Avoid adding to the QUEUE internal objects that have status 0.
                    rx_stat_queue.put(internal)
                    set_is_stat_received(True)
                    set_rx_status(internal.receiver.result)
            except queue.Empty:
                if(wait_for_timeout == 1):
                    if(time.time()-time_start > time_out_second):
                        time_end = time.time() - time_out_second
                        print("Timed out...")
                        break

        if(getExitFlag() == True):
            print("Exit receiving function.")
            break

def createPacketToBeSent(num_of_bytes,mcs,sequence_number,cqi,rx_gain,tx_gain):

    # Create packet to be sent.
    data = bytes()
    # Populate with necessary information.
    data = data + bytes([sequence_number + 1])
    data = data + bytes([mcs])
    data = data + bytes([cqi])
    data = data + bytes([rx_gain])
    data = data + bytes([tx_gain])
    # Fill the rest of the array with sequence number.
    for j in range(5,num_of_bytes):
        data = data + bytes([sequence_number + 1])

    return data

def sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, gain_value, slots, sequence_number, cqi, rx_gain, tx_gain, send_to_id, module, debug):

    # Basic Control
    trx_flag    = PHY_TX_ST         # TRX Mode. 1 TX - 0 RX; 2
    send_to     = send_to_id        # ID of the radio where to send a packet to.
    seq_number  = sequence_number   # Sequence number.
    bw_idx      = bandwidth         # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    gain        = gain_value        # TX gain.
    num_slots   = slots             # Number of slots. How many data slots we expect to receive from peer.

    # Calculate number of bytes.
    tb_size = getTransportBlockSize(bw_idx, mcs)
    num_of_bytes = num_slots*tb_size

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
    internal.send.basic_ctrl.mcs          = mcs
    internal.send.basic_ctrl.gain         = gain
    internal.send.basic_ctrl.length       = num_of_bytes
    internal.send.app_data.data           = createPacketToBeSent(num_of_bytes,mcs,sequence_number,cqi,rx_gain,tx_gain)

    # Check size of transmited data.
    if(len(internal.send.app_data.data) != internal.send.basic_ctrl.length):
        print("Length of data is diffrent of field length.")
        sys.exit(-1)

    # Print the basic control structure sent to PHY.
    if(debug == True): printBasicControl(internal.send.basic_ctrl, internal.transaction_index, internal.send.app_data.data)

    # Send basic control to PHY.
    if(debug == True): print("Sending basic control to PHY.")
    lc.send(Message(module, interf.MODULE_PHY, internal))

class LinkEstablished:
    'Class for Link Established context'
    mcs = 0
    rx_gain = 0
    tx_gain = 0

    def __init__(self, mcs = 0, rx_gain = 0, tx_gain = 0):
        self.mcs = mcs
        self.rx_gain = rx_gain
        self.tx_gain = tx_gain

    def getMcs():
        return self.mcs

    def getRxGain():
        return self.rx_gain

    def getTxGain():
        return self.tx_gain

MAX_TX_GAIN = 50 # Given in dB
MAX_RX_GAIN = 70 # Given in dB
INITIAL_RX_GAIN = 70 # Given in dB
CQI_THRESHOLD = 13 # CQI Threshold used to say that we have a very good link.
GAIN_STEP = 1
RX_GAIN_STEP = 3

# ********************************* Main code *********************************
if __name__ == '__main__':

    # Create object to store link established information.
    linkEstablished = LinkEstablished()

    # Parse any input option.
    debug, profiling, single, initiator, send_to_id, nof_prb = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # Decides if single or two host PCs.
    if(single == False):
        source_module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    else:
        source_module = interf.MODULE_DEBUG1

    print("Create CommManager object.")
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    # Create a QUEUE to store data.
    # Create a single input and a single output queue for all threads.
    rx_stat_queue = queue.Queue()

    # Start RX statistics thread.
    # Wait for PHY RX Statistics from PHY.
    try:
        thread_id = _thread.start_new_thread( receiveRxStatisticsFromPhyThread, (lc, rx_stat_queue, debug, ) )
    except:
        print ("Error: unable to start thread")
        sys.exit(-1)

    # This counter counter the number of times we received packets that does not match the sent byte.
    data_not_equal_cnt = 0

    numner_of_trials = 0

    # Configure common values for messages.
    send_to          = send_to_id
    bandwidth        = getBWIndex(nof_prb)
    chan             = 0
    num_data_slots   = 1
    seq_number       = 200
    byte_tx          = 0
    mcs_idx          = 0
    rx_cqi           = 100
    rx_gain          = 30#INITIAL_RX_GAIN
    tx_gain          = 0

    while(numner_of_trials != 1):
        # Fisrt send RX message to configure reception in IDLE mode.
        sendRxCtrlToPhy(lc, PHY_SENSING_ST, bandwidth, chan, num_data_slots, seq_number, mcs_idx, rx_gain, source_module, debug)

        # Wait until RX statistics is received from PHY.
        while(get_is_stat_received() == False and getExitFlag() == False):
            pass

        # Either timedout or received response from other node.
        # Get PHY RX Statistics from PHY.
        internal = rx_stat_queue.get()
        status = internal.receiver.result
        seq_number = internal.transaction_index

        if(status == PHY_SUCCESS):
            printPhyRxStat(internal, seq_number, status)

        numner_of_trials = numner_of_trials + 1
