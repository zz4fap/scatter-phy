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
INITIAL_RX_GAIN = 30 # Given in dB
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

    # Configure common values for messages.
    send_to          = send_to_id
    bandwidth        = getBWIndex(nof_prb)
    chan             = 0
    num_data_slots   = 1
    seq_number       = 200
    byte_tx          = 0
    mcs_idx          = 0
    rx_cqi           = 100
    rx_gain          = INITIAL_RX_GAIN
    tx_gain          = 0

    # Set some different gains for the relay radio
    if(initiator == False):
        rx_gain = 50
        tx_gain = 70

    # Fisrt send RX message to configure reception in IDLE mode.
    sendRxCtrlToPhy(lc, PHY_RX_ST, bandwidth, chan, num_data_slots, seq_number, mcs_idx, rx_gain, source_module, debug)

    # Wait fro the receiver to stabilize.
    sleep(1)

    # If this radio is the initiator it should send and wait for response from other node.
    if(initiator == True):
        print("Radio Initiator.")
        while(True):

            # Send message to other node.
            sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, tx_gain, num_data_slots, byte_tx, rx_cqi, rx_gain, tx_gain, send_to, source_module, debug)

            # Wait until RX statistics is received from PHY.
            while(get_is_stat_received() == False and getExitFlag() == False):
                pass

            # Either timedout or received response from other node.
            # Get PHY RX Statistics from PHY.
            internal = rx_stat_queue.get()
            status = internal.receiver.result

            # Algorithm for throughtput optimization.
            if(status == PHY_TIMEOUT): # If Reception timedout we need to increase power.
                if(tx_gain >= MAX_TX_GAIN): # We increased power beyond the MAXIMUM value.
                    if(rx_gain <= MAX_RX_GAIN): # Try again with new RX gain value.
                        rx_gain = rx_gain + RX_GAIN_STEP
                        tx_gain = 0
                        mcs_idx = 0
                        # Set new values for RX Gain and MCS.
                        sendRxCtrlToPhy(lc, PHY_RX_ST, bandwidth, chan, num_data_slots, seq_number, mcs_idx, rx_gain, source_module, debug)
                        print("[PHY_TIMEOUT]: Try again with new RX Gain:",rx_gain)
                    else:
                        print("[PHY_TIMEOUT]: It was NOT possible to establish a link.")
                        break
                else:
                    tx_gain = tx_gain + GAIN_STEP
                    print("[PHY_TIMEOUT]: Increasing TX gain to:",tx_gain)
            elif(status == PHY_SUCCESS):  # Data packet was received back.
                # Get received data bytes.
                rxd_bytes = internal.receiver.data
                seq_num = rxd_bytes[0]-1

                if(seq_num == byte_tx): # Link was established!
                    data_not_equal_cnt = 0 # reset data not equal the counter.
                    linkEstablished = LinkEstablished(mcs_idx,rx_gain,tx_gain)
                    rx_cqi = rxd_bytes[2] # Get Received CQI
                    if(rx_cqi < CQI_THRESHOLD):
                        if(tx_gain < MAX_TX_GAIN):
                            tx_gain = tx_gain + GAIN_STEP # Increase gain to see if CQI improves.
                            print("[PHY_LINK_ESTABLISHED]: Try to improve current CQI: ", rx_cqi, "by increasing TX gain to:",tx_gain, " - sequence number is:",seq_num)
                        else:
                            print("[PHY_LINK_ESTABLISHED]: Maximum TX Gain reached (", tx_gain, ") without any improvement on CQI (",rx_cqi, ") - RX gain is:", rx_gain," - sequence number is:",seq_num)
                            break
                    else:
                        # we keep the power but try to increase mcs.
                        mcs_idx = (mcs_idx + 1)%29
                        # We increment the data to be transmitted only when the MCS is updated.
                        byte_tx = (byte_tx + 1)%256
                        print("[PHY_LINK_ESTABLISHED]: CQI is very good:",rx_cqi, " - trying to increase MCS to: ",mcs_idx, " - next sequence number is: ",byte_tx)
                else: # Link was NOT established as data does not match.
                    data_not_equal_cnt = data_not_equal_cnt + 1
                    if(data_not_equal_cnt < 10):
                        if(mcs_idx == 0):
                            tx_gain = tx_gain + GAIN_STEP
                            print("[PHY_LINK_NOT_ESTABLISHED]: Increasing TX gain to:",tx_gain)
                        else:
                            mcs_idx = mcs_idx - 1
                            print("[PHY_LINK_NOT_ESTABLISHED]: Decreasing MCS to:",mcs_idx)
                    else:
                        print("[PHY_LINK_NOT_ESTABLISHED]: RX bytes didn't match the sent byte for ten times... exit...")
                        break
            else:
                print("Invalid status:",status)

            # Set flag back to false.
            set_is_stat_received(False)

            # Check exit flag.
            if(getExitFlag() == True):
                break

    else: # This is not the initiator radio, so it should listen for incoming transmissions.
        print("Radio Relay.")

        # Wait until RX statistics is received from PHY.
        while(get_rx_status() != PHY_SUCCESS and getExitFlag() == False):
            pass

        # Set an UNKNOWN status.
        set_rx_status(PHY_UNKNOWN)

        # At this point we are sure we have received a successfully decoded message from the other node.
        while(True):

            # While the internal object in the QUEUE does not have status equal to PHY_SUCCESS keep reading.
            while(True):
                # Get PHY RX Statistics from PHY.
                internal = rx_stat_queue.get()
                if(debug == True): print("Received status:", internal.receiver.result);
                if(internal.receiver.result == PHY_SUCCESS):
                    break

            # get received data bytes.
            rxd_bytes = internal.receiver.data

            # Get fisrt received byte minus 1.
            if(len(rxd_bytes) > 0):
                rx_byte = rxd_bytes[0] - 1;
                mcs     = rxd_bytes[1]
                rx_cqi  = rxd_bytes[2]
                rx_gain = rxd_bytes[3]
                tx_gain = rxd_bytes[4]
                print("[RADIO_RELAY]: Received message with: sequence number:", rx_byte, " - MCS:", mcs, " - CQI:", rx_cqi, " - RX Gain:", rx_gain, " - TX gain:", tx_gain)
            else:
                print("[RADIO_RELAY]: Number of received bytes is not greater than 0.");
                sys.exit(-1)

            # Set CQI with the received one.
            rx_cqi = internal.receiver.stat.rx_stat.gain

            # Send message to other node.
            sendTxCtrlToPhy(lc, bandwidth, chan, mcs_idx, tx_gain, num_data_slots, rx_byte, rx_cqi, rx_gain, tx_gain, send_to, source_module, debug)

            # Wait until RX statistics is received from PHY.
            while(get_is_stat_received() == False and getExitFlag() == False):
                pass

            # Set flag back to false.
            set_is_stat_received(False)

            # Check exit flag.
            if(getExitFlag() == True):
                break

    print("Terminating the application.")
