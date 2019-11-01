#include "srslte/rf_monitor/statistics_helpers.h"

float statistics_helper_fisher_f_dist_quantile(float pfa, int number_of_samples_in_subband, int x) {
  float alpha;
  boost::math::fisher_f_distribution<> fd(2*number_of_samples_in_subband, 2*number_of_samples_in_subband*x);
  alpha = quantile(fd, (1-pfa));
  return alpha;
}
