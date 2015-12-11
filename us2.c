/**************************************************************
*  authors: -Josafat Piraquive
*           -Harintsoa Ramanantoanina
* edf algorithm implementation
***************************************************************/

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
#include "errno.h"
#include "sysLib.h"
#include "msgQLib.h"

/*Define*/
#define MAX_PERIODIC    3
#define MAX_PERIOD    100
#define MAX_DEADLINE  100
#define MAX_PRIO      110
#define MIN_PRIO      254
#define STACK_SIZE   200000
#define MAX_BYTE        5
#define NB_BYTE         4
#define MAX_MSG_LENGTH  1000000
#define WAITING         0
#define READY           1
#define RUNNING         2
#define NULL            0



/* task parameter*/
typedef struct task_paramater
{
    int period;
    //int deadlines;
    int compute_time;
    unsigned int id;
    int idx;
} task_param;

/*queuing task
 *
 * ref: pdf embedded software
 * EDF scheduler implementation
 * */
typedef struct queue
{
   int abs_deadline;
   int period;
   int compute_time;
   int id;
   int idx;
   unsigned char status; /*WAIT - READY - RUN*/
}queue_param;

int t_scheduler;
MSG_Q_ID ready_Q_Id;
MSG_Q_ID Q_event;
SEM_ID sema_, new_task_, send, receive, no_preemption;
int new_task_to_run;
int task_finish;
struct  timespec gmytime;
int t_id_ran ;
int count_periodic[3];

int task_timer_set;
int task_scheduler;

/*Function declaration*/
void periodic(task_param* t_p, queue_param* task_in_queue, int idx, int n);
int create_periodic_task(task_param* t_p, queue_param* task_in_queue,int n);
void deleteTask(task_param* t_p, int n);
void spawn_task_scheduler(task_param* t_params, queue_param* task_in_queue, int nbtask);
void set_up_parameter(task_param* t_params, queue_param* task_in_queue ,int nbtask);
void scheduler(task_param* t_params, int nbtask, queue_param* task_in_queue);
void setup_timer();
//void sorted(queue_param* t_p, int n);
queue_param* sorted( queue_param* t_p, int n);
void display(queue_param* qp ,int n);
//void queued_tasks(MSG_Q_ID msgQID, queue_param* qp , int n );
void queued_tasks(MSG_Q_ID msgQID, queue_param* qp , int n );
void wake_up(timer_t callingtimer);
void update(queue_param task_in_queue);
void run(MSG_Q_ID msgQId);
double compute_utilization(task_param* t_params, int n);
unsigned char isFeasible(task_param* t_params, int n);
void assign_priority(queue_param* pending_task, int nbtask);

double compute_utilization(task_param* t_params, int n)
{
    /*TODO*/
    return 0.0;
}

unsigned char isFeasible(task_param* t_params, int n)
{
    double u = compute_utilization(t_params, n);
    if (u >= 1)
       return 0;
    else
       return 1;
}

/**************************************************************
* MAIN
*
***************************************************************/

