#ifndef _TIMER_H_
#define _TIMER_H_

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "srslte/config.h"
#include "srslte/utils/debug.h"

#define ENABLE_TIMER_PRINTS 1

#define TIMER_PRINT(_fmt, ...) do { if(ENABLE_TIMER_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[TIMER PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define TIMER_DEBUG(_fmt, ...) do { if(ENABLE_TIMER_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[TIMER DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define TIMER_INFO(_fmt, ...) do { if(ENABLE_TIMER_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[TIMER INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define TIMER_ERROR(_fmt, ...) do { fprintf(stdout, "[TIMER ERROR]: " _fmt, __VA_ARGS__); } while(0)

//********************* Functions **********************************************
SRSLTE_API int timer_register_callback(void (*callback)(int, siginfo_t *, void *));

SRSLTE_API int timer_init(timer_t *timer_id);

SRSLTE_API int timer_set(timer_t *timer_id, time_t t);

SRSLTE_API int timer_disarm(timer_t *timer_id);

#endif // _TIMER_H_
