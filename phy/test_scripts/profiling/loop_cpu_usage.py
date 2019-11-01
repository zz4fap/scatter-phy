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
import psutil

import sys
sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

PHY_PRB_LIST = [6, 25, 50]

if __name__ == '__main__':

    for phy_bw_idx in range(0, 3):

        for mcs_idx in range(0, 29):
            cmd = "sudo python3 ./cpu_usage.py --bw=" + str(PHY_PRB_LIST[phy_bw_idx]) + " --txgain=20 --rxgain=20 --txslots=1 --mcs=" + str(mcs_idx) + " --profilling"
            os.system(cmd)
