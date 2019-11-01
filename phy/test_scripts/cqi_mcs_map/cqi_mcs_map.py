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
sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

PRB_VECTOR = [7, 15, 25]

NOF_SLOTS_VECTOR = [1]

MCS_VECTOR = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]

tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def getExitFlag():
    global tx_exit_flag
    return tx_exit_flag

def kill_phy():
    os.system("~/radio_api/stop.sh")
    os.system("~/radio_api/kill_stack.py")
    os.system("killall -9 trx")
    os.system("killall -9 check_cqi_mcs_mapping")

def start_phy(nof_phys, bw, center_freq, comp_bw):
    cmd1 = "sudo ../../../build/phy/srslte/examples/trx -f " + str(center_freq) + " -E " + str(nof_phys) + " -B " + str(comp_bw) + " -p " + str(bw) + " &"
    os.system(cmd1)

def start_cqi_mcs_mapping(nof_prb, nof_slots, mcs, txgain, rxgain):
    cmd1 = "sudo ../../../build/phy/srslte/examples/check_cqi_mcs_mapping -d " + str(nof_phys) + " -n " + str(nof_slots) + " -m " + str(mcs) + " -p " + str(nof_prb)
    os.system(cmd1)

def start_scenario():
    cmd1 = "colosseumcli rf start 8981 -c"
    os.system(cmd1)

def save_files(bw_idx, nof_prb):
    cmd1 = "mkdir dir_cqi_mcs_map_prb_" + str(nof_prb)
    os.system(cmd1)
    cmd2 = "mv cqi_mcs_map_phy_bw_" + str(bw_idx) + "* dir_cqi_mcs_map_prb_" + str(nof_prb)
    os.system(cmd2)
    cmd3 = "tar -cvzf" + " dir_cqi_mcs_map_prb_" + str(nof_prb) + ".tar.gz"  + " dir_cqi_mcs_map_prb_" + str(nof_prb) + "/"
    os.system(cmd3)

def get_bw_from_prb(nof_prb):
    bw = 0
    if(nof_prb == 6):
        bw = 1400000
    elif(nof_prb == 7):
        bw = 1400000
    elif(nof_prb == 15):
        bw = 3000000
    elif(nof_prb == 25):
        bw = 5000000
    elif(nof_prb == 50):
        bw = 10000000
    else:
        printf("Invalid number of PRB")
        exit(-1)
    return bw

def inputOptions(argv):
    nof_prb = 25 # By dafult we set the number of resource blocks to 25, i.e., 5 MHz bandwidth.
    mcs = 0 # By default MCS is set to 0, the most robust MCS.
    txgain = 10 # By default TX gain is set to 10.
    rxgain = 10 # By default RX gain is set to 10.
    tx_slots = 1
    tx_channel = 0
    rx_channel = 0
    run_scenario = False
    center_freq = 2500000000
    comp_bw = 40000000
    nof_phys = 2

    try:
        opts, args = getopt.getopt(argv,"h",["help","bw=","mcs=","txgain=","rxgain=","txslots=","txchannel=","rxchannel=","runscenario","centerfreq=","compbw=","nofphys="])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("--help"):
            sys.exit()
        elif opt in ("--bw"):
            nof_prb = int(arg)
        elif opt in ("--mcs"):
            mcs = int(arg)
        elif opt in ("--txgain"):
            txgain = int(arg)
        elif opt in ("--rxgain"):
            rxgain = int(arg)
        elif opt in ("--txslots"):
            tx_slots = int(arg)
        elif opt in ("--txchannel"):
            tx_channel = int(arg)
        elif opt in ("--rxchannel"):
            rx_channel = int(arg)
        elif opt in ("--runscenario"):
            run_scenario = True
        elif opt in ("--centerfreq"):
            center_freq = int(arg)
        elif opt in ("--compbw"):
            comp_bw = int(arg)
        elif opt in ("--nofphys"):
            nof_phys = int(arg)

    return nof_prb, mcs, txgain, rxgain, tx_slots, tx_channel, rx_channel, run_scenario, center_freq, comp_bw, nof_phys

if __name__ == '__main__':

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    nof_prb, mcs, txgain, rxgain, tx_slots, tx_channel, rx_channel, run_scenario, center_freq, comp_bw, nof_phys = inputOptions(sys.argv[1:])

    if(run_scenario == True):
        start_scenario()

    time.sleep(10)

    for prb_idx in range(0,len(PRB_VECTOR)):

        nof_prb = PRB_VECTOR[prb_idx]
        bw = get_bw_from_prb(nof_prb)

        for mcs_idx in range(0,len(MCS_VECTOR)):

            for nof_slots_idx in range(0,len(NOF_SLOTS_VECTOR)):

                # Make sure PHY is not running.
                kill_phy()

                time.sleep(5)

                # Start PHY.
                start_phy(nof_phys, bw, center_freq, comp_bw)

                time.sleep(15)

                nof_slots = NOF_SLOTS_VECTOR[nof_slots_idx]

                mcs = MCS_VECTOR[mcs_idx]

                start_cqi_mcs_mapping(nof_prb, nof_slots, mcs, txgain, rxgain)

                if(getExitFlag() == True):
                    exit(0)

            if(getExitFlag() == True):
                exit(0)

        # Save files.
        save_files(prb_idx+1, nof_prb)

        if(getExitFlag() == True):
            exit(0)
