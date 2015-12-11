/* authors: - Josafat Piraquive
			- Harintsoa Ramanantoanina
 *          
 * 
 */


#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "vxWorks.h"
#include "taskLib.h"
#include "kernelLib.h"
#include "tickLib.h"
#include "time.h"
#include "sigLib.h"
#include "msgQLib.h"
#include "string.h"
#include "semLib.h"
#include "sysLib.h"

/* defines */
#define STACK_SIZE 20000
#define MAXSECONDS 100
#define MAXPERIOD 100
#define MAX_MSG_LEN 50
#define MAX_SIZE 2

/* task IDs*/
int t_producer_periodic;
int t_producer_aperiodic;
int t_consumer;


int g_p_message_number;
int g_ap_message_number;
char gsender_id[20] = "periodic:  ";
char* gmessage = "message";
unsigned char tx_message[MAX_SIZE];
unsigned char rx_message[MAX_SIZE];
unsigned char read_periodic;
SEM_ID reading_sema;

/*msg Queue ID respectively periodic
 * and aperiodic
 * */
MSG_Q_ID myMsg_P_Q_Id;
MSG_Q_ID myMsg_AP_Q_Id;

/*time global*/
struct timespec gmytime;

/* function declarations*/
void periodic(int period, int periodic_Q_size);
void aperiodic(int low, int up, int aperiodic_Q_size);
void timerHandlerPeriodic(timer_t callingtimer);
void timerHandlerAPeriodic(timer_t callingtimer);
void prep(unsigned char sender_ID, int number, unsigned char* message);
int produce(MSG_Q_ID msgQId, unsigned char msg[MAX_SIZE]);
void consume(int msgMax, int compute_time);
void display(unsigned char message[MAX_SIZE]);

int main(void)
{
  	struct timespec mytime;
	int nseconds = 0;
	int period = 0;
	int low_aperiod = 0;
	int up_aperiod = 0;
	int compute_time = 0;
	int periodic_Q_size = 0;
	int aperiodic_Q_size = 0;
	int nb_max_msg_toRead = 0;
	g_p_message_number = 0;
	g_ap_message_number = 0;
	read_periodic = 1;
	if (clock_gettime (CLOCK_REALTIME, &gmytime)==ERROR)
			printf("Error: clock_getTime \n");
	
	/* get simulation time*/
	printf("Enter overall simulation time [1-%d s]: ",MAXSECONDS);
	scanf("%d",&nseconds);	
	printf("Simulation %d seconds. \n\n", nseconds);
	
	
	/* depth of periodic queer*/
	printf("Enter depth of periodic queue [1-1000]: ");
	scanf("%d",&periodic_Q_size);
	printf("Depth of periodic queue set to %d entries \n \n",periodic_Q_size);
	
	
	/* depth of aperiodic queue*/
	printf("Enter depth of aperiodic queue [1-1000]: ");
	scanf("%d",&aperiodic_Q_size);
    printf("Depth of aperiodic queue set to %d entries \n \n",aperiodic_Q_size);
    	
		
	/* get the period for periodic producer*/
	printf("Enter period for periodic producer[1- %d s]: ", MAXPERIOD);
	scanf("%d",&period);
	printf("Period for the timer set to %ds. \n\n",period);

	/*lower bound for aperiodic timer*/
	printf("Enter lower bound for aperiodic timer [1 -100s]: ");
    scanf("%d",&low_aperiod);	
	printf("Lower bound for aperiodic timer set to %d s \n\n",low_aperiod);
	
	/*upper bound for aperiodic timer*/
	printf("Enter upper bound for aperiodic timer [1 -100s]: ");
	scanf("%d",&up_aperiod);	
	printf("upper bound for aperiodic timer set to %d s \n\n",up_aperiod);
	
	/*consumer computation time*/
	printf("Enter consumer computation time [1-100s]: ");
	scanf("%d",&compute_time);
	printf("Consumer computation time set to %d s \n\n", compute_time);
	
	/*max number of message read per consumer loop*/
	printf("Enter max number of messages read per consumer loop: ");
	scanf("%d", &nb_max_msg_toRead);
	printf("Max number of messages read per consumer loop set to %d \n\n", nb_max_msg_toRead);
	
	/* set clock to start at 0 */
	mytime.tv_sec = 0;
	mytime.tv_nsec = 0;
	gmytime.tv_sec = 0;
	gmytime.tv_nsec = 0;
	
	if (clock_settime(CLOCK_REALTIME, &mytime) < 0)
	    printf("Error clock_settime\n");
	else
	    printf("Current time set to %d sec %d ns \n\n",(int)mytime.tv_sec,(int)mytime.tv_nsec);
	
	/* set time slice to 100ms */
	kernelTimeSlice(6);
	
	/* spawn (create and start) */
	t_producer_periodic = taskSpawn("tPeriodic", 100, 0, STACK_SIZE,
					           (FUNCPTR)periodic, period, periodic_Q_size,
	                            0,0,0,0,0,0,0,0
							   );
	t_producer_aperiodic = taskSpawn("tAPeriodic", 100, 0, STACK_SIZE,
	                            (FUNCPTR)aperiodic,low_aperiod,up_aperiod, aperiodic_Q_size,
	                            0,0,0,0,0,0,0
	                           );
	t_consumer = taskSpawn("tconsumer", 100, 0, STACK_SIZE,
	                        (FUNCPTR)consume, nb_max_msg_toRead, compute_time,
	                        0,0,0,0,0,0,0,0
	                      );	 

       /* this is create to let the consumer know that the periodic or aperiodic producer have been write in the queue */
	reading_sema = semBCreate(SEM_Q_FIFO, SEM_FULL);
	
	/*run for given simulation time*/
	taskDelay(nseconds*sysClkRateGet());
	
	/* delete task*/
	taskDelete(t_producer_periodic);
	taskDelete(t_consumer);
	taskDelete(t_producer_aperiodic);
	/*delete semaphore*/
	semDelete(reading_sema);
	printf("Exiting. \n\n");
	return(0);
}

