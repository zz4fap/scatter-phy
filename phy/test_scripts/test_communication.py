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
    print("Usage: pyhton3 test_communication.py [-c radio_id] [-i] [-d] [-r nof_prb]")

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
    timeout_counter = 0
    rx_pkt_cnt = 0;
    while(getExitFlag() == False):
        if(lc.get_low_queue().empty() == False):
            try:
                #Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                time_start = time.time()
                wait_for_timeout = 1
                if(internal.receiver.result > 0 and internal.receiver.result == PHY_SUCCESS and internal.receiver.data[0] == internal.transaction_index): # Avoid adding to the QUEUE internal objects that have status 0.
                    if(debug == True): print("result:",internal.receiver.result," - internal.receiver.data[0]:",internal.receiver.data[0], " - internal.transaction_index:",internal.transaction_index)
                    rx_stat_queue.put(internal)
                    set_is_stat_received(True)
                    timeout_counter = 0
                    rx_pkt_cnt = rx_pkt_cnt + 1
                    if(debug == True): print("Received data for this node.")
                elif(rx_pkt_cnt > 0 and internal.receiver.result > 0 and internal.receiver.result == PHY_TIMEOUT):
                    timeout_counter = timeout_counter + 1
                    if(timeout_counter > 10000):
                        timeout_counter = 0
                        rx_stat_queue.put(internal)
                        set_is_stat_received(True)
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

def generateRandomData(num_slots, bw_idx, mcs):
    random_data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            random_data = random_data + bytes([random.randint(0, 255)])

    return random_data

def generateData(num_slots, seq_number, bw_idx, mcs):
    if(seq_number==0):
        seq_number = seq_number + 1
    data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            data = data + bytes([seq_number])
    return data

def generateData2(num_slots, bw_idx, mcs):
    data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            data = data + bytes([i%256])
    return data

def sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, gain_value, slots, sequence_number, data, module, debug):

    # Basic Control
    trx_flag    = PHY_TX_ST         # TRX Mode. 1 TX - 0 RX; 2
    seq_number  = sequence_number   # Sequence number.
    bw_idx      = bandwidth         # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    frame       = 0                 # Frame number (not used now, for future use)
    slot        = 0                 # Slot number (not used now, for future use)
    gain        = gain_value        # TX gain.
    num_slots   = slots             # Number of slots. How many data slots we expect to receive from peer.

    # Create an Internal message for TX procedure.
    internal = interf.Internal()
    # Add sequence number to internal message.
    internal.transaction_index = seq_number
    # Set basic control with proper values.
    internal.send.basic_ctrl.trx_flag     = trx_flag
    internal.send.basic_ctrl.bw_index     = bw_idx
    internal.send.basic_ctrl.ch           = channel
    internal.send.basic_ctrl.frame        = frame
    internal.send.basic_ctrl.slot         = slot
    internal.send.basic_ctrl.mcs          = mcs
    internal.send.basic_ctrl.gain         = gain
    internal.send.basic_ctrl.length       = num_slots*getTransportBlockSize(bw_idx, mcs)
    internal.send.app_data.data           = data

    # Check size of transmited data.
    if(len(internal.send.app_data.data) != internal.send.basic_ctrl.length):
        print("Length of data is diffrent of field length.")
        sys.exit(-1)

    # Print the basic control structure sent to PHY.
    if(debug == True): printBasicControl(internal.send.basic_ctrl, internal.transaction_index, internal.send.app_data.data)

    # Send basic control to PHY.
    if(debug == True): print("Sending basic control to PHY.")
    lc.send(Message(module, interf.MODULE_PHY, internal))

MAX_TX_GAIN = 50 # Given in dB
MAX_RX_GAIN = 70 # Given in dB
INITIAL_RX_GAIN = 30 # Given in dB
CQI_THRESHOLD = 13 # CQI Threshold used to say that we have a very good link.
GAIN_STEP = 1
RX_GAIN_STEP = 3
NUM_RX_SLOTS = 100
NUM_TX_SLOTS = 100

