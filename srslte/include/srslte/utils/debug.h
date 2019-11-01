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

/******************************************************************************
 *  File:         debug.h
 *
 *  Description:  Debug output utilities.
 *
 *  Reference:
 *****************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "srslte/config.h"
#include "srslte/common/phy_logger.h"

#define SRSLTE_VERBOSE_DEBUG 2
#define SRSLTE_VERBOSE_INFO  1
#define SRSLTE_VERBOSE_NONE  0

#define SCATTER_VERBOSE_DEBUG 2
#define SCATTER_VERBOSE_INFO  1
#define SCATTER_VERBOSE_NONE  0

#include <sys/time.h>
SRSLTE_API void get_time_interval(struct timeval * tdata);
SRSLTE_API int getScatterVerboseLevel();

#define SRSLTE_DEBUG_ENABLED 1
#define SCATTER_DEBUG_ENABLED 0
#define SCATTER_PROFILING_ENABLED 0
#define DEVELOPMENT_DEBUG_ENABLED 0

SRSLTE_API extern int srslte_verbose;
SRSLTE_API extern int scatter_verbose_level;
SRSLTE_API extern int development_debug_level;
SRSLTE_API extern int handler_registered;

#define SRSLTE_VERBOSE_ISINFO() (srslte_verbose>=SRSLTE_VERBOSE_INFO)
#define SRSLTE_VERBOSE_ISDEBUG() (srslte_verbose>=SRSLTE_VERBOSE_DEBUG)
#define SRSLTE_VERBOSE_ISNONE() (srslte_verbose==SRSLTE_VERBOSE_NONE)

#define PRINT_DEBUG srslte_verbose=SRSLTE_VERBOSE_DEBUG
#define PRINT_INFO srslte_verbose=SRSLTE_VERBOSE_INFO
#define PRINT_NONE srslte_verbose=SRSLTE_VERBOSE_NONE

#define DEBUG(_fmt, ...) if (SRSLTE_DEBUG_ENABLED && srslte_verbose >= SRSLTE_VERBOSE_DEBUG)\
        {  fprintf(stdout, "[DEBUG]: " _fmt, ##__VA_ARGS__);  }

#define INFO(_fmt, ...) if (SRSLTE_DEBUG_ENABLED && srslte_verbose >= SRSLTE_VERBOSE_INFO) \
        {  fprintf(stdout, "[INFO]: " _fmt, ##__VA_ARGS__);  }

#if CMAKE_BUILD_TYPE==Debug
/* In debug mode, it prints out the  */
#define ERROR(_fmt, ...) if (1)\
    {   fprintf(stderr, "\e[31m%s.%d: " _fmt "\e[0m\n", __FILE__, __LINE__, ##__VA_ARGS__);}

#else
#define ERROR(_fmt, ...) if (1)\
        {   fprintf(stderr, "[ERROR in %s]:" _fmt "\n", __FUNCTION__, ##__VA_ARGS__);}

#endif /* CMAKE_BUILD_TYPE==Debug */

#define SCATTER_DEBUG(_fmt, ...) do { if (SCATTER_DEBUG_ENABLED) \
  fprintf(stdout, "[SCATTER DEBUG]: " _fmt, __VA_ARGS__); } while(0)

// Development debug macros.
#define DEV_DEBUG(_fmt, ...) do { if (DEVELOPMENT_DEBUG_ENABLED && development_debug_level >= SRSLTE_VERBOSE_DEBUG) \
  fprintf(stdout, "[DEV DEBUG]: " _fmt, __VA_ARGS__); } while(0)

#define DEV_INFO(_fmt, ...) do { if (DEVELOPMENT_DEBUG_ENABLED && development_debug_level >= SRSLTE_VERBOSE_INFO) \
  fprintf(stdout, "[DEV INFO]:  " _fmt, __VA_ARGS__); } while(0)

#define PHY_PROFILLING_ENABLE 1

#define PHY_PROFILLING_AVG(_fmt, timediff) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time = 0.0; static uint64_t avg_counter = 0;\
  avg_time += timediff; \
  avg_counter++; \
  fprintf(stdout, "[PHY PROFILLING]: " _fmt, avg_time/avg_counter); } } while(0)

#define PHY_PROFILLING_AVG2(_fmt, timediff) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time = 0.0; static uint64_t avg_counter = 0; static double min_value = 100000.0; static double max_value = 0.0; static uint64_t max_counter = 0; static uint64_t one_milli_counter = 0;\
  avg_time += timediff; \
  avg_counter++; \
  if(timediff > max_value && avg_counter > 1000) {max_value = timediff; max_counter++;} \
  if(timediff < min_value && avg_counter > 1000) {min_value = timediff;} \
  if(timediff >= 0.001) one_milli_counter++; \
  if(avg_counter%100000 == 0) fprintf(stdout, "[PHY PROFILLING]: " _fmt, (avg_time/avg_counter), min_value, max_value, max_counter, one_milli_counter, avg_counter, (double)((double)one_milli_counter/avg_counter)*100 ); } } while(0)

