from datetime import datetime, time
from time import sleep
import os
import signal
import subprocess
import getopt
import sys

#static variables:
numtotaltb=10000
txslots = 20
numpktstotx=int(numtotaltb/txslots)

# test of august 8 settings:
# mcs_lists = list(range(4,16,4)) # end is not included, need to add a margin
# txgain_lists = list(range(16,33,4))
# rxgain_lists = list(range(16,33,4))


def help():
    print("loop_top.py calls test_lwei.py to perform a set of experiment, the following options can be used: ")
    print("-s: specify if this node is a receiver by -s 0, default transmitter. ")
    print("-b: specify the bandwidth, must be one of [1.4M, 3M, 5M].")
    print("-f: specify the location of the file and file prefix. ")
    print("-c: specify the channel in use, must be integer, take care it is within the competition bandwidth of your running scenario")
    print("--mcsrange [start:step:end]: specify the mcs range, eg 0:2:28 will loop through the mcs from 0 to 28 in step of 2")
    print("--txgainrange [start:step:end]: specify the tx gain range")
    print("--rxgainrange [start:step:end]: specify the rx gain range")
    print("example usage: python3 loop_top.py -s 0 -b 1.4M -c 5 -f /logs/test --txgainrange 20:2:26 --rxgainrange 20:2:26 --mcsrange 0:2:28")
    print("INFO: This script is only tested for 1.4 MHz bw, if you want to use it for 5 MHz or 3 MHz, please measure the duration of a single test with highest mcs, and replace the automatic loop time (see todo)")
    print("INFO: This script configures a single test to transmit 10k TB in group of 20, you may adapt the static variables to change this configuration")
    
def inputOptions(argv):
    is_sender = True
    bw = "1.4M"
    fileprefix="/logs/test_"
    channelinuse=2
    txgain_lists = []
    rxgain_lists = []
    mcs_lists = []

    try:
        opts, args = getopt.getopt(argv,"hs:b:f:c:",["help","issender=","bw=","fileprefix=","channel=","mcsrange=","txgainrange=","rxgainrange="])
    except getopt.GetoptError as err:
        help()
        print(err)
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            help()
            sys.exit()
        elif opt in ("-s","--issender"):
            temp = int(arg)
            if(temp==0):
                is_sender = False
        elif opt in ("-b","--bw"):
            bw=str(arg)
            if (bw == "1.4M"):
                rb = 6 # for 1.4 MHz
                pb = 1400000
            elif (bw == "3M"):    
                rb=15
                pb = 3000000
            elif (bw == "5M"):
                rb=25
                pb = 5000000
            else:
                print("Error, unrecognized bandwidth, must be 1.4M, 3M or 5M")
                sys.exit(2)
        elif opt in ("-f", "--fileprefix"):
            fileprefix = str(arg)
        elif opt in ("-c","--channel"):
            try:
                channelinuse=int(arg)
            except ValueError:
                print("Error, channel must be an integer")
                sys.exit(2)
        elif opt in ("--mcsrange"):
            try:
                tmp = list(map(lambda x: int(x), arg.split(":")))
                if(len(tmp) == 3):
                    mcs_lists = list(range(tmp[0],tmp[2]+1,tmp[1]))
                else:
                    mcs_lists = [tmp[0]]
            except ValueError:
                print("Error, mcs range must be specified as start:step:end, mcs must be within [0,28]")
                sys.exit(2)
        elif opt in ("--txgainrange"):
            try:
                tmp = list(map(lambda x: int(x), arg.split(":")))
                if(len(tmp) == 3):
                    txgain_lists = list(range(tmp[0],tmp[2]+1,tmp[1]))
                else:
                    txgain_lists = [tmp[0]]
            except ValueError:
                print("Error, txgain range must be specified as start:step:end, txgain must be within [0,32]")
                sys.exit(2)
        elif opt in ("--rxgainrange"):
            try:
                tmp = list(map(lambda x: int(x), arg.split(":")))
                if(len(tmp) == 3):
                    rxgain_lists = list(range(tmp[0],tmp[2]+1,tmp[1]))
                else:
                    rxgain_lists = [tmp[0]]
            except ValueError:
                print("Error, rxgain range must be specified as start:step:end, txgain must be within [0,32]")
                sys.exit(2)       
    return bw, rb, pb, is_sender, fileprefix, channelinuse, txgain_lists,rxgain_lists,mcs_lists
            
            

