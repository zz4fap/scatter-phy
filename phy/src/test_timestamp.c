#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LOGNAME_FORMAT "log/%Y%m%d_%H%M%S"
#define LOGNAME_SIZE 30

/* Return a character string representing the current date and time.  */

char* get_timestamp ()
{
  time_t now = time (NULL);
  return asctime (localtime (&now));
}

void get_timestampv2() {
   static char name[LOGNAME_SIZE];
   time_t now = time(0);
   strftime(name, sizeof(name), LOGNAME_FORMAT, localtime(&now));
   printf("name %s\n",name);
}

void get_timestampv3(char *timestamp, size_t max) {
   time_t now = time(NULL);
   strftime(timestamp, max, "%Y%m%d_%H%M%S", localtime(&now));
}

char* sensing_concat_timestamp_string(char *filename) {
  char timestamp[20];
  time_t now = time(NULL);
  strftime(timestamp, 20, "%Y%m%d_%H%M%S", localtime(&now));
  return strcat(filename,timestamp);
}

int main (int argc, char* argv[])
{
  /* The file to which to append the timestamp.  */
  char* filename = argv[1];
  /* Get the current timestamp.  */
  //char* timestamp = get_timestamp();

  char timestamp[LOGNAME_SIZE];
   ///get_timestampv3(timestamp, LOGNAME_SIZE);
//   printf("name: %s\n",timestamp);

            char output_file_name[200] = "scatter_iq_dump_";
            sensing_concat_timestamp_string(&output_file_name[16]);
            strcat(output_file_name,".dat");
            printf("name: %s\n",output_file_name);


  /* Open the file for writing.  If it exists, append to it;
     otherwise, create a new file.  */
  int fd = open (filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
  /* Compute the length of the timestamp string.  */
  size_t length = strlen (timestamp);
  /* Write the timestamp to the file.  */
  write (fd, timestamp, length);
  /* All done.  */
  close (fd);
  return 0;
}
