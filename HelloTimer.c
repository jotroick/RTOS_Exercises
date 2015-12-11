/*************************************************************************/
/*  HelloTimer.c                                                              */
/*                                                                       */
/*************************************************************************/


/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "kernelLib.h"
#include "tickLib.h"
#include "time.h"
#include "sigLib.h"


/* defines */
#define STACK_SIZE  20000
#define MAXSECONDS  100
#define MAXPERIOD   100

/* task IDs */
int tidPeriodic;			


/* function declarations */
void Periodic(int);
void TimerHandlerPeriodic  (timer_t);   



/*************************************************************************/
/*  main task                                                            */
/*                                                                       */
/*************************************************************************/

int main (void) {

	struct timespec mytime;
	int    nseconds = 0;
	int    period = 0;

	/* get the simulation time */ 
	printf("\n\n");
	while ((nseconds<1) || (nseconds > MAXSECONDS)) {
	   printf ("Enter overall simulation time [1-%d s]: ", MAXSECONDS);
	   scanf ("%d", &nseconds);
	 };
	printf("Simulating for %d seconds.\n\n", nseconds);
	
	
	/* get the period for the timer */ 
	while ((period<1) || (period > MAXPERIOD)) {
		printf ("Enter period for the timer [1-%d s]: ", MAXPERIOD);
		scanf ("%d", &period);
	};
	printf("Period for the timer set to %ds. \n\n", period);
	
	
	/* set clock to start at 0 */
	mytime.tv_sec  = 0;
	mytime.tv_nsec = 0;
  
	if ( clock_settime(CLOCK_REALTIME, &mytime) < 0)
		  printf("Error clock_settime\n");
	else
		  printf("Current time set to %d sec %d ns \n\n", (int) mytime.tv_sec, (int)mytime.tv_nsec);


	/* set time slice to 100 ms */	
	kernelTimeSlice(6); 	
     
	/* spawn (create and start) task */
	tidPeriodic = taskSpawn ("tPeriodic", 100, 0, STACK_SIZE,
			   (FUNCPTR) Periodic, period, 0,0,0,0,0,0,0,0,0);
	
	
	/* run for given simulation time */
	taskDelay (nseconds*60);
	
	/* delete task */
	taskDelete (tidPeriodic);

	printf("Exiting. \n\n");
	return(0);
}




/*************************************************************************/
/*  task "tPeriodic"                                                     */
/*                                                                       */
/*************************************************************************/

void Periodic(int period) {

	int i;
	timer_t ptimer;
	struct  itimerspec intervaltimer;
	

	/* create timer */
	if ( timer_create (CLOCK_REALTIME, NULL, &ptimer) == ERROR)
		  printf("Error create_timer\n");
	else
		  printf("Timer created.\n");


	/* connect timer to timer handler routine */
	if ( timer_connect (ptimer, (VOIDFUNCPTR) TimerHandlerPeriodic, (int) 0)  == ERROR)
		  printf("Error connect_timer\n");
	else
		  printf("Timer handler connected.\n");


	/* set and arm timer */
	intervaltimer.it_value.tv_sec = period;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = period;
	intervaltimer.it_interval.tv_nsec = 0;

	if ( timer_settime (ptimer, TIMER_ABSTIME, &intervaltimer, NULL)  == ERROR)
		  printf("Error set_timer\n");
	else
		  printf("Timer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);


	/* idle loop */
	while (1)  pause();
     
}



/*************************************************************************/
/*  function "TimerHandlerPeriodic"                                      */
/*                                                                       */
/*************************************************************************/

void TimerHandlerPeriodic  (timer_t callingtimer) {
   
	struct timespec mytime;

	if ( clock_gettime (CLOCK_REALTIME, &mytime) == ERROR) 
		 printf("Error: clock_gettime \n");

	printf("current time: %d s, %d ms\n", (int) mytime.tv_sec, 
		(int) (mytime.tv_nsec/1000000));
	
}  

