/*
 * Message.cpp
 *
 *  Created on: 29 Mar 2017
 *      Author: rubenmennes
 */

#include "Message.h"
#include <limits>
#include "utils.h"

using namespace std;

namespace communicator {
Message::Message():source(static_cast<communicator::MODULE>(0)), destination(static_cast<communicator::MODULE>(0)), message(nullptr),
		priority(nopriority), created(clock_get_time_ns()){

}

Message::Message(communicator::MODULE source, communicator::MODULE dest, std::shared_ptr<communicator::Internal> message):
		source(source), destination(dest), message(message), priority(nopriority), created(clock_get_time_ns()){
}

Message::Message(const Message& other): source(other.source), destination(other.destination), message(other.message), priority(other.priority), created(other.created){

}

Message::Message(Message&& other): source(std::move(other.source)), destination(std::move(other.destination)), message(std::move(other.message)),
		priority(std::move(other.priority)), created(std::move(other.created)){

}


Message::~Message(){

}

Message& Message::operator=(const Message& other){
	source = other.source;
	destination = other.destination;
	message = other.message;
	priority = other.priority;
	created = other.created;
	return *this;
}

Message& Message::operator=(Message&& other){
	source = std::move(other.source);
	destination = std::move(other.destination);
	message = std::move(other.message);
	priority = std::move(other.priority);
	created = std::move(other.created);
	return *this;
}

void Message::calculatePriority() const{
	if(message != nullptr){
		switch (message->payload_case()) {
			case communicator::Internal::kSend:
				{
					const communicator::Send& send = message->send();
					if(send.has_app_data()){
						const communicator::Application_data& app_data = send.app_data();
						if(app_data.packet_type() == Application_data::IPv4 && app_data.traffic_class() != Application_data::CONTROL){
							priority = IPTOS::calculatePriority(app_data.priority(), app_data.traffic_class());
							//priority = -(1.0 + (double)app_data.priority()/10.0);
						}else{
							priority = toppriority;
						}
					}
				}
				break;
			default:
				break;
		}
	}
}

ostream& operator<<(ostream& os, const Message& message){
	return os << '(' << communicator::MODULE_Name(message.source) << " => "
			<< communicator::MODULE_Name(message.destination) << " Priority = " << message.priority << ')';
			//<< '\n' << message.message->DebugString();
}

}
