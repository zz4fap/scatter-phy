#include "srslte/utils/timer.h"

int timer_register_callback(void (*callback)(int, siginfo_t *, void *)) {
  if(sigaction(SIGALRM, &(struct sigaction){ .sa_sigaction = callback, .sa_flags = SA_SIGINFO }, 0) < 0) {
    TIMER_ERROR("Error registering callback.\n", 0);
    return -1;
  }
  TIMER_PRINT("Callback registered.\n", 0);
  return 0;
}

int timer_init(timer_t *timer_id) {
  struct sigevent sev = {
    .sigev_notify = SIGEV_SIGNAL,
    .sigev_signo = SIGALRM
  };

  sev.sigev_value.sival_ptr = timer_id;
  if(timer_create(CLOCK_REALTIME, &sev, timer_id) < 0) {
    TIMER_ERROR("Error creating a timer.\n", 0);
    return -1;
  }

  return 0;
}

int timer_set(timer_t *timer_id, time_t t) {
  if(timer_settime(*timer_id, 0, &(struct  itimerspec const){ .it_value={t,0} } , NULL) < 0) {
    TIMER_ERROR("Error setting the requested timer.\n", 0);
    return -1;
  }

  return 0;
}

int timer_disarm(timer_t *timer_id) {
  if(timer_settime(*timer_id, 0, &(struct  itimerspec const){ .it_value={0,0} } , NULL) < 0) {
    TIMER_ERROR("Error disarming the requested timer.\n", 0);
    return -1;
  }

  return 0;
}
