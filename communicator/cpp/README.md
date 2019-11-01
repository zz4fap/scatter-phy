A typical layer template is added, layer_template.cpp. It can be tested by executing twice with arguments MODULE_MAC and MODULE_PHY (Mac should be started first).  
PHY will send commands to MAC and MAC will immidiately respont back. PHY is blocked until getting a response, to get miminum latency as a test and make sure no packets are lost. 

A high load exhcange of ping like messages is used to measure the round trip delay of ZMQ using TCP as transport layer.  
First results show on average around 1600 usec latency.  (We will need to test with IPC as it seems as well)  
Please check the internal workflow of the layer template and comment. It can surely serve as a first implementation skeleton for our architecture. 


> **Note:**
Remains to be tested: Data packet latency, Multiple message senders to one recipient. 
>
 

