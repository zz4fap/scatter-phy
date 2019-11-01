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
import measure_lbt_performance
import sys

sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def parseInputOptions(argv):
    phy_verbose = " "
    wait_for_tx_stats = ""
    tx_gain = 26
    rx_gain = 0
    num_tx_slots = 2
    rx_only = ""
    lbt_threshold = 1000
    lbt_sample_rate = 5760000
    delay_after_tx = 1 # in milliseconds.
    lbt_backoff = 32
    lbt_timeout = 10000000000000
    lbt_immediate_transmission = " -z "
    print_stats = ""
    center_frequency = 3000000000
    competition_bw = 20000000
    radio_id = 100
    td_noi = 20
    number_of_prb = 25
    mcs = 20
    send_to_id = 200

    try:
        opts, args = getopt.getopt(argv,"",["phyverbose","waittxstats","txgain=","rxgain=","txslots=","rxonly","lbtthreshold=","lbtsamplerate=","delayaftertx=","lbtbackoff=","lbttimeout=","lbtimmediatetx","printstats","frequency=","competitionbw=","radioid=","tdnoi=","prb=","mcs=","sendtoid="])
    except getopt.GetoptError:
        print("Error with getopt....")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("--phyverbose"):
            phy_verbose = " -v "
        elif opt in ("--waittxstats"):
            wait_for_tx_stats = "--waittxstats"
        elif opt in ("--txgain"):
            tx_gain = int(arg)
        elif opt in ("--rxgain"):
            rx_gain = int(arg)
        elif opt in ("--txslots"):
            num_tx_slots = int(arg)
        elif opt in ("--rxonly"):
            rx_only = "--rxonly"
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
            print_stats = "--printstats"
        elif opt in ("--frequency"):
            center_frequency = int(arg)
        elif opt in ("--competitionbw"):
            competition_bw = int(arg)
        elif opt in ("--radioid"):
            radio_id = int(arg)
        elif opt in ("--tdnoi"):
            td_noi = int(arg)
        elif opt in ("--prb"):
            number_of_prb = int(arg)
        elif opt in ("--mcs"):
            mcs = int(arg)
        elif opt in ("--sendtoid"):
            send_to_id = int(arg)

    return phy_verbose, wait_for_tx_stats, tx_gain, rx_gain, num_tx_slots, rx_only, lbt_threshold, lbt_sample_rate, delay_after_tx, lbt_backoff, lbt_timeout, lbt_immediate_transmission, print_stats, center_frequency, competition_bw, radio_id, td_noi, number_of_prb, mcs, send_to_id

def start_radio(args, lc, demo):
    # Parse input arguments.
    phy_verbose, wait_for_tx_stats, tx_gain, rx_gain, num_tx_slots, rx_only, lbt_threshold, lbt_sample_rate, delay_after_tx, lbt_backoff, lbt_timeout, lbt_immediate_transmission, print_stats, center_frequency, competition_bw, radio_id, td_noi, number_of_prb, mcs, send_to_id = parseInputOptions(args)

    # ************** Run PHY ****************
    phy_cmd = "sudo ../../../build/phy/srslte/examples/trx"+phy_verbose+"-p -x"+lbt_immediate_transmission+"-f "+str(center_frequency)+" -B "+str(competition_bw)+" -l "+str(lbt_threshold)+" -m "+str(td_noi)+" -i "+str(radio_id)+" -b "+str(number_of_prb)+" -s "+str(lbt_sample_rate)+" -o "+str(lbt_timeout)+" -X "+str(lbt_backoff)+" -g "+str(rx_gain)+" -t "+str(tx_gain)

    print(phy_cmd)

    pid_phy = subprocess.Popen(phy_cmd,shell=True).pid

    # Give PHY some time.
    time.sleep(7)

    # ************** Run APP ****************
    # Create argument list.
    args = []
    args.append("--mcs="+str(mcs))
    args.append("--txgain="+str(tx_gain))
    args.append("--rxgain="+str(rx_gain))
    args.append("--bw="+str(number_of_prb))
    args.append("--sendtoid="+str(send_to_id))
    args.append("--txslots="+str(num_tx_slots))
    args.append("--delayaftertx="+str(delay_after_tx))
    if(len(rx_only) > 0): args.append(rx_only)
    if(len(wait_for_tx_stats)): args.append(wait_for_tx_stats)
    if(len(print_stats)): args.append(print_stats)
    # Run LBT performance measurements.
    measure_lbt_performance.run_performance_measurement(args, lc, demo)
    # Print the arguments used to call the performance worker.
    print(args)

def stop_radio():
    measure_lbt_performance.stop_performance_measurement()
    os.system("sudo killall -9 trx")
    time.sleep(2)

if __name__ == '__main__':

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    start_radio(sys.argv[1:])

    while(tx_exit_flag == False):
        pass

    stop_radio()
