/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "rf_uhd_imp.h"
#include "srslte/srslte.h"
#include "srslte/rf/rf.h"
#include "uhd_c_api.h"

#define MEASURE_TX_TIMESTAMP_DIFF 0

#define DEBUG_TIMED_COMMANDS 0

// This mutex is used to synchronize the access to the USRP resource when reading RSSI.
pthread_mutex_t rf_handler_mutex;
// This mutex is used to synchronize the access to TX and RX resources like frequency, gain and sample rate.
// We use only one mutex per radio as we have TX and RX parameters being modified in different threads.
pthread_mutex_t tx_rx_mutex[2];

void suppress_handler(const char *x) {
  // do nothing
}

#if SCATTER_DEBUG_MODE
static void log_overflow(rf_uhd_handler_t *h) {
  if(h->uhd_error_handler) {
    srslte_rf_error_t error;
    bzero(&error, sizeof(srslte_rf_error_t));
    error.type = SRSLTE_RF_ERROR_OVERFLOW;
    h->uhd_error_handler(error);
  }
}

static void log_late(rf_uhd_handler_t *h) {
  if(h->uhd_error_handler) {
    srslte_rf_error_t error;
    bzero(&error, sizeof(srslte_rf_error_t));
    error.type = SRSLTE_RF_ERROR_LATE;
    h->uhd_error_handler(error);
  }
}

static void log_underflow(rf_uhd_handler_t *h) {
  if(h->uhd_error_handler) {
    srslte_rf_error_t error;
    bzero(&error, sizeof(srslte_rf_error_t));
    error.type = SRSLTE_RF_ERROR_UNDERFLOW;
    h->uhd_error_handler(error);
  }
}
#endif

void msg_handler(const char *msg, rf_uhd_handler_t *h) {
  srslte_rf_error_t error;
  bzero(&error, sizeof(srslte_rf_error_t));

  if(0 == strcmp(msg, "O")) {
    error.type = SRSLTE_RF_ERROR_OVERFLOW;
  } else if(0 == strcmp(msg, "D")) {
    error.type = SRSLTE_RF_ERROR_OVERFLOW;
  } else if(0 == strcmp(msg, "U")) {
    error.type = SRSLTE_RF_ERROR_UNDERFLOW;
  } else if(0 == strcmp(msg, "L")) {
    error.type = SRSLTE_RF_ERROR_LATE;
  }
  if(h->uhd_error_handler) {
    h->uhd_error_handler(error);
  }
}

char* print_uhd_error(uhd_error error) {

  char* error_string = NULL;

  switch(error) {
    case UHD_ERROR_NONE:
      error_string = "UHD_ERROR_NONE";
      break;
    case UHD_ERROR_INVALID_DEVICE:
      error_string = "UHD_ERROR_INVALID_DEVICE";
      break;
    case UHD_ERROR_INDEX:
      error_string = "UHD_ERROR_INDEX";
      break;
    case UHD_ERROR_KEY:
      error_string = "UHD_ERROR_KEY";
      break;
    case UHD_ERROR_NOT_IMPLEMENTED:
      error_string = "UHD_ERROR_NOT_IMPLEMENTED";
      break;
    case UHD_ERROR_USB:
      error_string = "UHD_ERROR_USB";
      break;
    case UHD_ERROR_IO:
      error_string = "UHD_ERROR_IO";
      break;
    case UHD_ERROR_OS:
      error_string = "UHD_ERROR_OS";
      break;
    case UHD_ERROR_ASSERTION:
      error_string = "UHD_ERROR_ASSERTION";
      break;
    case UHD_ERROR_LOOKUP:
      error_string = "UHD_ERROR_LOOKUP";
      break;
    case UHD_ERROR_TYPE:
      error_string = "UHD_ERROR_TYPE";
      break;
    case UHD_ERROR_VALUE:
      error_string = "UHD_ERROR_VALUE";
      break;
    case UHD_ERROR_RUNTIME:
      error_string = "UHD_ERROR_RUNTIME";
      break;
    case UHD_ERROR_ENVIRONMENT:
      error_string = "UHD_ERROR_ENVIRONMENT";
      break;
    case UHD_ERROR_SYSTEM:
      error_string = "UHD_ERROR_SYSTEM";
      break;
    case UHD_ERROR_EXCEPT:
      error_string = "UHD_ERROR_EXCEPT";
      break;
    case UHD_ERROR_BOOSTEXCEPT:
      error_string = "UHD_ERROR_BOOSTEXCEPT";
      break;
    case UHD_ERROR_STDEXCEPT:
      error_string = "UHD_ERROR_STDEXCEPT";
      break;
    case UHD_ERROR_UNKNOWN:
      error_string = "UHD_ERROR_UNKNOWN";
      break;
    default:
      error_string = "Unknown error code.....";
  }
  return error_string;
}

void rf_uhd_suppress_stdout(void *h) {
  rf_uhd_register_msg_handler_c(suppress_handler);
}

void rf_uhd_register_error_handler(void *h, srslte_rf_error_handler_t new_handler) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  handler->uhd_error_handler = new_handler;
}

static bool find_string(uhd_string_vector_handle h, char *str) {
  char buff[128];
  size_t n;
  uhd_string_vector_size(h, &n);
  for(int i=0;i<n;i++) {
    uhd_string_vector_at(h, i, buff, 128);
    if(strstr(buff, str)) {
      return true;
    }
  }
  return false;
}

static bool isRxLocked(rf_uhd_handler_t *handler, char *sensor_name, uhd_sensor_value_handle *value_h, size_t channel) {
  bool val_out = false;

  if(sensor_name) {
    uhd_usrp_get_rx_sensor(handler->usrp, sensor_name, channel, value_h);
    uhd_sensor_value_to_bool(*value_h, &val_out);
  } else {
    usleep(500);
    val_out = true;
  }

  return val_out;
}

