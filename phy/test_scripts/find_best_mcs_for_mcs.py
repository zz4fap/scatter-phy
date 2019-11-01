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

# PHY Results.
PHY_UNKNOWN             = 0
PHY_SUCCESS             = 100
PHY_ERROR               = 101
PHY_TIMEOUT             = 102
PHY_LBT_TIMEOUT         = 103

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
tx_exit_flag_lock = threading.Lock()
def handler(signum, frame):
    tx_exit_flag_lock.acquire()
    global tx_exit_flag
    tx_exit_flag = True
    tx_exit_flag_lock.release()

def getExitFlag():
    tx_exit_flag_lock.acquire()
    global tx_exit_flag
    flag = tx_exit_flag
    tx_exit_flag_lock.release()
    return flag

def setExitFlag(flag):
    tx_exit_flag_lock.acquire()
    global tx_exit_flag
    tx_exit_flag = flag
    tx_exit_flag_lock.release()

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

def printPhyTxStat(internal):
    print("************************ TX PHY Stats Packet ************************")
    print("seq_number: ",internal.transaction_index)
    print("status: ",internal.sendr.result)
    print("host_timestamp: ",internal.sendr.phy_stat.host_timestamp)
    print("frame: ",internal.sendr.phy_stat.frame)
    print("slot: ",internal.sendr.phy_stat.slot)
    print("ch: ",internal.sendr.phy_stat.ch)
    print("mcs: ",internal.sendr.phy_stat.mcs)
    print("Slots to be transmitted: ",internal.sendr.phy_stat.num_cb_total)
    print("Dropped slots: ",internal.sendr.phy_stat.num_cb_err)
    print("power: ",internal.sendr.phy_stat.tx_stat.power)
    print("channel_free_cnt: ",internal.sendr.phy_stat.tx_stat.channel_free_cnt)
    print("channel_busy_cnt: ",internal.sendr.phy_stat.tx_stat.channel_busy_cnt)
    print("free_energy: ",internal.sendr.phy_stat.tx_stat.free_energy)
    print("busy_energy: ",internal.sendr.phy_stat.tx_stat.busy_energy)
    print("Total number of dropped slots: ",internal.sendr.phy_stat.tx_stat.total_dropped_slots)
    print("Coding time: ",internal.sendr.phy_stat.tx_stat.coding_time)
    print("*********************************************************************\n")