#define PHY_PROFILLING_AVG3(_fmt, timediff, gt_value, update_rate) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time = 0.0; static uint64_t avg_counter = 0; static double min_value = 100000.0; static double max_value = 0.0; static uint64_t max_counter = 0; static uint64_t gt_value_counter = 0;\
  avg_time += timediff; \
  avg_counter++; \
  if(timediff > max_value && avg_counter > 1000) {max_value = timediff; max_counter++;} \
  if(timediff < min_value && avg_counter > 1000) {min_value = timediff;} \
  if(timediff >= gt_value) gt_value_counter++; \
  if(avg_counter%update_rate == 0) fprintf(stdout, "[PHY PROFILLING]: " _fmt, (avg_time/avg_counter), min_value, max_value, max_counter, gt_value_counter, avg_counter, (double)((double)gt_value_counter/avg_counter)*100 ); } } while(0)

#define PHY_PROFILLING_AVG4(_fmt, nof_slots_in_frame, timediff, gt_value, update_rate) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time = 0.0; static uint64_t avg_counter = 0; static double min_value = 100000.0; static double max_value = 0.0; static uint64_t max_counter = 0; static uint64_t gt_value_counter = 0;\
  avg_time += timediff; \
  avg_counter++; \
  if(timediff > max_value) {max_value = timediff; max_counter++;} \
  if(timediff < min_value) {min_value = timediff;} \
  if(timediff >= gt_value) {gt_value_counter++;} \
  if(avg_counter%update_rate == 0) fprintf(stdout, "[PHY PROFILLING]: " _fmt, (avg_time/avg_counter), min_value, max_value, max_counter, nof_slots_in_frame, gt_value_counter, avg_counter, (double)((double)gt_value_counter/avg_counter)*100 ); } } while(0)

#define PHY_PROFILLING_AVG5(_fmt, timediff, gt_value, update_rate) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time = 0.0; static uint64_t avg_counter = 0; static double min_value = 100000.0; static double max_value = 0.0; static uint64_t max_counter = 0; static uint64_t gt_value_counter = 0;\
  avg_time += timediff; \
  avg_counter++; \
  if(timediff > max_value) {max_value = timediff; max_counter++;} \
  if(timediff < min_value) {min_value = timediff;} \
  if(timediff >= gt_value) gt_value_counter++; \
  if(avg_counter%update_rate == 0) fprintf(stdout, "[PHY PROFILLING]: " _fmt, (avg_time/avg_counter), min_value, max_value, max_counter, gt_value_counter, avg_counter, (double)((double)gt_value_counter/avg_counter)*100 ); } } while(0)

#define PHY_PROFILLING_AVG6(_fmt, vphy_id, timediff, gt_value, update_rate) do { if (PHY_PROFILLING_ENABLE) { \
  static double avg_time[2] = {0.0, 0.0}; static uint64_t avg_counter[2] = {0, 0}; static double min_value[2] = {100000.0, 100000.0}; static double max_value[2] = {0.0, 0.0}; static uint64_t max_counter[2] = {0, 0}; static uint64_t gt_value_counter[2] = {0, 0}; \
  avg_time[vphy_id] += timediff; \
  avg_counter[vphy_id]++; \
  if(timediff > max_value[vphy_id]) {max_value[vphy_id] = timediff; max_counter[vphy_id]++;} \
  if(timediff < min_value[vphy_id]) {min_value[vphy_id] = timediff;} \
  if(timediff >= gt_value) gt_value_counter[vphy_id]++; \
  if(avg_counter[vphy_id]%update_rate == 0) fprintf(stdout, "[PHY PROFILLING]: " _fmt, vphy_id, (avg_time[vphy_id]/avg_counter[vphy_id]), min_value[vphy_id], max_value[vphy_id], max_counter[vphy_id], gt_value_counter[vphy_id], avg_counter[vphy_id], (double)((double)gt_value_counter[vphy_id]/avg_counter[vphy_id])*100 ); } } while(0)

SRSLTE_API inline void get_error_string(int rc, char * error) {
  switch(rc) {
    case EBUSY:
      sprintf(error,"EBUSY");
      break;
    case EINVAL:
      sprintf(error,"EINVAL");
      break;
    case EAGAIN:
      sprintf(error,"EAGAIN");
      break;
    case ENOMEM:
      sprintf(error,"ENOMEM");
      break;
    case EEXIST:
      sprintf(error,"EEXIST");
      break;
    case EACCES:
      sprintf(error,"EACCES");
      break;
    case ELOOP:
      sprintf(error,"ELOOP");
      break;
    default:
      sprintf(error,"Error not mapped");
  }
}

#endif // DEBUG_H
