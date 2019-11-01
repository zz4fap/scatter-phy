/*
 * utils.h
 *
 *  Created on: 01 Jun 2017
 *      Author: rubenmennes
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <logging/LogLevel.h>

uint64_t clock_get_time_ns();
uint32_t timestamp_to_delta(uint64_t start_time, uint64_t timestamp);
uint64_t delta_to_timestamp(uint64_t start_time, uint32_t delta);
void setup_signal_catching(void (*handler)(int));
char* strtolower(char *str);
loglevel parse_log_level(char* levelstring);

inline uint32_t get_short_mac(uint64_t mac_address){
	return mac_address & (uint64_t) 0xff;
}

#endif /* UTILS_H_ */
