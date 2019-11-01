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
import struct
import ctypes
import sys
import numpy as np
import math
import SenSysDemo2

sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

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

# Constants
NUMBER_OF_NECESSARY_STATS = 1500
NUMBER_OF_NECESSARY_ERROR_STATS = 150

CORRECT_ARRAY_SIZE = 1000
ERROR_ARRAY_SIZE = 1000

NUMBER_OF_NECESSARY_TX_STATS = 10
NUMBER_OF_NECESSARY_DECODED_STATS = 75
NUMBER_OF_NECESSARY_RX_STATS = 75

UPDATE_RATE_ERROR_RX = 50
UPDATE_RATE_CORRECT_RX = 100

correct_rssi_vector = np.zeros(CORRECT_ARRAY_SIZE)
correct_cqi_vector = np.zeros(CORRECT_ARRAY_SIZE)
correct_noise_vector = np.zeros(CORRECT_ARRAY_SIZE)

error_rssi_vector = np.zeros(ERROR_ARRAY_SIZE)
error_noise_vector = np.zeros(ERROR_ARRAY_SIZE)

updated_correct_values_cnt = 0
updated_wrong_values_cnt = 0

tx_stats_counter = 0
avg_coding_time = 0
avg_tx_slots = 0
rx_stats_counter = 0
decoded_counter = 0

channel_free_percentage = 0
channel_busy_percentage = 0
channel_free_avg_power = 0
channel_busy_avg_power = 0

# ************ Functions ************
tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def getExitFlag():
    global tx_exit_flag
    return tx_exit_flag

def setExitFlag(flag):
    global tx_exit_flag
    tx_exit_flag = flag

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
    print("Usage: pyhton3 ./measure_lbt_performance.py")

def printPhyTxStat(internal):
    print("************************ TX PHY Stats Packet ************************")
    print("seq_number: ",internal.transaction_index)
    print("status: ",internal.sendr.result)
    print("host_timestamp: ",internal.sendr.phy_stat.host_timestamp)
    print("fpga_timestamp: ",internal.sendr.phy_stat.fpga_timestamp)
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
    print("fpga_timestamp: ",internal.receiver.stat.fpga_timestamp)
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

def get_timestamp():
    return int(time.time()*10000)

def inputOptions(argv):
    print_stats = False # by default debug is enabled.
    profiling = False # by default profiling is disabled.
    single = False # By default we use two computers to run the tests.
    wait_for_tx_stats = False
    send_to_id = 0 # By defaylt we set the ID of the radio where to send a packet to to 0.
    nof_prb = 25 # By dafult we set the number of resource blocks to 25, i.e., 5 MHz bandwidth.
    mcs = 0 # By default MCS is set to 0, the most robust MCS.
    txgain = 0 # By default TX gain is set to 0.
    rxgain = 10 # By default RX gain is set to 10.
    tx_slots = 1
    rx_slots = 1
    rx_only = False
    tx_only = False
    calculate_tput = False
    delay_after_tx = 1 # this is given in milliseconds.

    try:
        opts, args = getopt.getopt(argv,"hdpswi:b:m:t:r:",["help","printstats","profile","single","waittxstats","sendtoid=","bw=","mcs=","txgain=","rxgain=","txslots=","rxslots=","rxonly","txonly","gettput","delayaftertx="])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            help()
            sys.exit()
        elif opt in ("-d", "--printstats"):
            print_stats = True
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
        elif opt in ("-m", "--mcs"):
            mcs = int(arg)
        elif opt in ("-t", "--txgain"):
            txgain = int(arg)
        elif opt in ("-r", "--rxgain"):
            rxgain = int(arg)
        elif opt in ("--txslots"):
            tx_slots = int(arg)
        elif opt in ("--rxslots"):
            rx_slots = int(arg)
        elif opt in ("--rxonly"):
            rx_only = True
        elif opt in ("--txonly"):
            tx_only = True
        elif opt in ("--gettput"):
            calculate_tput = True
        elif opt in ("--delayaftertx"):
            delay_after_tx = float(arg)

    return print_stats, profiling, single, wait_for_tx_stats, send_to_id, nof_prb, mcs, txgain, rxgain, tx_slots, rx_slots, rx_only, tx_only, calculate_tput, delay_after_tx