static bool isTxLocked(rf_uhd_handler_t *handler, char *sensor_name, uhd_sensor_value_handle *value_h, size_t channel) {
  bool val_out = false;

  if(sensor_name) {
    uhd_usrp_get_tx_sensor(handler->usrp, sensor_name, channel, value_h);
    uhd_sensor_value_to_bool(*value_h, &val_out);
  } else {
    usleep(500);
    val_out = true;
  }

  return val_out;
}

char* rf_uhd_devname(void* h) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  return handler->devname;
}

bool rf_uhd_rx_wait_lo_locked(void *h, size_t channel) {
  uhd_string_vector_handle mb_sensors;
  uhd_string_vector_handle rx_sensors;
  char *sensor_name;
  uhd_sensor_value_handle value_h;
  double report = 0.0;
  bool val;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_string_vector_make(&mb_sensors);
  uhd_string_vector_make(&rx_sensors);
  uhd_sensor_value_make_from_bool(&value_h, "", true, "True", "False");
  uhd_usrp_get_mboard_sensor_names(handler->usrp, 0, &mb_sensors);
  uhd_usrp_get_rx_sensor_names(handler->usrp, channel, &rx_sensors);

  if(find_string(rx_sensors, "lo_locked")) {
    sensor_name = "lo_locked";
  } else if(find_string(mb_sensors, "ref_locked")) {
    sensor_name = "ref_locked";
  } else {
    sensor_name = NULL;
  }

  while(!isRxLocked(handler, sensor_name, &value_h, channel) && report < 30.0) {
    report += 0.1;
    usleep(1000);
  }

  val = isRxLocked(handler, sensor_name, &value_h, channel);

  uhd_string_vector_free(&mb_sensors);
  uhd_string_vector_free(&rx_sensors);
  uhd_sensor_value_free(&value_h);

  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);

  return val;
}

bool rf_uhd_tx_wait_lo_locked(void *h, size_t channel) {
  uhd_string_vector_handle mb_sensors;
  uhd_string_vector_handle tx_sensors;
  char *sensor_name;
  uhd_sensor_value_handle value_h;
  double report = 0.0;
  bool val;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_string_vector_make(&mb_sensors);
  uhd_string_vector_make(&tx_sensors);
  uhd_sensor_value_make_from_bool(&value_h, "", true, "True", "False");
  uhd_usrp_get_mboard_sensor_names(handler->usrp, 0, &mb_sensors);
  uhd_usrp_get_tx_sensor_names(handler->usrp, channel, &tx_sensors);

  if(find_string(tx_sensors, "lo_locked")) {
    sensor_name = "lo_locked";
  } else if(find_string(mb_sensors, "ref_locked")) {
    sensor_name = "ref_locked";
  } else {
    sensor_name = NULL;
  }

  while(!isTxLocked(handler, sensor_name, &value_h, channel) && report < 30.0) {
    report += 0.1;
    usleep(1000);
  }

  val = isTxLocked(handler, sensor_name, &value_h, channel);

  uhd_string_vector_free(&mb_sensors);
  uhd_string_vector_free(&tx_sensors);
  uhd_sensor_value_free(&value_h);

  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);

  return val;
}

void rf_uhd_set_tx_cal(void *h, srslte_rf_cal_t *cal)
{

}

void rf_uhd_set_rx_cal(void *h, srslte_rf_cal_t *cal)
{

}

int rf_uhd_start_rx_stream(void *h, size_t channel)
{
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_stream_cmd_t stream_cmd = {
        .stream_mode = UHD_STREAM_MODE_START_CONTINUOUS,
        .stream_now = true
  };
  uhd_error error = uhd_rx_streamer_issue_stream_cmd(handler->channels[channel].rx_stream, &stream_cmd);
  return error;
}

int rf_uhd_stop_rx_stream(void *h, size_t channel) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_stream_cmd_t stream_cmd = {
        .stream_mode = UHD_STREAM_MODE_STOP_CONTINUOUS,
        .stream_now = true
  };
  uhd_error error = uhd_rx_streamer_issue_stream_cmd(handler->channels[channel].rx_stream, &stream_cmd);
  return error;
}

void rf_uhd_flush_buffer(void *h, size_t channel) {
  int n;
  cf_t tmp[1024];
  do {
    n = rf_uhd_recv_with_time(h, tmp, 1024, 0, NULL, NULL, channel);
  } while (n > 0);
}

bool rf_uhd_has_rssi(void *h, size_t channel) {
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&rf_handler_mutex);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  bool has_rssi = handler->channels[channel].has_rssi;
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&rf_handler_mutex);
  return has_rssi;
}

bool get_has_rssi(void *h, size_t channel) {
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&rf_handler_mutex);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_string_vector_handle rx_sensors;
  uhd_string_vector_make(&rx_sensors);
  uhd_usrp_get_rx_sensor_names(handler->usrp, channel, &rx_sensors);
  bool ret = find_string(rx_sensors, "rssi");
  uhd_string_vector_free(&rx_sensors);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&rf_handler_mutex);
  return ret;
}

float rf_uhd_get_rssi(void *h, size_t channel) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&rf_handler_mutex);
  double val_out = 0.0;
  if(handler->channels[channel].has_rssi) {
    uhd_usrp_get_rx_sensor(handler->usrp, "rssi", channel, &handler->channels[channel].rssi_value);
    uhd_sensor_value_to_realnum(handler->channels[channel].rssi_value, &val_out);
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&rf_handler_mutex);
  return val_out;
}

