/*
 * SafeQueue.h
 *
 *  Created on: 28 Mar 2017
 *      Author: rmennes
 */

#ifndef SAFEQUEUE_H_
#define SAFEQUEUE_H_

#define SafeQueueTimeLog

#include <queue>
#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <stdint.h>
#include <ostream>
#include "logging/Logger.h"
#include "Message.h"
#include "PriorityQueue.h"

namespace SafeQueue{

/*
 * SafeQueue is a tread safe queue. It can be used for all kind of types with all kind of containers.
 * @type T: Type of an element in the queue
 * @type Container: Type of the underlying container (default std::deque<T>). Container needs to have the following methods:
 * 		- bool empty() const
 * 		- size_t size() const
 * 		- T front() const;
 * 		- T back() const;
 * 		- void push(T);
 * 		- void pop();
 * 		- void swap(Container)
 */
template<class T, class Container = std::queue<T, std::deque<T>>>
class SafeQueue{
public:
	/*
	 * Create a SafeQueue with the elements of the existing container (copy the data)
	 * @param cnt: Original container
	 */
	explicit SafeQueue(const Container& cnt);

	/*
	 * Create a SafeQueue with the elements of the existing container (moving the data)
	 * @param cnt: Original container
	 */
	explicit SafeQueue(Container&& cnt = Container());

	/*
	 * Create a empty SafeQueue with specific Alloc
	 */
	template <class Alloc>
	explicit SafeQueue(const Alloc& alloc);

	/*
	 * Create a SafeQueue with th data of an existing container (copy the data) and a specific Alloc
	 */
	template <class Alloc>
	SafeQueue(const Container& ctnr, const Alloc& alloc);

	/*
	 * Create a SafeQueue with th data of an existing container (move the data) and a specific Alloc
	 */
	template <class Alloc>
	SafeQueue(Container&& ctnr, const Alloc& alloc);

	//DESTRUCTOR
	virtual ~SafeQueue();

	/*
	 * Thread safe empty()
	 * @return True if queue is empty, False otherwise
	 */
	bool empty() const;

	/*
	 * Thread safe size()
	 * @return get the size of the queue
	 */
	size_t size() const;

	/*
	 * Get the first element of the queue, if queue is empty, sleep until an other thread pushed somthing in the queue.
	 */
	T front_wait() const;

	/*
	 * Get the first element of the queue, if queue is empty sleep until an other thread pushed something in the queue or maximum rel_time
	 * @param rel_time: time you want to wait
	 * @param front_element: the element at the front of the queue if there was an element
	 * @return True if there was an element at the front of the queue else if we wait rel_time without having an element in the queue
	 */
	template<class Rep, class Period>
	bool front_wait_for(const std::chrono::duration<Rep, Period>& rel_time, T& front_element) const;

	/*
	 * Get the first element of queue if there was an element in the queue (without waiting)
	 * @param front_element: the frist element of the queue
	 * @return True if there was an element at the queue, False otherwise
	 */
	bool front(T& front_element) const;

	/*
	 * Get the last element of the queue, if queue is empty, sleep until an other thread pushed somthing in the queue.
	 */
	T back_wait() const;

	/*
	 * Get the last element of the queue, if queue is empty sleep until an other thread pushed something in the queue or maximum rel_time
	 * @param rel_time: time you want to wait
	 * @param back_element: the element at the back of the queue if there was an element
	 * @return True if there was an element in the queue else if we wait rel_time without having an element in the queue
	 */
	template<class Rep, class Period>
	bool back_wait_for(const std::chrono::duration<Rep, Period>& rel_time, T& back_element) const;

	/*
	 * Get the back element of queue if there was an element in the queue (without waiting)
	 * @param back_element: the last element of the queue
	 * @return True if there was an element at the queue, False otherwise
	 */
	bool back(T& back_element) const;

	/*
	 * Push a new element at the back of the queue by copying the value
	 */
	void push(const T& value);

	/*
	 * Push a new element at the back of the queue by moving it (will use the move constructor)
	 */
	void push(T&& value);

	/*
	 * Emplace a new element at the back of the queue
	 */
	template<class ...Args>
	void emplace(Args&&... args);

	/*
	 * Pop the first element of the queue if queue is not empty
	 * @return True if queue was not empty (before the pop), False otherwise
	 */
	bool pop();