def sendRxCtrlToPhy(lc, bandwidth, chan, mcs_idx, rx_gain, num_data_slots, seq_number, send_to_id, print_stats):

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

    # Send basic control to PHY.
    lc.send(Message(lc.getModule(), interf.MODULE_PHY, internal))

def sendTxCtrlToPhy(lc, chan, bandwidth, mcs_idx, gain_value, slots, sequence_number, send_to_id, data, print_stats):

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

    # Send basic control to PHY.
    lc.send(Message(lc.getModule(), interf.MODULE_PHY, internal))

def generateDataPacket(num_slots, seq_number, bw_idx, mcs):
    # Get the total number of bytes.
    tb_size = getTransportBlockSize(bw_idx, mcs);
    data = get_packed_timestamp()
    timestamp_len = len(data)
    for j in range(num_slots):
        slot_size = tb_size
        if(j == 0):
            slot_size = tb_size - timestamp_len
        for i in range(slot_size):
            data = data + bytes([seq_number+j])
    return data

def get_packed_timestamp():
   timestamp = int(time.time()*1000000000)
   return struct.pack(">Q", timestamp)

def get_unpacked_timestamp(timestamp):
   return struct.unpack("<Q", timestamp)[0]

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
        #print("Pkt content:",seq_number+j)
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

def calculate_throughput(end_timestamp, start_timestamp, num_of_bytes):
    unpacked_start_timestamp = get_unpacked_timestamp(start_timestamp)
    diff = time_diff(end_timestamp,unpacked_start_timestamp)
    return (num_of_bytes*8)/diff

def time_diff(end,start):
    return (end - start)/1000000000;

