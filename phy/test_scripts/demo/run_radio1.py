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
import SenSysDemo2
import sys
from gi.repository import Gtk
import radio

sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

# These are the list of arguments used to run each one of the scenarios.
cmd_single_radio = ['--delayaftertx=0', '--txslots=20', '--txgain=16', '--rxonly']
cmd_lbt_disabled = ['--delayaftertx=0', '--txslots=20', '--txgain=16'] #'--phyverbose'
cmd_lbt_enabled  = ['--delayaftertx=0', '--txslots=20', '--txgain=16', '--lbtthreshold=-78', '--lbtbackoff=16']

if __name__ == '__main__':

    try:
        new_value = os.nice(-20)
        print("Nice set to:",new_value)
    except OSError:
        print("Could not set niceness")

    source_module = interf.MODULE_MAC
    print("Create CommManager object.")
    lc = LayerCommunicator(source_module, [interf.MODULE_PHY])

    # Fix PHY Parameters.
    radio_id = 100
    # Fix APP Parameters.
    send_to_id = 200

    cmd_single_radio.append("--radioid="+str(radio_id))
    cmd_single_radio.append("--sendtoid="+str(send_to_id))

    cmd_lbt_disabled.append("--radioid="+str(radio_id))
    cmd_lbt_disabled.append("--sendtoid="+str(send_to_id))

    cmd_lbt_enabled.append("--radioid="+str(radio_id))
    cmd_lbt_enabled.append("--sendtoid="+str(send_to_id))

    cmds = []
    cmds.append(cmd_single_radio)
    cmds.append(cmd_lbt_disabled)
    cmds.append(cmd_lbt_enabled)

    # Start GUI.
    demo = SenSysDemo2.SenSysDemo2(lc)
    demo.set_commands(cmds)
    demo.init()

    Gtk.main()
