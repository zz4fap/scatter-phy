/*
 * CommManager.cpp
 *
 *  Created on: 17 Mar 2017
 *      Author: mcamelo
 */
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>

#include "logging/Logger.h"
#include "CommManager.h"
#include "interf.pb.h"
#include "utils.h"

using namespace std;

namespace communicator {

static string createUniqueFile(MODULE ownMODULE, MODULE otherMODULE, bool push){
	std::stringstream ss;
	ss << IPCDIR;
	if(push){
		ss << ownMODULE << "_" << otherMODULE;
	}else{
		ss << otherMODULE << "_" << ownMODULE;
	}
	return ss.str();
}

CommManager::CommManager(MODULE module_owner, MODULE other): m_running(true), own_module(module_owner), other_module(other),
				context(2), socketPull(context, ZMQ_PULL), socketPush(context, ZMQ_PUSH) {

	if (MODULE_IsValid(own_module) && MODULE_IsValid(other_module)){

		m_nameOwn = communicator::MODULE_Name(own_module);
		m_nameOther = communicator::MODULE_Name(other_module);

		const std::string pushFile = createUniqueFile(module_owner, other, true);
		const std::string pullFile = createUniqueFile(module_owner, other, false);

		//TOUCH FILE
		ofstream{pushFile};
		ofstream{pullFile};
		std::string ipcPush = "ipc://" + pushFile;
		std::string ipcPull = "ipc://" + pullFile;

		Logger::log_trace<CommManager>("Bind push socket from {0} to {1}", m_nameOwn, m_nameOther);
		socketPush.bind(ipcPush);

		Logger::log_trace<CommManager>("Connect pull socket from {0} to {1}", m_nameOwn, m_nameOther);
		socketPull.connect(ipcPull);

		Logger::log_info<CommManager>("Communicator manager started from {0} to {1}", m_nameOwn, m_nameOther);
	}
	else{
		Logger::log_error<CommManager>("Manager could not start communicator: Unknown MODULE. See {0}:{1}", __FILE__, __LINE__);
	}
}

void CommManager::stopRunning() {
	m_running = false;
}

CommManager::~CommManager() {
	Logger::log_trace<CommManager>("Close push socket from {0} to {1}", m_nameOwn, m_nameOther);
	int linger = 0;
	socketPush.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
	socketPush.close();
	Logger::log_trace<CommManager>("Close pull socket from {0} to {1}", m_nameOwn, m_nameOther);
	socketPull.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
	socketPull.close();

	Logger::log_info<CommManager>("Closed CommManager between {0} and {1}", m_nameOwn, m_nameOther);
}


bool CommManager::receive(Message& result, long long timeout){
	zmq::message_t message;

	int rc = zmq::poll(&pollitem, 1, timeout);
	if(rc < 0) {
		std::cout << "[CommManager] function: receive. Error polling..." << std::endl;
		exit(-1);
	}
	if(pollitem.revents & ZMQ_POLLIN){
		bool ret = socketPull.recv(&message);
		if(!ret) {
			std::cout << "[CommManager] function: receive. Error receiving..." << std::endl;
			exit(-1);
		}
		std::string data = std::string(static_cast<char *>(message.data()), message.size());
		result.message = deserializedData(data);
		result.destination = own_module;
    result.source = other_module;
	}else{
		return false;
	}

	return true;
}

void CommManager::send(const Message& message){
	bool ret = true;
	uint32_t trial_cnt = 0;
	zmq::message_t msg;
	// Data serialized.
	std::string serializedData;
	message.message->SerializeToString(&serializedData);
	const unsigned sz = serializedData.length();
	msg.rebuild(sz);
	memcpy((void *) msg.data (), serializedData.c_str(), sz);
	// Try sending the message.
	try {
		do {
			ret = socketPush.send(msg, ZMQ_NOBLOCK);
			// Sleep for a while before retrying again.
			if(!ret) {
				trial_cnt++;
				usleep(100);
			}
		} while(!ret && m_running && trial_cnt < 2);
		// Print message only if timedout, is running and counter greater than 10.
		if(!ret && m_running && trial_cnt > 0) {
			std::cout << "[CommManager] [Error] EAGAIN - Non-blocking mode was requested and the message cannot be sent at the moment." << std::endl;
		}
	} catch(const zmq::error_t& ex){
		std::cout << "[CommManager] [Exception] socket.send exception... num: " << ex.num() << " - what: " << ex.what() << std::endl;
		exit(-1);
	}
}

std::shared_ptr<communicator::Internal> CommManager::deserializedData(std::string &data){
	std::shared_ptr<communicator::Internal> message = std::make_shared<communicator::Internal>();
	message->ParseFromString(data);
	return message;
}

}
