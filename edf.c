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
#define MAX_PRIO      100
#define STACK_SIZE   20000
#define MAX_BYTE        5
#define NB_BYTE         4
#define MAX_MSG_LENGTH  1000000
#define WAITING         0
#define READY           1
#define RUNNING         2
#define NULL            0
#define MALLOC( _dstptr, _number) \
      if( NULL == (_dstptr = calloc(_number, sizeof(_dstptr*)))){\
        perror("allocation failed");\
        exit(EXIT_FAILURE);\
      }



/* task parameter*/
typedef struct task_paramater
{
    int period;
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
   int compute_time;
   unsigned int id;
   int idx;
   unsigned char status; /*WAIT - READY - RUN*/
}queue_param;

int t_id_Timer_mux;
MSG_Q_ID ready_Q_Id;
SEM_ID sema_, new_task_, no_preemption;
int new_task_to_run;
int update_ready_task;
struct  timespec gmytime;
int t_id_ran ;


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

void spawn_task_scheduler(task_param* t_params, queue_param* task_in_queue,int nbtask)
{
    t_id_Timer_mux = taskSpawn("tTimerMux", 10, 0, STACK_SIZE,
         (FUNCPTR)scheduler, (int)t_params, nbtask, (int)task_in_queue, 0, 0, 0, 0, 0, 0, 0);
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
    /* create periodic tasks */
    int idx;
    queue_param* qp;
    for (idx = 0; idx < n; idx++)
    {
        t_p[idx].id = taskCreate("tPeriodic_" + (char)idx, 10, 0, STACK_SIZE,
                (FUNCPTR)periodic,(int)t_p, (int)task_in_queue, idx, n, 0, 0, 0, 0, 0, 0);
        printf("task_id  %d:  %d \n", idx , t_p[idx].id);
        t_p[idx].idx = idx;
    }

    return 1;
}

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
    }
    queued_tasks(ready_Q_Id, task_in_queue , nbtask);
    setup_timer();
}

/*TODO
 *
 * raha vao ny voalohany no kely dia mety
 * raha vao no voalohany ny ngeza dia tsy mety
 * */
queue_param* sorted( queue_param* t_p, int n)
{
    int j, min_deadline, id, period, compute_time, idx;
    int i = 0;
    unsigned char status;
    queue_param* tmp;

    for(i = 0; i < n ; i++)
    {
        min_deadline = t_p[i].abs_deadline;
        for(j = i+1; j < n ; j++)
        {
            printf("iteration \n");
            if(min_deadline >= t_p[j].abs_deadline)
            {
                printf("permute \n");
                min_deadline = t_p[j].abs_deadline;

                id = t_p[j].id;
                period = t_p[j].abs_deadline;
                compute_time = t_p[j].compute_time;
                idx = t_p[j].idx;
                status = t_p[j].status;

                t_p[j].id = t_p[i].id;
                t_p[j].abs_deadline = t_p[i].abs_deadline;
                t_p[j].compute_time = t_p[i].compute_time;
                t_p[j].idx = t_p[i].idx;
                t_p[j].status = t_p[i].status;

                t_p[i].id = id;
                t_p[i].abs_deadline = period;
                t_p[i].compute_time = compute_time;
                t_p[i].idx = idx;
                t_p[i].status = status;
             }

        }
    }
    tmp = t_p;
    return tmp;
}

void queued_tasks(MSG_Q_ID msgQID, queue_param* qp , int n )
{
     unsigned int id_tmp[1];
     queue_param* tmp;

     //semTake(sema_, WAIT_FOREVER);
     tmp = sorted(qp, n);
     id_tmp[0] = tmp[0].id;
     if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                         printf("error");
     printf("task idx %d , %d   %d\n",tmp[0].id,tmp[0].idx, gmytime.tv_sec);
    // semGive(sema_);

     if (msgQSend(msgQID, (char*)id_tmp, NB_BYTE,
            NO_WAIT, MSG_PRI_NORMAL)==ERROR)
          printf("error sending task ... \nn");
}


