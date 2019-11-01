#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

volatile sig_atomic_t print_flag = false;

void alarm_handle(int sig) {
   if(sig == SIGALRM) {
      print_flag = true;
   }
}

void set_alarm(unsigned int seconds) {

}

int main() {

   int counter = 0;
   
   
   alarm(1);                  // before scheduling it to be called.

    for (;;) {
        if ( print_flag ) {
            printf( "Hello\n" );
            print_flag = false;
            
            if(counter < 3) {
               counter++;
               alarm( 1 );
            }
        }
    }
}