	/*
	 * Pop the first element of the queue if queue is not empty
	 * @param time: if return value was true param time is the amount of time the element spend in the queue
	 * @return True if queue was not empty (berfor the pop), False otherwise
	 */
	bool pop(uint64_t& time);

	/*
	 * Pop the first element of the queue
	 * @param front_element: The first element of the queue if the queue was not empty
	 * @return True if queue was not empty (before the pop), False otherwise
	 */
	bool pop(T& front_element);

	/*
	 * Pop the first element of the queue
	 * @param front_element: The first element of the queue if the queue was not empty
	 * @param time: if return value was true param time is the amount of time the element spend in the queue
	 * @return True if queue was not empty (before the pop), False otherwise
	 */
	bool pop(T& front_element, uint64_t& time);

	/*
	 * Pop the first element of the queue if exist. If queue is empty wait until someone push a new element to the queue
	 * @return the first element of the queue
	 */
	T pop_wait();

	/*
	 * Pop the first element of the queue if exist. If queue is empty wait until someone push a new element to the queue
	 * @param time: amount of time the element spend in the queue
	 * @return the first element of the queue
	 */
	T pop_wait(uint64_t& time);


	/*
	 * Pop the first element of the queue if exist. If queue is empty wait until someone push a new element to the queue or timeout by ref_time
	 * @param ref_time time to wait maximum
	 * @param front_element: The element that WAS in front of the queue
	 * @return: True if we poped an element from the queue false if timeout
	 */
	template<class Rep, class Period>
	bool pop_wait_for(const std::chrono::duration<Rep, Period>& rel_time, T& front_element);

	/*
	 * Pop the first element of the queue if exist. If queue is empty wait until someone push a new element to the queue or timeout by ref_time
	 * @param ref_time time to wait maximum
	 * @param front_element: The element that WAS in front of the queue
	 * @param time: amount of time the element spend in the queue
	 * @return: True if we poped an element from the queue false if timeout
	 */
	template<class Rep, class Period>
	bool pop_wait_for(const std::chrono::duration<Rep, Period>& rel_time, T& front_element, uint64_t& time);

	/*
	 * Swap 2 queues
	 * Sliding window can not be swapped!
	 */
	void swap(SafeQueue<T, Container>& other);

private:
	Container										m_queue;
	mutable std::mutex								m_mutex;
	std::condition_variable							m_cv;
};


//IMPLEMENTATION
template<class T, class Container>
SafeQueue<T, Container>::SafeQueue(const Container& cnt): m_queue(cnt), m_mutex(), m_cv(){

}

template<class T, class Container>
SafeQueue<T, Container>::SafeQueue(Container&& cnt): m_queue(std::move(cnt)), m_mutex(), m_cv(){

}

template<class T, class Container>
template<class Alloc>
SafeQueue<T, Container>::SafeQueue(const Alloc& alloc): m_queue(alloc), m_mutex(), m_cv(){

}

template<class T, class Container>
template<class Alloc>
SafeQueue<T, Container>::SafeQueue(const Container& ctnr, const Alloc& alloc): m_queue(ctnr, alloc),
	m_mutex(), m_cv(){

}

template<class T, class Container>
template<class Alloc>
SafeQueue<T, Container>::SafeQueue(Container&& ctnr, const Alloc& alloc): m_queue(std::move(ctnr), alloc),
	m_mutex(), m_cv(){

}

template<class T, class Container>
SafeQueue<T, Container>::~SafeQueue(){

}

template<class T, class Container>
bool SafeQueue<T, Container>::empty() const{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_queue.empty();
}

template<class T, class Container>
size_t SafeQueue<T, Container>::size() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_queue.size();
}

template<class T, class Container>
T SafeQueue<T, Container>::front_wait() const {
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		m_cv.wait(lock, [this]{return !this->m_queue.empty();});
	}
	 m_queue.front();
}

template<class T, class Container>
template<class Rep, class Period>
bool SafeQueue<T, Container>::front_wait_for(const std::chrono::duration<Rep, Period>& period, T& front_element) const{
	std::unique_lock<std::mutex> lock(m_mutex);
	lock.lock();
	if(m_queue.empty()){
		bool pred = m_cv.wait_for(lock, period, [this]{return !this->m_queue.empty();});
		if(!pred) return false;
	}
	front_element = m_queue.front();
	return true;
}

