/*
 * LayerCommunicator.h
 *
 *  Created on: 28 Mar 2017
 *      Author: rubenmennes
 */

#ifndef LAYERCOMMUNICATOR_H_
#define LAYERCOMMUNICATOR_H_

#include <thread>
#include <chrono>
#include <vector>
#include <deque>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <sched.h>
#include <iostream>
#include "Message.h"
#include "CommManager.h"
#include "SafeQueue.h"
#include "PriorityQueue.h"
#include "logging/Logger.h"

#define CHECK_LAYER_COMM_OUT_OF_SEQUENCE 0

namespace communicator {
/*
 * Helper struct for the threads
 */
struct CommManagerThreads {
	CommManager* commManager;
	std::thread receiving;

	CommManagerThreads(): commManager(nullptr), receiving(){}
	CommManagerThreads(CommManagerThreads&& other): commManager(other.commManager){ receiving.swap(other.receiving);}
};

class AbstractLayerCommunicator{
public:
	/*
	 * Constructor for the module
	 * @param module: This module
	 * @param connectTo: List of modules to connect to
	 */
	AbstractLayerCommunicator(MODULE module);

	/*
	 * Destructor
	 */
	virtual ~AbstractLayerCommunicator();

	void init(std::vector<MODULE> connectTo);
	void init_high_priority(std::vector<MODULE> connectTo);

	virtual bool highPriorityPop(Message&) = 0;
	virtual bool lowPriorityPop(Message&) = 0;

	/*
	 * Send a message by copying the data in the send queue
	 */
	virtual void send(const Message& m) = 0;

	/*
	 * Send a message by moving the data in the send queue
	 */
	virtual void send(Message&& m)		= 0;

	/*
	 * Stop LayerCommunicator (LayerCommunicator can not start again)
	 */
	inline void stop() {m_running = false; }

	/*
	 * Semaphore (with condition variable) to wait on message in total LayerCommunicator.
	 * Blocks thread until there is a message in Low or High Queue
	 * @return always TRUE because there will always be a message
	 */
	bool waitForMessage() const;

	/*
	 * Semaphore (with condition variable) to wait on message in total LayerCommunicator.
	 * Blocks thread until there is a message in Low or High Queue
	 * @param rel_time: maximal time to block the queue
	 * @return True is there was a message in a queue. Else false.
	 */
	template<class Rep, class Period>
	bool waitForMessage(const std::chrono::duration<Rep, Period>& rel_time){
		std::unique_lock<std::mutex> lck(m_mutex);
		return m_cv.wait_for(lck, rel_time, [this]{return !this->empty();});
	}

	inline MODULE getDestinationModule(uint32_t idx=0) {
		MODULE mod;
		if(idx < m_destModule.size()) {
			mod = m_destModule[idx];
		} else {
			mod = MODULE_UNKNOWN;
		}
		return mod;
	}

	virtual bool empty() const = 0;

protected:
	enum Queue{
		None	= 0,
		Low		= 1,
		High	= 2
	};

	/*
	 * Filter that decides if a messages belong to the high prior queue (return True) or low Priority queue (return False)
	 */
	Queue filter(const Message& m) const;

	void terminate_communicator();

	virtual void receiving(CommManager* comm) = 0;
	virtual void sending() = 0;

	bool									m_running;

	MODULE									m_ownModule;
	std::vector<MODULE>			m_destModule; // Destination module vector.
	std::map<MODULE, CommManagerThreads>	m_threads;
	std::thread								m_sending;

	mutable std::mutex						m_mutex;
	mutable std::condition_variable			m_cv;
};


/*
 * LayerCommunicator class will setup ZMQ and all queues necessary for the communication
 * @type Container: Container will be used as underlying container for all the queues. (By default std::deque<Message>)
 */
template<class ContainerLow = std::queue<Message>, class ContainerHigh = std::queue<Message>, class ContainerSending = std::queue<Message>>
class LayerCommunicator: public AbstractLayerCommunicator {
public:

	/*
	 * Constructor for the module
	 * @param module: This module
	 * @param connectTo: List of modules to connect to
	 */
	LayerCommunicator(MODULE module, std::vector<MODULE> connectTo);

	/*
	 * Destructor
	 */
	virtual ~LayerCommunicator();

	/*
	 * Get SafeQueue for all application messages
	 */
	inline SafeQueue::SafeQueue<Message, ContainerLow>& get_low_queue() {return m_low_receive;}

	/*
	 * Get SafeQueue for all CTRL messages
	 */
	inline SafeQueue::SafeQueue<Message, ContainerHigh>& get_high_queue() {return m_high_receive;}

	inline bool highPriorityPop(Message& m) { return get_high_queue().pop(m); }

	inline bool lowPriorityPop(Message& m) { return get_low_queue().pop(m); }

	/*
	 * Send a message by copying the data in the send queue
	 */
	inline void send(const Message& m) {m_send.push(m);}

	/*
	 * Send a message by moving the data in the send queue
	 */
	inline void send(Message&& m) {m_send.push((std::move(m)));}

	inline bool empty() const {return m_high_receive.empty() && m_low_receive.empty();}

protected:
	/*
	 * Receiving function (this will run in an own thread)
	 */
	void receiving(CommManager* comm);

	/*
	 * Sending function (this will run an own thread)
	 */
	void sending();

private:

