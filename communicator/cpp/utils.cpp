/*
 * utils.cpp
 *
 *  Created on: 01 Jun 2017
 *      Author: rubenmennes
 */

#include "utils.h"
#include <logging/Logger.h>
#include <chrono>
#include <signal.h>
#include <cstddef>

class Utils {}; //For logging purposes only

uint64_t clock_get_time_ns()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;
}

uint32_t timestamp_to_delta(uint64_t start_time, uint64_t timestamp) {
    uint32_t delta = 0;

    if (timestamp > start_time && start_time > 0) {
        delta = (uint32_t) ((timestamp - start_time) / 1000000L);
    } else if (start_time == 0) {
        Logger::log_warning<Utils>("Not getting delta from timestamp: start is {}", start_time);
    } else {
        Logger::log_warning<Utils>("Not getting delta from timestamp: {} <= {}",
                                                 timestamp, start_time);
    }

    Logger::log_debug<Utils>("Got delta {} from timestamp {} with start timestamp {}",
                                           delta, timestamp, start_time);

    return delta;
}

uint64_t delta_to_timestamp(uint64_t start_time, uint32_t delta) {
    return (uint64_t) (start_time + (delta * 1000000L));

}

void setup_signal_catching(void (*handler)(int))
{
    struct sigaction action;
    action.sa_handler = handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
};

char* strtolower(char *str)
{
    unsigned char *p = (unsigned char *)str;

    while (*p) {
        *p = tolower((unsigned char)*p);
        p++;
    }

    return str;
};

loglevel parse_log_level(char* levelstring){
    //By default, log ERROR and critical only
    loglevel ll = loglevel::ERROR;
    if(levelstring != NULL){
        char* v = strtolower(levelstring);
        if(strcmp(v, "debug")==0){
            ll = loglevel::DEBUG;
        }else if(strcmp(v, "warning")==0){
            ll = loglevel::WARNING;
        }else if(strcmp(v, "info")==0){
            ll = loglevel::INFO;
        }else if(strcmp(v, "trace")==0){
            ll = loglevel::TRACE;
        }else if(strcmp(v, "critical")==0){
            ll = loglevel::CRITICAL;
        }else if(strcmp(v, "off")==0){
            ll = loglevel::OFF;
        }
    }
    return ll;
};