def receiveStatisticsFromPhyThread(lc, stats_queue, num_of_tx_slots, wait_for_tx_stats, calculate_tput, min_seq_number_tx, max_seq_number_tx, min_seq_number_rx, max_seq_number_rx, print_stats, stats_fig):
    expected_rxd_byte = -1 # last received byte
    last_rxd_byte = -1
    out_of_seq_cnt = 0 # out of sequence error counter.
    in_seq_cnt = 0 # In sequence counter.
    correctly_received_pkt_cnt = 0 # Total number of correctly received packets, i.e., synchronized and decoded.
    error_cnt = 0
    same_packet_cnt = 0
    throughput = 0
    throughput_cnt = 0
    tx_stats_cnt = 0
    avg_coding_time = 0
    avg_tx_slots = 0

    while(getExitFlag() == False):
        # Check if QUEUE is empty.
        if(getExitFlag() == False and lc.get_low_queue().empty() == False):
            try:
                # Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                if(internal.sendr.result > 0 and internal.sendr.result == PHY_SUCCESS):
                    if(internal.transaction_index >= min_seq_number_tx and internal.transaction_index <= max_seq_number_tx):
                        # Calculate TX and LBT statistics.
                        if(print_stats == False):
                            calc_tx_and_lbt_stats(internal, stats_fig.tx_stats_queue)
                        # If wait for TX stats is enabled then it is necessary to change the flag and add internal to the QUEUE so that main thread can go on and consume the internal message.
                        if(wait_for_tx_stats == True and get_seq_number() == internal.transaction_index):
                            stats_queue.put(internal)
                            set_is_stat_received(True)
                        # Print LBT channel statistics.
                        tx_stats_cnt = tx_stats_cnt + 1
                        avg_coding_time = avg_coding_time + internal.sendr.phy_stat.tx_stat.coding_time
                        avg_tx_slots = avg_tx_slots + internal.sendr.phy_stat.num_cb_total
                        if(print_stats == True and tx_stats_cnt%NUMBER_OF_NECESSARY_STATS == 0):
                            print_phy_tx_stats(internal,(avg_tx_slots/tx_stats_cnt),(avg_coding_time/tx_stats_cnt))
                    else:
                        print("Sequence number out of sequence, expecting:", get_seq_number(), "and received: ", internal.transaction_index, "exiting.")
                        os._exit(-1)
                elif(internal.receiver.result > 0 and internal.transaction_index == 66):
                    if(print_stats == False):
                        # Calculate RX success and error rate.
                        calc_rx_success_error_rate(internal, stats_fig.rx_success_error_queue)

                    # Treat PHY_SUCCESS case.
                    if(internal.receiver.result == PHY_SUCCESS):
                       # Get PHY RX Statistics from PHY.
                       total_rxd_bytes = internal.receiver.data
                       host_timestamp = internal.receiver.stat.host_timestamp

                       if(len(total_rxd_bytes) > 0):
                           # Check if sequence number is inside of range.
                           if(total_rxd_bytes[10] < min_seq_number_rx or total_rxd_bytes[10] > max_seq_number_rx):
                               print("---------> Received seq. number out of range: %d" % (total_rxd_bytes[10]))

                           correctly_received_pkt_cnt = correctly_received_pkt_cnt + 1 # Increment the number of received packets.

                           if(expected_rxd_byte == -1):
                               expected_rxd_byte = total_rxd_bytes[10]

                           if(total_rxd_bytes[10] == expected_rxd_byte):
                               in_seq_cnt = in_seq_cnt + 1
                           else:
                               out_of_seq_cnt = out_of_seq_cnt + abs(total_rxd_bytes[10] - expected_rxd_byte)
                               #print("--------> Out of Seq:", out_of_seq_cnt,"- In Seq:", in_seq_cnt, "- Total # pkts:", correctly_received_pkt_cnt, "- % lost packets:", ((out_of_seq_cnt*100)/correctly_received_pkt_cnt),  " - Received:", total_rxd_bytes[10], " - Expected:", expected_rxd_byte, " - diff:",abs(total_rxd_bytes[10] - expected_rxd_byte))

                           if(last_rxd_byte != total_rxd_bytes[10]):
                               if(total_rxd_bytes[10] == max_seq_number_rx):
                                   expected_rxd_byte = min_seq_number_rx-1
                               else:
                                   expected_rxd_byte = total_rxd_bytes[10]
                               expected_rxd_byte = (expected_rxd_byte + 1)

                           # Store the last received byte.
                           last_rxd_byte = total_rxd_bytes[10]

                           # This function takes the TX timestamp and calculate the approximate throughput.
                           if(calculate_tput == True and (last_rxd_byte-1)%num_of_tx_slots == 0):
                               throughput_cnt = throughput_cnt + 1
                               throughput = throughput + calculate_throughput(host_timestamp, total_rxd_bytes[0:8], len(total_rxd_bytes))

                           # Print RX statistics.
                           if(print_stats == True and correctly_received_pkt_cnt%NUMBER_OF_NECESSARY_STATS == 0):
                               print_phy_success_rx_stats(correctly_received_pkt_cnt, out_of_seq_cnt, in_seq_cnt, throughput, throughput_cnt, internal)

                           if(print_stats == False):
                               # Calculate decoded stats.
                               calc_decoded_stats(in_seq_cnt, out_of_seq_cnt, stats_fig.decoded_stats_queue)
                               # Prepare values for plotting.
                               calc_phy_success_rx_values_for_plot(internal, stats_fig.rx_success_plot_queue)

                    # Treat PHY_ERROR case.
                    elif(internal.receiver.result == PHY_ERROR):
                        error_cnt = error_cnt + 1
                        if(print_stats == True and error_cnt%NUMBER_OF_NECESSARY_ERROR_STATS == 0):
                            print_phy_error_rx_stats(internal)

                        if(print_stats == False):
                            calc_phy_error_rx_values_for_plot(internal, stats_fig.rx_error_plot_queue)

                    else:
                        print("-----------> Received RX stats message that is neither SUCCESS or ERROR: %d" % (internal.receiver.result))

            except queue.Empty:
                print("QUEUE is empty.");
        # Check is exit flag is set.
        if(getExitFlag() == True):
            print("Finish statistics thread.")