	SafeQueue::SafeQueue<Message, ContainerSending>			m_send;
	SafeQueue::SafeQueue<Message, ContainerLow>				m_low_receive;
	SafeQueue::SafeQueue<Message, ContainerHigh>			m_high_receive;

};


//IMPLEMENTATION

template<class ContainerLow, class ContainerHigh, class ContainerSending>
LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>::LayerCommunicator(MODULE module, std::vector<MODULE> connectTo):
	AbstractLayerCommunicator(module)
{
	init_high_priority(connectTo);
	//init(connectTo);
}

template<class ContainerLow, class ContainerHigh, class ContainerSending>
LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>::~LayerCommunicator() {
	terminate_communicator();
}

template<class ContainerLow, class ContainerHigh, class ContainerSending>
void LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>::receiving(CommManager* comm){
	while(m_running){
		try{
			Message m;
			if(comm->receive(m, 1000)){
				m.calculatePriority();
				Queue q = filter(m);
				switch (q){
				case Low:
					Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Received message (low queue) from {0} with transaction index ({1},{2})", communicator::MODULE_Name(m.source), (int)m.message->owner_module(), m.message->transaction_index());
					m_low_receive.push(m);
					m_cv.notify_all();
					break;
				case High:
				{
					Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Received message (high queue) from {0} with transaction index ({1}, {2})", communicator::MODULE_Name(m.source), (int)m.message->owner_module(), m.message->transaction_index());
					m_high_receive.push(m);
					m_cv.notify_all();

#if(CHECK_LAYER_COMM_OUT_OF_SEQUENCE==1)
					static uint32_t data_cnt[2] = {0,0};
					uint32_t data_vector[2][32] = {{0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99, 128, 129, 130, 131, 160, 161, 162, 163, 192, 193, 194, 195, 224, 225, 226, 227},
					                               {4, 5, 6, 7, 36, 37, 38, 39, 68, 69, 70, 71, 100, 101, 102, 103, 132, 133, 134, 135, 164, 165, 166, 167, 196, 197, 198, 199, 228, 229, 230, 231}};

					// Cast protobuf message into a Internal message one.
					std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(m.message);

					unsigned char* data_str = (unsigned char*)internal->receiver().data().c_str();
					uint32_t vphy_id = internal->receiver().stat().vphy_id();
					if(vphy_id <= 1) {
						if(data_str[0] != data_vector[vphy_id][data_cnt[vphy_id]]) {
					  	std::cout << "[Layer comm] vPHY Id:" << vphy_id << " - Received: " << (int)data_str[0] << " - Expected: " << (int)data_vector[vphy_id][data_cnt[vphy_id]] << std::endl;
							exit(-1);
					 	}
					  data_cnt[vphy_id] = (data_cnt[vphy_id] + 1) % 32;
					}
#endif

					break;
				}
				default:
					Logger::log_warning<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Received message from {0} but not stored in queue.", communicator::MODULE_Name(comm->communicatorTo()));
					break;
			}
		}

		}catch(const zmq::error_t& ex){
			if(ex.num() != ETERM){
				Logger::log_error<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("zmq error ({0}): {1} ", ex.num(), ex.what());
				throw;
			}
		}
	}
	Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Stop receiving");
}

template<class ContainerLow, class ContainerHigh, class ContainerSending>
void LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>::sending(){
	while(m_running){
		Message m;
		Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Try to send a message");
		if(m_send.pop_wait_for(std::chrono::seconds(1), m)){
			Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Message in the queue");
			auto it = m_threads.find(m.destination);
			if(it != m_threads.end()){
				Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Send message to {0} with transaction index ({1}, {2})", communicator::MODULE_Name(m.destination), (int)m.message->owner_module(), m.message->transaction_index());
				it->second.commManager->send(m);

#if(CHECK_LAYER_COMM_OUT_OF_SEQUENCE==1)
				static uint32_t data_cnt[2] = {0,0};
				uint32_t data_vector[2][32] = {{0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99, 128, 129, 130, 131, 160, 161, 162, 163, 192, 193, 194, 195, 224, 225, 226, 227},
									                    {4, 5, 6, 7, 36, 37, 38, 39, 68, 69, 70, 71, 100, 101, 102, 103, 132, 133, 134, 135, 164, 165, 166, 167, 196, 197, 198, 199, 228, 229, 230, 231}};
				if(m.destination == communicator::MODULE::MODULE_MAC) {
					// Cast protobuf message into a Internal message one.
					std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(m.message);
					if(internal->payload_case() == communicator::Internal::kReceiver) {
						unsigned char* data_str = (unsigned char*)internal->receiver().data().c_str();
						uint32_t vphy_id = internal->receiver().stat().vphy_id();
						if(vphy_id <= 1) {
							if(data_str[0] != data_vector[vphy_id][data_cnt[vphy_id]]) {
								std::cout << "[Layer Comm. Sending] vPHY Id:" << vphy_id << " - Received: " << (int)data_str[0] << " - Expected: " << (int)data_vector[vphy_id][data_cnt[vphy_id]] << std::endl;
								exit(-1);
							}
							data_cnt[vphy_id] = (data_cnt[vphy_id] + 1) % 32;
						}
					}
				}
#endif

			}
		}
	}
	Logger::log_trace<LayerCommunicator<ContainerLow, ContainerHigh, ContainerSending>>("Stop sending");
}

typedef LayerCommunicator<>										DefaultLayerCommunicator;
typedef LayerCommunicator<PriorityQueue<Message,
		std::deque<Message>>,
		std::queue<Message>> 									PriorityLayerCommunicator;

}

#endif /* LAYERCOMMUNICATOR_H_ */
