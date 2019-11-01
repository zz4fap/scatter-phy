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
NUM_BYTES_PER_SUBFRAME_VS_MCS_3MHz  = [49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383]
NUM_BYTES_PER_SUBFRAME_VS_MCS_5MHz  = [85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292]
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

# ************ Functions ************
tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    print("Exit flag:",tx_exit_flag)
    tx_exit_flag = True

def getExitFlag():
    global tx_exit_flag
    return tx_exit_flag

tx_stat_flag_lock = threading.Lock()
is_tx_stat_received = False
def get_is_stat_received():
    tx_stat_flag_lock.acquire()
    global is_tx_stat_received
    flag = is_tx_stat_received
    tx_stat_flag_lock.release()
    return flag

def set_is_stat_received(flag):
    tx_stat_flag_lock.acquire()
    global is_tx_stat_received
    is_tx_stat_received = flag
    tx_stat_flag_lock.release()

txStatthreadLock = threading.Lock()
tx_sequence_number = 0
def set_seq_number(seq_num):
    # Get lock to synchronize threads
    txStatthreadLock.acquire()
    global tx_sequence_number
    tx_sequence_number = seq_num
    # Free lock to release next thread
    txStatthreadLock.release()

def get_seq_number():
    # Get lock to synchronize threads
    txStatthreadLock.acquire()
    global tx_sequence_number
    seq_num = tx_sequence_number
    # Free lock to release next thread
    txStatthreadLock.release()
    return seq_num

def help():
    print("Usage: pyhton3 continuous_transmissionv2.py")

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
    print("send_to: ",basic_control.send_to)
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
    debug = False # by default debug is enabled.
    profiling = False # by default profiling is disabled.
    single = False # By default we use two computers to run the tests.

    try:
        opts, args = getopt.getopt(argv,"hdps",["help","debug","profile","single"])
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

    return debug, profiling, single

def sendTxCtrlToPhy(lc, chan, bw_idx, mcs_idx, gain_value, slots, sequence_number, module, random_data, data, debug):

    # Basic Control
    trx_flag    = PHY_TX_ST         # TRX Mode. 1 TX - 0 RX; 2
    seq_number  = sequence_number   # Sequence number.
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
    internal.send.basic_ctrl.frame        = frame
    internal.send.basic_ctrl.slot         = slot
    internal.send.basic_ctrl.mcs          = mcs
    internal.send.basic_ctrl.gain         = gain
    internal.send.basic_ctrl.length       = num_slots*getTransportBlockSize(bw_idx, mcs)
    internal.send.app_data.data           = data

    # Send basic control to PHY.
    lc.send(Message(module, interf.MODULE_PHY, internal))

def generateRandomData(num_slots, bw_idx, mcs):
    random_data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            random_data = random_data + bytes([random.randint(0, 255)])

    return random_data

def generateData(num_slots, seq_number, bw_idx, mcs):
    data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            data = data + bytes([seq_number+1])
    return data

def generateData2(num_slots, bw_idx, mcs):
    data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            data = data + bytes([i%256])
    return data

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

def receiveTxStatisticsFromPhyThread(lc, tx_stat_queue, debug):
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
                seq_number = internal.transaction_index
                status = internal.sendr.result
                tx_stat_msg = internal.sendr.phy_stat
                time_start = time.time()
                wait_for_timeout = 1
                if(debug==True): print("Status:",internal.sendr.result)
                if(internal.sendr.result > 0): # Avoid adding to the QUEUE internal objects that have status 0.
                    tx_stat_queue.put(internal)
                    printPhyTxStat(tx_stat_msg, seq_number, status)
            except queue.Empty:
                if(wait_for_timeout == 1):
                    if(time.time()-time_start > time_out_second):
                        time_end = time.time() - time_out_second
                        print("Timed out...")
                        break
        if(getExitFlag() == True):
            print("Exit receiving function.")
            break

if __name__ == '__main__':

    # Parse any input option.
    debug, profiling, single = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # decides if single or two host PCs.
    if(single == False):
        source_module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    else:
        source_module = interf.MODULE_DEBUG2

    print("Create CommManager object.")
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    # Create a QUEUE to store data.
    # Create a single input and a single output queue for all threads.
    tx_stat_queue = queue.Queue()

    min_center_freq = 1500000000
    max_center_freq = 2000000000
    frequency_increment = 1000000

    center_freq = min_center_freq         # By default use center_freq: 2.4 GHz as center frequency. USRP x310 Tunable: from 1.2 GHz to 6GHz.
    channel = 0
    bw_idx = BW_IDX_Five
    mcs = 0
    gain = 30 # Good value for b200: 30 # for x310 use 200
    num_slots = 1
    seq_number = 0
    num_of_tx = -1
    # Generate random data.
    random_data = generateRandomData(num_slots, bw_idx, mcs)
    # Generate data based on sequence number.
    data = generateData(num_slots, seq_number, bw_idx, mcs)

    # Start thread used to wait for PHY TX Statistics sent by PHY.
    try:
        thread_id = _thread.start_new_thread( receiveTxStatisticsFromPhyThread, (lc, tx_stat_queue, debug, ) )
    except:
        print("Error: unable to start thread.")
        sys.exit(-1)

    # Wait some seconds so that you have time to swicth terminals or other other thing.
    time.sleep(4)

    while(tx_exit_flag == False):

        print("Sequence number:", seq_number, " - TX center frequency:",center_freq/1000000, "[MHz]")

        # Send TX control plus data to PHY.
        sendTxCtrlToPhy(lc, channel, bw_idx, mcs, gain, num_slots, seq_number, source_module, random_data, data, debug)

        # Increment sequence number.
        seq_number = (seq_number + 1)%256

        # Update TX center frequency.
        if(random.randint(0, 1) == 0): # Subtract from center frequency.
            center_freq = center_freq - random.randint(0, 1)*frequency_increment
            if(center_freq < min_center_freq):
                center_freq = max_center_freq
        else:  # add to center frequency
            center_freq = center_freq + random.randint(0, 1)*frequency_increment
            if(center_freq > max_center_freq):
                center_freq = min_center_freq

        # Wait until all TX stat is received.
        queue_size = 0
        while(getExitFlag() == False and queue_size == 0):
            queue_size = tx_stat_queue.qsize()

        # Remove TX stats from QUEUE so that it gets empty.
        internal = tx_stat_queue.get()
        if(debug == True): print("tx_stat_queue.qsize():",tx_stat_queue.qsize())

        # Wait sometime before transmitting again.
        #time.sleep(1)