int main()
{
    struct  timespec mytime;
    int idx;
    int i;
    task_param task_parameters[3];
    queue_param task_pending[3];

    int nbTask = 0; /*number of periodic task*/
    struct timespec gmytime;
    new_task_to_run = 0;
    task_finish = 1;
    t_id_ran = 0;
    sema_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    new_task_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
   // task_parameters = (task_param*)malloc(nbTask*sizeof(task_param));
    //task_pending = (queue_param*)malloc(nbTask*sizeof(queue_param));

    while(nbTask < 1 || nbTask > MAX_PERIODIC )
    {
         printf("Enter number of periodic task [1 - %d]: ", MAX_PERIODIC);
         scanf("%d",&nbTask);
    }

    /*TODO
     * free task_parameters
     * */


   for (idx = 0 ; idx <nbTask ; idx++ )
   {
        task_parameters[idx].period = 0;
        task_parameters[idx].compute_time = 0;
   }

    for (idx = 0; idx< nbTask ; idx++)
    {
        /*period of task*/
        do
        {
           printf("\n enter period of task %d: ",idx);
           scanf("%d",&task_parameters[idx].period);
        }while((task_parameters[idx].period < 1) ||
               (task_parameters[idx].period > MAX_PERIOD));

        //task_parameters[idx].period = task_parameters[idx].period;
        /*execution time of task*/
        do
        {
           printf("enter execution time of task %d: ",idx);
           scanf("%d",&task_parameters[idx].compute_time);
        }while((task_parameters[idx].compute_time < 1) ||
               (task_parameters[idx].compute_time > task_parameters[idx].period));
    }
  /* set clock to start at 0 */
  mytime.tv_sec  = 0;
  mytime.tv_nsec = 0;
  gmytime.tv_sec = 0;
  gmytime.tv_nsec = 0;

  if (clock_settime(CLOCK_REALTIME, &mytime) < 0)
      printf("Error clock_settime\n");
  else
      printf("Current time set to %d sec %d ns \n\n",
          (int) mytime.tv_sec, (int)mytime.tv_nsec);



         for(i=0; i<3 ; i++)
         {
             count_periodic[i] = 1;
         }
    /*TODO test feasibility*/

    no_preemption = semBCreate(SEM_Q_FIFO, SEM_FULL);
    new_task_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    ready_Q_Id = msgQCreate(nbTask, MAX_MSG_LENGTH ,MSG_Q_FIFO);
    Q_event = msgQCreate(nbTask,MAX_MSG_LENGTH, MSG_Q_FIFO);


/* TASK CREATION */

      /* TASK TIMER SET*/
    task_timer_set = taskSpawn("task_timer", 101, 0, STACK_SIZE,
            (FUNCPTR)setup_timer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    task_scheduler = taskCreate("t_scheduler", 102, 0, STACK_SIZE,
         (FUNCPTR)scheduler, (int)task_parameters, nbTask, (int)task_pending, 0, 0, 0, 0, 0, 0, 0);


    create_periodic_task(task_parameters, task_pending, nbTask);

    taskDelay(40*sysClkRateGet());
    printf("edf exiting ...\n\n");
    deleteTask(task_parameters, nbTask);
    taskDelete(t_scheduler);
    semDelete(sema_);
    semDelete(send);
    semDelete(receive);
    semDelete(new_task_);

    msgQDelete(ready_Q_Id);
    free(task_parameters);
    free(task_pending);
   return 0;
}

/**************************************************************
* SET TIMER
*
***************************************************************/
void setup_timer()
{
    timer_t ptimer;
    struct itimerspec intervaltimer;
    /* create timer */
    if ( timer_create(CLOCK_REALTIME, NULL, &ptimer) == ERROR)
        printf("Error create_timer\n");

    if ( timer_connect(ptimer, (VOIDFUNCPTR)wake_up, (int)0) == ERROR )
            printf("Error connect_timer\n");

    /* set and arm timer timeout each 1ms*/
    intervaltimer.it_value.tv_sec = 0;
    intervaltimer.it_value.tv_nsec = 1;
    intervaltimer.it_interval.tv_sec = 0;
    intervaltimer.it_interval.tv_nsec = 1;

    if ( timer_settime(ptimer, TIMER_ABSTIME, &intervaltimer, NULL) == ERROR )
        printf("Error set_timer\n");
         /* idle loop */
    while(1) pause();
}

void wake_up(timer_t callingtimer)
{
    taskActivate(task_scheduler);
}


/**************************************************************
* SCHEDULER
*
***************************************************************/

void scheduler(task_param* t_params, int nbtask, queue_param* task_in_queue)
{
   int i;
   int arrivaltime[MAX_PERIODIC];
   unsigned int sorted_deadline;
    /**
     * TODO initialize the struct
     *
     */
       for (i=0 ; i <nbtask ; i++)
            {
                arrivaltime[i]=0;
            }
    set_up_parameter(t_params, task_in_queue ,nbtask);
     while(1)
     {

          if (task_finish == 1)
          {
             semTake(new_task_ , WAIT_FOREVER);
             task_finish = 0;
             semGive(new_task_);
            sorted_a( task_in_queue, nbtask);
            for ( i=0 ; i<nbtask ; i++)
            {
                arrivaltime[i] = task_in_queue[i].abs_deadline - task_in_queue[i].period;

            }
            sorted_deadline =0;
            for (i=0 ; i<nbtask; i++)
            {

                    for (j=i+1; j<nbtask; j++)
                {
                    if( arrivaltime[i] == arrivaltime[j])
                    {
                            sorted_deadline = 1 ;
                            break;
                    }
                }
            }
            if (sorted_deadline = 1){
                sorted_deadline = 0;
                sorted( task_in_queue, nbtask);
            }


            if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");

            while(task_in_queue[0].abs_deadline > gmytime.tv_sec){
                 if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");
            }

             assign_priority(task_in_queue, nbtask);
             semGive(no_preemption);
          }
          taskSuspend(0);
     }
}

void set_up_parameter(task_param* t_params, queue_param* task_in_queue ,int nbtask)
{
    int idx;
    for (idx = 0; idx < nbtask ; idx++)
    {
        task_in_queue[idx].status = READY;
        task_in_queue[idx].abs_deadline = t_params[idx].period;
        task_in_queue[idx].id = t_params[idx].id;
        task_in_queue[idx].compute_time = t_params[idx].compute_time;
        task_in_queue[idx].idx = t_params[idx].idx;
        task_in_queue[idx].period = t_params[idx].period;
    }
    queued_tasks(ready_Q_Id, task_in_queue , nbtask);
}


queue_param* sorted_a( queue_param* t_p, int n)
{
    int j, min_deadline, id, period, deadline ,compute_time, idx;
    int i = 0;
    unsigned char status;
    queue_param* tmp;


    //("sorting...\n");
    for(i = 0; i < n ; i++)
    {

        arrival_time = t_p[i].abs_deadline - t_p[i].period;

        for(j = i+1; j < n ; j++)
        {

            if(arrival_time > (t_p[j].abs_deadline - t_p[j].period))
            {

                arrival_time = t_p[j].abs_deadline - t_p[j].period;
                id = t_p[j].id;
                deadline = t_p[j].abs_deadline;
                compute_time = t_p[j].compute_time;
                idx = t_p[j].idx;
                status = t_p[j].status;
                period = t_p[j].period;

                t_p[j].id = t_p[i].id;
                t_p[j].abs_deadline = t_p[i].abs_deadline;
                t_p[j].compute_time = t_p[i].compute_time;
                t_p[j].idx = t_p[i].idx;
                t_p[j].status = t_p[i].status;
                t_p[j].period = t_p[i].period;

                t_p[i].id = id;
                t_p[i].abs_deadline = deadline;
                t_p[i].compute_time = compute_time;
                t_p[i].idx = idx;
                t_p[i].status = status;
                t_p[i].period = period;
             }

        }
    }

    tmp = t_p;
    return tmp;
}


queue_param* sorted( queue_param* t_p, int n)
{
    int j, min_deadline, id, period, deadline ,compute_time, idx;
    int i = 0;
    unsigned char status;
    queue_param* tmp;
    //("sorting...\n");
    for(i = 0; i < n ; i++)
    {
        min_deadline = t_p[i].abs_deadline;
        for(j = i+1; j < n ; j++)
        {

            if(min_deadline > t_p[j].abs_deadline)
            {

                min_deadline = t_p[j].abs_deadline;
                id = t_p[j].id;
                deadline = t_p[j].abs_deadline;
                compute_time = t_p[j].compute_time;
                idx = t_p[j].idx;
                status = t_p[j].status;
                period = t_p[j].period;

                t_p[j].id = t_p[i].id;
                t_p[j].abs_deadline = t_p[i].abs_deadline;
                t_p[j].compute_time = t_p[i].compute_time;
                t_p[j].idx = t_p[i].idx;
                t_p[j].status = t_p[i].status;
                t_p[j].period = t_p[i].period;

                t_p[i].id = id;
                t_p[i].abs_deadline = deadline;
                t_p[i].compute_time = compute_time;
                t_p[i].idx = idx;
                t_p[i].status = status;
                t_p[i].period = period;
             }

        }
    }

    tmp = t_p;
    return tmp;
}


void assign_priority(queue_param* pending_task, int nbtask)
{
    int i, id, old_priority, new_priority, increment_priority;
    increment_priority = 0;
    for (i = 0 ; i<nbtask ; i++ )
    {
        id = pending_task[i].id;
        if (id < 0)
            printf("error id task");
        taskPriorityGet(id, &old_priority);
        new_priority = MAX_PRIO + increment_priority;
        if (old_priority != new_priority)
        {
            printf("new task's priority: %d \n",new_priority);
            taskPrioritySet(id, new_priority);
        }

        //taskActivate(id);
        increment_priority++;
        //printf("task %d %d\n",i,new_priority);
    }
    for(i = 0; i<nbtask; i++)
    {
        taskActivate(pending_task[i].id);
    }
}




void deleteTask(task_param* t_p, int n)
{
    int i;
    for (i = 0; i<n ; i++)
    {
        taskDelete(t_p[i].id);
    }
}

int create_periodic_task(task_param* t_p, queue_param* task_in_queue,int n)
{
    /* create periodic tasks
     * priority:  110 - 254*/
    int idx;
    queue_param* qp;
    for (idx = 0; idx < n; idx++)
    {
        t_p[idx].id = taskCreate("tPeriodic_" + (char)idx, 255, 0, STACK_SIZE,
                (FUNCPTR)periodic,(int)t_p, (int)task_in_queue, idx, n, 0, 0, 0, 0, 0, 0);
        printf("task_id  %d:  %d \n", idx , t_p[idx].id);
        t_p[idx].idx = idx;
    }

    return 1;
}

/*TODO
 *
 * raha vao ny voalohany no kely dia mety
 * raha vao no voalohany ny ngeza dia tsy mety
 * */

void queued_tasks(MSG_Q_ID msgQID, queue_param* qp , int n )
{
     queue_param* tmp;
     tmp = sorted(qp, n);
}

void update(queue_param task_in_queue)
{
    switch(task_in_queue.status)
    {
        case RUNNING:
              task_in_queue.status = READY;
              break;

        case READY:
              task_in_queue.status = RUNNING;
              break;

        default: break;
    }
}
/*This is FINEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/
void periodic(task_param* t_p, queue_param* task_in_queue, int identify, int n)
{
    int sleep = 0;
    int tick = 0 ;
    int counter = 0;
    int compute_time;
    int oldtick = 0;
    int mypriority;
    int period;
    char* event;
    while(1)
    {
        semTake(no_preemption, WAIT_FOREVER);
        printf("idx, task_in_Q[idx], task_in_Q_[0]: %d %d %d\n", idx, task_in_queue[idx].abs_deadline, task_in_queue[0].abs_deadline);
        task_in_queue[task_in_queue[idx].idx].status = RUNNING;
        if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");
        compute_time = t_p[idx].compute_time;
        period = t_p[idx].period;
        taskPriorityGet(taskIdSelf(), &mypriority);
        // printf("idx: %d  counter_periodic: %d  deadline: %d\n",idx, count_periodic[idx]
                                                     //,t_p[idx].deadlines);
        printf("task_%d | execution started at %ds  period: %d    exec_time: %d\n", taskIdSelf(), gmytime.tv_sec, task_in_queue[0].abs_deadline, compute_time);

        counter = 0;

        while (counter < compute_time *sysClkRateGet())
        {
            oldtick = tick;
            tick = tickGet();
            if (tick == oldtick + 1)
                  counter += 1;
        }
        if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");
        sleep = task_in_queue[0].abs_deadline - gmytime.tv_sec;

        printf("task_%d | execution finished at %ds prio  %d\n\n", taskIdSelf(), gmytime.tv_sec, mypriority);

        semTake(new_task_ , WAIT_FOREVER);
        task_finish = 1;

        //task_in_queue[0].abs_deadline = count_periodic[idx]*task_in_queue[0].period;
        if (identify != task_in_queue[0].idx)
        {
            printf("Happiness \n");
        }
        printf()
        task_in_queue[0].abs_deadline += task_in_queue[0].period;
        count_periodic[identify] += 1;
        semGive(new_task_);
        //taskDelay(sleep*sysClkRateGet());
        taskSuspend(0);
     }
}

void display(queue_param* qp ,int n)
{
    int idx;
    printf("queue parameters \n");
    for (idx = 0; idx < n; idx++)
        printf("task_%d:  %d \n",idx, qp[idx].abs_deadline);
    printf("\n\n");
}
