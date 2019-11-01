/*
 * IPTOS.cpp
 *
 *  Created on: 11 Nov 2017
 *      Author: rubenmennes
 */

#include "IPTOS.h"

namespace communicator{

IPTOS::IPTOS() : m_priority(0), m_class(0){

}

IPTOS::IPTOS(uint8_t tos) {
	m_priority = tos >> 4;
	m_class = tos & 0xF;
}

IPTOS::IPTOS(uint8_t priority, uint8_t type): m_priority(priority), m_class(type){

}

IPTOS::~IPTOS() {

}

IPTOS::operator uint8_t() const{
	return ((uint8_t)m_priority << 4) | ((uint8_t)m_class & 0x0F);
}

PriorityValue IPTOS::calculatePriority() const{
	double pi_t = 1.0 + (double)m_priority/10;
	return pi_t * trafficclass_weight[m_class];
}


const PriorityValue nopriority = -(std::numeric_limits<double>::infinity());
const PriorityValue toppriority = std::numeric_limits<double>::infinity();

															//0 Leaky Bucket	1 VoIP	2 FTP	3 HTTP	4	5	6	7	8	9	A	B	C	D	E AI 	F CONTROL
const std::array<double, 16>IPTOS::trafficclass_weight = 	{1, 				7.5, 	1, 		2.5, 	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	1,		10};

}
