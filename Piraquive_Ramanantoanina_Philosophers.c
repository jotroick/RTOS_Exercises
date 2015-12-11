/* authors: - Harintsoa Ramanantoanina
 *          - Josafat Piraquive
 * 
 */

#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "vxWorks.h"
#include "sysLib.h"
#include "kernelLib.h"

//defines
#define STACK_SIZE 20000
#define RIGHT i
#define LEFT (i+1)%n


/*global variable*/
int n; /* number of philo */
int* eatCounter; /* count how many time the philo ate */
int* ret; /* return value of creation of task - philo  */ 
int waiting_time; /* time that the philo should wait when he take the first fork*/ 
int overall_sim_time; /*   */ 
char* phi_name = "tphi"; 
/*resources*/
SEM_ID* fork;

/*fiunction declaration*/
void spawnPhilosopher(int number); /* creation of philo */ 
void destroy_task(int number);  /* destroy philo */
void createSemaphore(int number); 
void destroy_semaphore(int number);
void init_eatCounterArray(int size); 
void eating(int i); 
void philosopherStatusHandler(int i); /* no use */ 
void philosopher(int i); /* handle the status of philo think - eat */
void print(int* array, int size); 
void think(int i);

void spawnPhilosopher(int number)
{
   int idx;
   char* phi_name = "phi";
   ret = (int*) malloc(number*sizeof(int));
   for (idx = 0; idx < number ; idx++)
   {  
	   ret[idx] = taskSpawn(phi_name, 200, 0, STACK_SIZE,
				   (FUNCPTR)philosopher,idx,0,0,0,0,0,0,0,0,0);
   }
}
void destroy_task(int number)
{
    int idx;
	for(idx = 0; idx<number ; idx++)
	{
	   taskDelete(ret[idx]);	
	}
}

void createSemaphore(int number)
{
   int idx;
   fork = (SEM_ID*) malloc(number*sizeof(SEM_ID));
   for (idx = 0; idx < number; idx++)
   {
	  fork[idx] = semBCreate(SEM_Q_FIFO, 1);
   }
}

void destroy_semaphore(int number)
{
	int idx;
		for(idx = 0; idx<number ; idx++)
		{
		   semDelete(fork[idx]);	
		}
}

void init_eatCounterArray(int size)
{
	int idx;
	eatCounter= (int*)malloc(size*sizeof(int));
    for (idx = 0; idx< size ; idx++)
    {
		eatCounter[idx] = 0;	
	}	
}

void print(int* array, int size)
{
    int idx;
	printf("Eat counters: ");
    for(idx = 0; idx<size; idx++)
  	{
	   	printf("%d   ",array[idx]);
  	}
	printf("\n");
}


void eat(int i)
{
    printf("philosopher %d start eating \n",i);
}
void think(int i)
{
	printf ("philosopher %d start thinking \n",i);
}

void philosopher(int i)
{
	think(i);
	taskDelay(50);
	while(1)
	{
	    taskDelay(waiting_time);
		/* philosopher 0 - n-2 */ /* to take the RIGHT FORK*/
		if (i % n != n-1) /* if i am not the last philo */
		{
		   semTake(fork[RIGHT], WAIT_FOREVER);
		   taskDelay(waiting_time);
		   semTake(fork[LEFT], WAIT_FOREVER);
		   eat(i);
		   eatCounter[i] += 1;
		   taskDelay(10);
		}
		/*the last philosopher n-1*/ /* IF I AM THE LAST ONE */
		else
		{
		  semTake(fork[LEFT], WAIT_FOREVER); /* I SHOULD TAKE THE LEFT FIRST */
          taskDelay(waiting_time);
		  semTake(fork[RIGHT], WAIT_FOREVER);
		  eat(i);
		  eatCounter[i] += 1;
		  taskDelay(10);
		}
	        semGive(fork[RIGHT]);
		semGive(fork[LEFT]);
		think(i);
		taskDelay(50);
	}
}

int main (void)
{
	printf("Enter number of philosophers [3-20]: ");
	scanf("%d",&n);
	printf("\n");
	printf("Enter waiting time [1-100]ticks:");
	scanf("%d",&waiting_time);
    printf("\n");	
	printf("Enter overall simulation [1-100]s:");
	scanf("%d",&overall_sim_time);
	printf("\n");
	printf("simulating %d philosophers \n",n);	

    /*init eat_counter to 0*/
	init_eatCounterArray(n);
	kernelTimeSlice(6);
	
	/*spawn task (philosopher) according n*/	
	spawnPhilosopher(n);
	
   /*create semaphore*/
	createSemaphore(n);
	
	/*simulation time in sec*/
	taskDelay(overall_sim_time*sysClkRateGet());
	
	/*result of the philosopher dining*/
	printf("All philosophers are stopped\n");
	print(eatCounter,n);
	
	/*destructor*/
	destroy_task(n);
	destroy_semaphore(n);
	free(ret);
	free(eatCounter);
	free(fork);
	return(0);	
}
