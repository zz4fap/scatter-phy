#ifndef _RF_MONITOR_H_
#define _RF_MONITOR_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <fftw3.h>
#include <inttypes.h>

#include <uhd.h>

#include "srslte/config.h"
#include "srslte/rf/rf.h"
#include "srslte/utils/debug.h"
#include "srslte/common/timestamp.h"
#include "srslte/utils/vector.h"
#include "srslte/io/filesink.h"
#include "srslte/intf/intf.h"

#include "../../../examples/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#include "rf_monitor_types.h"
#include "lbt.h"
#include "iq_dumping.h"
#include "rssi_monitoring.h"
#include "spectrum_sensing_alg.h"
#include "iq_dump_plus_sensing.h"

#define ENABLE_RF_MONITOR_PRINTS 1

#define RF_MONITOR_PRINT(_fmt, args...) do { if(ENABLE_RF_MONITOR_PRINTS && scatter_verbose_level >= 0) \
  fprintf(stdout, "[RF MONITOR PRINT]: " _fmt, args); } while(0)

#define RF_MONITOR_DEBUG(_fmt, args...) do { if(ENABLE_RF_MONITOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[RF MONITOR DEBUG]: " _fmt, args); } while(0)

#define RF_MONITOR_INFO(_fmt, args...) do { if(ENABLE_RF_MONITOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[RF MONITOR INFO]: " _fmt, args); } while(0)

#define RF_MONITOR_ERROR(_fmt, args...) do { fprintf(stdout, "[RF MONITOR ERROR]: " _fmt, args); } while(0)

#define RF_MONITOR_PRINT_SENSING_STATS(phy_stat) do { if(HELPERS_DEBUG_MODE) \
  rf_monitor_print_sensing_statistics(phy_stat); } while(0)

// variable used to stop rf monitor thread. It was defined in rf_monitor.c
extern volatile sig_atomic_t run_rf_monitor_thread;

// handle used to send/receive messages from/to AI.
extern LayerCommunicator_handle rf_monitor_comm_handle;

// RF Monitor modules.
typedef enum {IQ_DUMP_MODULE=0, LBT_MODULE=1, RSSI_MODULE=2, SPECTRUM_SENSING_MODULE=3, IQ_DUMP_PLUS_SENSING_MODULE=4, UNKNOWN_MODULE=100} rf_monitor_modules_e;

// ********************** Declaration of function. **********************
SRSLTE_API int rf_monitor_initialize(srslte_rf_t *rf, transceiver_args_t *prog_args);

SRSLTE_API int rf_monitor_uninitialize();

SRSLTE_API void rf_monitor_print_handle();

SRSLTE_API void rf_monitor_send_sensing_statistics(uint64_t seq_number, uint32_t status, time_t full_secs, double frac_secs, float frequency, float sample_rate, float gain, float rssi, int32_t data_length, uchar *data);

SRSLTE_API void rf_monitor_change_rx_sample_rate(double sample_rate);

SRSLTE_API void rf_monitor_change_rx_gain(double sensing_rx_gain);

SRSLTE_API void rf_monitor_change_central_frequency(double central_frequency);

SRSLTE_API void rf_monitor_set_current_channel_to_monitor(uint32_t channel_to_monitor);

SRSLTE_API uint32_t rf_monitor_get_current_channel_to_monitor();

void rf_monitor_print_sensing_statistics(phy_stat_t *phy_stat);

#endif // _RF_MONITOR_H_