def calc_tx_and_lbt_stats(internal, tx_stats_queue):
    global tx_stats_counter
    global avg_coding_time
    global avg_tx_slots
    global channel_free_percentage
    global channel_busy_percentage
    global channel_free_avg_power
    global channel_busy_avg_power
    # Print LBT channel statistics.
    tx_stats_counter = tx_stats_counter + 1
    avg_coding_time = avg_coding_time + internal.sendr.phy_stat.tx_stat.coding_time
    avg_tx_slots = avg_tx_slots + internal.sendr.phy_stat.num_cb_total
    if(tx_stats_counter%NUMBER_OF_NECESSARY_TX_STATS == 0):
        # Update statistics.
        average_coding_time = avg_coding_time/tx_stats_counter # average coding Time
        average_num_of_tx_slots = avg_tx_slots/tx_stats_counter # average number of tx slots
        if(internal.sendr.phy_stat.tx_stat.channel_free_cnt > 0 or internal.sendr.phy_stat.tx_stat.channel_busy_cnt > 0):
            channel_free_percentage = (internal.sendr.phy_stat.tx_stat.channel_free_cnt*100) / (internal.sendr.phy_stat.tx_stat.channel_free_cnt + internal.sendr.phy_stat.tx_stat.channel_busy_cnt)
            channel_busy_percentage = (internal.sendr.phy_stat.tx_stat.channel_busy_cnt*100) / (internal.sendr.phy_stat.tx_stat.channel_free_cnt + internal.sendr.phy_stat.tx_stat.channel_busy_cnt)
            if(internal.sendr.phy_stat.tx_stat.channel_free_cnt > 0):
                channel_free_avg_power = internal.sendr.phy_stat.tx_stat.free_energy/internal.sendr.phy_stat.tx_stat.channel_free_cnt
            if(internal.sendr.phy_stat.tx_stat.channel_busy_cnt > 0):
                channel_busy_avg_power = internal.sendr.phy_stat.tx_stat.busy_energy/internal.sendr.phy_stat.tx_stat.channel_busy_cnt
        # Update statistics.
        tx_stats = SenSysDemo2.TxLbtStats(average_coding_time, average_num_of_tx_slots, channel_free_percentage, channel_busy_percentage, channel_free_avg_power, channel_busy_avg_power)
        tx_stats_queue.put(tx_stats)

def calc_decoded_stats(in_seq_cnt, out_of_seq_cnt, decoded_stats_queue):
    global decoded_counter
    decoded_counter = decoded_counter + 1
    if(decoded_counter >= NUMBER_OF_NECESSARY_DECODED_STATS):
        decoded_counter = 0
        if(out_of_seq_cnt > 0 or in_seq_cnt > 0):
            in_sequence_percentage = (in_seq_cnt*100)/(in_seq_cnt + out_of_seq_cnt) # In sequence
            out_of_sequence_percentage = (out_of_seq_cnt*100)/(in_seq_cnt + out_of_seq_cnt) # Out of sequence
            decoded = SenSysDemo2.Decoded(in_sequence_percentage, out_of_sequence_percentage)
            decoded_stats_queue.put(decoded)

def calc_rx_success_error_rate(internal, rx_success_error_queue):
    global rx_stats_counter
    rx_stats_counter = rx_stats_counter + 1
    if(rx_stats_counter >= NUMBER_OF_NECESSARY_RX_STATS):
        rx_stats_counter = 0
        correct = (internal.receiver.stat.num_cb_total*100)/internal.receiver.stat.rx_stat.total_packets_synchronized
        error = ((internal.receiver.stat.rx_stat.decoding_errors + internal.receiver.stat.rx_stat.detection_errors)*100)/internal.receiver.stat.rx_stat.total_packets_synchronized
        rx_success_error = SenSysDemo2.RxSuccessError(correct,error)
        rx_success_error_queue.put(rx_success_error)

