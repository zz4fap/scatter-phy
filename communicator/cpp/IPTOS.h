/*
 * IPTOS.h
 *
 *  Created on: 11 Nov 2017
 *      Author: rubenmennes
 */

#ifndef IPTOS_H_
#define IPTOS_H_
#include <limits.h>
#include <stdint.h>
#include <array>
#include "interf.pb.h"

namespace communicator{

typedef double PriorityValue;
extern const PriorityValue nopriority;
extern const PriorityValue toppriority;

class IPTOS {
public:
	IPTOS();
	IPTOS(uint8_t tos);
	IPTOS(uint8_t priority, uint8_t type);
	virtual ~IPTOS();

	operator uint8_t() const;

	PriorityValue calculatePriority() const;

	unsigned m_priority : 4;
	unsigned m_class	: 4;

	const static std::array<double, 16>	trafficclass_weight;

	static PriorityValue calculatePriority(uint8_t tos){ IPTOS e(tos); return e.calculatePriority();}
	static PriorityValue calculatePriority(uint8_t priority, uint8_t traffictype) { IPTOS e(priority, traffictype); return e.calculatePriority();}
	static PriorityValue calculatePriority(uint8_t priority, communicator::Application_data_TRAFFIC_CLASS& traffictype){
		return calculatePriority(priority, (uint8_t)traffictype);
	}
};

}

#endif /* IPTOS_H_ */
