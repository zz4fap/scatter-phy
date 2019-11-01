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

def help():
    print("Usage: pyhton3 mock_phy.py -m [TX|RX]")

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

def printBasicControl(basic_control, seq_num, tx_data):
    print("********************* Basic Control CMD Received ********************")
    print("trx_flag: ",basic_control.trx_flag)
    print("seq_number: ",seq_num)
    print("bw_index: ",basic_control.bw_index)
    print("ch: ",basic_control.ch)
    print("slot: ",basic_control.slot)
    print("mcs: ",basic_control.mcs)
    print("gain: ",basic_control.gain)
    print("length: ",basic_control.length)
    if(basic_control.trx_flag == 1): # TX Command
        data = []
        for d in tx_data:
            data.append(int(d))
        print("data: ",data)
    print("*********************************************************************\n")

def printPhyStats(stats, trx_flag, seq_number, status,rx_data):
    print("\n**************** PHY Stats to be sent to upper layer ****************")
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
    if(trx_flag == 0): # RX
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
    elif(trx_flag == 1): # RX
        print("power: ",stats.tx_stat.power)
    else:
        print("Invalid Mode.")
    print("*********************************************************************\n")

if __name__ == '__main__':

    # Parse any input option.
    debug = inputOptions(sys.argv[1:])

    # Instantiate the communicator module.
    module = interf.MODULE_PHY
    lc = LayerCommunicator(module)

    # Receive basic control.
    print("Waiting for basic control CMD.....")
    msg = lc.get_low_queue().get()

    # Print received basic control.
    internal = msg.message
    seq_num = internal.transaction_index

    # Decide wich message was received.
    tx_data = []
    if(internal.HasField("send")): # TX Procedure
        basic_control = internal.send.basic_ctrl
        tx_data = internal.send.app_data.data
    elif(internal.HasField("receive")): # RX Procedure
        basic_control = internal.receive.basic_ctrl
    else:
        print("Error message not expected!!!!")
        exit(-1)

    printBasicControl(basic_control, seq_num, tx_data)

    trx_flag = basic_control.trx_flag
    if(trx_flag == 0): # RX
        length = basic_control.length
    elif(trx_flag == 1): # TX
        length = 1
    else:
        print("Error: Invalid TRX Flag.")
        exit(-1)

    for packet in range(0,length,1):

        # Create PHY Statistics object.
        stats = interf.Internal()
        stats.transaction_index = seq_num

        # Decide which kind of PHY Stats should be sent upwards.
        if(trx_flag == 0): # RX
            stats.receiver.result = 0 # PHY_SUCCESS
            stats.receiver.stat.host_timestamp = int(str(time.time()).split('.')[0])
            stats.receiver.stat.fpga_timestamp = int(str(time.time()).split('.')[0])
            stats.receiver.stat.frame = basic_control.frame
            stats.receiver.stat.slot = basic_control.slot
            stats.receiver.stat.ch = basic_control.ch
            stats.receiver.stat.mcs = basic_control.mcs
            stats.receiver.stat.num_cb_total = 0
            stats.receiver.stat.num_cb_err = 0
            stats.receiver.stat.rx_stat.gain = basic_control.gain
            stats.receiver.stat.rx_stat.cqi = 15
            stats.receiver.stat.rx_stat.rssi = -100.12
            stats.receiver.stat.rx_stat.rsrp = 30.45
            stats.receiver.stat.rx_stat.rsrq = 23.56
            stats.receiver.stat.rx_stat.sinr = 15.77
            stats.receiver.stat.rx_stat.length = 85
            stats.receiver.data = bytes(range(0,stats.receiver.stat.rx_stat.length,1))
            phy_stats = stats.receiver.stat
            result = stats.receiver.result
            rx_data = stats.receiver.data
        elif(trx_flag == 1): # TX
            stats.sendr.result = 0 # PHY_SUCCESS
            stats.sendr.phy_stat.host_timestamp = int(str(time.time()).split('.')[0])
            stats.sendr.phy_stat.fpga_timestamp = int(str(time.time()).split('.')[0])
            stats.sendr.phy_stat.frame = basic_control.frame
            stats.sendr.phy_stat.slot = basic_control.slot
            stats.sendr.phy_stat.ch = basic_control.ch
            stats.sendr.phy_stat.mcs = basic_control.mcs
            stats.sendr.phy_stat.num_cb_total = 0
            stats.sendr.phy_stat.num_cb_err = 0
            stats.sendr.phy_stat.tx_stat.power = 100
            phy_stats = stats.sendr.phy_stat
            result = stats.sendr.result
            rx_data = []
        else:
            print("Invalid Flag option.")
            exit(-1)

        # Send PHY Statistics upwards.
        print("Sending PHY Stats", packet,  "to upper layer.....")
        lc.send(Message(module, interf.MODULE_MAC, stats))

        # Print PHY Statistics.
        printPhyStats(phy_stats, trx_flag, seq_num, result, rx_data)

    # Wait some seconds before leaving.
    sleep(3)
