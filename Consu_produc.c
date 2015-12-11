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
#include "msgQLib.h"
#include "time.h"
#include "sigLib.h"


/* defines */
#define STACK_SIZE  20000
#define MAXSECONDS  100
#define MAXPERIOD   100
#define MAXMESSAGES 100

#define MSG_Q_PRIORITY 100
#define MAX_MSG_LEN 100
#define MAXLENGTH   1000

/* task IDs */
int tidPeriodic;
int tidAperiodic;
int Consumer;

/* function declarations */
void Periodic(int);   /* function returns nothing*/
void TimerHandlerPeriodic  (timer_t);   /* function returns nothing*/



/*************************************************************************/
/*  main task                                                            */
/*                                                                       */
/*************************************************************************/

int main (void) {

	struct timespec mytime;
	int    nseconds = 0;
	int    period_qlength = 0; /* periodic queue*/
	int    aperiod_qlength = 0; /* aperiodic queue */
	int    period_p = 0; /* periodic producer */
	int    aperiod_l = 0; /* aperiodic lower */
	int    aperiod_u = 0; /* aperiodic upper */
	int    consumer_t = 0; /* consumer time  */
	int    period = 0;

	/* get the simulation time */
	printf("\n\n");
	while ((nseconds<1) || (nseconds > MAXSECONDS)) {
	   printf ("Enter overall simulation time [1-%d s]: ", MAXSECONDS);
	   scanf ("%d", &nseconds);
	 };
	printf("Simulating for %d seconds.\n\n", nseconds);

			/* get the DEPTH for the PERIODIC QUEUE */
	while ((period_q<1) || (period_q > MAXLENGTH)) {
		printf ("Enter depth of periodic queue [1-%d ]: ", MAXLENGTH);
		scanf ("%d", &period_qlength);
	};
	printf("Depth of periodic queue set to %ds. \n\n", period_q);

		/* get the DEPTH For the APERIODIC QUEUE */
	while ((aperiod_q < 1) || (aperiod_q> MAXLENGTH)) {
		printf ("Enter depth of aperiodic queue [1-%d ]: ", MAXLENGTH);
		scanf ("%d", &aperiod_qlength);
	}
	printf("Depth of aperiodic queue set to %ds. \n\n", aperiod_q);

	/* get the period for the PERIODIC PRODUCER */
	while ((period_p<1) || (period_p > MAXPERIOD)) {
		printf ("Enter period for periodic producer [1-%d s]: ", MAXPERIOD);
		scanf ("%d", &period_p);
	};
	printf("Period for periodic producer set to %ds. \n\n", period_p);

	/* get the lower bound fot the aperiodioc timer*/
    while ((aperiod_l<1)) {
		printf ("Enter lower bound for aperiodic timer" [1-%d s]: ", MAXPERIOD);
		scanf ("%d", &aperiod_l);
	};
	printf("Lower bound for aperiodic timer set to %ds. \n\n", aperiod_l);

	/* get the upper bound fot the aperiodioc timer*/
	/*   CAMBIAR EL RANGO!!!*/
    while ((aperiod_u<1) || (aperiod_u > MAXPERIOD)) {
		printf ("Enter upper bound for aperiodic timer" [1-%d s]: ", MAXPERIOD);
		scanf ("%d", &aperiod_u);
	};
	printf("Upper bound for aperiodic timer set to %ds. \n\n", aperiod_u);

   /*  get the consumer computation time   */
    while ((consumer_t<1) || (consumer_t > MAXPERIOD)) {
		printf ("Enter consumer computation time" [1-%d s]: ", MAXPERIOD);
		scanf ("%d", &consumer_t;
	};
	printf("Consumer computation time set to %ds. \n\n", consumer_t);

    /* get the max number of messades read per consumer loop */
    while ((consumer_m<2) || consumer_m > MAXMESSAGES)) {
        printf ("Enter max number of messages read per consumer loop " [2-%d s]: ", MAXMESSAGES);
		scanf ("%d", &consumer_t;
	};
	printf("Max number of messages read per consumer loop set to %ds. \n\n", consumer_m);



	/* set clock to start at 0 */
	mytime.tv_sec  = 0;
	mytime.tv_nsec = 0;

	if ( clock_settime(CLOCK_REALTIME, &mytime) < 0)
		  printf("Error clock_settime\n");
	else
		  printf("Current time set to %d sec %d ns \n\n", (int) mytime.tv_sec, (int)mytime.tv_nsec);


	/* set time slice to 100 ms */
	kernelTimeSlice(6);

	/* spawn (create and start) task */ /* ( name, priority, options, stacksize, main, arg1, ...arg10 );
  */
	tidPeriodic = taskSpawn ("tPeriodic", 100, 0, STACK_SIZE,
			   (FUNCPTR) Periodic, period_q, 0,0,0,0,0,0,0,0,0);

    tidAperiodic = taskSpawn ("tAperiodic", 100, 0, STACK_SIZE,
			   (FUNCPTR) Aperiodic, period, 0,0,0,0,0,0,0,0,0);

    tidConsumer = taskSpawn ("tConsumer", 100, 0, STACK_SIZE,
			   (FUNCPTR) Consumer, period, 0,0,0,0,0,0,0,0,0);

    /* create of queue */
    MSG_Q_ID myMsgQId_1;
    MSG_Q_ID myMsgQId_2;


	/* run for given simulation time */
	taskDelay (nseconds*60);

	/* delete task */
	taskDelete (tidPeriodic);
	taskDelete (tidAperiodic);
	taskDelete (tidConsumer);

	printf("Exiting. \n\n");
	return(0);

}




/*************************************************************************/
/*  task "tPeriodic"                                                     */
/*                                                                       */
/*************************************************************************/

void Periodic(int period_p) {

	int i;
	int ID;
	ID = 0;
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
	intervaltimer.it_value.tv_sec = period_p;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = period_p;
	intervaltimer.it_interval.tv_nsec = 0;

	if ( timer_settime (ptimer, TIMER_ABSTIME, &intervaltimer, NULL)  == ERROR)
		  printf("Error set_timer\n");
	else
		  printf("Timer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);

   /* create of the message queue */

    if ((myMsgQId_1 = msgQCreate (period_qlength, MAX_MSG_LEN, MSG_Q_PRIORITY))== NULL)
        return (ERROR);

	/* idle loop */
	while (1)
	{

	        char* strID;
	        sprintf(strID, "%d" , ID);
	        char MESSAGE[3] =  {"PERIODIC:","message", strID}
                /* send a normal priority message, blocking if queue is full */
            if (msgQSend (myMsgQId_1, MESSAGE, sizeof (MESSAGE), WAIT_FOREVER,
            MSG_PRI_NORMAL) == ERROR)
            return (ERROR);
            ID += 1;

	}  //pause();

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


/*************************************************************************/
/*  task "tAPeriodic"                                                     */
/*                                                                       */
/*************************************************************************/

void Aperiodic(int ) {


	int i;
	int r =0;
	int ID;
	ID = 0;
	int range;
	timer_t ptimer;
	struct  itimerspec intervaltimer;


	/* create timer */
	if ( timer_create (CLOCK_REALTIME, NULL, &ptimer) == ERROR)
		  printf("Error create_timer_APERIODIC\n");
	else
		  printf("Timer created FOR APERIODIC.\n");


	/* connect timer to timer handler routine */
	if ( timer_connect (ptimer, (VOIDFUNCPTR) TimerHandlerPeriodic, (int) 0)  == ERROR)
		  printf("Error connect_timer APERIODIC\n");
	else
		  printf("Timer handler connected. APERIODIC\n");

    range = (int) ((rand()%(aperiod_u+1-aperiod_l))+aperiod_l);

	/* set and arm timer */
	intervaltimer.it_value.tv_sec = range;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = range;
	intervaltimer.it_interval.tv_nsec = 0;

	if ( timer_settime (ptimer, TIMER_ABSTIME, &intervaltimer, NULL)  == ERROR)
		  printf("Error set_timer\n");
	else
		  printf("Timer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);


   /* create of the message queue */

    if ((myMsgQId_2 = msgQCreate (aperiod_qlength, MAX_MSG_LEN, MSG_Q_PRIORITY))== NULL)
        return (ERROR);

	/* idle loop */
	while (1)
	{

	        char* strID;
	        sprintf(strID, "%d" , ID);
	        char MESSAGE[3] =  {"APERIODIC:","message", strID}
                /* send a normal priority message, blocking if queue is full */
            if (msgQSend (myMsgQId_2, MESSAGE, sizeof (MESSAGE), WAIT_FOREVER,
            MSG_PRI_NORMAL) == ERROR)
            return (ERROR);
            ID += 1;

	}  //pause();



}


/*************************************************************************/
/*  task "tConsumer"                                                     */
/*                                                                       */
/*************************************************************************/

/* read the messages fromo queue */
/* TO DO*/

void Consumer(int period) {

    char msgBuf[MAX_MSG_LEN];
        /* get message from queue; if necessary wait until msg is available */

        if (msgQReceive(myMsgQId, msgBuf, MAX_MSG_LEN, WAIT_FOREVER) == ERROR)
        return (ERROR);

        /* SET TO READ FROM ONE QUEUE TO ANOTHER */
        /* DELAY OR SLEEP SIGNAL FOR THE COMPUTATION TIME OF THE COSUMER!!!*/

        /* display message */
        printf ("Message from task 1:\n%s\n", msgBuf);




}


