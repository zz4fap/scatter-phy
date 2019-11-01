#include "plot.h"

/**********************************************************************
 *  Plotting Functions
 **********************************************************************/
static plot_context_t *plot_context = NULL;

int init_plot() {
  // Allocate memory for a new plot handle object.
  plot_context = (plot_context_t*)srslte_vec_malloc(sizeof(plot_context_t));
  // Check if memory allocation was correctly done.
  if(plot_context == NULL) {
    printf("Error allocating memory for plot context.\n");
    return -1;
  }

  sdrgui_init();

  plot_scatter_init(&plot_context->pscatequal);
  plot_scatter_setTitle(&plot_context->pscatequal, "PDSCH - Equalized Symbols");
  plot_scatter_setXAxisScale(&plot_context->pscatequal, -2, 2);
  plot_scatter_setYAxisScale(&plot_context->pscatequal, -2, 2);

  plot_scatter_addToWindowGrid(&plot_context->pscatequal, (char*)"pdsch_ue", 0, 0);

  plot_real_init(&plot_context->pce);
  plot_real_setTitle(&plot_context->pce, "Channel Response - Magnitude");
  plot_real_setLabels(&plot_context->pce, "Index", "dB");
  plot_real_setYAxisScale(&plot_context->pce, -40, 40);

  plot_real_init(&plot_context->p_sync);
  plot_real_setTitle(&plot_context->p_sync, "PSS Cross-Corr abs value");
  plot_real_setYAxisScale(&plot_context->p_sync, 0, 1);

  plot_scatter_init(&plot_context->pscatequal_pdcch);
  plot_scatter_setTitle(&plot_context->pscatequal_pdcch, "PDCCH - Equalized Symbols");
  plot_scatter_setXAxisScale(&plot_context->pscatequal_pdcch, -2, 2);
  plot_scatter_setYAxisScale(&plot_context->pscatequal_pdcch, -2, 2);

  plot_real_addToWindowGrid(&plot_context->pce, (char*)"pdsch_ue",    0, 1);
  plot_real_addToWindowGrid(&plot_context->pscatequal_pdcch, (char*)"pdsch_ue", 1, 0);
  plot_real_addToWindowGrid(&plot_context->p_sync, (char*)"pdsch_ue", 1, 1);

  return 0;
}

void free_plot() {
  // Free memory used to store plot context object.
  if(plot_context) {
    free(plot_context);
    plot_context = NULL;
  }
}

void plot_info(srslte_ue_dl_t* const ue_dl, srslte_ue_sync_t* const ue_sync) {

  bool plot_track = false;
  bool disable_plots_except_constellation = false;

  int i;
  uint32_t nof_re = SRSLTE_SF_LEN_RE(ue_dl->cell.nof_prb, ue_dl->cell.cp);

  uint32_t nof_symbols = ue_dl->pdsch_cfg.nbits.nof_re;
  if(!disable_plots_except_constellation) {
    for(i = 0; i < nof_re; i++) {
      plot_context->tmp_plot[i] = 20 * log10f(cabsf(ue_dl->sf_symbols[i]));
      if(isinf(plot_context->tmp_plot[i])) {
        plot_context->tmp_plot[i] = -80;
      }
    }
    int sz = srslte_symbol_sz(ue_dl->cell.nof_prb);
    bzero(plot_context->tmp_plot2, sizeof(float)*sz);
    int g = (sz - 12*ue_dl->cell.nof_prb)/2;
    for(i = 0; i < 12*ue_dl->cell.nof_prb; i++) {
      plot_context->tmp_plot2[g+i] = 20 * log10(cabs(ue_dl->ce[0][i]));
      if(isinf(plot_context->tmp_plot2[g+i])) {
        plot_context->tmp_plot2[g+i] = -80;
      }
    }
    plot_real_setNewData(&plot_context->pce, plot_context->tmp_plot2, sz);

    if(plot_track) {
      srslte_pss_synch_t *pss_obj = srslte_sync_get_cur_pss_obj(&ue_sync->strack);
      int max = srslte_vec_max_fi(pss_obj->conv_output_avg_plot, pss_obj->frame_size+pss_obj->fft_size-1);
      srslte_vec_sc_prod_fff(pss_obj->conv_output_avg_plot,
                      1/pss_obj->conv_output_avg_plot[max],
                      plot_context->tmp_plot2,
                      pss_obj->frame_size+pss_obj->fft_size-1);
      plot_real_setNewData(&plot_context->p_sync, plot_context->tmp_plot2, pss_obj->frame_size);
    } else {
      int max = srslte_vec_max_fi(ue_sync->sfind.pss.conv_output_avg_plot, ue_sync->sfind.pss.frame_size+ue_sync->sfind.pss.fft_size-1);
      srslte_vec_sc_prod_fff(ue_sync->sfind.pss.conv_output_avg_plot,
                      1/ue_sync->sfind.pss.conv_output_avg_plot[max],
                      plot_context->tmp_plot2,
                      ue_sync->sfind.pss.frame_size+ue_sync->sfind.pss.fft_size-1);
      plot_real_setNewData(&plot_context->p_sync, plot_context->tmp_plot2, ue_sync->sfind.pss.frame_size);
    }

    plot_scatter_setNewData(&plot_context->pscatequal_pdcch, ue_dl->pdcch.d, 36*ue_dl->pdcch.nof_cce);
  }

  plot_scatter_setNewData(&plot_context->pscatequal, ue_dl->pdsch.d, nof_symbols);
}