def printPhyRxStat(internal):
    print("***************** RX PHY Stats Packet ****************")
    print("seq_number: ",internal.transaction_index)
    print("status: ",internal.receiver.result)
    print("host_timestamp: ",internal.receiver.stat.host_timestamp)
    print("frame: ",internal.receiver.stat.frame)
    print("slot: ",internal.receiver.stat.slot)
    print("ch: ",internal.receiver.stat.ch)
    print("mcs: ",internal.receiver.stat.mcs)
    print("num_cb_total: ",internal.receiver.stat.num_cb_total)
    print("num_cb_err: ",internal.receiver.stat.num_cb_err)
    print("gain: ",internal.receiver.stat.rx_stat.gain)
    print("cqi: ",internal.receiver.stat.rx_stat.cqi)
    print("rssi: ",internal.receiver.stat.rx_stat.rssi)
    print("rsrp: ",internal.receiver.stat.rx_stat.rsrp)
    print("rsrq: ",internal.receiver.stat.rx_stat.rsrq)
    print("sinr: ",internal.receiver.stat.rx_stat.sinr)
    print("detection_errors: ",internal.receiver.stat.rx_stat.detection_errors)
    print("decoding_errors: ",internal.receiver.stat.rx_stat.decoding_errors)
    print("peak_value: ",internal.receiver.stat.rx_stat.peak_value)
    print("noise: ",internal.receiver.stat.rx_stat.noise)
    print("decoded_cfi: ",internal.receiver.stat.rx_stat.decoded_cfi)
    print("found_dci: ",internal.receiver.stat.rx_stat.found_dci)
    print("last_noi: ",internal.receiver.stat.rx_stat.last_noi)
    print("total_packets_synchronized: ",internal.receiver.stat.rx_stat.total_packets_synchronized)
    if(len(internal.receiver.data) > 0):
        print("length: ",internal.sendr.phy_stat.rx_stat.length)
        data = []
        for d in internal.receiver.data:
            data.append(int(d))
        print("data: ",data)
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
    debug = False # by default debug is enabled.
    profiling = False # by default profiling is disabled.
    single = False # By default we use two computers to run the tests.
    wait_for_tx_stats = False
    send_to_id = 0 # By defaylt we set the ID of the radio where to send a packet to to 0.
    nof_prb = 25 # By dafult we set the number of resource blocks to 25, i.e., 5 MHz bandwidth.
    initial_mcs = 0 # By default MCS is set to 0, the most robust MCS.
    txgain = 0 # By default TX gain is set to 0.
    rxgain = 10 # By default RX gain is set to 10.
    tx_slots = 1
    rx_slots = 1
    tx_channel = 0
    rx_channel = 0
    num_of_packets_to_tx = 1000

    try:
        opts, args = getopt.getopt(argv,"hdpswi:b:m:t:r:",["help","debug","profile","single","waittxstats","sendtoid=","bw=","initialmcs=","txgain=","rxgain=","txslots=","rxslots=","txchannel=","rxchannel=","numpktstotx="])
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
        elif opt in ("-w", "--waittxstats"):
            wait_for_tx_stats = True
        elif opt in ("-i", "--sendtoid"):
            send_to_id = int(arg)
        elif opt in ("-b", "--bw"):
            nof_prb = int(arg)
        elif opt in ("-m", "--initialmcs"):
            initial_mcs = int(arg)
        elif opt in ("-t", "--txgain"):
            txgain = int(arg)
        elif opt in ("-r", "--rxgain"):
            rxgain = int(arg)
        elif opt in ("--txslots"):
            tx_slots = int(arg)
        elif opt in ("--rxslots"):
            rx_slots = int(arg)
        elif opt in ("--txchannel"):
            tx_channel = int(arg)
        elif opt in ("--rxchannel"):
            rx_channel = int(arg)
        elif opt in ("--numpktstotx"):
            num_of_packets_to_tx = int(arg)

    return debug, profiling, single, wait_for_tx_stats, send_to_id, nof_prb, initial_mcs, txgain, rxgain, tx_slots, rx_slots, tx_channel, rx_channel, num_of_packets_to_tx

def sendRxCtrlToPhy(lc, bandwidth, chan, mcs_idx, rx_gain, num_data_slots, seq_number, send_to_id, debug):

    # Basic Control
    trx_flag    = PHY_RX_ST         # TRX Mode. 1 TX - 0 RX;
    send_to     = send_to_id        # Radio ID to send slot to.
    seq_number  = seq_number        # Sequence number.
    bw_idx      = bandwidth         # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    frame       = 0
    gain        = rx_gain           # RX gain. We use -1 for AGC mode.
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
    lc.send(Message(lc.getModule(), interf.MODULE_PHY, internal))

def sendTxCtrlToPhy(lc, chan, bandwidth, mcs_idx, gain_value, slots, sequence_number, send_to_id, data, debug):

    # Basic Control
    trx_flag    = PHY_TX_ST         # TRX Mode. 1 TX - 0 RX; 2
    send_to     = send_to_id        # ID of the radio where to send a packet to.
    seq_number  = sequence_number   # Sequence number.
    bw_idx      = bandwidth         # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = mcs_idx           # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    frame       = 0                 # Frame number.
    gain        = gain_value        # RX gain. We use -1 for AGC mode.
    num_slots   = slots             # Number of slots. How many data slots we expect to receive from peer.

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
    internal.send.basic_ctrl.timestamp    = 0
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
    lc.send(Message(lc.getModule(), interf.MODULE_PHY, internal))

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
            data = data + bytes([seq_number])
    return data

def generateData2(num_slots, seq_number, bw_idx, mcs):
    data = bytes()
    for j in range(num_slots):
        for i in range(getTransportBlockSize(bw_idx, mcs)):
            data = data + bytes([seq_number+j])
    return data

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

