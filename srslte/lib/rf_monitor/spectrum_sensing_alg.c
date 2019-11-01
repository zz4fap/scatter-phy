#include "srslte/rf_monitor/spectrum_sensing_alg.h"

// ******************** Definitions and global varibales for Sensing *************************
fftwf_complex *fft_out_samples, *fft_in_samples;
fftwf_plan sensing_fft_plan;
int number_of_samples_in_subband, number_of_subbands;
cf_t *freq_samples;
float *subband_energy, *sorted_subband_energy;
float TCME; // for 16 samples
float pfa; // Probability of false alarm.
uint8_t *detection_array;
//*******************************************************************************************

void spectrum_sensing_alg_init() {

  // Allocate Memory.
  fft_in_samples = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SPECTRUM_SENSING_SAMPLES);
  fft_out_samples = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex)*NUM_OF_SPECTRUM_SENSING_SAMPLES);

  // This is the scale factor applied to the noise reference in order to achieve the desired probability of false alarm.
  pfa = 0.01/100;
  // Initialize the number of samples per subband.
  number_of_samples_in_subband = 16;
  // Discard value for 16 samples in subband.
  TCME = 1.9528; // Value calculated for 16 samples
  // Calculate the number of subbands.
  number_of_subbands = NUM_OF_SPECTRUM_SENSING_SAMPLES/number_of_samples_in_subband;
  // Allocate memory for subbands.
  subband_energy = (float*)srslte_vec_malloc(sizeof(float) * number_of_subbands);
  if (!subband_energy) {
    SENSING_ALG_ERROR("Error allocating subband_energy memory.\n",0);
    exit(-1);
  }
  // Allocate memory for sorted_ subband energy samples.
  sorted_subband_energy = (float*)srslte_vec_malloc(sizeof(float) * number_of_subbands);
  if (!sorted_subband_energy) {
    SENSING_ALG_ERROR("Error allocating sorted_subband_energy memory.\n",0);
    exit(-1);
  }
  // Allocate memory for subbands.
  freq_samples = (cf_t*)srslte_vec_malloc(sizeof(cf_t) * NUM_OF_SPECTRUM_SENSING_SAMPLES);
  if (!freq_samples) {
    SENSING_ALG_ERROR("Error allocating freq_samples memory.\n",0);
    exit(-1);
  }

  detection_array = (uint8_t*)srslte_vec_malloc(sizeof(uint8_t) * number_of_subbands);
  if (!detection_array) {
    SENSING_ALG_ERROR("Error allocating detection_array memory.\n",0);
    exit(-1);
  }

  // Apply FFT to the received base band samples.
  sensing_fft_plan = fftwf_plan_dft_1d(NUM_OF_SPECTRUM_SENSING_SAMPLES, fft_in_samples, fft_out_samples, FFTW_FORWARD, FFTW_ESTIMATE);
}

void spectrum_sensing_alg_free() {
  // Free allocated resources
  fftwf_destroy_plan(sensing_fft_plan);
  if(fft_in_samples) {
    fftwf_free(fft_in_samples);
  }
  if(fft_out_samples) {
    fftwf_free(fft_out_samples);
  }
  if(subband_energy) {
    free(subband_energy);
  }
  if(sorted_subband_energy){
    free(sorted_subband_energy);
  }
  if(freq_samples){
    free(freq_samples);
  }
  if(detection_array) {
    free(detection_array);
  }
}

void spectrum_sensing_alg_calculate_subband_energy(cf_t *base_band_samples) {
  // Copy base band samples into fft type for fftw execution.
  memcpy((uint8_t*)fft_in_samples,(uint8_t*)base_band_samples,sizeof(cf_t)*NUM_OF_SPECTRUM_SENSING_SAMPLES);
  // Execute FFT.
  fftwf_execute(sensing_fft_plan);
  // Copy the result from the fft type into the array for sensing.
  memcpy((uint8_t*)freq_samples,(uint8_t*)fft_out_samples,sizeof(cf_t)*NUM_OF_SPECTRUM_SENSING_SAMPLES);
  // Execute the subband energy calculation.
  for(int i = 0; i < number_of_subbands; i++) {
    subband_energy[i] = crealf(srslte_vec_dot_prod_conj_ccc(freq_samples + i*number_of_samples_in_subband, freq_samples + i*number_of_samples_in_subband, number_of_samples_in_subband));
    SENSING_ALG_DEBUG("subband_energy[%d]: %f\n",i,subband_energy[i]);
  }
  memcpy((uint8_t*)sorted_subband_energy,(uint8_t*)subband_energy,sizeof(float)*number_of_subbands);
}

// TODO: Implement other sorting algortihm like quicksort.
void spectrum_sensing_alg_sort_energy() {
	float temp;
	//Sort segmented subband energy vector in increasing order of energy.
	for(int i = 0; i < number_of_subbands; i++) {
		for(int k = 0; k < (number_of_subbands-1); k++) {
			if(sorted_subband_energy[k] > sorted_subband_energy[k+1]) {
				temp = sorted_subband_energy[k+1];
				sorted_subband_energy[k+1] = sorted_subband_energy[k];
				sorted_subband_energy[k] = temp;
			}
		}
	}
}

// Algorithm used to discard samples. Tt is used to find a speration between noisy samples and signal+noise samples.
float spectrum_sensing_alg_calculate_noise_reference(int* number_of_zref_segs) {

	float limiar, energy[number_of_subbands];
	int Index = (number_of_subbands < 20) ? 3:20;

	for(;Index < number_of_subbands; Index++) {
		limiar = (float)(TCME/Index);
		energy[Index] = 0.0;
		for(int k = 0; k < Index; k++) {
			energy[Index] = energy[Index] + sorted_subband_energy[k];
		}
		if(sorted_subband_energy[Index] > limiar*energy[Index] || Index == (number_of_subbands-1)) {
			break;
		}
	}

	(*number_of_zref_segs) = Index;
	return energy[Index];
}

