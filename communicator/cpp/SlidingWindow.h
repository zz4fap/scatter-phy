/*
 * SlidingWindow.h
 *
 *  Created on: 01 Jun 2017
 *      Author: rubenmennes
 */

#ifndef SLIDINGWINDOW_H_
#define SLIDINGWINDOW_H_

#include <algorithm>

template<class E, size_t size = 1024>
class SlidingWindow {
public:
	class iterator{
		public:
			typedef iterator self_type;
			typedef E 	value_type;
			typedef E&	reference_type;
			typedef E*	pointer_type;
			typedef std::random_access_iterator_tag iterator_category;

			explicit iterator(SlidingWindow<E, size>&, unsigned int start_pos);
			iterator(const iterator& other);
			virtual ~iterator();

			self_type& operator=(const iterator& other);
			self_type& operator++();
			self_type operator++(int);
			self_type& operator--();
			self_type operator--(int);

			self_type operator+(unsigned int) const;
			self_type operator-(unsigned int) const;

			self_type& operator+=(unsigned int);
			self_type& operator-=(unsigned int);

			bool operator<(const iterator& other) const;
			bool operator<=(const iterator& other) const;
			bool operator>(const iterator& other) const;
			bool operator>=(const iterator& other) const;

			reference_type operator[](unsigned int i) {return window[window.elementPosition(i)];}

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;

			inline reference_type operator*() {return window[window.elementPosition(position)];}
			pointer_type operator->() {return &window[window.elementPosition(position)];}

		private:
			unsigned int relativePosition(long i) const {return (unsigned int)std::min((long)0, std::max((long)size, (long)position + i));}
			SlidingWindow<E, size>& window;
			unsigned int			position;
	};

	class const_iterator{
		public:
			typedef iterator self_type;
			typedef E 	value_type;
			typedef const E&	reference_type;
			typedef const E*	pointer_type;
			typedef std::random_access_iterator_tag iterator_category;

			explicit const_iterator(SlidingWindow<E, size>&, unsigned int start_pos);
			const_iterator(const iterator& other);
			virtual ~const_iterator();

			self_type& operator=(const iterator& other);
			self_type& operator++();
			self_type operator++(int);
			self_type& operator--();
			self_type operator--(int);

			self_type operator+(unsigned int) const;
			self_type operator-(unsigned int) const;

			self_type& operator+=(unsigned int);
			self_type& operator-=(unsigned int);

			bool operator<(const const_iterator& other) const;
			bool operator<=(const const_iterator& other) const;
			bool operator>(const const_iterator& other) const;
			bool operator>=(const const_iterator& other) const;

			reference_type operator[](unsigned int i) const {return window[window.elementPosition(i)];}

			bool operator==(const const_iterator& other) const;
			bool operator!=(const const_iterator& other) const;

			reference_type operator*() const {return window[window.elementPosition(position)];}
			pointer_type operator->() const {return &window[window.elementPosition(position)];}

		private:
			unsigned int relativePosition(long i) const {return (unsigned int)std::min((long)0, std::max((long)size, (long)position + i));}
			SlidingWindow<E, size>& window;
			unsigned int			position;
	};



	SlidingWindow();
	virtual ~SlidingWindow();

	void push(E e);
	void emplace(E&& e);

	//Get element i places back in time;
	inline const E& operator[](unsigned int i) const {return m_window[elementPosition(i)];}
	inline E& operator[](unsigned int i) {return m_window[elementPosition(i)];}

	inline const E& front() const {return m_window[firstElement()];}
	inline E& front() {return m_window[firstElement()];};
	inline const E& last() const {return m_window[m_i];};
	inline E& last() {return m_window[m_i];}

	inline size_t sizeWindow() const {return m_size;}
	inline size_t maxSize() const {return size;}

	inline iterator begin() {return iterator(*this, firstElement());}
	inline iterator end() 	{return iterator(*this, m_i);}

	const_iterator begin() const {return const_iterator(*this, firstElement());};
	inline const_iterator end() const {return const_iterator(*this, m_i);}

	inline E average() const {return m_sum/m_size;}

private:
	inline unsigned int elementPosition(unsigned int i) const {return (m_i+i)%size;}
	unsigned int firstElement() const;

	E							m_window[size];
	E							m_sum;
	unsigned int 				m_i;
	unsigned int				m_size;
};

template<class E, size_t size>
SlidingWindow<E, size>::SlidingWindow(): m_window(), m_i(0), m_size(0){
}

template<class E, size_t size>
SlidingWindow<E, size>::~SlidingWindow(){
}

template<class E, size_t size>
void SlidingWindow<E, size>::push(E e){
	m_i = (m_i + 1) % size;
	m_sum -= m_window[m_i] + e;
	m_window[m_i] = e;
	if(m_size < size) m_size++;
}

template<class E, size_t size>
void SlidingWindow<E, size>::emplace(E&& e){
	m_i = (m_i + 1) % size;
	m_sum -= m_window[m_i] + e;
	m_window[m_i] = E(std::move(e));
	if(m_size < size) m_size++;
}

template<class E, size_t size>
unsigned int SlidingWindow<E, size>::firstElement() const{
	return m_size < size ? (m_i - m_size) % size : (m_i + 1) % size;
}

