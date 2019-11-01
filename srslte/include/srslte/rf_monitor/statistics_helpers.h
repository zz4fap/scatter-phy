#ifndef _STATISTICS_HELPER_H_
#define _STATISTICS_HELPER_H_

#ifdef __cplusplus

#include <math.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <boost/math/distributions.hpp>

extern "C" {
#endif

float statistics_helper_fisher_f_dist_quantile(float pfa, int number_of_samples_in_subband, int x);

#ifdef __cplusplus
}
#endif

#endif /* _STATISTICS_HELPER_H_ */