void run(MSG_Q_ID msgQId)
{
    unsigned int id[1];
    semTake(no_preemption, WAIT_FOREVER);
    if(msgQReceive(msgQId, (char*)&id,
            MAX_BYTE, WAIT_FOREVER)==ERROR)
         printf("Q cannot OKKUR! \n");
    //printf("task activate : %d   \n",id[0]);
    if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                             printf("error");
    printf("task should run... %d \n ",id[0], gmytime.tv_sec);
    taskActivate(id[0]);
    semGive(no_preemption);
}
void scheduler(task_param* t_params, int nbtask, queue_param* task_in_queue)
{
    int id_task_to_be_ran;
    set_up_parameter(t_params, task_in_queue ,nbtask);
    int actual_time = 0;
    while(1)
    {
        if(update_ready_task == 1)
            {

                semTake(new_task_ , WAIT_FOREVER);
                update_ready_task = 0;
                semGive(new_task_);
                //taskSuspend(task_in_queue[0].id);
                queued_tasks(ready_Q_Id, task_in_queue , nbtask );
                run(ready_Q_Id);
                //semGive(no_preemption);
            }
        pause();
    }
}
void wake_up(timer_t callingtimer)
{
    taskResume(t_id_Timer_mux);
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
void periodic(task_param* t_p, queue_param* task_in_queue, int idx, int n)
{
    int sleep = 0;
    int tick = 0 ;
    int counter = 0;
    int compute_time;
    int oldtick = 0;
    while(1)
    {
        if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");
        printf("task_%d | execution started at %d \n", taskIdSelf(), gmytime.tv_sec);
        compute_time = task_in_queue[task_in_queue[idx].idx].compute_time;

        while (counter < compute_time *sysClkRateGet())
        {
            oldtick = tick;
            tick = tickGet();
            if (tick == oldtick + 1)
                  counter += 1;
        }
        counter = 0;

        if(clock_gettime(CLOCK_REALTIME, &gmytime)== ERROR)
                    printf("error");
        sleep = task_in_queue[task_in_queue[idx].idx].abs_deadline - gmytime.tv_sec;
        task_in_queue[task_in_queue[idx].idx].abs_deadline += t_p[t_p[idx].idx].period;
        printf("task_%d | execution finished at %d\n\n", taskIdSelf(), gmytime.tv_sec);
        semTake(new_task_ , WAIT_FOREVER);
        update_ready_task = 1;
        semGive(new_task_);

        taskDelay(sleep*sysClkRateGet());
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

int main()
{
    struct  timespec mytime;
    int idx;
    task_param* task_parameters;
    queue_param* task_pending;
    int nbTask = 0; /*number of periodic task*/
    struct timespec gmytime;
    new_task_to_run = 0;
    update_ready_task = 1;
    t_id_ran = 0;
    sema_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    new_task_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    task_parameters = (task_param*)malloc(nbTask*sizeof(task_param));
    task_pending = (queue_param*)malloc(nbTask*sizeof(queue_param));


    /*MALLOC(task_param, nbTask);
    MALLOC(queue_param, nbTask);*/
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
    /*TODO test feasibility*/
    sema_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    no_preemption = semBCreate(SEM_Q_FIFO, SEM_FULL);
    new_task_ = semBCreate(SEM_Q_FIFO, SEM_FULL);
    ready_Q_Id = msgQCreate(nbTask, MAX_MSG_LENGTH ,MSG_Q_FIFO);

    spawn_task_scheduler(task_parameters, task_pending, nbTask);
    create_periodic_task(task_parameters, task_pending, nbTask);

    taskDelay(15*sysClkRateGet());
    printf("edf exiting ...\n\n");
    deleteTask(task_parameters, nbTask);
    taskDelete(t_id_Timer_mux);
    semDelete(sema_);
    semDelete(no_preemption);
    semDelete(new_task_);

    msgQDelete(ready_Q_Id);
    free(task_parameters);
    free(task_pending);
   return 0;
}