def receiveStatisticsFromPhyThread(lc, stats_queue, rx_stats_queue, num_of_tx_slots, wait_for_tx_stats, debug):
    expected_rxd_byte = -1 # last received byte
    last_rxd_byte = -1
    out_of_seq_cnt = 0 # out of sequence error counter.
    in_seq_cnt = 0 # In sequence counter.
    total_pkt_number = 0 # Total number of received packets.
    correct_tx_slot_counter = 0
    same_packet_cnt = 0
    out_of_sync_percentage = 0
    rxStats = RxStats()

    while(getExitFlag() == False):
        # Check if QUEUE is empty.
        if(getExitFlag() == False and lc.get_low_queue().empty() == False):
            try:
                #Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                if(wait_for_tx_stats == True and internal.sendr.result > 0 and internal.sendr.result == PHY_SUCCESS):
                    if(internal.transaction_index >= 100 and internal.transaction_index < 200 and get_seq_number() == internal.transaction_index):
                        stats_queue.put(internal)
                        set_is_stat_received(True)
                    else:
                        print("Sequence number out of sequence, expecting:", get_seq_number(), "and received: ", internal.transaction_index, "exiting.")
                        os._exit(-1)
                elif(internal.receiver.result > 0):
                    if(internal.transaction_index == 66):
                       # Get PHY RX Statistics from PHY.
                       total_rxd_bytes = internal.receiver.data
                       host_timestamp = internal.receiver.stat.host_timestamp
                       # Increment the number of received packets with or without error.
                       total_pkt_number = total_pkt_number + 1
                       # Deal with SUCCESSFULLY decoded packets.
                       if(internal.receiver.result == PHY_SUCCESS):
                           if(len(total_rxd_bytes) > 0):
                               if(expected_rxd_byte == -1):
                                   expected_rxd_byte = total_rxd_bytes[10]

                               if(total_rxd_bytes[10] == expected_rxd_byte):
                                   if(debug == True): print("Received:",total_rxd_bytes[10]," - Expected:",expected_rxd_byte)
                                   in_seq_cnt = in_seq_cnt + 1
                               else:
                                   out_of_seq_cnt = out_of_seq_cnt + abs(total_rxd_bytes[10] - expected_rxd_byte)
                                   out_of_sync_percentage = ((out_of_seq_cnt*100)/(in_seq_cnt + out_of_seq_cnt))
                                   if(debug == True): print("--------> Out of Seq:", out_of_seq_cnt,"- In Seq:", in_seq_cnt, "- Total # pkts:", total_pkt_number, "- % lost packets:", out_of_sync_percentage,  " - Received:", total_rxd_bytes[10], " - Expected:", expected_rxd_byte, "- Host timestamp:",host_timestamp)

                               if(last_rxd_byte != total_rxd_bytes[10]):
                                   if(total_rxd_bytes[10] == 100):
                                       expected_rxd_byte = 0
                                   else:
                                       expected_rxd_byte = total_rxd_bytes[10]
                                   expected_rxd_byte = (expected_rxd_byte + 1)

                               # Store the last received byte.
                               last_rxd_byte = total_rxd_bytes[10]

                               if(total_pkt_number%50 == 0 and debug == True):
                                   print("Total number of received packets:",total_pkt_number," - number of in sequence packets:",in_seq_cnt)

                           if(debug == True): printPhyRxStat(internal)
                       # calculate/update/accumulate RX stats.
                       caculateRXStats(internal, rxStats, total_pkt_number, out_of_sync_percentage)
                    else:
                        print("Sequence number out of sequence, expecting:", get_seq_number(), "and received: ", internal.transaction_index, "exiting.")
            except queue.Empty:
                print("QUEUE is empty.");
        # Check is exit flag is set.
        if(getExitFlag() == True):
            if(debug == True): print("Finish statistics thread.")
    # Push rx stats to the QUEUE.
    rx_stats_queue.put(rxStats)
    return rx_stats_queue