if __name__ == '__main__':

    bw, rb, pb, is_sender, fileprefix, channelinuse, txgain_lists,rxgain_lists,mcs_lists =inputOptions(sys.argv[1:])
    print(bw+fileprefix)
    print(is_sender)
    print(rb)
    print(pb)
    print(channelinuse)
    print(mcs_lists)
    print(txgain_lists)
    print(rxgain_lists)
    enter_loop = True
    
    if enter_loop:
        os.system('killall trx ')
        os.system('killall test_lwei')
        sleep(5)
        print("hi, starting trx ")
        proc1 = subprocess.Popen(['/root/gitrepo/default/scatter/build/phy/srslte/examples/trx', '-f', '1000000000', '-p', str(pb)])
        sleep(10)

        mcs_idx = 0 
        txgain_idx = 0 
        rxgain_idx = 0
        while True:
            a = datetime.now()
            if(is_sender):
                # TODO, now only measured the single test duration of 1.4 MHz is 2 minutes, for 3 MHz and 5 MHz you need to measure and adapt 
                if (a.second == 8 and (a.minute % 2 )==0): 
                    # get new parameter
                    mcs = mcs_lists[mcs_idx]
                    txgain = txgain_lists[txgain_idx]
                    rxgain = rxgain_lists[rxgain_idx]
                    # start executing
                    print("sending with mcs "+str(mcs)+", txgain "+ str(txgain))
                    proc2 = subprocess.Popen(['python3', '/root/gitrepo/default/scatter/phy/test_scripts/test_lwei.py', '-b', str(rb), '--stats', '--txchannel', str(channelinuse), '--rxchannel', str(channelinuse+1), '--rxgain', str(0), '--txgain', str(txgain), '--mcs', str(mcs), '--txslots', str(txslots), '--numpktstotx', str(numpktstotx)])
                    proc2.wait()
                    
                    # increase the idx pointers 
                    mcs_idx = mcs_idx + 1
                    if (mcs_idx == len(mcs_lists)):
                        txgain_idx = txgain_idx + 1
                        mcs_idx = 0 
                        if(txgain_idx ==len(txgain_lists)):
                            rxgain_idx = rxgain_idx + 1 
                            txgain_idx = 0
                            if(rxgain_idx ==len(rxgain_lists)):
                                break
                    
            else: # receiver case 
                if (a.second == 0 and (a.minute % 2 )==0):
                    try: proc2.send_signal(signal.SIGINT)
                    except: print("first time no process found to kill")
                    sleep(3)
                    # get new parameter
                    mcs = mcs_lists[mcs_idx]
                    txgain = txgain_lists[txgain_idx]
                    rxgain = rxgain_lists[rxgain_idx]
                    # start executing
                    filename = fileprefix+'_bw'+bw+'_txgain'+str(txgain)+'_rxgain'+str(rxgain)+'_mcs'+str(mcs)+'.txt'
                    print("logging to: "+filename)
                    proc2 = subprocess.Popen(['python3', '/root/gitrepo/default/scatter/phy/test_scripts/test_lwei.py', '-b', str(rb), '--stats', '--txchannel', str(channelinuse+2), '--rxchannel', str(channelinuse), '--rxgain', str(rxgain), '--txgain', str(0), '--mcs', str(mcs), '--txslots', str(txslots), '--rfboost', '0.0001','--filename', filename]) # mute tx, transmit on a different channel. 
                    
                    # increase the idx pointers
                    mcs_idx = mcs_idx + 1
                    if (mcs_idx == len(mcs_lists)):
                        txgain_idx = txgain_idx + 1
                        mcs_idx = 0 
                        if(txgain_idx ==len(txgain_lists)):
                            rxgain_idx = rxgain_idx + 1
                            txgain_idx = 0
                            if(rxgain_idx == len(rxgain_lists)):
                                sleep(110) # needed for the last execution 
                                proc2.send_signal(signal.SIGINT)
                                sleep(5)
                                break

        sleep(1)
        print("terminated")

        
        
