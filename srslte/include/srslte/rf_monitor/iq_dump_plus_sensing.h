#ifndef _IQ_DUMP_PLUS_SENSING_H_
#define _IQ_DUMP_PLUS_SENSING_H_

#include "rf_monitor.h"

#define ENABLE_IQ_DUMP_PLUS_SENSING_PRINTS 1

#define SW_RF_MON_FFT_SIZE 512 // Number of FFT bins.

#define RF_MON_SENSING_PERIODICITY 0

#define IQ_DUMP_PLUS_SENSING_PRINT(_fmt, ...) do { if(ENABLE_IQ_DUMP_PLUS_SENSING_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[IQ DUMP + SENSING PRINT]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMP_PLUS_SENSING_DEBUG(_fmt, ...) do { if(ENABLE_IQ_DUMP_PLUS_SENSING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[IQ DUMP + SENSING DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMP_PLUS_SENSING_INFO(_fmt, ...) do { if(ENABLE_IQ_DUMP_PLUS_SENSING_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[IQ DUMP + SENSING INFO]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMP_PLUS_SENSING_ERROR(_fmt, ...) do { fprintf(stdout, "[IQ DUMP + SENSING ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define IQ_DUMP_PLUS_SENSING_FILE_NAME "scatter_iq_dump_plus_sensing_node_id_"

#define IQ_DUMP_PLUS_SENSING_TIMESTAMP_FILE_NAME "timestamp_scatter_iq_dump_plus_sensing_node_id_"

typedef enum {IQ_DUMP_PLUS_SENSING_CHECK_FILE_EXIST_ST=0, IQ_DUMP_PLUS_SENSING_WAIT_BEFORE_DUMP_ST=1, IQ_DUMP_PLUS_SENSING_DUMP_SAMPLES_ST=2, IQ_DUMP_PLUS_SENSING_RSSI_ST=3} iq_dump_plus_sensing_states_t;

// ********************** Declaration of function. **********************
void iq_dump_plus_sensing_alarm_handle(int sig);

char* iq_dump_plus_sensing_concat_timestamp_string(char *filename);

void *iq_dump_plus_sensing_work(void *h);

void iq_dump_plus_sensing_get_energy(cf_t *base_band_samples);

void iq_dump_plus_sensing_fopen(const char *filename, FILE **f);

void iq_dump_plus_sensing_fwrite(FILE **f, uint64_t timestamp);

void iq_dump_plus_sensing_fclose(FILE **f);

#endif // _IQ_DUMP_PLUS_SENSING_H_
