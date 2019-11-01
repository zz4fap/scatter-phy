In order to compile the example and link it there are some steps that need to be covered.  
All of the below steps are tested on Ubuntu 14.04.5 LTS 64 bit.  
  
1) Install google protobufs library:   
We need to build it from source, as the native Ubuntu libprotobuf-dev library (that you can install through apt-get) is old  

    #setup dependencies  
    sudo apt-get install autoconf automake libtool curl make g++ unzip pkg-config

    #get source code  
    git clone https://github.com/google/protobuf.git  

    cd protobuf  
    ./autogen.sh  
    ./configure  
    make  
    make check  
    #If no errors are present  
    sudo make install  
    #refresh shared library cache.  
    sudo ldconfig   
  
2) Install zmq devel library  
  
    sudo apt-get install libzmq3-dev (TESTED AND OK)  

OR if you want the latest version follow the instruction at:   
http://zeromq.org/intro:get-the-software (Alternative)  

Afterwards the example.cpp should be able to compile successfully.  
  
To execute the example successfully you need to start the ../python/Forwarder.py first, then initiate 2 example instances, each one with argument one of the enumerations below that can be found in ../interf.proto.  

enum MODULE {  
	MODULE_PHY 	= 0;  
	MODULE_MAC 	= 1;  
	MODULE_AI	= 2;  
};  

If you have problems with setting up the Forwarder, execute next commands

    #install pip3
    sudo apt-get install python3-pip
    #use pip3 to install the python zmq packages
    pip3 install pyzmq
    #run the ZMQ Forwarder
    python3 ../python/Forwarder.py

    
