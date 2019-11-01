#ifndef _IQ_DUMPING_H_
#define _IQ_DUMPING_H_

#include "rf_monitor.h"

#define ENABLE_IQ_DUMPING_PRINTS 1

#define IQ_DUMPING_PRINT(_fmt, ...) do { if(ENABLE_IQ_DUMPING_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[IQ DUMPING PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMPING_DEBUG(_fmt, ...) do { if(ENABLE_IQ_DUMPING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[IQ DUMPING DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMPING_INFO(_fmt, ...) do { if(ENABLE_IQ_DUMPING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[IQ DUMPING INFO]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMPING_ERROR(_fmt, ...) do { fprintf(stdout, "[IQ DUMPING ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define DUMP_FILE_NAME "scatter_iq_dump_node_id_"

typedef enum {IQ_DUMPING_CHECK_FILE_EXIST_ST=0, IQ_DUMPING_WAIT_BEFORE_DUMP_ST=1, IQ_DUMPING_DUMP_SAMPLES_ST=2, IQ_DUMPING_RSSI_ST=3} iq_dumping_states_t;

// ********************** Declaration of function. **********************
void iq_dumping_alarm_handle(int sig);

char* iq_dumping_concat_timestamp_string(char *filename);

void *iq_dumping_work(void *h);

#endif // _IQ_DUMPING_H_
