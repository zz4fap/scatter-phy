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

def sendRxCtrlToPhy(lc, chan, num_data_slots, seq_number, module, debug):

    # Basic Control
    trx_flag    = PHY_RX_ST         # TRX Mode. 1 TX - 0 RX;
    seq_number  = seq_number        # Sequence number.
    bw_idx      = BW_IDX_Five       # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = 0                 # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    gain        = -1                # RX gain. We use -1 for AGC mode.
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
    while(True):
        if(lc.get_low_queue().empty() == False):
            try:
                #Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                time_start = time.time()
                wait_for_timeout = 1
                if(internal.transaction_index >= 200 and internal.transaction_index == get_seq_number()):
                    rx_stat_queue.put(internal)
                    set_is_stat_received(True)
                else:
                    print("Sequence number out of sequence, expecting:", get_seq_number(), "and received: ", seq_number, "exiting...............")
                    os._exit(-1)
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

    # Create a QUEUE to store data.
    # Create a single input and a single output queue for all threads.
    rx_stat_queue = queue.Queue()

    seq_number = 0
    channel = 0
    num_slots = 1
    inc = 0
    last_rxd_byte = -1 # last received byte
    out_of_seq_cnt = 0 # out of sequence error counter.
    in_seq_cnt = 0 # In sequence counter.
    total_pkt_number = 0 # Total number of received packets.

    # Start RX statistics thread.
    # Wait for PHY RX Statistics from PHY.
    try:
        thread_id = _thread.start_new_thread( receiveRxStatisticsFromPhyThread, (lc, rx_stat_queue, debug, ) )
    except:
        print ("Error: unable to start thread")
        sys.exit(-1)

    while(True):

        seq_number = inc + 200

        # Send sequence number to RX stats thread.
        set_seq_number(seq_number)

        # Send RX control information to PHY.
        sendRxCtrlToPhy(lc, channel, num_slots, seq_number, source_module, debug)

        # Wait until RX statistics is received from PHY.
        while(get_is_stat_received() == False and getExitFlag() == False):
            pass

        # Get PHY RX Statistics from PHY.
        internal = rx_stat_queue.get()
        total_rxd_bytes = internal.receiver.data
        host_timestamp = internal.receiver.stat.host_timestamp

        # Set flag back to false.
        set_is_stat_received(False)

        if(len(total_rxd_bytes) > 0):
            total_pkt_number = total_pkt_number + 1 # Increment the number of received packets.

            if(last_rxd_byte == -1):
                last_rxd_byte = total_rxd_bytes[0]

            if(total_rxd_bytes[0] == last_rxd_byte):
                if(last_rxd_byte == 100):
                    last_rxd_byte = 0
                last_rxd_byte = (last_rxd_byte + 1)
                in_seq_cnt = in_seq_cnt + 1
            else:
                last_rxd_byte = (total_rxd_bytes[0] + 1)
                if(last_rxd_byte > 100):
                    last_rxd_byte = 1
                out_of_seq_cnt = out_of_seq_cnt + 1
                print("--------> Out of Seq:", out_of_seq_cnt,"- In Seq:", in_seq_cnt, "- Total # pkts:", total_pkt_number, "- % lost packets:", ((out_of_seq_cnt*100)/total_pkt_number), "- Host timestamp:",host_timestamp)

        # Increment sequence number counter.
        inc = (inc + 1)%100

        if(getExitFlag() == True):
            break
