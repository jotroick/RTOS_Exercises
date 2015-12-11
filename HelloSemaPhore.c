/*************************************************************************/
/*  HelloSemaPhore.c                                                     */
/*                                                                       */
/*************************************************************************/


/* includes */
#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"

/* defines */
#define DELAY_TICKS  60
#define STACK_SIZE   20000
#define MAXSECONDS   100

/* task IDs */
int tidHello;			
int tidSema;
int tidPhore;

/* Semaphore IDs */
SEM_ID Sema_HelloSema;	
SEM_ID Sema_SemaPhore;		
SEM_ID Sema_PhoreHello;	

/* function declarations */
void Hello (void);
void Sema  (void);
void Phore (void);



/*************************************************************************/
/*  main task                                                            */
/*                                                                       */
/*************************************************************************/

int main (void) {

	int nseconds = 0;
	
	/* get the simulation time */ 
	while ((nseconds<1) || (nseconds > MAXSECONDS)) {
		printf ("Enter overall simulation time [1-%d s]: ", MAXSECONDS);
		scanf ("%d", &nseconds);
    };

	printf("\nSimulating HelloSemaPhore for %d seconds ...\n\n", nseconds);
	
	/* create binary semaphores */
	Sema_HelloSema  = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
	Sema_SemaPhore  = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
	Sema_PhoreHello = semBCreate (SEM_Q_FIFO, SEM_FULL);
	
    
	/* spawn (create and start) tasks */
	tidHello = taskSpawn ("tHello", 200, 0, STACK_SIZE, 
		(FUNCPTR) Hello, 0,0,0,0,0,0,0,0,0,0);

	tidSema = taskSpawn ("tSema", 200, 0, STACK_SIZE,
		(FUNCPTR) Sema, 0,0,0,0,0,0,0,0,0,0);

	tidPhore = taskSpawn ("tPhore", 200, 0, STACK_SIZE,
		(FUNCPTR) Phore, 0,0,0,0,0,0,0,0,0,0);


	/* run for the given simulation time */
	taskDelay (nseconds*60);
	
	/* delete tasks and semaphores */
	taskDelete (tidHello);
	taskDelete (tidSema);
	taskDelete (tidPhore);
	semDelete (Sema_HelloSema);
	semDelete (Sema_SemaPhore);
	semDelete (Sema_PhoreHello);
		
	printf("\n\nHelloSemaPhore stopped.\n");	
	return(0);
}   



/*************************************************************************/
/*  task "tHello"                                                        */
/*                                                                       */
/*************************************************************************/

void Hello (void)   {
	while (1) {
			semTake (Sema_PhoreHello, WAIT_FOREVER);
			taskDelay (DELAY_TICKS);
	            printf("Hello ");
			semGive (Sema_HelloSema);
	};
}

/*************************************************************************/
/*  task "tSema"                                                         */
/*                                                                       */
/*************************************************************************/

void Sema (void)   {
	while (1) {
			semTake (Sema_HelloSema, WAIT_FOREVER);
			printf("Sema");
			semGive (Sema_SemaPhore);
	};
}

/*************************************************************************/
/*  task "tPhore"                                                        */
/*                                                                       */
/*************************************************************************/

void Phore (void)   { 
	while (1) {
		semTake (Sema_SemaPhore, WAIT_FOREVER);
		printf("phore!\n");
		semGive (Sema_PhoreHello);
	};
}