def calc_phy_success_rx_values_for_plot(internal, rx_success_plot_queue, update_rate=UPDATE_RATE_CORRECT_RX):
    global updated_correct_values_cnt
    # Shift the data for RSSI.
    correct_rssi_vector[:-1] = correct_rssi_vector[1:]
    correct_rssi_vector[-1:] = internal.receiver.stat.rx_stat.rssi
    # Shift the data for CQI.
    correct_cqi_vector[:-1] = correct_cqi_vector[1:]
    correct_cqi_vector[-1:] = internal.receiver.stat.rx_stat.cqi
    # Shift the data for Noise.
    correct_noise_vector[:-1] = correct_noise_vector[1:]
    correct_noise_vector[-1:] = 10*math.log10(internal.receiver.stat.rx_stat.noise)
    # Update counter.
    updated_correct_values_cnt = updated_correct_values_cnt + 1
    if(updated_correct_values_cnt >= update_rate):
        updated_correct_values_cnt = 0
        vectors = SenSysDemo2.SuccessRxForPlot(correct_rssi_vector, correct_cqi_vector, correct_noise_vector)
        rx_success_plot_queue.put(vectors)

def calc_phy_error_rx_values_for_plot(internal, rx_error_plot_queue, update_rate=UPDATE_RATE_ERROR_RX):
    global updated_wrong_values_cnt
    # Shift the data for RSSI.
    error_rssi_vector[:-1] = error_rssi_vector[1:]
    error_rssi_vector[-1:] = internal.receiver.stat.rx_stat.rssi
    # Shift the data for CQI.
    error_noise_vector[:-1] = error_noise_vector[1:]
    error_noise_vector[-1:] = 10*math.log10(internal.receiver.stat.rx_stat.noise)
    # Update counter.
    updated_wrong_values_cnt = updated_wrong_values_cnt + 1
    if(updated_wrong_values_cnt >= update_rate):
        updated_wrong_values_cnt = 0
        vectors = SenSysDemo2.ErrorRxForPlot(error_rssi_vector, error_noise_vector)
        rx_error_plot_queue.put(vectors)

def print_phy_success_rx_stats(correctly_received_pkt_cnt, out_of_seq_cnt, in_seq_cnt, throughput, throughput_cnt, internal):
    print("*************** PHY Success RX Stats ***************")
    print("# Decoded packets: %d - in seq: %d - out of seq: %d" % (correctly_received_pkt_cnt, in_seq_cnt, out_of_seq_cnt), end="")
    if(in_seq_cnt > 0 or out_of_seq_cnt > 0):
        print(" - In sequence: %1.2f %%" % ((in_seq_cnt*100)/(in_seq_cnt + out_of_seq_cnt)), end="")
        print(" - Out of sequence: %1.2f %%" % ((out_of_seq_cnt*100)/(in_seq_cnt + out_of_seq_cnt)), end="")
    if(throughput_cnt > 0):
        print(" - Throughput: %1.2 [Mbps]" % ((throughput/throughput_cnt)/1000000), end="")
    print("")
    print("CQI: %d - RSSI: %1.2f - noise: %1.5f" % (internal.receiver.stat.rx_stat.cqi,internal.receiver.stat.rx_stat.rssi,internal.receiver.stat.rx_stat.noise), end="")
    print(" - peak: %1.2f  - last noi: %d" % (internal.receiver.stat.rx_stat.peak_value,internal.receiver.stat.rx_stat.last_noi), end="")
    print("")

