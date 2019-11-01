/*
 * CircularBuffer.h
 *
 *  Created on: 21 Nov 2017
 *      Author: rubenmennes
 */

#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include <stdint.h>
#include <array>
#include <initializer_list>
#include <stdexcept>

template<class T, size_t Size>
class CircularBuffer {
public:
	CircularBuffer(std::initializer_list<T> init = {});
	CircularBuffer(const CircularBuffer<T, Size>& other);
	CircularBuffer(CircularBuffer<T, Size>&& other);
	virtual ~CircularBuffer();

	CircularBuffer<T, Size>& operator=(const CircularBuffer<T, Size>& other);
	CircularBuffer<T, Size>& operator=(CircularBuffer<T, Size>&& other);
	CircularBuffer<T, Size>& operator=(std::initializer_list<T> init);

	T& at(size_t pos);
	const T& at(size_t pos) const;
	inline T& operator[](size_t pos) {return m_array[getPosition(pos)]; }
	inline const T& operator[](size_t pos) const {return m_array[getPosition(pos)]; };

	T& front();
	const T& front() const;
	T& back();
	const T& back() const;

	//TODO create iterators
	bool empty() const { return m_front == m_end; }
	size_t size() const;
	size_t max_size() const { return Size-1; }

	void clear();
	inline void push(const T& value) {push_back(value);}
	inline void push(T&& value) { push_back(std::move(value));}
	void push_back(const T& value);
	void push_back(T&& value);

	template<class... Args>
	inline void emplace(Args&&... args) {emplace_back(std::forward<Args>(args)...);}

	template<class... Args>
	void emplace_back(Args&&... args);

	void pop_front();
	inline void pop() {pop_front();}

private:
	inline size_t getPosition(size_t offset) const {return m_front + offset % Size;}
	size_t updateEnd();
	size_t updateFront();

	size_t 				m_front;
	size_t 				m_end;
	std::array<T, Size>	m_array;
};


template<class T, size_t Size>
CircularBuffer<T, Size>::CircularBuffer(std::initializer_list<T> init): m_front(0), m_end(0), m_array(){
	for(const T& e: init){
		push_back(e);
	}
}

template<class T, size_t Size>
CircularBuffer<T, Size>::CircularBuffer(const CircularBuffer<T, Size>& other): m_front(other.m_front), m_end(other.m_end), m_array(){
	for(size_t i = 0; i < Size; ++i){
		m_array[i] = other.m_array[i];
	}
}

template<class T, size_t Size>
CircularBuffer<T, Size>::CircularBuffer(CircularBuffer<T, Size>&& other): m_front(std::move(other.m_front)), m_end(std::move(other.m_end)), m_array(){
	m_array.swap(other.m_array);
}

template<class T, size_t Size>
CircularBuffer<T, Size>::~CircularBuffer(){

}
template<class T, size_t Size>
CircularBuffer<T, Size>& CircularBuffer<T, Size>::operator=(const CircularBuffer<T, Size>& other){
	m_front = other.m_front;
	m_end = other.m_end;
	for(size_t i = 0; i < Size; ++i){
		m_array[i] = other.m_array[i];
	}
}

template<class T, size_t Size>
CircularBuffer<T, Size>& CircularBuffer<T, Size>::operator=(CircularBuffer<T, Size>&& other){
	m_front = std::move(other.m_front);
	m_end = std::move(other.m_end);
	m_array.swap(other.m_array);
}

template<class T, size_t Size>
CircularBuffer<T, Size>& CircularBuffer<T, Size>::operator=(std::initializer_list<T> init){
	clear();
	for(const T& e: init){
		push_back(e);
	}

}

template<class T, size_t Size>
T& CircularBuffer<T, Size>::at(size_t pos){
	if(pos > size()){
		throw std::runtime_error("Out of array bounds");
	}
	return m_array[getPosition(pos)];
}

template<class T, size_t Size>
const T& CircularBuffer<T, Size>::at(size_t pos) const{
	if(pos > size()){
		throw std::runtime_error("Out of array bounds");
	}
	return m_array[getPosition(pos)];
}

template<class T, size_t Size>
T& CircularBuffer<T, Size>::front(){
	if(empty()){
		throw std::runtime_error("Try to access empty container");
	}
	return m_array[m_front];
}

template<class T, size_t Size>
const T& CircularBuffer<T, Size>::front() const{
	if(empty()){
		throw std::runtime_error("Try to access empty container");
	}
	return m_array[m_front];
}

template<class T, size_t Size>
T& CircularBuffer<T, Size>::back(){
	if(empty()){
		throw std::runtime_error("Try to access empty container");
	}
	return m_array[getPosition(size()-1)];
}

template<class T, size_t Size>
const T& CircularBuffer<T, Size>::back() const{
	if(empty()){
		throw std::runtime_error("Try to access empty container");
	}
	return m_array[getPosition(size()-1)];
}

template<class T, size_t Size>
size_t CircularBuffer<T, Size>::size() const {
	if(m_end < m_front){
		return m_end + Size - m_front;
	}
	return m_end - m_front;
}

template<class T, size_t Size>
void CircularBuffer<T, Size>::clear(){
	m_front = 0;
	m_end = 0;
}

template<class T, size_t Size>
void CircularBuffer<T, Size>::push_back(const T& value){
	m_array[updateEnd()] = value;
	if(m_end == m_front){
		updateFront();
	}
}

template<class T, size_t Size>
void CircularBuffer<T, Size>::push_back(T&& value){
	m_array[updateEnd()] = std::move(value);
	if(m_end == m_front){
		updateFront();
	}
}

template<class T, size_t Size>
template<class... Args>
void CircularBuffer<T, Size>::emplace_back(Args&&... args){
	m_array[updateEnd()] = T(std::forward<Args>(args)...);
	if(m_end == m_front){
		updateFront();
	}
}

template<class T, size_t Size>
void CircularBuffer<T, Size>::pop_front(){
	if(!(empty())){
		m_array[m_front] = T();				// It is possible that this line will break the code if T = a pointer. Just remove this line in that case.
		updateFront();
	}
}

template<class T, size_t Size>
size_t CircularBuffer<T, Size>::updateEnd(){
	size_t tmp = m_end;
	m_end = (m_end + 1)%Size;
	return tmp;
}

template<class T, size_t Size>
size_t CircularBuffer<T, Size>::updateFront(){
	size_t tmp = m_front;
	m_front = (m_front + 1)%Size;
	return tmp;
}

#endif /* CIRCULARBUFFER_H_ */
