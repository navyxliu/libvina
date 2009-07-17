// This file is not supposed to be distributed.
#include "warp.h"
// x86_64 Linux implementation
#include <sys/types.h>
#include <unistd.h>
#include <sys/sem.h> /*for sysV semaphore*/
#include <pthread.h> /*FIXME: temporarily use pthread_mutex*/
#include <sched.h>   /*linux sheduler and affinity*/
#include <signal.h>  /*posix singal*/
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define SPMD_LEADER_STACK_SIZE (2*1024*1024)
#define SPMD_LEADER_STACK_ALGN (16)
#define SPMD_TASK_STACK_SIZE (4*1024)
#define SPMD_TASK_STACK_ALGN (16)
#define __leader
#define __task
#define __spmd_export

typedef pid_t             thread_t;
typedef pid_t             leader_t;
typedef pid_t             task_t;
typedef pthread_mutex     spinlock_t;
typedef void (*hook_handler_t)(void);
typedef void (*task_entry_t)(void *);
typedef void (*task_funcion_t)(...);

typedef struct task_struct {
  task_t           task_id;
  unsigned short   oc;     /*sem num, indicated occupied*/
  leader_t         gid;    /*group id*/
  void *           args;   /*variable-length arguments*/
}* task_struct_p;

typedef struct task_environment {
  leader_t         gid;
  task_function_t  fn;
  void *           args;
}* task_environment_p;

typedef struct warp_struct {
  leader_t        id; 
  spin_lock_t     lock;    /*protect oc, rsv_tbl*/
  int             oc;      /*occupied*/
  unsigned int    stk_sz;  /*stack size per task*/
  void *          fn;      /*task function*/
  void **         stks;    /*stacks for children*/
  hook_handler_t  hook;    /*hook function, call after wait*/
  int             nr;      /*number of task thread*/
  task_struct_p * rsv_tbl; /*reservation table*/
}* warp_struct_p;

typedef struct spmd_thread_slot{
  int             warp_width;
  int             warp_size;

  void        **  leaders_stacks
  void       ***  children_stacks;

  thread        * threads;         /*write back to pool*/
  struct spmd_thread_slot * next;  /*point to finer slot*/
}* smpd_thread_slot_p;

typedef struct spmd_thread_pool
{
  int                nr_pe;
  int                nr_slot;
  smpd_thread_slot_p slots;
  thread_t         * threads;      /* handler of all native threads*/
}* spmd_thread_pool_p;

//globals
struct spmd_thread_pool the_pool;

static int
read_line_of(FILE * f, int n/*lines*/)
{
  int ch;
  for (; n-- > 0; ) {
    while( (ch=fgetc(f)) != '\n' && (ch != EOF) );
  }
  return ch != EOF;
}

static int 
probe_nr_processor()
{
  FIFO cpuinfo;
  char buffer[80];
  int idx = 0;
  int cores;

  cpuinfo = fopen("/proc/cpuinfo", "r");
  if ( cpuinfo == NULL || read_line_of(cpuinfo, 11/*skip front 11 lines*/)) {
    perror("open /proc/cpuinfo");
    return -1;
  }

  /*the #12 line*/
  fgets(buffer, 79, cpuinfo);
  assert(0 == strncmp(buffer, "cpu cores", 9)
	 && "wrong lines of cpuinfo");
  while ( ':' != buffer[idx++]);
  assert( 1 == sscanf(buffer+idx, "%d", &cores)
	  && "wrong line of cpuinfo");

  return cores;
}

static void
spinlock_init(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_init(lck, NULL) )
    perror("pthread mutex init failed");
}

static void
spinlock_lock(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_lock(lck) )
    perror("pthread mutex lock faied");
}

static  void
spinlock_unlock(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_unlock(lck) ) 
    perror("pthread mutex unlock failed");
}

static int 
spinlock_trylock(spinlock_t * lck)
{
  int eno;
  if ( 0 == (eno=pthread_mutex_trylock(lck)) ) 
    return 1;/*return true*/
  else if ( EBUSY == eno ) 
    return 0;/*return false*/
  else 
    perror("pthread mutex trylock failed");
}

void __task
default_task_entry(void * arg)
{
  printf("task created, getppid=%d, getpid=%d gettid=%d\n",
	 getppid(), getpid(), gettid());
  
  wait(getpid());
}