template<class E, size_t size>
SlidingWindow<E, size>::iterator::iterator(SlidingWindow<E, size>& window, unsigned int start_pos): window(window), position(start_pos){

}

template<class E, size_t size>
SlidingWindow<E, size>::iterator::iterator(const iterator& other): window(other.window), position(other.position){

}

template<class E, size_t size>
SlidingWindow<E, size>::iterator::~iterator(){
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type& SlidingWindow<E, size>::iterator::operator=(const iterator& other){
	window = other.window;
	position = other.position;
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type& SlidingWindow<E, size>::iterator::operator++(){
	position = this->relativePosition(1);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type SlidingWindow<E, size>::iterator::operator++(int){
	iterator tmp(*this);
	position = this->relativePosition(1);
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type& SlidingWindow<E, size>::iterator::operator--(){
	position = this->relativePosition(-1);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type SlidingWindow<E, size>::iterator::operator--(int){
	iterator tmp(*this);
	position = this->relativePosition(-1);
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type SlidingWindow<E, size>::iterator::operator+(unsigned int i) const{
	iterator tmp(*this);
	tmp.position = this->relativePosition((long)(i));
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type SlidingWindow<E, size>::iterator::operator-(unsigned int i ) const{
	iterator tmp(*this);
	tmp.position -= this->relativePosition(-(long)(i));;
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type& SlidingWindow<E, size>::iterator::operator+=(unsigned int i){
	position += this-relativePosition(i);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::iterator::self_type& SlidingWindow<E, size>::iterator::operator-=(unsigned int i){
	position += this-relativePosition(-(long)i);
	return *this;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator<(const SlidingWindow<E, size>::iterator& other) const{
	if(position < window.m_i){
		if(other.position < window.m_i){
			return position < other.position;
		}
	}else{
		if(other.position < window.m_i){
			return true;
		}
	}
	return position < other.position;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator<=(const SlidingWindow<E, size>::iterator& other) const{
	if(position == other.position) return true;
	return *this < other;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator>(const SlidingWindow<E, size>::iterator& other) const{
	return !(*this <= other);
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator>=(const SlidingWindow<E, size>::iterator& other) const{
	return !(*this < other);
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator==(const SlidingWindow<E, size>::iterator& other) const{
	return position == other.position;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::iterator::operator!=(const SlidingWindow<E, size>::iterator& other) const{
	return position != other.position;
}



template<class E, size_t size>
SlidingWindow<E, size>::const_iterator::const_iterator(SlidingWindow<E, size>& window, unsigned int start_pos): window(window), position(start_pos){

}

template<class E, size_t size>
SlidingWindow<E, size>::const_iterator::const_iterator(const iterator& other): window(other.window), position(other.position){

}

template<class E, size_t size>
SlidingWindow<E, size>::const_iterator::~const_iterator(){
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type& SlidingWindow<E, size>::const_iterator::operator=(const iterator& other){
	window = other.window;
	position = other.position;
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type& SlidingWindow<E, size>::const_iterator::operator++(){
	position = this->relativePosition(1);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type SlidingWindow<E, size>::const_iterator::operator++(int){
	iterator tmp(*this);
	position = this->relativePosition(1);
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type& SlidingWindow<E, size>::const_iterator::operator--(){
	position = this->relativePosition(-1);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type SlidingWindow<E, size>::const_iterator::operator--(int){
	iterator tmp(*this);
	position = this->relativePosition(-1);
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type SlidingWindow<E, size>::const_iterator::operator+(unsigned int i) const{
	iterator tmp(*this);
	tmp.position = this->relativePosition((long)(i));
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type SlidingWindow<E, size>::const_iterator::operator-(unsigned int i ) const{
	iterator tmp(*this);
	tmp.position -= this->relativePosition(-(long)(i));;
	return tmp;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type& SlidingWindow<E, size>::const_iterator::operator+=(unsigned int i){
	position += this-relativePosition(i);
	return *this;
}

template<class E, size_t size>
typename SlidingWindow<E, size>::const_iterator::self_type& SlidingWindow<E, size>::const_iterator::operator-=(unsigned int i){
	position += this-relativePosition(-(long)i);
	return *this;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator<(const SlidingWindow<E, size>::const_iterator& other) const{
	if(position < window.m_i){
		if(other.position < window.m_i){
			return position < other.position;
		}
	}else{
		if(other.position < window.m_i){
			return true;
		}
	}
	return position < other.position;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator<=(const SlidingWindow<E, size>::const_iterator& other) const{
	if(position == other.position) return true;
	return *this < other;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator>(const SlidingWindow<E, size>::const_iterator& other) const{
	return !(*this <= other);
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator>=(const SlidingWindow<E, size>::const_iterator& other) const{
	return !(*this < other);
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator==(const SlidingWindow<E, size>::const_iterator& other) const{
	return position == other.position;
}

template<class E, size_t size>
bool SlidingWindow<E, size>::const_iterator::operator!=(const SlidingWindow<E, size>::const_iterator& other) const{
	return position != other.position;
}


#endif /* SLIDINGWINDOW_H_ */