int rf_uhd_open(char *args, void **h) {
  if(h) {
    *h = NULL;

    rf_uhd_handler_t *handler = (rf_uhd_handler_t*) malloc(sizeof(rf_uhd_handler_t));
    if(!handler) {
      perror("malloc");
      return -1;
    }
    bzero(handler, sizeof(rf_uhd_handler_t));
    *h = handler;

    // Initialize mutexes.
    if(pthread_mutex_init(&rf_handler_mutex, NULL) != 0) {
      RF_UHD_ERROR("Mutex init failed.\n",0);
      return -1;
    }

    // Set priority to UHD threads
    uhd_set_thread_priority(uhd_default_thread_priority, true);

    /* Find available devices */
    uhd_string_vector_handle devices_str;
    uhd_string_vector_make(&devices_str);
    uhd_usrp_find("", &devices_str);

    char args2[512];

    handler->dynamic_rate = true;

    // Allow NULL parameter
    if(args == NULL) {
      args = "";
    }
    handler->devname = NULL;

    // Initialize handler
    handler->uhd_error_handler = NULL;

    /* If device type or name not given in args, choose a B200 */
    if(args[0]=='\0') {
      if(find_string(devices_str, "type=b200") && !strstr(args, "recv_frame_size")) {
        // If B200 is available, use it
        args = "type=b200";
        handler->devname = DEVNAME_B200;
      } else if(find_string(devices_str, "type=x300")) {
        // Else if X300 is available, set master clock rate now (with x310 it can't be changed later)
        args = "type=x300,master_clock_rate=184.32e6";// If no argument is passed we set master clock rate to 184.32 MHz in order to use standard LTE rates, i.e., carrier sepration of 15 KHz.
        //args = "type=x300,master_clock_rate=200e6"; // In case we need longer CP periods, we should set master clock rate to 200 MHz in order to be able to set the sampling rate to 5 MHz and have bigger CPs.
        handler->dynamic_rate = false;
        handler->devname = DEVNAME_X300;
      }
    } else {
      // If args is set and x300 type is specified, make sure master_clock_rate is defined
      if(strstr(args, "type=x300") && !strstr(args, "master_clock_rate")) {
        sprintf(args2, "%s,master_clock_rate=184.32e6",args);
        args = args2;
        handler->dynamic_rate = false;
        handler->devname = DEVNAME_X300;
      } else if(strstr(args, "type=b200")) {
        handler->devname = DEVNAME_B200;
      }
    }

    uhd_string_vector_free(&devices_str);

    /* Create UHD handler */
    printf("Opening USRP with args: %s\n", args);
    uhd_error error = uhd_usrp_make(&handler->usrp, args);
    if(error) {
      fprintf(stderr, "Error opening UHD: code %d\n", error);
      return -1;
    }

    if(!handler->devname) {
      char dev_str[1024];
      uhd_usrp_get_mboard_name(handler->usrp, 0, dev_str, 1024);
      if(strstr(dev_str, "B2") || strstr(dev_str, "B2")) {
        handler->devname = DEVNAME_B200;
      } else if(strstr(dev_str, "X3") || strstr(dev_str, "X3")) {
        handler->devname = DEVNAME_X300;
      }
    }
    if(!handler->devname) {
      handler->devname = "uhd_unknown";
    }

    // Set external clock reference
    if(strstr(args, "clock=external")) {
      uhd_usrp_set_clock_source(handler->usrp, "external", 0);
    }

    // ************* Initialize Channes 0 and 1. *************
    size_t channels[2] = {0,1};
    uhd_stream_args_t stream_args = {
      .cpu_format = "fc32",
      .otw_format = "sc16",
      .args = "",
      .channel_list = channels,
      .n_channels = 2
    };

    handler->num_of_channels = (strcmp(handler->devname,DEVNAME_X300) != 0) ? 1:2;

    RF_DEBUG("USRP number of channels: %d\n", handler->num_of_channels);

    // Create streams for channels 0 and 1, otherwise only one stream is created.
    for(size_t channel = 0; channel < handler->num_of_channels; channel++) {

      if(pthread_mutex_init(&tx_rx_mutex[channel], NULL) != 0) {
        RF_UHD_ERROR("RX Mutex init failed.\n",0);
        return -1;
      }

      handler->channels[channel].has_rssi = get_has_rssi(handler, channel);
      if(handler->channels[channel].has_rssi) {
        uhd_sensor_value_make_from_realnum(&handler->channels[channel].rssi_value, "rssi", 0, "dBm", "%f");
      }

      // Initialize RX and TX streamers.
      uhd_rx_streamer_make(&handler->channels[channel].rx_stream);
      stream_args.channel_list = &channels[channel];
      stream_args.n_channels = 1;
      error = uhd_usrp_get_rx_stream(handler->usrp, &stream_args, handler->channels[channel].rx_stream);
      if(error) {
        fprintf(stderr, "Error opening RX stream for channel %d with error code: %d\n",channel, error);
        return -1;
      }
      uhd_tx_streamer_make(&handler->channels[channel].tx_stream);
      error = uhd_usrp_get_tx_stream(handler->usrp, &stream_args, handler->channels[channel].tx_stream);
      if(error) {
        fprintf(stderr, "Error opening TX stream: %d\n", error);
        return -1;
      }

      uhd_rx_streamer_max_num_samps(handler->channels[channel].rx_stream, &handler->channels[channel].rx_nof_samples);
      uhd_tx_streamer_max_num_samps(handler->channels[channel].tx_stream, &handler->channels[channel].tx_nof_samples);

      uhd_meta_range_make(&handler->channels[channel].rx_gain_range);
      uhd_usrp_get_rx_gain_range(handler->usrp, "", channel, handler->channels[channel].rx_gain_range);

      // Make metadata objects for RX/TX.
      uhd_async_metadata_make(&handler->channels[channel].async_md);  // Make metadata for asychronous messages.
      uhd_rx_metadata_make(&handler->channels[channel].rx_md);
      uhd_rx_metadata_make(&handler->channels[channel].rx_md_first);
      uhd_tx_metadata_make(&handler->channels[channel].tx_md, false, 0, 0, false, false);
    }
    // Everything went well.
    return 0;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}

int rf_uhd_close(void *h) {
  // Stop RX stream for specified channel.
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  for(int channel = 0; channel < handler->num_of_channels; channel++) {
    rf_uhd_stop_rx_stream(h,channel);
    // Free an async metadata handle.
    uhd_async_metadata_free(&handler->channels[channel].async_md);
    uhd_rx_metadata_free(&handler->channels[channel].rx_md);
    uhd_rx_metadata_free(&handler->channels[channel].rx_md_first);
    uhd_tx_metadata_free(&handler->channels[channel].tx_md);
    uhd_tx_streamer_free(&handler->channels[channel].tx_stream);
    uhd_rx_streamer_free(&handler->channels[channel].rx_stream);
    // Destroy radio mutexes.
    pthread_mutex_destroy(&tx_rx_mutex[channel]);
  }
  uhd_usrp_free(&handler->usrp);
  free(handler);

  // Destroy mutexes.
  pthread_mutex_destroy(&rf_handler_mutex);
  /** Something else to close the USRP?? */
  return 0;
}

void rf_uhd_set_master_clock_rate(void *h, double rate) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  if(handler->dynamic_rate) {
    uhd_usrp_set_master_clock_rate(handler->usrp, rate, 0);
  }
}