def print_phy_error_rx_stats(internal):
    print("*************** PHY Error RX Stats ***************")
    print("Error: %1.3f %%" % (((internal.receiver.stat.rx_stat.decoding_errors + internal.receiver.stat.rx_stat.detection_errors)*100)/internal.receiver.stat.rx_stat.total_packets_synchronized), end="")
    print(" - Correct: %1.3f %%" % ((internal.receiver.stat.num_cb_total*100)/internal.receiver.stat.rx_stat.total_packets_synchronized))
    print("# Total: %d:" % (internal.receiver.stat.rx_stat.total_packets_synchronized), end="")
    print(" - # Decoding errors: %d - # Detection errors: %d" % (internal.receiver.stat.rx_stat.decoding_errors,internal.receiver.stat.rx_stat.detection_errors))
    print("RSSI: %1.2f - peak: %1.2f" % (internal.receiver.stat.rx_stat.rssi,internal.receiver.stat.rx_stat.peak_value), end="")
    print(" - noise: %1.5f - last noi: %d" % (internal.receiver.stat.rx_stat.noise,internal.receiver.stat.rx_stat.last_noi), end="")
    print("")

def print_phy_tx_stats(internal, avg_tx_slots, avg_coding_time):
    print("*************** PHY TX Stats ***************")
    print("Avg. transmitted slots: %1.2f - Avg. coding time %1.3f [ms]" % (avg_tx_slots,avg_coding_time))
    if(internal.sendr.phy_stat.tx_stat.channel_free_cnt > 0 or internal.sendr.phy_stat.tx_stat.channel_busy_cnt > 0):
        channel_free_percentage = (internal.sendr.phy_stat.tx_stat.channel_free_cnt*100) / (internal.sendr.phy_stat.tx_stat.channel_free_cnt + internal.sendr.phy_stat.tx_stat.channel_busy_cnt)
        channel_busy_percentage = (internal.sendr.phy_stat.tx_stat.channel_busy_cnt*100) / (internal.sendr.phy_stat.tx_stat.channel_free_cnt + internal.sendr.phy_stat.tx_stat.channel_busy_cnt)
        print("free: %1.2f %% - busy: %1.2f %%" % (channel_free_percentage,channel_busy_percentage), end="")
        if(internal.sendr.phy_stat.tx_stat.channel_free_cnt > 0):
            channel_free_avg_power = internal.sendr.phy_stat.tx_stat.free_energy/internal.sendr.phy_stat.tx_stat.channel_free_cnt
            print(" - free power: %1.2f" % (channel_free_avg_power), end="")
        else:
            print(" - free power: NA", end="")
        if(internal.sendr.phy_stat.tx_stat.channel_busy_cnt > 0):
            channel_busy_avg_power = internal.sendr.phy_stat.tx_stat.busy_energy/internal.sendr.phy_stat.tx_stat.channel_busy_cnt
            print(" - busy power: %1.2f" % (channel_busy_avg_power), end="")
        else:
            print(" - busy power: NA", end="")
        print(" - # dropped slots: %d" % (internal.sendr.phy_stat.tx_stat.total_dropped_slots), end="")
        print("")

