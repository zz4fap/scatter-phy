/*
 * Maybe.h
 *
 *  Created on: 20 Nov 2017
 *      Author: rubenmennes
 */

#ifndef MAYBE_H_
#define MAYBE_H_

#include <utility>

template<class T>
class Maybe {
public:
	Maybe();
	Maybe(const T& value);
	Maybe(T&& value);
	Maybe(const Maybe<T>& other);
	Maybe(Maybe<T>&& other);

	Maybe<T>& operator=(const Maybe<T>& other);
	Maybe<T>& operator=(const T& value);
	Maybe<T>& operator=(Maybe<T>&& other);
	Maybe<T>& operator=(T&& value);

	bool operator==(const Maybe<T>& other) const;
	inline bool operator==(const T& value) const {return m_set && m_value == value;}

	void unset() {m_set = false;}
	inline bool isvalid() const { return m_set;}

	const T& value() const {return m_value;}
	const T* getValueIfSet() const;
	inline const T* operator()() const {return getValueIfSet();}

	operator bool() const {return m_set;}

private:
	bool	m_set;
	T		m_value;
};

//IMPLEMENTATION

template<class T>
Maybe<T>::Maybe(): m_set(false), m_value(){

}

template<class T>
Maybe<T>::Maybe(const T& value): m_set(true), m_value(value){

}

template<class T>
Maybe<T>::Maybe(T&& value): m_set(true), m_value(std::move(value)){

}

template<class T>
Maybe<T>::Maybe(const Maybe<T>& other): m_set(other.m_set), m_value(other.m_value){

}

template<class T>
Maybe<T>::Maybe(Maybe<T>&& other): m_set(std::move(other.m_set)), m_value(std::move(other.m_value)){

}

template<class T>
Maybe<T>& Maybe<T>::operator=(const Maybe<T>& other){
	m_set = other.m_set;
	m_value = other.m_value;
	return *this;
}

template<class T>
Maybe<T>& Maybe<T>::operator=(const T& value){
	m_set = true;
	m_value = value;
	return *this;
}

template<class T>
Maybe<T>& Maybe<T>::operator=(Maybe<T>&& other){
	m_set = std::move(other.m_set);
	m_value = std::move(other.m_value);
	return *this;
}

template<class T>
Maybe<T>& Maybe<T>::operator=(T&& other){
	m_set = true;
	m_value = std::move(other);
	return *this;
}

template<class T>
bool Maybe<T>::operator==(const Maybe<T>& other) const{
	if(m_set == other.m_set){
		if(m_set){
			return m_value == other.m_value;
		}else{
			return true;
		}
	}
	return false;
}

template<class T>
const T* Maybe<T>::getValueIfSet() const {
	if(!m_set){
		return nullptr;
	}else{
		return &m_value;
	}
}

#endif /* MAYBE_H_ */