bool rf_uhd_is_master_clock_dynamic(void *h) {
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  return handler->dynamic_rate;
}

double rf_uhd_set_rx_srate(void *h, double rate, size_t channel) {
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_rx_rate(handler->usrp, rate, channel);
  uhd_usrp_get_rx_rate(handler->usrp, channel, &rate);
  RF_DEBUG("Set RX sample rate of channel %d: %1.2f [MHz]\n",channel,rate/1e6);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return rate;
}

double rf_uhd_get_rx_srate(void *h, size_t channel) {
  double rate;
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_get_rx_rate(handler->usrp, channel, &rate);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return rate;
}

double rf_uhd_set_tx_srate(void *h, double rate, size_t channel) {
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_tx_rate(handler->usrp, rate, channel);
  uhd_usrp_get_tx_rate(handler->usrp, channel, &rate);
  handler->channels[channel].tx_rate = rate;
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return rate;
}

void rf_uhd_set_fir_taps(void *h, size_t nof_prb, size_t channel) {
#if(ENABLE_HW_RF_MONITOR==1)
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_fir_taps(handler->usrp, nof_prb, channel);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
#endif
}

double rf_uhd_set_rx_gain(void *h, double gain, size_t channel) {
  uhd_error error;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  RF_DEBUG("Rx Channel: %d - gain: %f\n", channel, gain);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  if((error = uhd_usrp_set_rx_gain(handler->usrp, gain, channel, "")) != UHD_ERROR_NONE) {
    printf("Error setting RX gain: %s...\n", print_uhd_error(error));
    return -1.0;
  }
  if((error = uhd_usrp_get_rx_gain(handler->usrp, channel, "", &gain)) != UHD_ERROR_NONE) {
    printf("Error getting RX gain: %s...\n", print_uhd_error(error));
    return -1.0;
  }
  RF_DEBUG("Set RX gain for channel %d: %1.2f dB\n", channel, gain);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return gain;
}

double rf_uhd_set_tx_gain(void *h, double gain, size_t channel) {
  uhd_error error;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_set_tx_gain(handler->usrp, gain, channel, "");
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting Tx gain: %f [dB]. Error code: %d................\n", gain, error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to rf_uhd_set_tx_gain: %s\n",error_out);
    return -1.0;
  }
  error = uhd_usrp_get_tx_gain(handler->usrp, channel, "", &gain);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting Tx gain: %f [dB]. Error code: %d................\n", gain, error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to rf_uhd_set_tx_gain: %s\n",error_out);
    return -1.0;
  }
  //RF_UHD_PRINT("Set Tx gain for channel %d: %1.2f dB\n", channel, gain);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return gain;
}

double rf_uhd_get_rx_gain(void *h, size_t channel) {
  double gain;
  uhd_error error;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_get_rx_gain(handler->usrp, channel, "", &gain);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error getting Rx gain. Error code: %d................\n", error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to rf_uhd_get_rx_gain: %s\n", error_out);
    return -1.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return gain;
}

double rf_uhd_get_tx_gain(void *h, size_t channel) {
  double gain;
  uhd_error error;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_get_tx_gain(handler->usrp, channel, "", &gain);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error getting Tx gain. Error code: %d................\n", error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to rf_uhd_get_tx_gain: %s\n",error_out);
    return -1.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return gain;
}

double rf_uhd_set_rx_freq2(void *h, double freq, double lo_off, size_t channel) {
  uhd_tune_request_t tune_request;
  uhd_tune_result_t tune_result;
  double freq_set;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  // Set comman tune request parameters.
  tune_request.target_freq = freq;
  tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
  tune_request.dsp_freq = 0.0;
  tune_request.args = "mode_n=integer";
  // If local oscillator offset is zero then default tune request is used.
  if(lo_off == 0.0) {
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
    tune_request.rf_freq = 0.0;
  } else { // Any value different than 0.0 means we want to give an offset in frequency.
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
    tune_request.rf_freq = freq + lo_off;
  }
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_rx_freq(handler->usrp, &tune_request, channel, &tune_result);
  uhd_usrp_get_rx_freq(handler->usrp, channel, &freq_set);
  //RF_UHD_PRINT("PHY ID: %d - Requested Rx freq.: %1.4f - Actual Rx Freq.: %1.4f\n",channel, (freq/1e6), (freq_set/1e6));
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq_set;
}

double rf_uhd_set_rx_freq(void *h, double freq, size_t channel) {
  uhd_tune_result_t tune_result;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  uhd_tune_request_t tune_request = {
      .target_freq = freq,
      .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
      .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
  };
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_rx_freq(handler->usrp, &tune_request, channel, &tune_result);
  uhd_usrp_get_rx_freq(handler->usrp, channel, &freq);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

double rf_uhd_set_tx_freq2(void *h, double freq, double lo_off, size_t channel) {
  uhd_error error;
  uhd_tune_request_t tune_request;
  uhd_tune_result_t tune_result;
  double freq_set;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  // Set common tune request parameters.
  tune_request.target_freq = freq;
  tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
  tune_request.dsp_freq = 0.0;
  tune_request.args = "mode_n=integer";
  // If local oscillator offset is zero then default tune request is used.
  if(lo_off == 0.0) {
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
    tune_request.rf_freq = 0.0;
  } else { // Any value different than 0.0 means we want to give an offset in frequency.
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
    tune_request.rf_freq = freq + lo_off;
  }
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_set_tx_freq(handler->usrp, &tune_request, channel, &tune_result);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
  }
  error = uhd_usrp_get_tx_freq(handler->usrp, channel, &freq_set);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error getting TX frequency. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_get_tx_freq: %s\n",error_out);
  }
  //RF_UHD_PRINT("PHY ID: %d - Requested Tx freq.: %1.4f - Actual Tx Freq.: %1.4f\n", channel, (freq/1e6), (freq_set/1e6));
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq_set;
}

