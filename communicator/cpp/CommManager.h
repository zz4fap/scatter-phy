/*
 * CommManager.h
 *
 *  Created on: 17 Mar 2017
 *      Author: mcamelo
 */

#ifndef COMMMANAGER_H_

#define COMMMANAGER_H_

#include <ostream>
#include <memory>
#include <string>
#include <zmq.hpp>
#include "Message.h"
#include "interf.pb.h"

#ifndef IPCDIR
#define IPCDIR "/tmp/IPC_"
#endif

namespace communicator {

class CommManager {
public:
	CommManager(MODULE module_owner, MODULE other);
	virtual ~CommManager();
	void send(const Message& message);
	bool receive(Message& result, long long timeout = -1);
	void stopRunning();

	inline communicator::MODULE	communicatorTo() const {return other_module;}

private:
	std::shared_ptr<communicator::Internal> deserializedData(std::string &data);

	bool 												m_running;
	const communicator::MODULE	own_module;
	const communicator::MODULE	other_module;
	zmq::context_t 							context;
	zmq::socket_t 							socketPull;
	zmq::socket_t								socketPush;
	zmq::pollitem_t							pollitem = {static_cast<void *>(socketPull), 0, ZMQ_POLLIN, 0 };
	std::string 								m_nameOwn;
	std::string 								m_nameOther;

};

} /* END NAMESPACE */


#endif /* COMMMANAGER_H_ */
