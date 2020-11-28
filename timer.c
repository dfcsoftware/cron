/************************************************************************/
/* Program:     timer                                                   */
/* Purpose:     read timer file and wait for shortest time, start script*/
/* Copyright:   D. F. Cohoon Oct 1996 - All rights reserved             */
/* Input:       timer, bug                                              */
/*              timer is a file with the following format :             */
/*              <UNIX time_t><script file to start>                     */
/* Output:      Start the script associated with shortest time and      */
/*              return the record number. -1 is an error.               */
/* Description: The timer file contains one line for each script to run.*/
/*              The first field is a UNIX time_t of when the script     */
/*              should run and the second field is the script name.     */
/*              Typcially the whole file is processed and whenver a     */   
/*              script is run, it is re-scheduled with the record       */
/*              parameter.                                              */
/* Scripts:     Each script will be given the ORACLE_SID and ORACLE_HOME*/
/*              for each database in the /etc/oratab file with the auto */
/*              set to Y.  They will return zero for good and non-zero  */
/*              for bad with a file called <script>.mail to be used as  */
/*              notification. See the events script for more info.      */
/* Modified:                                                            */ 
/************************************************************************/
#include <stdio.h>
#include <time.h>
#include "schedule.h"

int main(argc, argv)
int argc;
char *argv[];
{
#define  bugger (bug > 2)

  char script[MAX_SCRIPT];
  long sleeptime;
  FILE *fTim;
  int bug;
  int inx = 0;
  int next_inx = 0;
  time_t now;
  time_t smallest;

  if (argc < 2) {
     fprintf(stderr,"Usage: %s <timer file> [bug]\n",argv[0]);
     exit(-1);
  }
  if ((fTim = fopen(argv[1], "r")) == NULL) {
     fprintf(stderr,"Error: could not open timer file (%s)\n",argv[1]);
     exit(-1);
  }
  bug = argc;
  time(&now);
  if bugger
     printf("time_t is %d bytes long\n",sizeof(time_t));
  /* smallest = 4294967295; */
  smallest =  0x7fffffff;
  if bugger
     printf("smallest is %ld\n",smallest);
  while (fread(&Timer,sizeof(Timer),1,fTim) != NULL)  {
        inx++;
        if (Timer.starttime < smallest) {
           smallest = Timer.starttime;
           strcpy(script,Timer.command);
           next_inx = inx;
        }
        if bugger {
           printf("Time is %ld for %s\n",Timer.starttime,Timer.command);
           printf("smallest is %ld\n",smallest);
        }
  }
  close(fTim);
  if bugger
     printf("now is %ld\n",now);
  if (now < smallest) {
      sleeptime = difftime(smallest,now);
      printf("Sleeping for %ld seconds to start %s\n",sleeptime,script);
      sleep(sleeptime); 
  }
  printf("Time to start %s\n",script);
  return (next_inx);
}
