// x86_64 Linux implementation
#define _GNU_SOURCE  // need this for some linux features.
#include <sys/types.h>
#include <unistd.h>
#include <linux/unistd.h>/*for gettid*/
#include <sys/ipc.h>
#include <sys/sem.h> /*for sysV semaphore*/
#include <sys/time.h>
#include <pthread.h> /*FIXME: temporarily use pthread_mutex*/
#include <sched.h>   /*linux sheduler and affinity*/
#include <signal.h>  /*posix singal*/
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define SPMD_LEADER_STACK_SIZE (16*1024*1024)
#define SPMD_LEADER_STACK_ALGN (32)
#define SPMD_TASK_STACK_SIZE (64*1024*1024)
#define SPMD_TASK_STACK_ALGN (32)
#define SPMD_SPINLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define SPMD_MAXIMAL_FILE_LENGTH 80

#ifndef SPMD_SEM_KEY
#define SPMD_SEM_KEY  "/tmp/tmp"
#endif

#define __leader
#define __task
#define __spmd_export

/*variable parameter is unimplemented*/
#define SPMD_FIXED_PARAMETER

typedef pid_t             thread_t;
typedef pid_t             leader_t;
typedef pid_t             task_t;
typedef pthread_mutex_t   spinlock_t;
typedef void (*hook_handler_t)(void *);
typedef void (*task_entry_t)(void);

#ifdef SPMD_FIXED_PARAMETER 
typedef void (*task_func_t)(void * arg); 
#else
typedef void* task_func_t;
#endif

struct leader_struct;

typedef struct task_struct {
  int              tid;
  int              idx;
  unsigned short   oc;      /* sem num in sem_pe, indicated occupied */
  struct leader_struct *  ldr;

  void *           arg;
  FILE *           log_fd;
  int              counter;
}* task_struct_p;

typedef struct warp_struct {
  task_func_t     fn;      /* task function */
  hook_handler_t  hook;    /* hook function, call after wait */
  task_struct_p   tsks;    
  void *          hook_arg;  /* the argument for hook function */
  int             width;
  FILE *          log_fd;

  long            time_on_fly;
  long            time_in_reduce;
  long            time_in_wait;

  int             counter;
  struct timeval  last_stamp; /* the timestamp before a warp is fired up*/

  // helper data for intialization 
  struct tag_init_list {
    unsigned int stk_sz;   /* stack size per task */
    void **      stks;     /* stacks for children */
    thread_t *   wb_tsks;  /* write back task id to pool */
    int          sem_task_prep;
  } _init_list;

}* warp_struct_p;

typedef struct leader_struct{
  leader_t           gid;
  spinlock_t         lck;
  int                nr;   /* num. of task installed on warp */
  int                sem;  /* current linux kernel lacks syscall to wait for all thread group
			    * after discussed to tomida, i decided to simulate it using
			    * semaphore temporarily and hopefully it wouldn't hurt performance
			    * to much. the affect of this change from [DESIGN] will be evaluated
			    * in later experiment.
			    */
  int                sem_task;
  struct warp_struct warp;
}* leader_struct_p;

typedef struct spmd_thread_slot{
  int             warp_width;
  int             warp_size;

  void        **  leaders_stacks;
  void       ***  children_stacks;

  thread_t        * wb_leaders;      /* write back leaders in the slot to pool */
  thread_t        * wb_tasks;        /* write back tasks in the slot to pool */

  struct spmd_thread_slot * next; 
}* spmd_thread_slot_p;

typedef struct spmd_thread_pool
{
  int                nr_pe;            /* number of Processing Element(PE) */
  int                nr_slot;          /* number of Slot, a slot consists of the same leaders */
  int                nr_leader;        /* total number of leader thread in pool */
  int                nr_task;          /* total number of task thread in pool */
  int                nr_thread; 

  leader_struct_p    leaders;
  spmd_thread_slot_p slots;
  thread_t         * thr_leaders;      /* handler of native threads, role as leaders*/
  thread_t         * thr_tasks;        /* handler of native threads, role as tasks*/
  int                sem_pe;	       /* semaphore set of pe */
}* spmd_thread_pool_p;