// Calculate the scale factor.
float spectrum_sensing_alg_calculate_scale_factor(int x, float pfa) {
	float alpha = 0.0;
  alpha = statistics_helper_fisher_f_dist_quantile(pfa, number_of_samples_in_subband, x);
	return alpha/x;
}

void spectrum_sensing_alg_detect_primary_user(float alpha, float zref, int Index) {
  float ratio;
  // Iterate over all subbands for primary user detection.
  for(int i = 0; i < number_of_subbands; i++) {
    ratio = subband_energy[i]/zref;
    detection_array[i] = (ratio > alpha) ? true:false;
    SENSING_ALG_DEBUG("ratio: %f - alpha: %f - segment[%d]: %f - zref: %f - I: %d - is detected: %s\n",ratio,alpha,i,subband_energy[i],zref,Index,IS_DETECTED(detection_array[i]));
  }
}

int spectrum_sensing_alg_get_detection_array(uint8_t **data) {
  *data = detection_array;
  return number_of_subbands;
}

int spectrum_sensing_alg_get_subband_energy_array(uint8_t **data) {
  *data = (uint8_t*)subband_energy;
  return (number_of_subbands*sizeof(float));
}

void spectrum_sensing_alg_print_subband_energy() {
  for(int i = 0; i < number_of_subbands; i++) {
    SENSING_ALG_PRINT("subband_energy[%d]: %1.2f\n",i,subband_energy[i]);
  }
}

void spectrum_sensing_alg_print_sorted_subband_energy() {
  for(int i = 0; i < number_of_subbands; i++) {
    SENSING_ALG_PRINT("sorted_subband_energy[%d]: %1.2f\n",i,sorted_subband_energy[i]);
  }
}

void spectrum_sensing_alg_print_detection_array(uint8_t *data, int data_length) {
  for(int i = 0; i < data_length; i++) {
    SENSING_ALG_PRINT("pos: %d: %d\n",i,data[i]);
  }
  printf("\n\n\n",0);
}

// The spectrum sensing module sends data to AI module layer.
void *spectrum_sensing_alg_work(void *h) {

  SENSING_ALG_INFO("Start: spectrum_sensing_alg_work()\n",0);

  rf_monitor_handle_t *sensing_handle = (rf_monitor_handle_t *)h;
  cf_t base_band_samples[NUM_OF_SPECTRUM_SENSING_SAMPLES];
  int number_of_zref_segs, data_length;
  uint8_t *data = NULL;
  int num_read_samples = 0;
  time_t full_secs;
  double frac_secs;

  // Initialize all the necessary resources.
  spectrum_sensing_alg_init();

  // Set priority to spectrum sesing thread.
  uhd_set_thread_priority(1.0, true);

  /******** Do the real sensing here ********/
  while(run_rf_monitor_thread) {

#if PROFILLING_SPECTRUM_SENSING_ALG
    struct timespec start_sensing, end_sensing;
    clock_gettime(CLOCK_REALTIME, &start_sensing);
#endif

    // Receive IQ samples (BB samples).
    num_read_samples = srslte_rf_recv_channel_with_time(sensing_handle->channel_handler, base_band_samples, NUM_OF_SPECTRUM_SENSING_SAMPLES, true, &full_secs, &frac_secs);
    if(num_read_samples < 0) {
      SENSING_ALG_ERROR("Problem reading BB samples.\n",0);
    }

    // This function calculates groups FFT bins into subbands and calculates the energy in that band.
    spectrum_sensing_alg_calculate_subband_energy(base_band_samples);
    // This function sorts the energy samples in ascending order so that we can find a good reference for the noise.
    spectrum_sensing_alg_sort_energy();
    // Given the sorted energy we calculate a noise reference that will be used to evulate if the channel is occupied or not.
    float zref = spectrum_sensing_alg_calculate_noise_reference(&number_of_zref_segs);
    // This is the scale factor applied to the noise reference in order to achieve the desired probability of false alarm.
    float alpha = spectrum_sensing_alg_calculate_scale_factor(number_of_zref_segs, pfa);
    // Compare each of the subbands against the scaled noise reference.
    spectrum_sensing_alg_detect_primary_user(alpha, zref, number_of_zref_segs);
    // Retrieve a boolean vector indicating if each one of the subbands is occupied or busy.
    data_length = spectrum_sensing_alg_get_detection_array(&data);
    // Instead of sending booleans send the energy.
    //data_length = sensing_get_subband_energy_array(&data);

    //spectrum_sensing_alg_print_detection_array(data, data_length);

    // Send sensed data to upper layer. In fact it is queued in a safe queue.
    rf_monitor_send_sensing_statistics(0, PHY_SUCCESS, full_secs, frac_secs, sensing_handle->central_frequency, sensing_handle->sample_rate, sensing_handle->sensing_rx_gain, 0.0, data_length, data);

#if PROFILLING_SPECTRUM_SENSING_ALG
    clock_gettime(CLOCK_REALTIME, &end_sensing);
    double diff = time_diff(start_sensing, end_sensing);
    SENSING_ALG_PRINT("Sensing Elapsed time = %f milliseconds.\n", diff);
#endif
  }

  // uninitialize all the used resources.
  spectrum_sensing_alg_free();

  SENSING_ALG_DEBUG("Leaving Spectrum Sensing Algorithm module thread.\n",0);
  // Exit thread with result code.
  pthread_exit(0);
}