def caculateRXStats(internal, rxStats, total_pkt_number, out_of_sync_percentage):
    # Update values.
    rxStats.out_of_sequence     = out_of_sync_percentage
    rxStats.decoding_errors     = internal.receiver.stat.rx_stat.decoding_errors
    rxStats.sync_errors         = internal.receiver.stat.rx_stat.detection_errors
    rxStats.rssi_avg            = rxStats.rssi_avg + internal.receiver.stat.rx_stat.rssi
    if(internal.receiver.result == PHY_SUCCESS):
        rxStats.cqi_avg         = rxStats.cqi_avg + internal.receiver.stat.rx_stat.cqi
    rxStats.noise_avg           = rxStats.noise_avg + internal.receiver.stat.rx_stat.noise
    rxStats.total_pkt_number    = total_pkt_number

class RxStats():
    # Member data.
    out_of_sequence     = 0
    decoding_errors     = 0
    sync_errors         = 0
    rssi_avg            = 0
    cqi_avg             = 0
    noise_avg           = 0
    total_pkt_number    = 0

    def __init__(self, out_of_sequence=0, decoding_errors=0, sync_errors=0, rssi_avg=0, cqi_avg=0, noise_avg=0, total_pkt_number=0):
        self.out_of_sequence    = out_of_sequence
        self.decoding_errors    = decoding_errors
        self.sync_errors        = sync_errors
        self.rssi_avg           = rssi_avg
        self.cqi_avg            = cqi_avg
        self.noise_avg          = noise_avg
        self.total_pkt_number   = total_pkt_number

    def printStats(self):
        print("Out of sync percentage: %f" % (self.out_of_sequence))
        print("Decoding errors: %d" % (self.decoding_errors))
        print("Sync errors: %d" % (self.sync_errors))
        print("RSSI Avg.: %f" % (self.rssi_avg/self.total_pkt_number))
        print("CQI Avg.: %f" % (self.cqi_avg/self.total_pkt_number))
        print("Noise Avg.: %f" % (self.noise_avg/self.total_pkt_number))
        print("Total pkt number: %d\n" % (self.total_pkt_number))

def createLayerCommunicator(single):
    # Decides if single or two host PCs.
    if(single == False):
        source_module = interf.MODULE_MAC # Make believe it is the MAC layer sending controls to PHY.
    else:
        source_module = interf.MODULE_DEBUG2

    print("Create CommManager object.")
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    return lc

def measure_perfomance(lc, debug, profiling, wait_for_tx_stats, send_to_id, nof_prb, mcs, txgain, rxgain, tx_slots, rx_slots, tx_channel, rx_channel, num_of_packets_to_tx):

    # Create a QUEUE to store data.
    # Create a single input and a single output queue for all threads.
    stats_queue = queue.Queue()

    rx_stats_queue = queue.Queue()

    send_to = send_to_id
    bandwidth = getBWIndex(nof_prb)
    mcs_idx = mcs
    tx_gain = txgain
    rx_gain = rxgain
    num_of_tx_slots = tx_slots
    num_of_rx_slots = rx_slots
    seq_number = 100
    data = generateRandomData(num_of_tx_slots, bandwidth, mcs_idx)
    packet_counter = 0

    # Start TX statistics thread.
    # Wait for PHY TX Statistics from PHY.
    try:
        thread_id = _thread.start_new_thread( receiveStatisticsFromPhyThread, (lc, stats_queue, rx_stats_queue, num_of_tx_slots, wait_for_tx_stats, debug, ) )
    except:
        print("Error: unable to start thread")
        sys.exit(-1)

    # Give the thread some time.
    time.sleep(2)

    # Send RX control information to PHY.
    sendRxCtrlToPhy(lc, bandwidth, rx_channel, mcs_idx, rx_gain, num_of_rx_slots, 66, send_to, debug)

    while(True):

        packet_counter = packet_counter + 1

        # Send sequence number to TX stats thread.
        set_seq_number(seq_number)

        # Timestamp time of transmission.
        if(profiling == True):
            start = time.time()

        # Generate data according to sequence number.
        data = generateData2(num_of_tx_slots, seq_number-99, bandwidth, mcs_idx)

        # Send TX control information to PHY.
        sendTxCtrlToPhy(lc, tx_channel, bandwidth, mcs_idx, tx_gain, num_of_tx_slots, seq_number, send_to, data, debug)

        if(wait_for_tx_stats == True):
            # Wait until TX statistics is received from PHY.
            while(get_is_stat_received() == False and getExitFlag() == False):
                pass

            if(profiling == True):
                # Timestamp time of reception.
                end = time.time()
                # Print time difference
                print("TX Elapsed time:",end-start,"seconds")

            # Remove TX stats from QUEUE so that it gets empty.
            internal = stats_queue.get()
            if(debug == True): printPhyTxStat(internal)

            # Set flag back to false.
            set_is_stat_received(False)

        # Increment sequence number.
        seq_number = (seq_number + num_of_tx_slots)%200
        if(seq_number == 0):
            seq_number = 100

        # Wait sometime before transmitting again.
        if(wait_for_tx_stats == False):
            time.sleep(0.000001)

        if(getExitFlag() == True or packet_counter == num_of_packets_to_tx):
            break

    if(getExitFlag() == False and packet_counter == num_of_packets_to_tx):
        # sleep for some time to make sure all packets are received.
        time.sleep(3)
        # Leave threads.
        setExitFlag(True)
        # Wait until we have something in the QUEUE.
        while(rx_stats_queue.qsize() == 0):
            pass
        rxStats = rx_stats_queue.get()
    else:
        print("We left the measure performance function without waiting for number of packets.")

    return rxStats

