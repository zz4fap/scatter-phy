#ifndef _RSSI_MONITORING_H_
#define _RSSI_MONITORING_H_

#include "rf_monitor.h"

#define ENABLE_RSSI_MONITORING_PRINTS 1

#define RSSI_MONITORING_PRINT(_fmt, ...) do { if(ENABLE_RSSI_MONITORING_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[RSSI MONITORING PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define RSSI_MONITORING_DEBUG(_fmt, ...) do { if(ENABLE_RSSI_MONITORING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[RSSI MONITORING DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define RSSI_MONITORING_INFO(_fmt, ...) do { if(ENABLE_RSSI_MONITORING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[RSSI MONITORING INFO]: " _fmt, __VA_ARGS__); } while(0)

#define RSSI_MONITORING_ERROR(_fmt, ...) do { fprintf(stdout, "[RSSI MONITORING ERROR]: " _fmt, __VA_ARGS__); } while(0)

// ******************* Declaration of functions *******************
void rssi_monitoring_alarm_handle(int sig);

void *rssi_monitoring_work(void *h);

#endif //_RSSI_MONITORING_H_
