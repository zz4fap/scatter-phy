#ifndef _PLOT_H_
#define _PLOT_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "srsgui/srsgui.h"

#include "srslte/srslte.h"

typedef struct {
  plot_real_t p_sync;
  plot_real_t pce;
  plot_scatter_t pscatequal;
  plot_scatter_t pscatequal_pdcch;
  float tmp_plot[110*15*2048];
  float tmp_plot2[110*15*2048];
} plot_context_t;

int init_plot();

void free_plot();

void plot_info(srslte_ue_dl_t* const ue_dl, srslte_ue_sync_t* const ue_sync);

#endif
