/*
 * PriorityQueue.h
 *
 *  Created on: 10 Nov 2017
 *      Author: rubenmennes
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

#include <queue>
#include <vector>
#include <ostream>
#include <string>
#include <sstream>

template<class T,
		class Container = std::vector<T>,
		class Compare = std::less<typename Container::value_type>
		>
class PriorityQueue: public std::priority_queue<T, Container, Compare>  {
public:
#ifdef NOCPP11
	explicit PriorityQueue(const Compare& compare = Compare(), const Container& cont = Container());
#endif
#ifndef NOCPP11
	PriorityQueue(const Compare& compare, const Container& cont);
	explicit PriorityQueue(const Compare& compare = Compare(), Container&& cont = Container());
#endif
	PriorityQueue(const std::priority_queue<T, Container, Compare>& other);
#ifndef NOCPP11
	PriorityQueue(std::priority_queue<T, Container, Compare>&& other);
#endif

	template<class Alloc>
	explicit PriorityQueue(const Alloc& alloc);

	template<class Alloc>
	PriorityQueue(const Compare& compare, const Alloc& alloc);

	template<class Alloc>
	PriorityQueue(const Compare& compare, const Container& cont, const Alloc& alloc);

#ifndef NOCPP11
	template<class Alloc>
	PriorityQueue(const Compare& compare, Container&& cont, const Alloc& alloc);
#endif

	template<class Alloc>
	PriorityQueue(const std::priority_queue<T, Container, Compare>& other, const Alloc& alloc);

#ifndef NOCPP11
	template<class Alloc>
	PriorityQueue(std::priority_queue<T, Container, Compare>&& other, const Alloc& alloc);
#endif

	template<class InputIt>
	PriorityQueue(InputIt first, InputIt last,
            const Compare& compare, const Container& cont);

#ifndef NOCPP11
	template<class InputIt>
	PriorityQueue(InputIt first, InputIt last,
            const Compare& compare = Compare(),
            Container&& cont = Container());
#endif

	virtual ~PriorityQueue() {};

	inline const T& front() const { return std::priority_queue<T, Container, Compare>::top(); }

	inline void push_back(const T& value) { std::priority_queue<T, Container, Compare>::push(value); }

#ifndef NOCPP11
	inline void push_back(T&& value) { std::priority_queue<T, Container, Compare>::push(std::move(value)); }
	template<class... Args>
	void emplace_back(Args&&... args);
#endif

	inline void pop_front() { std::priority_queue<T, Container, Compare>::pop(); }
	inline void pop_back() { std::priority_queue<T, Container, Compare>::c.pop_back() ;}

	friend std::ostream& operator<<(std::ostream& os, const PriorityQueue<T, Container, Compare>& queue){
		os << '[';
		bool first = true;
		for(typename Container::const_iterator it = queue.c.begin(); it != queue.c.end(); ++it){
			if(!first) os << ", ";
			os << *it;
		}
		os << ']';
		return os;
	}
};

template<class T, class Container, class Compare>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Compare& compare, const Container& cont):
	std::priority_queue<T, Container, Compare>(compare, cont){

}

#ifndef NOCPP11
template<class T, class Container, class Compare>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Compare& compare, Container&& cont):
	std::priority_queue<T, Container, Compare>(compare, std::move(cont))
{

}
#endif

template<class T, class Container, class Compare>
PriorityQueue<T, Container, Compare>::PriorityQueue(const std::priority_queue<T, Container, Compare>& other):
	std::priority_queue<T, Container, Compare>(other)
{

}

#ifndef NOCPP11
template<class T, class Container, class Compare>
PriorityQueue<T, Container, Compare>::PriorityQueue(std::priority_queue<T, Container, Compare>&& other):
	std::priority_queue<T, Container, Compare>(std::move(other))
{

}
#endif

template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Alloc& alloc):
	std::priority_queue<T, Container, Compare>(alloc)
{

}

template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Compare& compare, const Alloc& alloc):
std::priority_queue<T, Container, Compare>(compare, alloc){

}

template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Compare& compare, const Container& cont, const Alloc& alloc):
std::priority_queue<T, Container, Compare>(compare, cont, alloc){

}

#ifndef NOCPP11
template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(const Compare& compare, Container&& cont, const Alloc& alloc):
std::priority_queue<T, Container, Compare>(compare, std::move(cont), alloc)
{

}
#endif

template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(const std::priority_queue<T, Container, Compare>& other, const Alloc& alloc):
std::priority_queue<T, Container, Compare>(other, alloc)
{

}

#ifndef NOCPP11
template<class T, class Container, class Compare>
template<class Alloc>
PriorityQueue<T, Container, Compare>::PriorityQueue(std::priority_queue<T, Container, Compare>&& other, const Alloc& alloc):
std::priority_queue<T, Container, Compare>(std::move(other), alloc)
{

}
#endif

template<class T, class Container, class Compare>
template<class InputIt>
PriorityQueue<T, Container, Compare>::PriorityQueue(InputIt first, InputIt last,
            const Compare& compare, const Container& cont):
			std::priority_queue<T, Container, Compare>(first, last, compare, cont)
{

}

#ifndef NOCPP11
template<class T, class Container, class Compare>
template<class InputIt>
PriorityQueue<T, Container, Compare>::PriorityQueue(InputIt first, InputIt last,
            const Compare& compare,
            Container&& cont):
			std::priority_queue<T, Container, Compare>(first, last, compare, std::move(cont))
{

}
#endif

#ifndef NOCPP11
template<class T, class Container, class Compare>
template<class... Args>
void PriorityQueue<T, Container, Compare>::emplace_back(Args&&... args){
	T element(std::forward<Args>(args)...);
	push(element);
}
#endif

#endif /* PRIORITYQUEUE_H_ */
