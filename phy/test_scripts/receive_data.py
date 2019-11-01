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

# ************ Functions ************
exit_flag = False
def handler(signum, frame):
    global exit_flag
    exit_flag = True

def getExitFlag():
    global exit_flag
    return exit_flag

def help():
    print("Usage: pyhton3 receive_data.py")

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

def sendRxCtrlToPhy(lc, chan, rx_gain, num_data_slots, seq_number, source_module):

    # Basic Control
    trx_flag    = TRX_FLAG_RX       # TRX Mode. 2->TX - 1->RX
    seq_number  = seq_number        # Sequence number.
    bw_idx      = BW_INDEX_5_MHZ    # By default use BW: 5 MHz. Possible values: 0 - 1.4 MHz, 1 - 3 MHz, 2 - 5 MHz, 3 - 10 MHz.
    mcs         = 0                 # It has no meaning for RX. MCS is recognized automatically by receiver. MCS varies from 0 to 28.
    channel     = chan              # By default use channel 0.
    slot        = 0                 # Slot number (not used now, for future use)
    frame       = 0                 # Frame number (not used now, for future use)
    gain        = rx_gain           # RX gain. We use -1 for AGC mode.
    num_slots   = num_data_slots    # Number of slots. How many data slots we expect to receive from peer.
    send_to     = 0

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
    internal.receive.basic_ctrl.frame        = frame
    internal.receive.basic_ctrl.mcs          = mcs
    internal.receive.basic_ctrl.gain         = gain
    internal.receive.basic_ctrl.length       = num_slots

    # Print the basic control structure sent to PHY.
    printBasicControl(internal.receive.basic_ctrl, internal.transaction_index)

    # Send basic control to PHY.
    print("Sending basic control to PHY.")
    lc.send(Message(source_module, interf.MODULE_PHY, internal))

def receiveRxStatisticsFromPhy(lc, num_slots, seq_number, debug):

    pkt_count = 0
    byte_received = 0
    time_start = time.time()
    wait_for_timeout = 0;
    time_out_second = 1 # Wait 1 second.
    print("Waiting for PHY RX Statistics from PHY....")
    while(True):
        if(~lc.get_high_queue().empty()):
            try:
                #Try to get next message without waiting.
                msg = lc.get_low_queue().get_nowait()
                internal = msg.message
                if(internal.transaction_index == seq_number):
                    print("PHY RX Stats received from PHY layer.")
                    time_start = time.time()
                    pkt_count = pkt_count + 1
                    rx_stat_msg = internal.receiver.stat
                    printPhyRxStat(rx_stat_msg, pkt_count, internal.transaction_index, internal.receiver.result, internal.receiver.data)
                    byte_received = byte_received + len(internal.receiver.data)
                    wait_for_timeout = 1
            except queue.Empty:
                if(pkt_count==num_slots):
                    time_end = time.time()
                    break
                if(wait_for_timeout == 1):
                    if(time.time()-time_start > time_out_second):
                        time_end = time.time() - time_out_second
                        print("Timed out...")
                        break
                if(getExitFlag() == True):
                    print("Exit receiving function.")
                    break
    if(pkt_count > 0):
        num_received_bits = (byte_received*8.0)
        elapsed_time = time_end-time_start
        print("Packet size in bytes: ", byte_received)
        print("Number of received packets: ", pkt_count)
        print("Elapsed time: ", elapsed_time," [s]")
        print("Rate: ",num_received_bits/elapsed_time," [bits/s]")
    else:
        print("No packet received.")

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

    # Create a new layer Communicator.
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    seq_number = 200

    while(True):
        print('')
        option = int(input("Options: \n\tReception: 0 \n\tExit: 1\nSelected option: "))
        if(option == 1):
            break
        channel = int(input("Please input the channel number (0~79): "))
        gain = int(input("Please input gain: "))
        num_slots = int(input("Please input the number of data slots (1~18): ")) # 18 is the maximum value

        # Send RX control information to PHY.
        sendRxCtrlToPhy(lc, channel, gain, num_slots, seq_number, source_module)

        # Wait for PHY RX Statistics from PHY.
        receiveRxStatisticsFromPhy(lc, num_slots, seq_number, debug)

        if(getExitFlag() == True):
            break