void periodic(int period, int periodic_Q_size)//, int nb_max_msg_toProduce, int msg_max_len, int priority)
{
    int i;
	timer_t ptimer;
	char* msg;
	unsigned char sender_ID = 0;
	struct itimerspec intervaltimer;
	
	myMsg_P_Q_Id = msgQCreate( periodic_Q_size, MAX_MSG_LEN,
								 MSG_Q_FIFO);
	/* create timer */
	if (timer_create(CLOCK_REALTIME, NULL, &ptimer)== ERROR)
	    printf("Error create_timer \n");
	else
	    printf("Timer for periodic screated. \n");
		
	/* connect timer to timer handler routine */
	if (timer_connect(ptimer, (VOIDFUNCPTR)timerHandlerPeriodic,(int)0)==ERROR)
	    printf("error connect_timer\n");
    else
	    printf("Timer handler for producer periodic connected. \n");
		
	/* set and arm timer*/
	intervaltimer.it_value.tv_sec = period;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = period;
	intervaltimer.it_interval.tv_nsec = 0;
	if (timer_settime(ptimer, TIMER_ABSTIME, &intervaltimer, NULL)== ERROR)
	    printf("Error set_timer \n");
    else
	    printf("Time for periodic producer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);
	while(1)
	{
	   prep(sender_ID, g_p_message_number, tx_message);
	   produce(myMsg_P_Q_Id, tx_message);
	   semTake(reading_sema, WAIT_FOREVER); /* periodic and the consumer */
	   read_periodic = 1;
	   semGive(reading_sema);
	   pause();
	}
}

void timerHandlerPeriodic(timer_t callingtimer)
{
    struct timespec mytime;
  	if (clock_gettime(CLOCK_REALTIME, &mytime)==ERROR)
	    printf("Error: clock_getTime \n");		
    taskResume(t_producer_periodic);
}

void aperiodic(int low, int up, int aperiodic_Q_size)
{
	int i;
	timer_t ptimer;
	struct itimerspec intervaltimer;
	int random_period;
	char* msg;
	unsigned char sender_ID = 1;
	myMsg_AP_Q_Id = msgQCreate( aperiodic_Q_size, MAX_MSG_LEN,
									 MSG_Q_FIFO);
	if (timer_create(CLOCK_REALTIME, NULL, &ptimer)== ERROR)
		printf("Error create_timer \n");
	else
		printf("Timer for aperiodic producer created. \n\n");
		
	if (timer_connect(ptimer, (VOIDFUNCPTR)timerHandlerAPeriodic,(int)0)==ERROR)
		printf("error connect_timer\n");
	else
		printf("Timer handler for aperiodic producer connected. \n");
	while(1)
	{
		 random_period = (int)((rand()%(up + 1 - low))+low);
		 intervaltimer.it_value.tv_sec = random_period;
		 intervaltimer.it_value.tv_nsec = 0;
		 intervaltimer.it_interval.tv_sec = random_period;
		 intervaltimer.it_interval.tv_nsec = 0;
		if (timer_settime(ptimer, TIMER_ABSTIME, &intervaltimer, NULL)== ERROR)
			printf("Error set_timer \n");
		else
			printf(" ");
		pause();
		prep(sender_ID, g_ap_message_number, tx_message);
		produce(myMsg_AP_Q_Id, tx_message);
		printf("                Time for aperiodic set to %d s \n", intervaltimer.it_interval.tv_sec);
		semTake(reading_sema, WAIT_FOREVER); /* aperiodic and consumer */
		read_periodic = 0;
		semGive(reading_sema);
		pause();
	}	
}

void timerHandlerAPeriodic(timer_t callingtimer)
{
	struct timespec mytime;
	if (clock_gettime(CLOCK_REALTIME, &mytime)==ERROR)
		printf("Error: clock_getTime \n");
	 taskResume(t_producer_aperiodic);
}

/*prepare msg to send*/
void prep(unsigned char sender_ID, int number, unsigned char* message)
{  
	unsigned char temp_msg[MAX_SIZE];
	int i;
	temp_msg[0] = sender_ID;
	temp_msg[1] = number;
	for(i = 0; i<MAX_SIZE ; i++)
	{
		message[i] = temp_msg[i];
	}	
}

/*produce message and put this in the msgQ*/
int produce(MSG_Q_ID msgQId, unsigned char msg[MAX_SIZE])
{
	int length = MAX_SIZE;
	if (msgQSend(msgQId, (char*)msg, length, NO_WAIT, MSG_PRI_NORMAL== ERROR)
				return 0;
	 else
	 {		
		if (msg[0] == 0) /* checking the sender ID */ 
		{ 
			/*semTake(reading_sema, WAIT_FOREVER);
			read_periodic = 1;
			semGive(reading_sema);*/
		    g_p_message_number += 1;
		}
		else
		    /*semTake(reading_sema, WAIT_FOREVER);
			read_periodic = 0;
			semGive(reading_sema);*/
		    g_ap_message_number += 1;
		display(msg);
	    return 1;
	}
}

void consume(int msgMax_to_read, int compute_time)
{
	int length = MAX_SIZE;
	int counter_periodic = 0;
	int counter_aperiodic = 0;
	unsigned int eat_p = 1;
	unsigned int eat_ap = 1;
	while(1)
	{	
		taskDelay(compute_time*sysClkRateGet()); /* simulation computation time in seconds */	
		//printf("read periodic: %d\n\n",read_periodic);
		if (clock_gettime (CLOCK_REALTIME, &gmytime)==ERROR) /* call the global time */
				{ printf("Error: clock_getTime \n");}
		if ((read_periodic == 1) && (eat_p == 1))
		{
			semTake(reading_sema, WAIT_FOREVER);
		    read_periodic = 2;
		    semGive(reading_sema);
			printf("counter periodic: %d \n\n", counter_periodic); 
			if(msgQReceive(myMsg_P_Q_Id, (char*)&rx_message, length+1, 
								WAIT_FOREVER)==ERROR)
				  printf("reading msgQ of pp cannot okkur");
			 else
			 {
		          printf("                    CONSUMER: message 00%u  from PERIODIC @ 00%d\n", rx_message[1]
						                             , (int)gmytime.tv_sec);	 
			 }
			 if(counter_periodic > msgMax_to_read)
			 {      
				    counter_periodic = 0;
				    eat_p = 0;   /* this is to control the max message read by the consumer */
				    printf("Go consume aperiodic \n\n");
			 }
+			 else
			      counter_periodic += 1;
			 eat_ap = 1;
		}
		else if((read_periodic == 0) && (eat_ap == 1))
		{
			 semTake(reading_sema, WAIT_FOREVER);
			 read_periodic = 2;
			 semGive(reading_sema);
			 printf("COUCOU I'm reading aperiodic Q \n\n");
			 if (msgQReceive(myMsg_AP_Q_Id, (char*)&rx_message, length+1, 
					     WAIT_FOREVER)==ERROR)
				  printf("reading msgQ of pa cannot okkur");
			 else
			 {
				  if (clock_gettime (CLOCK_REALTIME, &gmytime)==ERROR)
				  { printf("Error: clock_getTime \n");}
				
				  printf("                           CONSUMER: message 00%u  from APERIODIC @ 00%d \n", rx_message[1]
						            , (int)gmytime.tv_sec);
		     }
			if (counter_aperiodic > msgMax_to_read)
		    {	   
				counter_aperiodic = 0;
				eat_ap = 0;
				printf("Go consume periodic \n\n"); 
		    }
			else
			    counter_aperiodic += 1;
			eat_p = 1;
		 }
		 else
		    {
			   eat_p = 1;
			   eat_ap = 1;	
		    }
	}
}

void display(unsigned char message[MAX_SIZE])
{
	//struct timespec mytime;
	if (clock_gettime (CLOCK_REALTIME, &gmytime)==ERROR)
	   printf("Error: clock_getTime \n");
	if (message[0] == 0)
	{
	   //periodic
	   if (message[1] <= 9)
		   printf("PERIODIC: message 00%d @ 00%d\n", message[1], (int)gmytime.tv_sec);
	   else if(message[1]>= 10 && message[1]<=99)
	       printf("PERIODIC: message 0%d @ 00%d\n", message[1], (int)gmytime.tv_sec);
	   else
	       printf("PERIODIC: message %d @ 00%d\n", message[1], (int)gmytime.tv_sec);
	}
	else
	{
          //periodic
		if (message[1] <= 9)
		    printf("        APERIODIC: message 00%d @ 00%d \n", message[1], (int)gmytime.tv_sec);
		else if(message[1]>= 10 && message[1]<=99)
			printf("        APERIODIC: message 0%d @ 00%d \n", message[1], (int)gmytime.tv_sec);
		else
			printf("        APERIODIC: message %d  @ 00%d \n", message[1], (int)gmytime.tv_sec);
	}
}