def printTrialStats(txgain, rxgain, rxStats):
    print("Tx gain: %d" % (txgain))
    print("Rx gain: %d" % (rxgain))
    rxStats.printStats()
    print("")

def printTrialStatsv2(mcs, num_of_packets_to_tx, txgain, rxgain, rxStats):
    print("%d\t%d\t%d\t%f\t%d\t%d\t%f\t%f\t%f\t%d" % (mcs,txgain,rxgain,rxStats.out_of_sequence,rxStats.decoding_errors,rxStats.sync_errors,(rxStats.rssi_avg/rxStats.total_pkt_number),(rxStats.cqi_avg/rxStats.total_pkt_number),(rxStats.noise_avg/rxStats.total_pkt_number), num_of_packets_to_tx))

def adjustErrorCounters(rxStats, lastRxStats):
    adjRxStats = RxStats()
    adjRxStats.decoding_errors = rxStats.decoding_errors - lastRxStats.decoding_errors
    adjRxStats.sync_errors = rxStats.sync_errors - lastRxStats.sync_errors
    adjRxStats.out_of_sequence = rxStats.out_of_sequence
    adjRxStats.rssi_avg = rxStats.rssi_avg
    adjRxStats.cqi_avg = rxStats.cqi_avg
    adjRxStats.noise_avg = rxStats.noise_avg
    adjRxStats.total_pkt_number = rxStats.total_pkt_number
    return adjRxStats

if __name__ == '__main__':

    # Parse any input option.
    debug, profiling, single, wait_for_tx_stats, send_to_id, nof_prb, initial_mcs, txgain, rxgain, tx_slots, rx_slots, tx_channel, rx_channel, num_of_packets_to_tx = inputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    lc = createLayerCommunicator(single)

    lastRxStats = RxStats()

    for mcs_idx in range(initial_mcs,0,-1):
        mcs = mcs_idx
        for tx_power in range(0,33,3):
            txgain = tx_power
            for rx_power in range(0,33,3):
                rxgain = rx_power

                setExitFlag(False)

                rxStats = measure_perfomance(lc, debug, profiling, wait_for_tx_stats, send_to_id, nof_prb, mcs, txgain, rxgain, tx_slots, rx_slots, tx_channel, rx_channel, num_of_packets_to_tx)

                adjRxStats = adjustErrorCounters(rxStats, lastRxStats)

                lastRxStats = rxStats

                if(adjRxStats.total_pkt_number > 0): printTrialStatsv2(mcs, num_of_packets_to_tx, txgain, rxgain, adjRxStats)

                time.sleep(2)

        print("")
