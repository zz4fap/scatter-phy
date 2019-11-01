#!/usr/bin/python3
import zmq
import functools
import time

import sys
sys.path.append('../../')
sys.path.append('../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

print = functools.partial(print, flush=True)

ipc_location = "/tmp/IPC_"
my_module = interf.MODULE_ENVIRONMENT
target_modules = [interf.MODULE_AI, interf.MODULE_COLLAB_CLIENT, interf.MODULE_PHY]

# Create the message
message = interf.Internal()
message.owner_module = my_module
message.set.environment_updated = True
serialized = message.SerializeToString()

context = zmq.Context(1)
for target in target_modules:
    target_str = interf.MODULE.Name(target)
    print("Send out message to {}".format(target_str))
    ipcfile = "ipc://{}{}_{}".format(ipc_location, my_module, target)
    socket = context.socket(zmq.PUSH)
    socket.bind(ipcfile)
    # Apparently we need some time here to let the other module to connect
    time.sleep(0.5)
    try:
        socket.send(serialized, flags=zmq.NOBLOCK)
    except zmq.error.Again:
        print("  Could not send message to {}".format(target_str))
    socket.close()
context.destroy()

print("All messages send")