void __leader 
default_creator_entry(void *arg)
{
  warp_struct_p warp = (struct warp_struct *)arg;
  int nr =  warp->nr;
  void * stacks[nr]; // gcc ext.
  int i, status;
  tast_t tid;

  printf("leader created, getppid=%d, getpid=%d\n",
	 getppid(), getpid());

  if ( warp->stk_sz != 0 && 
       warp->stk_sz > SPM_TASK_STACK_SIZE )
    for(i=0; i<nr; ++i){
      if ( 0 != posix_memalign(&stacks[i], SPMD_TASK_STACK_ALGN,
			       warp->stk_sz)) {
	perror("posix memalign failed");
	exit(EXIT_FAILURE);
      }      
    }
  else {// pre-allcated stack
    for(i=0; i<nr; ++i)
      stacks[i] = warp->stks[i];
  }
  
  for(int=0; i<nr; ++i) {
    if ( -1 != (tid =clone(warp->fn, stacks[i], 
			   CLONE_THREAD | CLONE_SIGHAND | CLONE_SYSVSEM | CLONE_VM | CLONE_FS \
			   | SIGCHLD/*signal sent to parent when the child dies*/,
			   NULL/*arg*/)) )
      {
	switch(errno)
	  {
	  default:
	    perror("clone failed");
	    break;
	  ENOMEM:
	    fprintf(stderr, "cannot allocate memory");
	    break;	    
	  }
	exit(EXIT_FAILURE);
      }
  }
  waitpid(-1, &status, 0);

  printf("leader wait return\n");
}

static void
allocate_slot(int width, int size, smpd_thread_slot_p slot)
{
  int i, j;
  leader_t pid;
  struct warp_struct warps[size];

  slot->warp_width = width;
  slot->warp_size  = size;
  
  slot->leaders_stacks  = (void *)malloc(sizeof(void*) * size);
  assert( slot->leader_stacks && "default stack malloc failed");
  slot->children_stacks = (void **)malloc(sizeof(void**) * size);
  assert( children_stacks && "default stack malloc failed");
    
  for (i=0; i<size; ++i){
    slot->children_stacks[i] = (void *)malloc(sizeof(void*) * width);
    assert( slot->children_stacks[i] && "default stack malloc failed");

    if ( 0 != posix_memalign(&(slot->leaders_stacks[i]), SPMD_LEADER_STACK_ALGN,
			     SPMD_LEADER_STACK_SIZE)) {
      perror("posix memalign failed");
      exit(EXIT_FAILURE);
    }
    for (j=0; j<width; ++j){
      if ( 0 != posix_memalign(&(slot->children_stacks[i][j]), SPMD_TASK_STACK_ALGN,
			     SPMD_TASK_STACK_SIZE)) {
	perror("posix memalign failed");
	exit(EXIT_FAILURE);
      }      
    }
    //spinlock_init(&(warp->lock));
    //spinlock_lock(&(warp->lock));
    //warp->oc = 1;
    warps[i]->stk_sz = SPMD_TASK_STACK_SIZE;
    warps[i]->fn     = (void *)(&default_task_entry);
    warps[i]->stks   = slot->children_stacks[i];
    warps[i]->hook   = NULL;
    warps[i]->nr     = width;
    // semop 4 rsv_tbl ...
    if (-1 == (pid = clone(default_creator_entry, leaders_stacks[i], 
			   CLONE_VM | CLONE_FS, (void *)(warps[i])) ) {
	  switch(errno)
	    {
	    default:
	      perror("clone failed");
	      break;
	    ENOMEM:
	      fprintf(stderr, "cannot allocate memory");
	      break;	    
	  }
	  exit(EXIT_FAILURE);
	}
  }  
}

int __spmd_export
spmd_initialize()
{
  int nr_thr, nr_slot;
  int nr_pe = probe_in_processor();
  int i, j;
    
  /*also works for systems with non-expnientail cores
   */
  for(i=nr_pe, nr_thr=0, nr_slot=0; i>0; 
      i = i>>1, nr_slot++) 
    nr_thr += nr_pe + i; 
  
  the_pool.nr_pe    = nr_pe;
  the_pool.nr_slot  = nr_slot;
  the_pool.slots    = (spmd_thread_slot_p)malloc
    (nr_slot * sizeof(struct spmd_thread_slot));
  if ( the_pool.slots == NULL ) 
    return -1;
  the_pool.threads  = (thread_t *)malloc
    (nr_thr  * sizeof(thread_t));
  if ( the_pool.threads == NULL ) {
    free(the_pool.slots);
    return -1;
  }
  
  for(i=1, j=nr_pe; i<=nr_slot; ++i, j<<=1)
    allocate_slot(j, nr_pe/j, the_pool.slots[i]);
}
