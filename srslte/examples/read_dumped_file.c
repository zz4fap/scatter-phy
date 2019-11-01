#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "srslte/srslte.h"

#define MAX_SHORT_VALUE 32767.0

char *input_file_name = NULL;
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

char* concat_timestamp_string(char *filename) {
  char timestamp[20];
  time_t now = time(NULL);
  strftime(timestamp, 20, "%Y%m%d_%H%M%S", localtime(&now));
  return strcat(filename,timestamp);
}

int main(int argc, char **argv) {
  srslte_filesource_t fsrc;
  void *input;
  FILE *output_file;
  int size;
  unsigned long int number_of_samples_read;
  _Complex float *cbuf ;
  _Complex short *sbuf;

  if (argc < 2) {
    usage(argv[0]);
    exit(-1);
  }

  parse_args(argc,argv);

  printf("Opening file with data type: %s\n",SENSING_DATA_TYPE_STRING[data_type]);

  if(input_file_name == NULL) {
    printf("File name was not specified.\n");
    exit(-1);
  }

  if(filesource_init(&fsrc, input_file_name, data_type)) {
    fprintf(stderr, "Error opening file %s\n", input_file_name);
    exit(-1);
  }

  if(create_output_file) {
    output_file_name = strcat(strtok(input_file_name, "."),append_string);
    output_file_name = strcat(output_file_name,".dat");
    output_file = fopen(output_file_name, "w");
  }

  switch(data_type) {
    case SRSLTE_FLOAT_BIN:
      size = sizeof(float);
      break;
    case SRSLTE_COMPLEX_FLOAT_BIN:
      size = sizeof(_Complex float);
      break;
    case SRSLTE_COMPLEX_SHORT_BIN:
      size = sizeof(_Complex short);
      break;
    default:
      size = sizeof(_Complex short);
      break;
  }

  input = malloc(number_of_samples*size);
  if (!input) {
    perror("malloc");
    exit(-1);
  }

  // read all file.
  number_of_samples_read = filesource_read(&fsrc, input, number_of_samples);

  cbuf = (_Complex float*) input;
  sbuf = (_Complex short*) input;
  float re_f, im_f;
  short re_s, im_s;

  //SHRT_MAX
  for(int i = 0; i < number_of_samples_read; i++) {
    if(data_type == SRSLTE_COMPLEX_SHORT_BIN) {
      re_s =  __real__ sbuf[i];
      re_f = (float)re_s/MAX_SHORT_VALUE;
      im_s = __imag__ sbuf[i];
      im_f = (float)im_s/MAX_SHORT_VALUE;
      if(print_samples) {
        printf("sample float[%d]: (%f,%f) - sample short[%d]: (%d,%d)\n",i, re_f, im_f, i, re_s, im_s);
      }
      if(create_output_file) {
        fprintf(output_file, "%f\t%f\n", re_f, im_f);
      }
    }

    if(data_type == SRSLTE_COMPLEX_FLOAT_BIN) {
      if(print_samples) {
        printf("sample[%d]: (%f,%f)\n",i, __real__ cbuf[i], __imag__ cbuf[i]);
      }
      if(create_output_file) {
        fprintf(output_file, "%f\t%f\n", __real__ cbuf[i], __imag__ cbuf[i]);
      }
    }
  }

  // Free source file.
  filesource_free(&fsrc);

  free(input);

  if(create_output_file) {
    fclose(output_file);
  }

  printf("Done\n");
  exit(0);
}
