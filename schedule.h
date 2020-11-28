/* schedule.h -  Shared definitions between schedule.c and timer.c          */

#define MAX_SCRIPT 64

  struct Tim {
              time_t starttime;
              char command[MAX_SCRIPT];
             };
  struct Tim Timer;