double rf_uhd_set_tx_freq(void *h, double freq, size_t channel) {
  uhd_tune_result_t tune_result;
  uhd_error error;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  // Create tune request.
  uhd_tune_request_t tune_request = {
      .target_freq = freq,
      .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
      .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
  };
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_set_tx_freq(handler->usrp, &tune_request, channel, &tune_result);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
  }
  error = uhd_usrp_get_tx_freq(handler->usrp, channel, &freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error getting TX frequency. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_get_tx_freq: %s\n",error_out);
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

double rf_uhd_get_tx_freq(void *h, size_t channel) {
  double freq;
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_get_tx_freq(handler->usrp, channel, &freq);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

double rf_uhd_get_rx_freq(void *h, size_t channel) {
  double freq;
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_get_rx_freq(handler->usrp, channel, &freq);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

void rf_uhd_get_time(void *h, time_t *secs, double *frac_secs) {
  // Lock the mutexes prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[0]);
  pthread_mutex_lock(&tx_rx_mutex[1]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_get_time_now(handler->usrp, 0, secs, frac_secs);
  // Unlock the mutexes upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[1]);
  pthread_mutex_unlock(&tx_rx_mutex[0]);
}

int rf_uhd_recv_with_time(void *h,
                    void *data,
                    uint32_t nsamples,
                    bool blocking,
                    time_t *secs,
                    double *frac_secs,
                    size_t channel)
{

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  size_t rxd_samples = 0;
  uhd_rx_metadata_handle *md = &handler->channels[channel].rx_md_first;
  int trials = 0;
  if(blocking) {
    int n = 0;
    cf_t *data_c = (cf_t*) data;
    do {
      // Always read the maxium number of samples per packet per buffer allowed by this specific USRP, i.e., it is hardware dependent.
      size_t rx_samples = handler->channels[channel].rx_nof_samples;

      if (rx_samples > nsamples - n) {
        rx_samples = nsamples - n;
      }
      void *buff = (void*) &data_c[n];
      void **buffs_ptr = (void**) &buff;

      uhd_error error = uhd_rx_streamer_recv(handler->channels[channel].rx_stream, buffs_ptr, rx_samples, md, 1.0, false, &rxd_samples);

      if(error) {
        printf("Error receiving from UHD: %d\n", error);
        return -1;
      }
      md = &handler->channels[channel].rx_md;
      n += rxd_samples;
      trials++;

#if SCATTER_DEBUG_MODE
      uhd_rx_metadata_error_code_t error_code;
      uhd_rx_metadata_error_code(*md, &error_code);
      if (error_code == UHD_RX_METADATA_ERROR_CODE_OVERFLOW) {
        log_overflow(handler);
      } else if (error_code == UHD_RX_METADATA_ERROR_CODE_LATE_COMMAND) {
        log_late(handler);
      } else if (error_code != UHD_RX_METADATA_ERROR_CODE_NONE) {
        fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.\n", error_code);
      }
#endif

    } while (n < nsamples && trials < 100);
  } else {
    void **buffs_ptr = (void**) &data;
    return uhd_rx_streamer_recv(handler->channels[channel].rx_stream, buffs_ptr, nsamples, md, 0.0, false, &rxd_samples);
  }
  if(secs && frac_secs) {
    uhd_rx_metadata_time_spec(handler->channels[channel].rx_md_first, secs, frac_secs);
  }
  return nsamples;
}

int rf_uhd_recv_channel_with_time(void *h,
                    void *data,
                    uint32_t nsamples,
                    bool blocking,
                    time_t *secs,
                    double *frac_secs)
{
  rf_uhd_channel_handler_t *handler = (rf_uhd_channel_handler_t*) h;
  size_t rxd_samples = 0;
  uhd_rx_metadata_handle *md = &handler->rx_md_first;
  int trials = 0;
  if (blocking) {
    int n = 0;
    cf_t *data_c = (cf_t*) data;
    do {
      // Always read the maxium number of samples per packet per buffer allowed by this specific USRP, i.e., it is hardware dependent.
      size_t rx_samples = handler->rx_nof_samples;

      if (rx_samples > nsamples - n) {
        rx_samples = nsamples - n;
      }
      void *buff = (void*) &data_c[n];
      void **buffs_ptr = (void**) &buff;

      uhd_error error = uhd_rx_streamer_recv(handler->rx_stream, buffs_ptr, rx_samples, md, 1.0, false, &rxd_samples);

      if (error) {
        printf("Error receiving from UHD: %d\n", error);
        return -1;
      }
      md = &handler->rx_md;
      n += rxd_samples;
      trials++;

    } while (n < nsamples && trials < 100);
  } else {
    void **buffs_ptr = (void**) &data;
    return uhd_rx_streamer_recv(handler->rx_stream, buffs_ptr, nsamples, md, 0.0, false, &rxd_samples);
  }
  if (secs && frac_secs) {
    uhd_rx_metadata_time_spec(handler->rx_md_first, secs, frac_secs);
  }
  return nsamples;
}

int rf_uhd_send_timed(void *h,
                     void *data,
                     int nsamples,
                     time_t secs,
                     double frac_secs,
                     bool has_time_spec,
                     bool blocking,
                     bool is_start_of_burst,
                     bool is_end_of_burst,
                     bool is_lbt_enabled,
                     void *lbt_stats_void_ptr,
                     size_t channel)
{
  rf_uhd_handler_t* handler = (rf_uhd_handler_t*) h;
  size_t txd_samples;
#if(ENABLE_LBT_FEATURE==1)
  lbt_stats_t *lbt_stats = (lbt_stats_t*)lbt_stats_void_ptr;
#endif
  if(has_time_spec) {
    uhd_tx_metadata_set_time_spec(&handler->channels[channel].tx_md, secs, frac_secs);
  }
  int trials = 0;
  if(blocking) {
    int n = 0;
    cf_t *data_c = (cf_t*) data;
    do {
      size_t tx_samples = handler->channels[channel].tx_nof_samples;

      // First packet is start of burst if so defined, others are never
      if(n == 0) {
        uhd_tx_metadata_set_start(&handler->channels[channel].tx_md, is_start_of_burst);
      } else {
        uhd_tx_metadata_set_start(&handler->channels[channel].tx_md, false);
      }

      // middle packets are never end of burst, last one as defined
      if(nsamples - n > tx_samples) {
        uhd_tx_metadata_set_end(&handler->channels[channel].tx_md, false);
      } else {
        tx_samples = nsamples - n;
        uhd_tx_metadata_set_end(&handler->channels[channel].tx_md, is_end_of_burst);
      }

      void *buff = (void*) &data_c[n];
      const void **buffs_ptr = (const void**) &buff;

#if(ENABLE_LBT_FEATURE==1)
      // Check if the channel state is FREE or BUSY only once before we start transmitting even if we transmit more than 1 slot per time.
      // If the number of RF channels is greater than 1 then we listen before we talk. LBT is not performed if there is no sensing thread running, i.e., if there is only one channel as it is the case with b200 and b200mini.
      // The slot is split into small packets that are sent to the USRP, LBT is only applyed to the very fisrt packet, i.e., when n==0.
      if(is_lbt_enabled && handler->num_of_channels > 1 && is_start_of_burst && n == 0) {
        if(!lbt_execute_procedure(lbt_stats)) {
          RF_UHD_DEBUG("LBT timed-out, drop slot.\n",0);
          return RF_LBT_TIMEOUT_CODE;
        }
        RF_UHD_DEBUG("LBT done, transmit slot.\n",0);
      }
#endif

#if(MEASURE_TX_TIMESTAMP_DIFF==1)
      time_t fpga_full_secs;
      double fpga_frac_secs;
      // Print time info only for the very fisrt packet being transfererd to the USRP.
      if(is_start_of_burst && n == 0) {
        // Retrieve FPGA time.
        rf_uhd_get_time(handler, &fpga_full_secs, &fpga_frac_secs);
        // Convert time into timestamp format.
        uint64_t fpga_time_now = (fpga_full_secs*1000000 + fpga_frac_secs*1000000);
        uint64_t tx_timestamp = (secs*1000000 + frac_secs*1000000);
        RF_UHD_PRINT("PHY ID: %d - Time difference: %d\n", channel, (int)(tx_timestamp-fpga_time_now));
        // Calculate difference in miliseconds.
        //double time_diff = fpga_time_diff(fpga_full_secs, fpga_frac_secs, secs, frac_secs);
        //RF_UHD_PRINT("PHY ID: %d - Time difference: %f\n", channel, time_diff);
        //RF_UHD_PRINT("PHY ID: %d - Tx time: %" PRIu64 " - FPGA time: %" PRIu64 "\n", channel, tx_timestamp, fpga_time_now);
      }
#endif

      uhd_error error = uhd_tx_streamer_send(handler->channels[channel].tx_stream, buffs_ptr,
                                             tx_samples, &handler->channels[channel].tx_md, 1.0, &txd_samples);
      if(error) {
        RF_UHD_ERROR("Error sending to UHD: %d\n", error);
        return -1;
      }
      // Increase time spec.
      uhd_tx_metadata_add_time_spec(&handler->channels[channel].tx_md, txd_samples/handler->channels[channel].tx_rate);

      n += txd_samples;
      trials++;
    } while (n < nsamples && trials < 100);
    return nsamples;
  } else {
    const void **buffs_ptr = (const void**) &data;
    uhd_tx_metadata_set_start(&handler->channels[channel].tx_md, is_start_of_burst);
    uhd_tx_metadata_set_end(&handler->channels[channel].tx_md, is_end_of_burst);
    return uhd_tx_streamer_send(handler->channels[channel].tx_stream, buffs_ptr, nsamples, &handler->channels[channel].tx_md, 0.0, &txd_samples);
  }
}

int rf_uhd_check_async_msg(rf_uhd_handler_t* handler, size_t channel) {
  bool valid;
  uhd_error ret;
  int status = 0;
  uhd_async_metadata_event_code_t event_code_out;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  // Receive an asynchronous message from this streamer.
  if((ret = uhd_tx_streamer_recv_async_msg(handler->channels[channel].tx_stream, &handler->channels[channel].async_md, 0.01, &valid)) != UHD_ERROR_NONE) {
    printf("Error when waiting for async msg. Error code: %d\n",ret);
    status = -1;
  } else {
    if(valid) {
      uhd_async_metadata_event_code(handler->channels[channel].async_md, &event_code_out);
      if(event_code_out != UHD_ASYNC_METADATA_EVENT_CODE_BURST_ACK) {
        switch(event_code_out) {
          case UHD_ASYNC_METADATA_EVENT_CODE_TIME_ERROR:
            printf("Packet had time that was late.\n");
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_SEQ_ERROR :
            printf("Packet loss error between host and device.\n");
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_UNDERFLOW_IN_PACKET:
            printf("Underflow occurred inside a packet.\n");
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_UNDERFLOW :
            printf("An internal send buffer has emptied.\n");
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_SEQ_ERROR_IN_BURST:
            printf("Packet loss within a burst..\n");
            break;
          default:
            printf("Async message got unexpected event code 0x%x.\n", event_code_out);
            break;
        }
#if SCATTER_DEBUG_MODE
      } else {
        printf("A burst was successfully transmitted.\n");
#endif
      }
    } else {
      printf("Reception of async message timed out, wait a little bit more........\n");
    }
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return status;
}

bool rf_uhd_is_burst_transmitted(void *h, size_t channel) {
  bool valid, status = false;
  uhd_error ret;
  uhd_async_metadata_event_code_t event_code_out;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t* handler = (rf_uhd_handler_t*) h;
  // Receive an asynchronous message from this streamer.
  if((ret = uhd_tx_streamer_recv_async_msg(handler->channels[channel].tx_stream, &handler->channels[channel].async_md, 0.00001, &valid)) != UHD_ERROR_NONE) {
    RF_UHD_DEBUG("Error when waiting for async msg. Error code: %d\n",ret);
  } else {
    if(valid) {
      uhd_async_metadata_event_code(handler->channels[channel].async_md, &event_code_out);
      if(event_code_out != UHD_ASYNC_METADATA_EVENT_CODE_BURST_ACK) {
        switch(event_code_out) {
          case UHD_ASYNC_METADATA_EVENT_CODE_TIME_ERROR:
            RF_UHD_DEBUG("Packet had time that was late.\n",0);
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_SEQ_ERROR :
            RF_UHD_DEBUG("Packet loss error between host and device.\n",0);
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_UNDERFLOW_IN_PACKET:
            RF_UHD_DEBUG("Underflow occurred inside a packet.\n",0);
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_UNDERFLOW :
            RF_UHD_DEBUG("An internal send buffer has emptied.\n",0);
            break;
          case UHD_ASYNC_METADATA_EVENT_CODE_SEQ_ERROR_IN_BURST:
            RF_UHD_DEBUG("Packet loss within a burst..\n",0);
            break;
          default:
            RF_UHD_DEBUG("Async message got unexpected event code 0x%x.\n", event_code_out);
            break;
        }
      } else {
        status = true;
        RF_UHD_DEBUG("Burst successfully transmitted.\n",0);
      }
    } else {
      RF_UHD_DEBUG("Reception of async message timed out.\n",0);
    }
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return status;
}

void rf_uhd_set_time_now(void *h, time_t full_secs, double frac_secs) {
  // Lock the mutexes prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[0]);
  pthread_mutex_lock(&tx_rx_mutex[1]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_time_now(handler->usrp, full_secs, frac_secs, 0);
  // Unlock the mutexes upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[1]);
  pthread_mutex_unlock(&tx_rx_mutex[0]);
}

double rf_uhd_set_tx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  uhd_error error;
  uhd_tune_request_t tune_request;
  uhd_tune_result_t tune_result;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  if(lo_off >= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_cmd: Negative offset detected...\n",0);
    return -1.0;
  }

  // Set common tune request parameters.
  tune_request.target_freq = freq;
  tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
  tune_request.dsp_freq = 0.0;
  tune_request.args = "mode_n=integer";
  // If local oscillator offset is zero then default tune request is used.
  if(lo_off == 0.0) {
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
    tune_request.rf_freq = 0.0;
  } else { // Any value different than 0.0 means we want to give an offset in frequency.
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
    tune_request.rf_freq = freq + lo_off;
  }
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Schedule tx frequency change.
  error = uhd_usrp_set_tx_freq(handler->usrp, &tune_request, channel, &tune_result);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

double rf_uhd_set_tx_freq_and_gain_cmd(void *h, double freq, double lo_off, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  uhd_error error;
  uhd_tune_request_t tune_request;
  uhd_tune_result_t tune_result;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_and_gain_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_and_gain_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  if(gain < 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_and_gain_cmd: Negative gain detected...\n",0);
    return -1.0;
  }

  if(lo_off > 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_freq_and_gain_cmd: Negative offset detected...\n",0);
    return -1.0;
  }

  // Set common tune request parameters.
  tune_request.target_freq = freq;
  tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
  tune_request.dsp_freq = 0.0;
  tune_request.args = "mode_n=integer";
  // If local oscillator offset is zero then default tune request is used.
  if(lo_off == 0.0) {
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
    tune_request.rf_freq = 0.0;
  } else { // Any value different than 0.0 means we want to give an offset in frequency.
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
    tune_request.rf_freq = freq + lo_off;
  }
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Set Tx frequency.
  error = uhd_usrp_set_tx_freq(handler->usrp, &tune_request, channel, &tune_result);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
    return -10.0;
  }
  // Set Tx gain.
  error = uhd_usrp_set_tx_gain(handler->usrp, gain, channel, "");
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX gain: %f [dB]. Error code: %d................\n",gain,error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_gain: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

double rf_uhd_set_tx_gain_cmd(void *h, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  uhd_error error;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_tx_gain_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(gain < 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_gain_cmd: Negative gain detected...\n",0);
    return -1.0;
  }

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Set Tx gain.
  error = uhd_usrp_set_tx_gain(handler->usrp, gain, channel, "");
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX gain: %f [dB]. Error code: %d................\n",gain,error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_gain: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return gain;
}

double rf_uhd_set_rx_freq_cmd(void *h, double freq, double lo_off, uint64_t timestamp, uint64_t time_advance, size_t channel) {
  uhd_error error;
  uhd_tune_request_t tune_request;
  uhd_tune_result_t tune_result;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_rx_freq_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_rx_freq_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  if(lo_off <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_rx_freq_cmd: Negative offset detected...\n",0);
    return -1.0;
  }

  // Set common tune request parameters.
  tune_request.target_freq = freq;
  tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
  tune_request.dsp_freq = 0.0;
  tune_request.args = "mode_n=integer";
  // If local oscillator offset is zero then default tune request is used.
  if(lo_off == 0.0) {
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
    tune_request.rf_freq = 0.0;
  } else { // Any value different than 0.0 means we want to give an offset in frequency.
    tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_MANUAL;
    tune_request.rf_freq = freq + lo_off;
  }

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Set Rx frequency.
  error = uhd_usrp_set_rx_freq(handler->usrp, &tune_request, channel, &tune_result);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting RX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_rx_freq: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
}

//******************************************************************************
// *********** Customized UHD/FPGA implementations for scatter PHY. ************
//******************************************************************************
void rf_uhd_rf_monitor_initialize(void *h, double freq, double rate, double lo_off, size_t fft_size, size_t avg_size) {
#if(ENABLE_HW_RF_MONITOR==1)
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_rf_monitor_initialize(handler->usrp, freq, rate, lo_off, fft_size, avg_size);
#endif
}

void rf_uhd_rf_monitor_uninitialize(void *h) {
#if(ENABLE_HW_RF_MONITOR==1)
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_rf_monitor_uninitialize(handler->usrp);
#endif
}

double rf_uhd_set_rf_mon_srate(void *h, double srate) {
#if(ENABLE_HW_RF_MONITOR==1)
  pthread_mutex_lock(&tx_rx_mutex[1]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_rf_mon_rate(handler->usrp, srate);
  uhd_usrp_get_rf_mon_rate(handler->usrp, &srate);
  pthread_mutex_unlock(&tx_rx_mutex[1]);
  return srate;
#else
  return -1.0;
#endif
}

double rf_uhd_get_rf_mon_srate(void *h) {
#if(ENABLE_HW_RF_MONITOR==1)
  double srate = -1.0;
  pthread_mutex_lock(&tx_rx_mutex[1]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_get_rf_mon_rate(handler->usrp, &srate);
  pthread_mutex_unlock(&tx_rx_mutex[1]);
  return srate;
#else
  return -1.0;
#endif
}

double rf_uhd_set_tx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel) {

#if(ENABLE_HW_RF_MONITOR==1)
  uhd_error error;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_tx_channel_freq_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_channel_freq_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  error = uhd_usrp_set_tx_channel_freq(handler->usrp, channel, freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
#else
  return -1.0;
#endif
}

double rf_uhd_set_tx_channel_freq_and_gain_cmd(void *h, double freq, double gain, uint64_t timestamp, uint64_t time_advance, size_t channel) {

#if(ENABLE_HW_RF_MONITOR==1)

  uhd_error error;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_tx_channel_freq_and_gain_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_channel_freq_and_gain_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  if(gain < 0.0) {
    RF_UHD_ERROR("rf_uhd_set_tx_channel_freq_and_gain_cmd: Negative gain detected...\n",0);
    return -1.0;
  }

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Set Tx frequency.
  error = uhd_usrp_set_tx_channel_freq(handler->usrp, channel, freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
    return -10.0;
  }
  // Set Tx gain.
  error = uhd_usrp_set_tx_gain(handler->usrp, gain, channel, "");
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX gain: %f [dB]. Error code: %d................\n",gain,error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_gain: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
#else
  return -1.0;
#endif
}

double rf_uhd_set_rx_channel_freq_cmd(void *h, double freq, uint64_t timestamp, uint64_t time_advance, size_t channel) {

#if(ENABLE_HW_RF_MONITOR==1)

  uhd_error error;
  time_t full_secs = 0;
  double frac_secs = 0.0;

  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);

  if((timestamp-time_advance) <= 0) {
    RF_UHD_ERROR("rf_uhd_set_rx_channel_freq_cmd: Negative timestamp detected...\n",0);
    return -1.0;
  }

  if(freq <= 0.0) {
    RF_UHD_ERROR("rf_uhd_set_rx_channel_freq_cmd: Negative frequency detected...\n",0);
    return -1.0;
  }

  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;

  // Convert timestamp to full and frac secs.
  helpers_convert_host_timestamp_into_uhd_timestamp_us((timestamp-time_advance), &full_secs, &frac_secs);

#if(DEBUG_TIMED_COMMANDS==1)
  if(full_secs <= 0) {
    printf("[ERROR] Timed command full secs: %d\n",full_secs);
  }
  if(frac_secs <= 0.0) {
    printf("[ERROR] Timed command frac secs: %f\n",frac_secs);
  }
#endif

  // sets command time on the device so that the next commands are all timed.
  error = uhd_usrp_set_command_time(handler->usrp, full_secs, frac_secs, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_command_time: %s\n",error_out);
    return -10.0;
  }
  // Set Rx frequency.
  error = uhd_usrp_set_rx_channel_freq(handler->usrp, channel, freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting RX channel frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_rx_channel_freq: %s\n",error_out);
    return -10.0;
  }
  // End timed command.
  error = uhd_usrp_clear_command_time(handler->usrp, 0);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error cleaning command time. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_clear_command_time: %s\n",error_out);
    return -10.0;
  }
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
#else
  return -1.0;
#endif
}

double rf_uhd_set_rx_channel_freq(void *h, double freq, size_t channel) {
#if(ENABLE_HW_RF_MONITOR==1)
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  uhd_usrp_set_rx_channel_freq(handler->usrp, channel, freq);
  uhd_usrp_get_rx_channel_freq(handler->usrp, channel, &freq);
  //RF_UHD_PRINT("Setting Rx frequency to %1.3f\n",freq/1e6);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
#else
  return -1.0;
#endif
}

double rf_uhd_set_tx_channel_freq(void *h, double freq, size_t channel) {
#if(ENABLE_HW_RF_MONITOR==1)
  uhd_error error;
  // Lock a mutex prior to using the USRP object.
  pthread_mutex_lock(&tx_rx_mutex[channel]);
  rf_uhd_handler_t *handler = (rf_uhd_handler_t*) h;
  error = uhd_usrp_set_tx_channel_freq(handler->usrp, channel, freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error setting TX frequency: %1.2f [MHz]. Error code: %d................\n",(freq/1000000.0),error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_set_tx_freq: %s\n",error_out);
    return -10.0;
  }
  error = uhd_usrp_get_tx_channel_freq(handler->usrp, channel, &freq);
  if(error != UHD_ERROR_NONE) {
    RF_UHD_ERROR("Error getting TX frequency. Error code: %d................\n",error);
    size_t strbuffer_len = 512;
    char error_out[strbuffer_len];
    error = uhd_usrp_last_error(handler->usrp, error_out, strbuffer_len);
    RF_UHD_ERROR("uhd_usrp_last_error for call to uhd_usrp_get_tx_freq: %s\n",error_out);
    return -10.0;
  }
  //RF_UHD_PRINT("Setting Tx frequency to %1.3f\n",freq/1e6);
  // Unlock mutex upon using USRP object.
  pthread_mutex_unlock(&tx_rx_mutex[channel]);
  return freq;
#else
  return -1.0;
#endif
}
