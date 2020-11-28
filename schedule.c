/************************************************************************/
/* Program:     schedule                                                */
/* Purpose:     compute schedule, create timer file for use by timer    */
/* Copyright:   D. F. Cohoon Oct 1996 - All rights reserved             */
/* Copyright:   The greg procedure is by W. M. McKeeman (C)1986.        */ 
/*                                                                      */
/* Input:       schedule, timer, record, bug                            */
/*              -schedule is a text file with the following format :    */ 
/*                                                                      */
/*              Min    Hour     Day    Month   Day    Command   Comment */
/*               of     of       of     of      of                      */
/*              Hour   Day      Month  Year    Week                     */
/*              0-59,  0-23,    1-31,  1-12,   0-6; (Sunday=0)          */ 
/*                                                                      */
/*              If it looks like cron, then good. Astricks may be used  */
/*              just like cron. One BIG difference, if only minites are */
/*              used the script will be run  every X minutes.  Note the */
/*              commas and semicolons seperating minor and major        */
/*              catagories.                                             */
/*              -record is zero (0) to process entire schedule file or  */
/*              a number to re-process only one record in the file      */
/*                                                                      */
/* Output:      -timer is a file with the following format :            */
/*              <UNIX time_t><script file to start>                     */
/*                                                                      */
/* Description: The schedule file contains one line for each script to  */
/*              schedule. Example schedule entries are as follows:      */
/*              2,15,*,4,3; cleanup.sh  remove tmp files                */
/*              will run cleanup.sh at 15:02 every Tuesday in April. Or */
/*              360,*,*,*,*; watch.sh  watch for new files              */
/*              will run watch.sh every six hours.                      */    
/*              The comment field can be any kind of free-form comment, */
/*              which will show up when the schedule is displayed to    */
/*              stdout.  stderr is used to show bad things od debug.    */
/*              The timer file contains one line for each script to run.*/
/*              The first field is a UNIX time_t of when the script     */
/*              should run and the second field is the script name.     */
/*              Typcially the whole file is processed and whenver a     */   
/*              script is run, it is re-scheduled with the record       */
/*              parameter.                                              */
/*                                                                      */
/* Scripts:     Each script will be given the ORACLE_SID and ORACLE_HOME*/
/*              for each database in the /etc/oratab file with the auto */
/*              set to Y.  They will return zero for good and non-zero  */
/*              for bad with a file called <script>.mail to be used as  */
/*              notification. See the events script for more info.      */
/*                                                                      */
/* Modified:    19 Nov 1996 Don Cohoon - Add hours,days,day-o-month,etc.*/ 
/*              11 Feb 1997 Don Cohoon - Fix high-low boot schedule and */
/*                                       multiple schedules per minute. */
/*              02 Sep 1997 Don Cohoon - Fix high-low for hour          */
/************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "schedule.h"

/* Greg parameters */
#define greg_year(x) (x[1])
#define greg_mon(x)  (x[2])
#define greg_mday(x) (x[3])
#define greg_wday(x) (x[4])
#define greg_yday(x) (x[6])
#define greg_jday(x) (x[7])
#define success 0
#define inconsistent 1
#define incomplete 2
#define systemfailure 3

/* Misc macros */
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define bugger (bug > 4)

/* Globals */
int bug;

static int monthtab[2][13] = {
              {0,31,28,31,30,31,30,31,31,30,31,30,31},
              {0,31,29,31,30,31,30,31,31,30,31,30,31}
             };

/*----------------------------------------------------------------------------*/
  int leapyear (year)
  int year;
  { /* Given the year(yyyy); return 0 if not a leap year, 1 if it is */
    return (year%4 == 0 && year%100 != 0 || year%400 == 0);
  } /* of leapyear */
/*----------------------------------------------------------------------------*/
  int get_value(c,l,h)
  int c,l,h;
  { /* given currrent, low and high, return current if within range
     or low if current <= l~h (where l==h) otherwise negative low */
     if (l == h) {
        if (c > l) {
           return((l * -1));
        } else {
           return(l);
        }
     } else {
       if ((c >= l) && (c <= h)) {
          return(c);
       } else {
          return((l * -1));
       }
     }
  } /* end of get_value */