# ********************************* Main code *********************************
if __name__ == '__main__':

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

    # Configure common values for messages.
    bandwidth               = getBWIndex(25)
    chan                    = 0
    num_data_slots          = 1
    initiator_seq_number    = 100
    relay_seq_number        = 200
    byte_tx                 = 0
    mcs_idx                 = 0
    rx_cqi                  = 100
    rx_gain                 = 30
    tx_gain                 = 30

    tx_slots_cnt = 0
    rx_slots_cnt = 0

    # Decide whcih sequence number should be expected by this node.
    if(initiator == True):
        seq_number = relay_seq_number
        # Generate data to be sent to the other node.
        data = generateData(num_data_slots, initiator_seq_number, bandwidth, mcs_idx)
    else:
        seq_number = initiator_seq_number
        # Generate data to be sent to the other node.
        data = generateData(num_data_slots, relay_seq_number, bandwidth, mcs_idx)

    # Fisrt send RX message to configure reception in IDLE mode.
    sendRxCtrlToPhy(lc, PHY_RX_ST, bandwidth, chan, num_data_slots, seq_number, mcs_idx, rx_gain, source_module, debug)

    # Wait fro the receiver to stabilize.
    sleep(1)

    # If this radio is the initiator it should send and wait for response from other node.
    if(initiator == True):
        print("Radio Initiator.")
        while(getExitFlag() == False):

            # Send message to other node.
            sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, tx_gain, num_data_slots, initiator_seq_number, data, source_module, debug)

            # Count the number of transmitted slots.
            tx_slots_cnt = tx_slots_cnt + 1

            # Wait until RX statistics is received from PHY with status equal to PHY_SUCCESS and data macthes expected sequence number.
            while(get_is_stat_received() == False and getExitFlag() == False):
                pass

            # Received response from other node.
            # Get PHY RX Statistics from PHY.
            internal = rx_stat_queue.get()

            if(internal.receiver.result == PHY_SUCCESS):
                # Count the number of received slots.
                rx_slots_cnt = rx_slots_cnt + 1
                print("[INITIATOR]: Data packet received from relay. BW:",bandwidth," - TB size:",len(internal.receiver.data))
            elif(internal.receiver.result == PHY_TIMEOUT):
                print("[INITIATOR]: TIMED-OUT, send data packet again.")

            # Set flag back to false.
            set_is_stat_received(False)

            # Leave loop after transmitting NUM_RX_SLOTS slots.
            if(tx_slots_cnt == NUM_TX_SLOTS):
                break;

    else: # This is not the initiator radio, so it should listen for incoming transmissions.
        print("Radio Relay.")

        # Wait until RX statistics is received from PHY with status equal to PHY_SUCCESS and data macthes expected sequence number.
        while(get_is_stat_received() == False and getExitFlag() == False):
            pass

        # Received response from other node.
        # Get PHY RX Statistics from PHY.
        # It is done to empty the QUEUE.
        internal = rx_stat_queue.get()

        # Count the number of received slots.
        rx_slots_cnt = rx_slots_cnt + 1

        # Set flag back to false.
        set_is_stat_received(False)

        print("[RELAY]: First data packet received from initiator. BW:",bandwidth," - TB size:",len(internal.receiver.data))

        # At this point we are sure we have received a successfully decoded message from the other node.
        while(getExitFlag() == False):

            # Send message to other node.
            sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, tx_gain, num_data_slots, relay_seq_number, data, source_module, debug)

            # Count the number of transmitted slots.
            tx_slots_cnt = tx_slots_cnt + 1

            # Wait until RX statistics is received from PHY.
            while(get_is_stat_received() == False and getExitFlag() == False):
                pass

            # Received response from other node.
            # Get PHY RX Statistics from PHY.
            # It is done to empty the QUEUE.
            internal = rx_stat_queue.get()

            if(internal.receiver.result == PHY_SUCCESS):
                # Count the number of received slots.
                rx_slots_cnt = rx_slots_cnt + 1
                print("[RELAY]: Data packet received from initiator. BW:",bandwidth," - TB size:",len(internal.receiver.data))
            elif(internal.receiver.result == PHY_TIMEOUT):
                print("[RELAY]: TIMED-OUT, send data packet again.")

            # Set flag back to false.
            set_is_stat_received(False)

            # Leave loop after receiving NUM_RX_SLOTS slots.
            if(rx_slots_cnt == NUM_RX_SLOTS):
                break;

    print("# TX slots:",tx_slots_cnt,"# RX slots:",rx_slots_cnt)
    print("Terminating the communication test application. Bye...")
