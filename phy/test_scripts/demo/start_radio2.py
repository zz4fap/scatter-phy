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
import shlex, subprocess

import sys
sys.path.append('../../')
sys.path.append('../../communicator/python/')

tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def parseInputOptions(argv):
    phy_verbose = " "
    wait_for_tx_stats = " "
    tx_gain = 26
    rx_gain = 0
    num_tx_slots = 2
    rx_only = " "
    lbt_threshold = 1000
    lbt_sample_rate = 5760000
    delay_after_tx = 1 # in milliseconds.
    lbt_backoff = 32
    lbt_timeout = 10000000000000
    lbt_immediate_transmission = " -z "
    print_stats = " "

    try:
        opts, args = getopt.getopt(argv,"",["phyverbose","waittxstats","txgain=","rxgain=","txslots=","rxonly","lbtthreshold=","lbtsamplerate=","delayaftertx=","lbtbackoff=","lbttimeout=","lbtimmediatetx","printstats"])
    except getopt.GetoptError:
        print("Error with getopt....")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("--phyverbose"):
            phy_verbose = " -v "
        elif opt in ("--waittxstats"):
            wait_for_tx_stats = " --waittxstats "
        elif opt in ("--txgain"):
            tx_gain = int(arg)
        elif opt in ("--rxgain"):
            rx_gain = int(arg)
        elif opt in ("--txslots"):
            num_tx_slots = int(arg)
        elif opt in ("--rxonly"):
            rx_only = " --rxonly "
        elif opt in ("--lbtthreshold"):
            lbt_threshold = int(arg)
        elif opt in ("--lbtsamplerate"):
            lbt_sample_rate = int(arg)
        elif opt in ("--lbtbackoff"):
            lbt_backoff = int(arg)
        elif opt in ("--lbttimeout"):
            lbt_timeout = int(arg)
        elif opt in ("--lbtimmediatetx"):
            lbt_immediate_transmission = " "
        elif opt in ("--delayaftertx"):
            delay_after_tx = float(arg)
        elif opt in ("--printstats"):
            print_stats = " --printstats "

    return phy_verbose, wait_for_tx_stats, tx_gain, rx_gain, num_tx_slots, rx_only, lbt_threshold, lbt_sample_rate, delay_after_tx, lbt_backoff, lbt_timeout, lbt_immediate_transmission, print_stats

if __name__ == '__main__':

    # Parse input arguments.
    phy_verbose, wait_for_tx_stats, tx_gain, rx_gain, num_tx_slots, rx_only, lbt_threshold, lbt_sample_rate, delay_after_tx, lbt_backoff, lbt_timeout, lbt_immediate_transmission, print_stats = parseInputOptions(sys.argv[1:])

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    # Fix PHY Parameters.
    center_frequency = 3000000000 # default_center_frequency
    competition_bw = 20000000
    radio_id = 200
    td_noi = 20
    number_of_prb = 25

    # Fix APP Parameters.
    mcs = 20
    send_to_id = 100

    # ************** Run PHY ****************
    phy_cmd = "sudo ../../build/phy/srslte/examples/trx"+phy_verbose+"-p -x"+lbt_immediate_transmission+"-f "+str(center_frequency)+" -B "+str(competition_bw)+" -l "+str(lbt_threshold)+" -m "+str(td_noi)+" -i "+str(radio_id)+" -b "+str(number_of_prb)+" -s "+str(lbt_sample_rate)+" -o "+str(lbt_timeout)+" -X "+str(lbt_backoff)

    print(phy_cmd)

    pid_phy = subprocess.Popen(phy_cmd,shell=True).pid

    # Give PHY some time.
    time.sleep(10)

    # ************** Run APP ****************
    app_cmd = "sudo python3 ./measure_lbt_performance.py --mcs="+str(mcs)+" --txgain="+str(tx_gain)+" --rxgain="+str(rx_gain)+" --bw="+str(number_of_prb)+" --sendtoid="+str(send_to_id)+" --txslots="+str(num_tx_slots)+" --delayaftertx="+str(delay_after_tx) + rx_only + wait_for_tx_stats + print_stats

    print(app_cmd)

    app_pid = subprocess.Popen(app_cmd,shell=True).pid

    while(tx_exit_flag == False):
        pass

    os.system("sudo killall -9 trx")
    os.system("sudo killall -9 python3")