/******************************************************************************/
int future_date(minutes,hourl,hourh,dayl,dayh,monthl,monthh,dowl,dowh,tp,next)
int minutes,hourl,hourh,dayl,dayh,monthl,monthh,dowl,dowh,next;
struct tm **tp;
{ /* Given a number of variables, return a timestamp into the future */

  int hour,day,month,year,dow;
  int i,leap;
  int jumped = 0; /* ignore hour and minute if day jumps ahead */
  char *ts[8];
  long jd;
  time_t tyme;
  struct tm *tp2;

  /* validate variables */
  if (hourl == -1) {
     fprintf(stderr,"Hour cannot be * while others are not\n");
     return(incomplete);
  }
  
  /* get the current time */
  tyme = time(&tyme);
  tp2 = localtime(&tyme);
  if bugger {
      fprintf(stderr,"Current time is %d/%d/%d,%d:%d\n",
         tp2->tm_year+1900,tp2->tm_mon+1,tp2->tm_mday,tp2->tm_hour,tp2->tm_min);
      fprintf(stderr,"Input time is min=%d,hourl=%d,hourh=%d,dayl=%d,dayh=%d,monthl=%d,monthh=%d,dowl=%d,dowh=%d\n",
              minutes,hourl,hourh,dayl,dayh,monthl,monthh,dowl,dowh);
  }
  /* validate acording to todays date */
  year = (tp2->tm_year+1900);
  leap = leapyear(year);
  if (monthl == -1) {
     month = (tp2->tm_mon+1);
  } else {
     month = get_value(tp2->tm_mon+1,monthl,monthh);
     if (month != (tp2->tm_mon+1)) {
        jumped = 1;
     }
     if (month < 0) { /* add a year */
        month = (month * -1);
        /* add year */
        year++;
        leap = leapyear(year);
     }
  }
  if bugger
     fprintf(stderr,"After month %d/%d/%d\n",month,tp2->tm_mday,year);
  if (jumped) {
     day = MAX(dayl,1);
  } else if (dayl == -1) {
     day = tp2->tm_mday;
     if ((hourl == hourh) && next) {  /* next day */
        jumped = 1;
        /* add day */
        day++;
        if ((dayh != -1 && day > dayh)
           || day > monthtab[leap][month]) { /* add a month */
           /* add month */
           day=MAX(1,dayl);
           month++;
           if ((monthh != -1 && month > monthh) 
              || month > 12) { /* add a year */
              month=MAX(1,monthl);
              /* add year */
              year++;
              leap = leapyear(year);
           }
        }
     }
  } else {
     day = get_value(tp2->tm_mday,dayl,dayh);
     if (day != tp2->tm_mday) {
        jumped = 1;
     }
     if (day < 0) { /* add a month */
        day = (day * -1);
        /* add month */
        month++;
        if ((monthh != -1 && month > monthh) 
           || month > 12) { /* add a year */
           month=MAX(1,monthl);
           /* add year */
           year++;
           leap = leapyear(year);
        }
     }
  }
  if bugger
     fprintf(stderr,"After day %d/%d/%d\n",month,day,year);
  if (jumped) {
     hour = MAX(hourl,0);
  } else if (hourl == -1) {
     hour = tp2->tm_hour;
  } else {
     if ((hourl != hourh) && next) { /* next hour */
	 tp2->tm_hour++;
     }
     hour = get_value(tp2->tm_hour,hourl,hourh);
     if (hour > tp2->tm_hour) {
        jumped = 1;
     }
     if (hour < 0) { /* add a day */
        hour = (hour * -1);
        jumped = 1;
        /* add day */
        day++;
        if ((dayh != -1 && day > dayh)
           || day > monthtab[leap][month]) { /* add a month */
           /* add month */
           day=MAX(1,dayl);
           month++;
           if ((monthh != -1 && month > monthh) 
              || month > 12) { /* add a year */
              month=MAX(1,monthl);
              /* add year */
              year++;
              leap = leapyear(year);
           }
        }
     }
  } 
  if bugger
     fprintf(stderr,"After  hour %d/%d/%d %d:%d\n",month,day,year,hour,tp2->tm_min);
  if (!jumped) {
    if (minutes < tp2->tm_min) {
       hour++;
       if (hour > hourh || hour > 23) {
          hour=MAX(0,hourl);
          /* add day */
          day++;
          if ((dayh != -1 && day > dayh)
             || day > monthtab[leap][month]) { /* add a month */
             /* add month */
             day=MAX(1,dayl);
             month++;
             if ((monthh != -1 && month > monthh) 
                || month > 12) { /* add a year */
                month=MAX(1,monthl);
                /* add year */
                year++;
                leap = leapyear(year);
             }
          }
       }
    }
  }
  if bugger
     fprintf(stderr,"After minute %d/%d/%d %d:%d\n",month,day,year,hour,minutes);