def performance_measurement_work(args, lc, demo):

    # Parse any input option.
    print_stats, profiling, single, wait_for_tx_stats, send_to_id, nof_prb, mcs_idx, tx_gain, rx_gain, num_of_tx_slots, num_of_rx_slots, rx_only, tx_only, calculate_tput, delay_after_tx = inputOptions(args)

    # Create a QUEUE to store data.
    # Create a single input and a single output queue for all threads.
    stats_queue = queue.Queue()

    channel = 0
    bandwidth = getBWIndex(nof_prb)
    num_of_tx = -1
    data = generateRandomData(num_of_tx_slots, bandwidth, mcs_idx)
    packet_counter = 0

    # Decide which sequence numbers will be used.
    if(send_to_id == 100):
        min_seq_number_tx = 1 # or 101
        max_seq_number_tx = 100 # or 200
        min_seq_number_rx = 101
        max_seq_number_rx = 200
    elif(send_to_id == 200):
        min_seq_number_tx = 101 # or 101
        max_seq_number_tx = 200 # or 200
        min_seq_number_rx = 1
        max_seq_number_rx = 100
    else:
        print("Wrong send_to ID.")
        os._exit(-1)

    # Set sequence number to be used.
    seq_number = min_seq_number_tx

    # Start statistics thread.
    # Wait for PHY TX/RX Statistics from PHY.
    try:
        thread_id = _thread.start_new_thread( receiveStatisticsFromPhyThread, (lc, stats_queue, num_of_tx_slots, wait_for_tx_stats, calculate_tput, min_seq_number_tx, max_seq_number_tx, min_seq_number_rx, max_seq_number_rx, print_stats, demo, ) )
    except:
        print("Error: unable to start statistics thread")
        sys.exit(-1)

    # Give the thread some time.
    time.sleep(2)

    # Send RX control information to PHY.
    sendRxCtrlToPhy(lc, bandwidth, channel, mcs_idx, rx_gain, num_of_rx_slots, 66, send_to_id, print_stats)

    if(rx_only == True):
        print("Radio on receive only mode.")
        while(getExitFlag() == False):
            pass
    else:
        while(getExitFlag() == False):

            packet_counter = packet_counter + 1

            # Send sequence number to TX stats thread.
            set_seq_number(seq_number)

            # Timestamp time of transmission.
            if(profiling == True):
                start = time.time()

            # Generate data according to sequence number.
            data = generateData2(num_of_tx_slots, seq_number, bandwidth, mcs_idx)

            # Send TX control information to PHY.
            sendTxCtrlToPhy(lc, channel, bandwidth, mcs_idx, tx_gain, num_of_tx_slots, seq_number, send_to_id, data, print_stats)

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

                # Set flag back to false.
                set_is_stat_received(False)

            # Increment sequence number.
            seq_number = seq_number + num_of_tx_slots
            if(seq_number > max_seq_number_tx):
                seq_number = min_seq_number_tx;

            # Wait sometime in milliseconds before transmitting again.
            if(delay_after_tx > 0.0 and wait_for_tx_stats == False):
                time.sleep(delay_after_tx/1000.0)

            if(getExitFlag() == True or packet_counter == num_of_tx):
                break

def resetGlobalVaribales():
    global correct_rssi_vector
    global correct_cqi_vector
    global correct_noise_vector
    global error_rssi_vector
    global error_noise_vector
    global updated_correct_values_cnt
    global updated_wrong_values_cnt
    global tx_stats_counter
    global avg_coding_time
    global avg_tx_slots
    global rx_stats_counter
    global decoded_counter
    global channel_free_percentage
    global channel_busy_percentage
    global channel_free_avg_power
    global channel_busy_avg_power

    correct_rssi_vector = np.zeros(CORRECT_ARRAY_SIZE)
    correct_cqi_vector = np.zeros(CORRECT_ARRAY_SIZE)
    correct_noise_vector = np.zeros(CORRECT_ARRAY_SIZE)
    error_rssi_vector = np.zeros(ERROR_ARRAY_SIZE)
    error_noise_vector = np.zeros(ERROR_ARRAY_SIZE)
    updated_correct_values_cnt = 0
    updated_wrong_values_cnt = 0
    tx_stats_counter = 0
    avg_coding_time = 0
    avg_tx_slots = 0
    rx_stats_counter = 0
    decoded_counter = 0
    channel_free_percentage = 0
    channel_busy_percentage = 0
    channel_free_avg_power = 0
    channel_busy_avg_power = 0

def run_performance_measurement(args, lc, demo):
    # Set flag to false so that we can run the thread.
    setExitFlag(False)
    resetGlobalVaribales()
    # Start measurement thread.
    try:
        thread_id = _thread.start_new_thread( performance_measurement_work, (args, lc, demo, ) )
    except:
        print("Error: unable to start transmission thread")
        sys.exit(-1)

def stop_performance_measurement():
    setExitFlag(True)

if __name__ == '__main__':

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # Run LBT performance measurements.
    run_performance_measurement(sys.argv[1:])
