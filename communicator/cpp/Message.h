/*
 * Message.h
 *
 *  Created on: 29 Mar 2017
 *      Author: rubenmennes
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "interf.pb.h"
#include "IPTOS.h"
#include <ostream>
#include <stdint.h>
#include "utils.h"

#ifdef SafeQueueTimeLog
#define SQ_NOW clock_get_time_ns()
#else
#define SQ_NOW 0
#endif

namespace communicator {

class TimeStamp{
public:
	TimeStamp(): m_timestamp(SQ_NOW) {};
	TimeStamp(uint64_t time) :m_timestamp(time) {};
	TimeStamp(const TimeStamp& other) : m_timestamp(other.m_timestamp) {};
	TimeStamp(TimeStamp&& other): m_timestamp(std::move(other.m_timestamp)) {};
	virtual ~TimeStamp() {};

	uint64_t getTimestamp() const {return m_timestamp;}
	void setTimestamp(uint64_t timestamp) const { m_timestamp = timestamp; }
	void setTimestamp() const { m_timestamp = SQ_NOW; }

	operator uint64_t() const { return m_timestamp; }

private:
	mutable uint64_t	m_timestamp;
};

template<class T>
class TimestampObject: public T, TimeStamp{
public:
	template<class ...Args>
	TimestampObject(Args&&... args);

	template<class ...Args>
	TimestampObject(uint64_t timestamp, Args&&... args);

	TimestampObject(const T& object);
	TimestampObject(uint64_t timestamp, const T& object);

	TimestampObject(T&& object);
	TimestampObject(uint64_t timestamp, T&& object);

	TimestampObject(const TimestampObject<T>& other);
	TimestampObject(TimestampObject<T>&& other);

	virtual ~TimestampObject() {};
};

template<class T>
template<class ...Args>
TimestampObject<T>::TimestampObject(Args&&... args): T(args...), TimeStamp(){

}

template<class T>
template<class ...Args>
TimestampObject<T>::TimestampObject(uint64_t timestamp, Args&&... args):
	T(args...), TimeStamp(timestamp)
{

}

template<class T>
TimestampObject<T>::TimestampObject(const T& object): T(object), TimeStamp(){

}

template<class T>
TimestampObject<T>::TimestampObject(uint64_t timestamp, const T& object): T(object), TimeStamp(timestamp){

}

template<class T>
TimestampObject<T>::TimestampObject(T&& object): T(std::move(object)), TimeStamp(){

}

template<class T>
TimestampObject<T>::TimestampObject(uint64_t timestamp, T&& object): T(std::move(object)), TimeStamp(timestamp){

}

template<class T>
TimestampObject<T>::TimestampObject(const TimestampObject<T>& other): T(other), TimeStamp(other){

}

template<class T>
TimestampObject<T>::TimestampObject(TimestampObject<T>&& other): T(std::move(other)), TimeStamp(std::move(other)){

}

class Message{
public:
	Message();
	Message(communicator::MODULE source, communicator::MODULE dest, std::shared_ptr<communicator::Internal>);
	Message(const Message& other);
	Message(Message&& other);
	virtual ~Message();

	Message& operator=(const Message& other);
	Message& operator=(Message&& other);

	bool operator<(const Message& other) const 		{ if(priority!=other.priority){return priority < other.priority;}else{return created > other.created;} }
	bool operator<=(const Message& other) const		{ if(priority!=other.priority){return priority <= other.priority;}else{return created >= other.created;} }
	bool operator>(const Message& other) const		{ if(priority!=other.priority){return priority > other.priority;}else{return created < other.created;} }
	bool operator>=(const Message& other) const		{ if(priority!=other.priority){return priority >= other.priority;}else{return created <= other.created;} }
	bool operator==(const Message& other) const 	{ return (priority == other.priority) && (created == other.created); }

	friend std::ostream& operator<<(std::ostream&, const Message&);

	double getPriority() const { return priority; }
	void calculatePriority() const;

	communicator::MODULE						source;
	communicator::MODULE						destination;
	std::shared_ptr<communicator::Internal>		message;

private:
	mutable PriorityValue						priority;
	uint64_t 									created;
};

typedef TimestampObject<Message> TimestampedMessage;

}

#endif /* MESSAGE_H_ */