  /* Get the julian date for date */
  for (i=0;i < 8; i++) {
      ts[i] = (char *)malloc(sizeof(int)+1);
      strcpy(ts[i],"0");
  }
  sprintf(greg_year(ts),"%d",year);
  sprintf(greg_mon(ts),"%d",month);
  sprintf(greg_mday(ts),"%d",day);
  if bugger
     fprintf(stderr,"Requested date is %s/%s/%s %d:%d\n",
             greg_mon(ts), greg_mday(ts),greg_year(ts),
             hour,minutes); 
  if (i = greg(4,&ts) != 0) {
     fprintf(stderr,"Greg error #%d getting first date\n",i);
     return(i);
  }
  if bugger
     fprintf(stderr,"Julian date for %s/%s/%s is %s\n",
             greg_mon(ts), greg_mday(ts),greg_year(ts),greg_jday(ts)); 

  /* find day of week match into the future */
  if (dowl != -1) {
     while ( (dowl == dowh && dowl != (atoi(greg_wday(ts))-1)) ||
             (atoi(greg_wday(ts)) < (dowl+1) &&
              atoi(greg_wday(ts)) > (dowh+1)) ) { 
       jd = atol(greg_jday(ts)) + 1;
       sprintf(greg_jday(ts),"%ld",jd);

       /*  get the new date info */
       for (i=0;i < 7; i++)
           strcpy(ts[i],"0");
       if (i = greg(8,&ts) != 0) {
          fprintf(stderr,"Greg error #%d getting future date\n",i);
          return(i);
       }
       if bugger
          fprintf(stderr,"Day of week for %s/%s/%s is %d\n",
                greg_mon(ts), greg_mday(ts),greg_year(ts),
                atoi(greg_wday(ts))-1); 
     }
  }
 
  /* create tm structure */
  tp2->tm_sec  = 0;
  tp2->tm_min  = minutes;
  tp2->tm_hour = hour;
  tp2->tm_mday = atoi(greg_mday(ts));
  tp2->tm_mon  = atoi(greg_mon(ts)) -1;
  tp2->tm_year = atoi(greg_year(ts)) -1900;
  tp2->tm_wday = atoi(greg_wday(ts)) -1;
  tp2->tm_yday = atoi(greg_yday(ts)) -1;
  *tp = tp2;
  
  return success;
} /* end of future_date */
/******************************************************************************/
int future_minutes(minutes,tp)
long minutes;
struct tm **tp;
{ /* Given a number of minutes; return a timestamp X minutes into the future */

  int i;
  char *ts[8];
  long hours = 0;
  long jd;
  int days = 0;
  time_t tyme;
  struct tm *tp2;

  /* get the current time */
  tyme = time(&tyme);
  tp2 = localtime(&tyme);
  if bugger
      fprintf(stderr,"Current time is %d/%d/%d,%d:%d\n",
         tp2->tm_year+1900,tp2->tm_mon+1,tp2->tm_mday,tp2->tm_hour,tp2->tm_min);

