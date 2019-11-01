#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>

#include <uhd.h>

#include "srslte/srslte.h"

char *input_file_name;
char *append_string = "_two_columns_format";
char *output_file_name;
unsigned long int number_of_samples = 9600;
bool create_output_file = false, print_samples = false;
unsigned int data_type = SRSLTE_COMPLEX_SHORT_BIN;

const char SENSING_DATA_TYPE_STRING[6][30] = {"SRSLTE_FLOAT", "SRSLTE_COMPLEX_FLOAT", "SRSLTE_COMPLEX_SHORT", "SRSLTE_FLOAT_BIN", "SRSLTE_COMPLEX_FLOAT_BIN", "SRSLTE_COMPLEX_SHORT_BIN"};

void usage(char *prog) {
  printf("Usage: %s [iopltv] -i input_file\n", prog);
  printf("\t-o create double column output file\n");
  printf("\t-p print IQ samples on screen\n");
  printf("\t-l number_of_samples [Default %d]\n", number_of_samples);
  printf("\t-t data type to be used [Default: %s]\n",SENSING_DATA_TYPE_STRING[data_type]);
  printf("\t Other types are: \n\t\t0 - SRSLTE_FLOAT\n\t\t1 - SRSLTE_COMPLEX_FLOAT\n\t\t2 - SRSLTE_COMPLEX_SHORT\n\t\t3 - SRSLTE_FLOAT_BIN\n\t\t4 - SRSLTE_COMPLEX_FLOAT_BIN\n\t\t5 - SRSLTE_COMPLEX_SHORT_BIN\n");
  printf("\t-v srslte_verbose\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "iopltv")) != -1) {
    switch(opt) {
    case 'i':
      input_file_name = argv[optind];
      break;
    case 'o':
      create_output_file = true;
      break;
    case 'l':
      number_of_samples = atoi(argv[optind]);
      break;
    case 'p':
      print_samples = true;
      break;
    case 'v':
      srslte_verbose++;
      break;
    case 't':
      data_type = atoi(argv[optind]);
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
  if (!input_file_name) {
    usage(argv[0]);
    exit(-1);
  }
}

int main(int argc, char **argv) {
  srslte_filesource_t fsrc;
  srslte_filesink_t fsink;
  cf_t *input;
  unsigned long int number_of_samples_read;
  _Complex short *sbuf;
  number_of_samples = 3;

  _Complex float my_cf_samples[3];

  my_cf_samples[0] = -12.1234 + 9.6789i;
  my_cf_samples[1] = 1200.1234 + 0.6789i;
  my_cf_samples[2] = 4.99999 + 1.6789i;

  _Complex short *my_cs_samples = (_Complex short *)my_cf_samples;

  if(srslte_filesink_init(&fsink, "test_complex_short_samples.dat", SRSLTE_COMPLEX_SHORT_BIN)) {
    fprintf(stderr, "Error opening sink file\n");
    exit(-1);
  }

  int num_samples_written = srslte_filesink_write(&fsink, my_cs_samples, number_of_samples);
  printf("Number of samples written: %d\n",num_samples_written);

  // Free sink file.
  srslte_filesink_free(&fsink);

  if(srslte_filesource_init(&fsrc, "test_complex_short_samples.dat", SRSLTE_COMPLEX_SHORT_BIN)) {
    fprintf(stderr, "Error opening source file\n");
    exit(-1);
  }

  input = malloc(number_of_samples*sizeof(_Complex short));
  if (!input) {
    perror("malloc");
    exit(-1);
  }

  // read all file.
  number_of_samples_read = srslte_filesource_read(&fsrc, input, number_of_samples);

  sbuf = (_Complex short*) input;

  //SHRT_MAX
  printf("Number of samples read: %d\n",number_of_samples_read);
  for(int i = 0; i < number_of_samples_read; i++) {
    printf("sample[%d]: (%d,%d)\n",i, __real__ sbuf[i], __imag__ sbuf[i]);
  }

  // Free source file.
  srslte_filesource_free(&fsrc);

  free(input);

  printf("Done\n");
  exit(0);
}