template<class T, class Container>
bool SafeQueue<T, Container>::front(T& front_element) const{
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	else{
		front_element = m_queue.front();
		return true;
	}
}

template<class T, class Container>
T SafeQueue<T, Container>::back_wait() const {
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		m_cv.wait(lock, [this]{return !this->m_queue.empty();});
	}
	return m_queue.back();
}

template<class T, class Container>
template<class Rep, class Period>
bool SafeQueue<T, Container>::back_wait_for(const std::chrono::duration<Rep, Period>& period, T& front_element) const{
	std::unique_lock<std::mutex> lock(m_mutex);
	lock.lock();
	if(m_queue.empty()){
		bool pred = m_cv.wait_for(lock, period, [this]{return !this->m_queue.empty();});
		if(!pred) return false;
	}
	front_element = m_queue.back();
	return true;
}


template<class T, class Container>
bool SafeQueue<T, Container>::back(T& front_element) const{
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	else{
		front_element = m_queue.back();
		return true;
	}
}

template<class T, class Container>
void SafeQueue<T, Container>::push(const T& value){
	std::lock_guard<std::mutex> lock(m_mutex);
	m_queue.push(value);
	m_cv.notify_one();
}

template<class T, class Container>
void SafeQueue<T, Container>::push(T&& value){
	std::lock_guard<std::mutex> lock(m_mutex);
	m_queue.push(std::move(value));
	m_cv.notify_one();
}

template<class T, class Container>
template<class ...Args>
void SafeQueue<T, Container>::emplace(Args&&... args){
	std::lock_guard<std::mutex> lock(m_mutex);
	m_queue.emplace(args...);
	m_cv.notify_one();
}

template<class T, class Container>
bool SafeQueue<T, Container>::pop(){
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	T& element = m_queue.front();
	m_queue.pop();
	return true;
}

template<class T, class Container>
bool SafeQueue<T, Container>::pop(uint64_t& time){
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	T& element = m_queue.front();
	m_queue.pop();
	return true;
}

template<class T, class Container>
bool SafeQueue<T, Container>::pop(T& front_element){
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	const T& element = m_queue.front();
	front_element = element;
	m_queue.pop();
	return true;
}

template<class T, class Container>
bool SafeQueue<T, Container>::pop(T& front_element, uint64_t& time){
	std::lock_guard<std::mutex> lock(m_mutex);
	if(m_queue.empty()) return false;
	const T& element = m_queue.front();
	front_element = element;
	m_queue.pop();
	return true;
}

template<class T, class Container>
T SafeQueue<T, Container>::pop_wait(){
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		m_cv.wait(lock, [this]{return !this->m_queue.empty();});
	}
	const T& result = m_queue.front();
	m_queue.pop();
	return result;
}

template<class T, class Container>
T SafeQueue<T, Container>::pop_wait(uint64_t& time){
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		m_cv.wait(lock, [this]{return !this->m_queue.empty();});
	}
	const T& result = m_queue.front();
	m_queue.pop();
	return result;
}

template<class T, class Container>
template<class Rep, class Period>
bool SafeQueue<T, Container>::pop_wait_for(const std::chrono::duration<Rep, Period>& period, T& pop_element){
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		bool pred = m_cv.wait_for(lock, period, [this]{return !this->m_queue.empty();});
		if(!pred) return false;
	}
	const T& result = m_queue.front();
	pop_element = result;
	m_queue.pop();
	return true;
}

template<class T, class Container>
template<class Rep, class Period>
bool SafeQueue<T, Container>::pop_wait_for(const std::chrono::duration<Rep, Period>& period, T& pop_element, uint64_t& time){
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_queue.empty()){
		bool pred = m_cv.wait_for(lock, period, [this]{return !this->m_queue.empty();});
		if(!pred) return false;
	}
	const T& result = m_queue.front();
	pop_element = result;
	m_queue.pop();
	return true;
}


template<class T, class Container>
void SafeQueue<T, Container>::swap(SafeQueue<T, Container>& other){
	std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
	std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
	std::lock(lock1, lock2);
	m_queue.swap(other.m_queue);
}

} //End nanespace
#endif