  /* Now convert minutes to hours and days */
  hours = minutes / 60;
  days = hours / 24;
  if (days > 0)
     hours = hours % 24;
  minutes = minutes % 60;

  /* add the time into the future */
  hours = hours + tp2->tm_hour;
  minutes = minutes + tp2->tm_min;
  if (minutes > 59) {
     hours = hours + (minutes / 60);
     minutes = minutes % 60;
  }
  if (hours > 23) {
     days = days + (hours / 24);
     hours = hours % 24;
  }
  if bugger
     fprintf(stderr,"Going forward %d days, to %d:%d\n",
            days,hours,minutes); 

  /* get the julian day for today */
  for (i=0;i < 8; i++) {
      ts[i] = (char *)malloc(sizeof(int)+1);
      strcpy(ts[i],"0");
  }
  sprintf(greg_year(ts),"%d",tp2->tm_year+1900);
  sprintf(greg_mon(ts),"%d",tp2->tm_mon+1);
  sprintf(greg_mday(ts),"%d",tp2->tm_mday);
  if (i = greg(4,&ts) != 0) {
     fprintf(stderr,"Greg error #%d getting today's date\n",i);
     return(i);
  }
  if bugger
     fprintf(stderr,"Julian date for %s/%s/%s is %s\n",
             greg_mon(ts), greg_mday(ts),greg_year(ts),greg_jday(ts)); 

  /* add the days into the future */
  jd = atol(greg_jday(ts)) + days;
  sprintf(greg_jday(ts),"%ld",jd);

  /*  get the new date info */
  for (i=0;i < 7; i++)
      strcpy(ts[i],"0");
  if (i = greg(8,&ts) != 0) {
     fprintf(stderr,"Greg error #%d getting future date\n",i);
     return(i);
  }
  if bugger
     fprintf(stderr,"Julian date for %s/%s/%s is %s\n",
             greg_mon(ts), greg_mday(ts),greg_year(ts),greg_jday(ts)); 
 
  /* create tm structure */
  tp2->tm_sec  = 0;
  tp2->tm_min  = minutes;
  tp2->tm_hour = hours;
  tp2->tm_mday = atoi(greg_mday(ts));
  tp2->tm_mon  = atoi(greg_mon(ts)) -1;
  tp2->tm_year = atoi(greg_year(ts)) -1900;
  tp2->tm_wday = atoi(greg_wday(ts)) -1;
  tp2->tm_yday = atoi(greg_yday(ts)) -1;
  *tp = tp2;
  
  return 0;
} /* end of future_minutes */

