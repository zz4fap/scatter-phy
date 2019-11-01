#ifndef _CH_EMULATOR_
#define _CH_EMULATOR_

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <complex.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include "srslte/config.h"
#include "srslte/channel/ch_awgn.h"
#include "srslte/utils/debug.h"
#include "srslte/utils/vector.h"
#include "srslte/sync/cfo.h"
#include "srslte/common/phy_common.h"

#include "liquid/liquid.h"

#include "../../../examples/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define ENABLE_CH_EMULATOR_PRINTS 1

#define CH_EMULATOR_NOF_CHANNELS 2

#define CH_EMULATOR_NP_FILENAME_CH "/tmp/channel_emulator_np_ch"

#define DEFAULT_SUBFRAME_LEN 23040

#define DEFAULT_NOF_PRB 100

#define ENABLE_WRITING_ZEROS 0

#define ENABLE_WRITING_RANDOM_ZEROS_PREFIX 0

#define ADD_DELAY_BEFORE_RANDOM_SAMPLES 0

#define ENABLE_WRITING_RANDOM_ZEROS_SUFFIX 0

#define OUTPUT_FILENAME "channel_output_psd.m"

// *********** Defintion of debugging macros ***********
#define CH_EMULATOR_PRINT(_fmt, ...) do { if(ENABLE_CH_EMULATOR_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[CH EMULATOR PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define CH_EMULATOR_DEBUG(_fmt, ...) do { if(ENABLE_CH_EMULATOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[CH EMULATOR DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define CH_EMULATOR_INFO(_fmt, ...) do { if(ENABLE_CH_EMULATOR_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[CH EMULATOR INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define CH_EMULATOR_ERROR(_fmt, ...) do { fprintf(stdout, "[CH EMULATOR ERROR]: " _fmt, __VA_ARGS__); } while(0)

// *********** Definition of types ***********
typedef struct {
  channel_cccf impairments;
  srslte_cfo_t cfocorr;
  float cfo_freq;
  uint32_t fft_size;
  float noise_variance; // noise variance used with srsLTE library AWGN channel.
  float snr;
  // Callbacks used to configure impairments.
  void (*add_awgn_func_ptr)(void *h, float noise_floor, float SNRdB);
  void (*add_simple_awgn_func_ptr)(void *h, float noise_variance);
  void (*add_carrier_offset_func_ptr)(void *h, float dphi, float phi);
  void (*add_multipath_func_ptr)(void *h, cf_t *hc, unsigned int hc_len);
  void (*add_shadowing_func_ptr)(void *h, float sigma, float fd);
  void (*print_channel_func_ptr)(void *h);
  void (*estimate_psd_func_ptr)(cf_t *sig, uint32_t nof_samples, unsigned int nfft, float *psd);
  void (*create_psd_script_func_ptr)(unsigned int nfft, float *psd);
  void (*set_cfo_freq_func_ptr)(void* h, float freq);
} channel_impairments_t;

typedef struct {
  int wr_fd;
  int rd_fd;
  int nof_reads;
} channel_t;

typedef struct {
  channel_t channels[CH_EMULATOR_NOF_CHANNELS];
  bool enable_channel_impairments;
  bool use_simple_awgn_channel;
  void (*set_channel_impairments_func_ptr)(void* h, bool flag);
  void (*set_channel_simple_awgn_func_ptr)(void* h, bool flag);
  channel_impairments_t channel_impairments;
  uint32_t subframe_length;
  LayerCommunicator_handle handle;
  // Variables used by the change parameters thread.
  volatile sig_atomic_t run_change_parameters_thread;
  pthread_attr_t change_parameters_thread_attr;
  pthread_t change_parameters_thread_id;
#if(ENABLE_WRITING_RANDOM_ZEROS_SUFFIX==1 || ENABLE_WRITING_RANDOM_ZEROS_PREFIX==1)
  cf_t *null_sample_vector;
#endif
} channel_emulator_t;

// *********** Declaration of functions ***********
SRSLTE_API int channel_emulator_initialization(channel_emulator_t* chann_emulator);

SRSLTE_API int channel_emulator_uninitialization(channel_emulator_t* chann_emulator);

SRSLTE_API int channel_emulator_create_named_pipe(uint32_t channel_id);

SRSLTE_API int channel_emulator_get_named_pipe_for_writing(uint32_t channel_id);

SRSLTE_API int channel_emulator_get_named_pipe_for_reading(uint32_t channel_id);

SRSLTE_API int channel_emulator_close_reading_pipe(int fd, uint32_t channel_id);

SRSLTE_API int channel_emulator_close_writing_pipe(int fd, uint32_t channel_id);

SRSLTE_API int channel_emulator_send(void* h, void *data, int nof_samples, bool blocking, bool is_start_of_burst, bool is_end_of_burst, uint32_t channel_id);

SRSLTE_API int channel_emulator_recv(void *h, void *data, uint32_t nof_samples, bool blocking, uint32_t channel_id);

SRSLTE_API void channel_emulator_add_awgn(void *h, float noise_floor, float SNRdB);

SRSLTE_API void channel_emulator_add_simple_awgn(void *h, float noise_variance);

SRSLTE_API void channel_emulator_add_simple_awgn_snr(void *h, float snr);

SRSLTE_API void channel_emulator_add_carrier_offset(void *h, float dphi, float phi);

SRSLTE_API void channel_emulator_add_multipath(void *h, cf_t *hc, unsigned int hc_len);

SRSLTE_API void channel_emulator_add_shadowing(void *h, float sigma, float fd);

SRSLTE_API void channel_emulator_print_channel(void *h);

SRSLTE_API void channel_emulator_estimate_psd(cf_t *sig, uint32_t nof_samples, unsigned int nfft, float *psd);

SRSLTE_API void channel_emulator_create_psd_script(unsigned int nfft, float *psd);

SRSLTE_API int channel_emulator_set_subframe_length(channel_emulator_t* chann_emulator, double freq);

SRSLTE_API void channel_emulator_set_channel_impairments(void* h, bool flag);

SRSLTE_API void channel_emulator_set_simple_awgn_channel(void* h, bool flag);

SRSLTE_API void channel_emulator_set_cfo_freq(void* h, float freq);

SRSLTE_API int channel_emulator_start_config_thread(channel_emulator_t* const chann_emulator);

SRSLTE_API int channel_emulator_stop_config_thread(channel_emulator_t* const chann_emulator);

int recv_samples(void *h, void *data, uint32_t nof_samples, uint32_t channel_id);

void *channel_emulator_change_parameters_work(void *h);

#endif // _CH_EMULATOR_
