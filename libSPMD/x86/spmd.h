// x86_64 Linux implementation
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <linux/unistd.h>/*for gettid*/
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

#define SPMD_LEADER_STACK_SIZE (2*1024*1024)
#define SPMD_LEADER_STACK_ALGN (16)
#define SPMD_TASK_STACK_SIZE (16*1024)
#define SPMD_TASK_STACK_ALGN (16)
#define SPMD_SPINLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#ifndef SPMD_SEM_KEY
#define SPMD_SEM_KEY  "/home/liu"
#endif

#define __leader
#define __task
#define __spmd_export

#define FIXED_PARAMETER

typedef pid_t             thread_t;
typedef pid_t             leader_t;
typedef pid_t             task_t;
typedef pthread_mutex_t     spinlock_t;
typedef void (*hook_handler_t)(void);
typedef void (*task_entry_t)(void *);

#ifdef FIXED_PARAMETER 
typedef void (*task_function_t)(void * ret, void * arg0, void * arg1);
#else
typedef void* task_function_t;
#endif

typedef struct task_struct {
  task_t           tid;
  unsigned short   oc;     /*sem num, indicated occupied*/
  leader_t         gid;    /*group id*/
  void *           args[3];   
}* task_struct_p;

typedef struct task_environment {
  leader_t         gid;
  task_function_t  fn;
  void *           args[3];
}* task_environment_p;

typedef struct warp_struct {
  leader_t        gid; 
  int             nr;      /* number of task thread */
  void *          fn;      /* task function */
  hook_handler_t  hook;    /* hook function, call after wait */
  task_struct_p   tsks;    
}* warp_struct_p;

typedef struct warp_init_struct {
  leader_t        gid; 
  int             nr;        /* number of task thread */
  void *          fn;        /* task function */
  unsigned int    stk_sz;    /* stack size per task */
  void **         stks;      /* stacks for children */
  thread_t *      wb_tsks;   /* write back task id to pool */
}* warp_init_struct_p;

typedef struct leader_struct{
  leader_t           gid;
  spinlock_t         lock;
  int                oc;   /* occupied */
  int                nr;
  struct warp_struct warp;
}* leader_struct_p;

typedef struct spmd_thread_slot{
  int             warp_width;
  int             warp_size;

  void        **  leaders_stacks;
  void       ***  children_stacks;

  thread_t        * wb_leaders;      /* write back leaders in the slot to pool */
  thread_t        * wb_tasks;        /* write back tasks in the slot to pool */
  struct spmd_thread_slot * next;    /* point to finer slot */
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