/******************************************************************************/
int main(argc, argv)
int argc;
char **argv;
{

  char buf[80];
  char nbr[3];
  char *seps = ",;";
  char timeout[4];
  char *tok;
  FILE *fSch;
  FILE *fTim;
  int dayl,dayh,dowl,dowh,monthl,monthh,hourl,hourh;
  int i,j;
  int inx = 0;
  int recno; 
  long minutes = 0;
  struct tm ft;
  struct tm *ftp;
  char *spaces = " ";

  if (argc < 4) {
     fprintf(stderr,"Usage: %s <schedule> <timer> <record> [bug]\n",argv[0]);
     fprintf(stderr,"schedule - input schedule file\n");
     fprintf(stderr,"timer    - output timer file\n");
     fprintf(stderr,"record   - zero for all, or number for one\n");
     fprintf(stderr,"bug      - print debug messages\n");
     fflush(stderr);
     exit(1);
  }
  bug = argc;
  ftp = &ft;
  recno = atoi(argv[3]);
  if (recno == 0) {
     strcpy(timeout,"wb");
     printf("All records in schedule will be processed\n");
  } else {
     strcpy(timeout,"r+b");
     printf("Record number %d in schedule will be processed\n",recno);
  }    
  if ((fSch = fopen(argv[1], "r")) == NULL) {
     fprintf(stderr,"Schedule file not available (%s)\n",argv[1]);
     fflush(stderr);
     exit(1);
  } else {
  if ((fTim = fopen(argv[2], timeout)) == NULL) {
     fprintf(stderr,"Timer file not available (%s)\n",argv[2]);
     fflush(stderr);
     exit(1);
  } else {
     /* get the future */
     while (fgets(buf,80,fSch) != NULL) {
       inx++;
       if ((inx == recno) || (recno == 0)) {
           tok = strtok (buf, seps);
           if (tok != "*") { 
              minutes = atol(tok);
           } else {
              minutes = 0;
           }
           hourl = hourh = -1;
           tok = strtok (NULL, seps);
           if (*tok != '*') { 
              for (i=0;;tok++) {
                  nbr[i] = *tok;
                  i++;
                  if (*tok == '-') {
                     nbr[i] = '\0';
                     hourl = atoi(nbr);
                     i = 0;
                     strcpy(nbr,"\0");
                  } else if (*tok == '\0') {
                     if (hourl == -1) 
                        hourl = hourh = atoi(nbr);
                     else
                        hourh = atoi(nbr);
                     break;
                  }
              }
           }
           dayl = dayh = -1;
           tok=0;
           tok = strtok (NULL, seps);
           if (*tok != '*') { 
              for (i=0;;tok++) {
                  nbr[i] = *tok;
                  i++; 
                  if (*tok == '-') {
                     nbr[i] = '\0';
                     dayl = atoi(nbr);
                     i = 0;
                  } else if (*tok == '\0') {
                     if (dayl == -1) 
                        dayl = dayh = atoi(nbr);
                     else
                        dayh = atoi(nbr);
                     break;
                  }
              }
           }
           monthl = monthh = -1;
           tok = strtok (NULL, seps);
           if (*tok != '*') { 
              for (i=0;;tok++) {
                  nbr[i] = *tok;
                  i++; 
                  if (*tok == '-') {
                     nbr[i] = '\0';
                     monthl = atoi(nbr);
                     i = 0;
                  } else if (*tok == '\0') {
                     if (monthl == -1) 
                        monthl = monthh = atoi(nbr);
                     else
                        monthh = atoi(nbr);
                     break;
                  }
              }
           }
           dowl = dowh = -1;
           tok = strtok (NULL, seps);
           if (*tok != '*') { 
              for (i=0;;tok++) {
                  nbr[i] = *tok;
                  i++; 
                  if (*tok == '-') {
                     nbr[i] = '\0';
                     dowl = atoi(nbr);
                     i = 0;
                  } else if (*tok == '\0') {
                     if (dowl == -1) 
                        dowl = dowh = atoi(nbr);
                     else
                        dowh = atoi(nbr);
                     break;
                  }
              }
           }
           printf("------------------------------------------------\n");
           printf("Parse time is min=%d,hourl=%d,hourh=%d,dayl=%d,dayh=%d,monthl=%d,monthh=%d,dowl=%d,dowh=%d\n",
              minutes,hourl,hourh,dayl,dayh,monthl,monthh,dowl,dowh);
           if (recno != 0) {
               for (i=1;i < inx;i++)  /* re-position Timer file */
                   fread(&Timer,sizeof(Timer),1,fTim);
               fflush(fTim);
           }
           tok = strtok (NULL, spaces);
           strcpy(Timer.command,tok); /* Get script after re-syncing Timer */
           if ((hourl) < 0) { 
              i = future_minutes(minutes,&ftp);
           } else {
              i = future_date(minutes,hourl,hourh,dayl,dayh,
                              monthl,monthh,dowl,dowh,&ftp,recno);
           }
           /* Voilla! Print the future date nice and pretty. */
           if (i == 0) {
               printf("(%d)%s scheduled for %s",
                      inx,Timer.command,asctime(ftp));
               Timer.starttime = mktime(ftp);
               fwrite(&Timer,sizeof(Timer),1,fTim);
           } else {
               fprintf(stderr,"Error #%d in greg, rec %d, script %s\n",
                              i,inx,Timer.command);
               fflush(stderr);
               exit(1);
           } /* error on future_date */
        } /* inx==recno || recno==0 */
     } /* end of sch fgets */
  } /* end of good sch file open */
  } /* end of good tim file open */
  return(0);
}
