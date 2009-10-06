// This file is not supposed to be distributed.
// History
// Sep. 12, fix a bug about pointer calculation
// Sep. 22, 
//          add file-based execution breakdown and __TIMELOG macro
//          add SMP machine support for probe_cpu function
// Sep. 23, replace pause with sigsuspended syscall to solve mysterious hang.
// Oct. 05, rewrite wait_for_tg to add atomatic reset. see aux.c
#include "warp.h"
#include "x86/spmd.h"
#include <time.h>

/*globals*/
struct spmd_thread_pool the_pool;

#if 0
static spinlock_t              the_lock = SPMD_SPINLOCK_INITIALIZER;
#endif

static int                     spmd_initialized = 0;


/*aux funtions*/
extern int  probe_nr_processor();
extern void spinlock_init(spinlock_t * lck);
extern void spinlock_destroy(spinlock_t * lck);
extern void spinlock_lock(spinlock_t * lck);
extern void spinlock_unlock(spinlock_t * lck);
extern int  spinlock_trylock(spinlock_t * lck);

extern pid_t gettid(void);
extern int tkill(int tid, int sig);
extern void thread_exit();
extern void wait_for_tg(leader_struct_p ldr);
extern  void set_fifo(int pid, int prio);
extern  void set_normal(int pid);

/*aux func end*/
static void spmd_runtime_dump();

void children_handler(int signum)
{
/*
  assert ( signum == SIGCONT ); 
  printf("p:t caught this signal %d:%d\n", 
	 getpid(), gettid());
 */
}
void kill_handler(int signum)
{
  thread_exit();
}

int __task 
default_task_entry(void * arg)
{
#ifndef __NDEBUG
  fprintf(stderr, "@task created, getppid=%d, getpid=%d gettid=%d, arg=%p\n",
	  getppid(), getpid(), gettid(), arg);
#endif

  task_struct_p task = (task_struct_p)arg;
  task->tid = gettid();
  set_fifo(task->tid, sched_get_priority_max(SCHED_FIFO));
#ifdef __TIMELOG
  char log[80];
  sprintf(log, "timelog-%d.tsk", task->tid);
  FILE * fh = fopen(log, "w");
  if ( fh == NULL ) {
    fprintf(stderr, "[ERROR] fopen failed: %s\n", strerror(errno));
    thread_exit();
  }
  task->log_fd = fh;
#endif
 
  struct sembuf buf;
#if 0						
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCONT); 
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
#endif

  while(1) {
    // open for SIGCONT
    //sigsuspend(&oldmask); 
    struct sembuf sb = {.sem = 1, .sem_op = -1, .sem_flg=0};
    semop(task->sem_ldr, &sb, 1); 
#ifndef __NDEBUG
    fprintf(stderr, "@task [%d:%d] start working, task ptr = %p fn = %p\n", 
	    getpid(), gettid(), task, task->fn);
#endif

#ifdef __TIMELOG
    struct timeval fntv_start, fntv_end;
    long execspan;
    
    fprintf(task->log_fd, "task thread counter #%2d\n=================\n", task->counter++);
#if defined(LINUX)
    struct timespec myts;
    clock_gettime(CLOCK_REALTIME, &myts);
    fprintf(task->log_fd, "task start timestamp %d: %ld\n", myts.tv_sec, myts.tv_nsec);
#endif
    gettimeofday(&fntv_start, NULL); 
#endif 

    /* usually, the first paramter is pointer of function object */
    task->fn(task->args[0], task->args[1], task->args[2], task->args[3]);

#ifdef __TIMELOG
    gettimeofday(&fntv_end, NULL);
    fprintf(task->log_fd, "task execution span %d usec\n", 
    	(execspan = (fntv_end.tv_sec  - fntv_start.tv_sec) * 1000000 
	+ (fntv_end.tv_usec - fntv_start.tv_usec)));
#endif

#ifndef __NDEBUG
    printf("task [%d:%d] gonna release pe: %d\n",
	   getpid(), gettid(), task->oc);
#endif

    /*release physical pe */
    buf.sem_num = task->oc;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if ( -1 == semop(task->sem_pe, &buf, 1) ){
      fprintf(task->log_fd, "[ERROR] release pe failed: %s\n", strerror(errno));
      thread_exit();
    }
    /*notify leader */
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    int before = semctl(task->sem_ldr, 0, GETVAL);
    if ( -1 == semop(task->sem_ldr, &buf, 1) ) {
      fprintf(task->log_fd, "[ERROR] notify leader failed: %s\n", strerror(errno));
      thread_exit();
    }
#ifndef __NDEBUG
    int semval = semctl(task->sem_ldr, 0, GETVAL);
    fprintf(stderr, "@task [%d:%d] complete working, leader->sem = %d before %d now=%d task ptr = %p fn = %p\n", 
	    getpid(), gettid(), task->sem_ldr, before,  semval, task, task->fn);    
#endif

#ifdef __TIMELOG
#if defined(LINUX)
    long elapsed = myts.tv_sec;
    long elapsed_nsec = myts.tv_nsec;
    clock_gettime(CLOCK_REALTIME, &myts);
    elapsed = ((myts.tv_sec - elapsed) * 1000000000 
    	+ (myts.tv_nsec - elapsed_nsec)) / 1000;
    fprintf(task->log_fd, "task close timestamp %d:%d\ntask active time is %d, effectivity is %.2f\%\n",
    	myts.tv_sec, myts.tv_nsec, elapsed, (100.0 * execspan) / elapsed );
#endif
   fflush(task->log_fd);
#endif
  }

#ifndef __NDEBUG
  fprintf(stderr, "@got signal, task quit");
#endif
  return 0;
}

void __leader
default_leader_entry(leader_struct_p leader)
{

  leader->nr = 0;
  while (1) {

#ifndef __NDEBUG
    fprintf(stderr, "#leader %d, leader->sem = %d, sem number = %d\n", 
	   leader->gid, leader->sem, leader->warp.nr);
    fprintf(stderr, "#waiting for thread group %d\n", leader->gid);
#endif    

#ifdef __TIMELOG
    struct timeval tv;
    long elapsed;
#endif
    
    // reset the last value, otherwise, 
    wait_for_tg(leader);
#ifndef __NDEBUG
    fprintf(stderr, "#wait returned, reduce...%d\n", leader->gid);
#endif

#ifdef __TIMELOG
    struct timeval hook_start, hook_end;
    gettimeofday(&tv, NULL);
    // elapsed time  of waiting
    elapsed = (tv.tv_sec - leader->warp.last_stamp/1000000) * 1000000 
    	+ (tv.tv_usec - leader->warp.last_stamp%1000000);
    fprintf(leader->warp.log_fd, "leader counter = %d warp num. = %d wait_return=%d\n", 
    	leader->warp.counter++, leader->warp.nr, elapsed);
    leader->warp.time_in_wait += elapsed; 
    gettimeofday(&hook_start, NULL);
#endif

    void * arg = leader->warp.hk_arg;
    if ( NULL != leader->warp.hook ) 
      (leader->warp.hook)(arg);

#ifdef __TIMELOG
    gettimeofday(&hook_end, NULL);
    // elapsed time of hooked function
    elapsed = (hook_end.tv_sec - hook_start.tv_sec) * 1000000
      + (hook_end.tv_usec - hook_start.tv_usec);

    fprintf(leader->warp.log_fd, "hook function consumes %d us\n", elapsed);
    leader->warp.time_in_reduce += elapsed;
#endif
    //reset task counter
    //
    spinlock_lock(&(leader->lck));

    leader->oc = 0;
    leader->nr = 0;

    spinlock_unlock(&(leader->lck));

#ifdef __TIMELOG
    gettimeofday(&tv, NULL);
    elapsed = (tv.tv_sec - leader->warp.last_stamp/1000000) * 1000000 
    	+ (tv.tv_usec - leader->warp.last_stamp%1000000);
    fprintf(leader->warp.log_fd, "time on fly %d\n", elapsed);
    leader->warp.time_on_fly += elapsed;
    fflush(leader->warp.log_fd);
#endif

  }
}

int __leader 
default_creator_entry(void *arg)
{
  leader_struct_p ldr = (leader_struct_p)arg;
  warp_struct_p warp = &(ldr->warp);
  int nr = warp->nr;
  void * stacks[nr]; // gcc ext.
  int i, tid;

  ldr->gid = warp->gid = getpid();

#ifdef __TIMELOG
      char log[80];
      sprintf(log, "timelog-%d.ldr", ldr->gid);
      FILE * fh = fopen(log, "w");
      if ( fh == NULL ) {
        fprintf(stderr, "[ERROR] fopen failed: %s\n", strerror(errno));
	thread_exit();
      }
      ldr->warp.log_fd = fh; 
      ldr->warp.time_on_fly = 0;
      ldr->warp.time_in_reduce = 0;
      ldr->warp.time_in_wait = 0;
      ldr->warp.counter = 0;
#endif
 
 
#ifndef __NDEBUG
  fprintf(stderr, "leader created, getppid=%d, getpid=%d ldr=%p\n",
	 getppid(), getpid(), arg);
#endif

  
  for(i=0; i<nr; ++i) {
    task_struct_p task = &(warp->tsks[i]);
#ifndef __NDEBUG
    printf("#leader %d task %d, task ptr = %p ldr->sem=%d\n", warp->gid, i, task, ldr->sem);
#endif
    task->gid = warp->gid;
    task->sem_pe = the_pool.sem_pe;
    task->sem_ldr = ldr->sem;
    task->fn = warp->fn;
    stacks[i] = warp->init_list.stks[i];

    if ( -1 == (tid = clone(&default_task_entry, stacks[i], 
			CLONE_THREAD | CLONE_SIGHAND | CLONE_SYSVSEM | \
			CLONE_VM | CLONE_FS			       \
			    | SIGCHLD/*signal sent to parent when the child dies*/,
			    task/*arg*/)) )  {
	perror("clone task failed");
	exit(EXIT_FAILURE);
    }
    else {
       *((warp->init_list).wb_tsks + i) = tid;
    }
  }/*for*/

  // set handler of this thread group for SIGCONT.
#if 0 
  signal(SIGCONT, children_handler);
  signal(SIGINT, kill_handler);
#endif
  unsigned short array[2] = {nr, 0};

  // lock up semaphore of leader
  if ( 0 != semctl(ldr->sem, 0, SETALL, array) ) {
    fprintf(ldr->warp.log_fd, "[ERROR] failed set up leader sem: %s\n", strerror(errno));
    thread_exit();
  }

  default_leader_entry(ldr);
  return 0;
}

static int
init_semphore(int nr)
{
  key_t key;
  int semid, i;
  unsigned short array[nr];

  key = ftok(SPMD_SEM_KEY, 0xff);
  if ( key == -1 ) {
    perror("ftok failed in init");
    return -1;
  }
  semid = semget(key, nr, 0666 | IPC_CREAT);
  if ( semid == -1 ) {
    perror("semget failed in init");
    return -1;
  }

  for (i=0; i<nr; ++i) array[i] = 1;
  
  semctl(semid, 0, SETALL, array);
  return semid;
}

static void
allocate_slot(int width, int size, spmd_thread_slot_p slot, 
	      leader_struct_p ldrs)
{
  int i, j;
  leader_t pid;
#ifndef __NDEBUG
  printf("allocate_slot width=%d, size=%d, slot=%p, ldrs=%p\n",
	 width, size, slot, ldrs);
#endif
  slot->warp_width = width;
  slot->warp_size  = size;
  
  slot->leaders_stacks  = (void *)malloc(sizeof(void*) * size);
  assert( slot->leaders_stacks && "default stack malloc failed");
  slot->children_stacks = (void ***)malloc(sizeof(void**) * size);
  assert( slot->children_stacks && "default stack malloc failed");

  for (i=0; i<size; ++i) {
    slot->children_stacks[i] = (void *)malloc(sizeof(void*) * width);
    assert( slot->children_stacks[i] && "default stack malloc failed");

    if ( 0 != posix_memalign(&(slot->leaders_stacks[i]), SPMD_LEADER_STACK_ALGN,
			     SPMD_LEADER_STACK_SIZE)) {
      perror("posix memalign failed");
      exit(EXIT_FAILURE);
    }
    else {
      slot->leaders_stacks[i] = (char *)slot->leaders_stacks[i] + SPMD_LEADER_STACK_SIZE;
    }

    for (j=0; j<width; ++j){
      if ( 0 != posix_memalign(&(slot->children_stacks[i][j]), SPMD_TASK_STACK_ALGN,
			     SPMD_TASK_STACK_SIZE)) {
	perror("posix memalign failed");
	exit(EXIT_FAILURE);	
      }				
      else {
	slot->children_stacks[i][j] = (char *)slot->children_stacks[i][j] + SPMD_TASK_STACK_SIZE;
	//printf("children[%d][%d] stack position=%p\n", i, j, slot->children_stacks[i][j]);
      }
    }
    /* postpone gid to creator function */
    ldrs[i].warp.nr = width;   
    ldrs[i].warp.fn = NULL;
    ldrs[i].warp.hook = NULL;
    ldrs[i].warp.tsks = malloc(sizeof(struct task_struct) * width);
    if ( ldrs[i].warp.tsks == NULL ) {
      perror("malloc failed");
      exit(EXIT_FAILURE);
    }
    
    ldrs[i].warp.init_list.stk_sz = SPMD_TASK_STACK_SIZE;
    ldrs[i].warp.init_list.stks   = slot->children_stacks[i];
    ldrs[i].warp.init_list.wb_tsks = slot->wb_tasks + i * width;

    //printf("alloc clone stack pointer=%p\n", slot->leaders_stacks[i]);
    if (-1 == (pid = clone(&default_creator_entry, slot->leaders_stacks[i], 
			   CLONE_VM | CLONE_FS, ldrs+i)) ) {
      perror("clone leader failed");
      exit(EXIT_FAILURE);
    }
    else {
      *(slot->wb_leaders + i) = pid; /*writeback*/

    }/*if*/
  }/*for*/
}

//FIXME: I doubt it is desirable to make this function thread-safe.
//this function should be called in primary thread/process, otherwise is error-prone.
int __spmd_export
_spmd_initialize(int nr_pe)
{
  int nr_thr, nr_slot, nr_leader=0;
  int nr_pe_ = probe_nr_processor();
  int i, j;	
  int semid;
  key_t key;

  //FIXME: lock protection or re-entrance
  if ( nr_pe > nr_pe_ ) {
    return -1;
  }

  if ( 0 != spmd_initialized ) {
    return -1;
  }
  else {
    spmd_initialized = 1;
  }

  /*
   * also works for systems with non-exponential cores
   */

  for(i=nr_pe, nr_thr=0, nr_slot=0; i>0; 
      i = i>>1, nr_slot++) { 
    nr_thr += nr_pe + i; 
    nr_leader += i;
  }

  if ( -1 == (semid=init_semphore(nr_pe)) )
    return -1; /* fail to obtain sem set */
  else 
    the_pool.sem_pe = semid;

  /*reallocate resouce*/
  the_pool.nr_pe    = nr_pe;
  the_pool.nr_slot  = nr_slot;
  the_pool.nr_thread = nr_thr;
  the_pool.nr_leader = nr_leader;
  the_pool.nr_task   = nr_thr - nr_leader;

  if ( nr_leader > 0xfe ) { /*too many leaders.
			      currently, we use sem to simulates
			      signal wait_for_tg.
			      SEMs are identified by file. see ftok(3)
			      0xff has already been associated with sem_pe.
			    */
			      
    return -1;
  }

  the_pool.slots    = (spmd_thread_slot_p)malloc
    (nr_slot * sizeof(struct spmd_thread_slot));
  if ( the_pool.slots == NULL ) 
    return -1;

  the_pool.thr_leaders  = (thread_t *)malloc
    (nr_leader  * sizeof(thread_t));
  if( the_pool.thr_leaders == NULL ) 
    goto err_happened;

  the_pool.thr_tasks    = (thread_t *)malloc
    (the_pool.nr_task * sizeof(thread_t));
  if ( the_pool.thr_tasks == NULL ) 
    goto err_happened1;
#ifndef __NDEBUG
  else  {
    printf("the_pool.thr_tasks = %lp\n", 
      the_pool.thr_tasks);
  }
#endif
  the_pool.leaders = (struct leader_struct *)malloc
    (nr_leader * sizeof(struct leader_struct));
  if ( the_pool.leaders == NULL ) 
    goto err_happened2;
  
  for (i=0; i<nr_leader; ++i) {
    key = ftok(SPMD_SEM_KEY, i);
    
    if ( key == -1 ) {
      perror("ftok failed");
      goto err_happened3;
    }
    
    the_pool.leaders[i].sem = 
      semget(key, 2, 0666 | IPC_CREAT);
    if ( the_pool.leaders[i].sem == -1 ) {
     perror("semget fail");
     goto err_happened3;
    }
   //printf("sem leader[%1d].sem = %d\n", i, the_pool.leaders[i].sem);
  }

  thread_t * ldr = the_pool.thr_leaders;
  thread_t * tsk = the_pool.thr_tasks;
  leader_struct_p leaders = the_pool.leaders;

  for(i=0, j=nr_pe; i<nr_slot; ++i, j>>=1) {
    spmd_thread_slot_p slot = the_pool.slots + i;

    slot->wb_leaders = ldr;
    slot->wb_tasks = tsk;
    allocate_slot(j/*width*/, nr_pe/j/*size*/, slot, leaders);
    ldr = ldr + (nr_pe/j);
    tsk = tsk + nr_pe;
    leaders += (nr_pe/j);
 }
        
 ldr = the_pool.thr_leaders;
 for (i=0; i<nr_leader; ++i) {
   spinlock_init(&(the_pool.leaders[i].lck));
   the_pool.leaders[i].oc = 0; 
 }

 spmd_runtime_dump();

 return nr_pe;
 
 err_happened3:
        free(the_pool.leaders);       
 err_happened2:
	free(the_pool.thr_tasks);
 err_happened1:
        free(the_pool.thr_leaders);
 err_happened:
        free(the_pool.slots);
  return -1;
}

int __spmd_export
spmd_initialize()
{ 
   return _spmd_initialize(probe_nr_processor());
}

void  __spmd_export
spmd_cleanup()
{
  int i, j;
  int dummy;
  
  while ( !spmd_all_complete() );
  for ( i=0; i<the_pool.nr_leader; ++i) {
    leader_struct_p ldr = &(the_pool.leaders[i]);  
    assert( 0 == kill(ldr->gid, SIGTERM) 
    	   && "kill function failed.");
    
    spinlock_destroy(&(ldr->lck));
    if ( -1 == semctl(ldr->sem, 0, IPC_RMID, dummy) ) {
      perror("semctl IPC RMID sem");
    }
  }
  if ( -1 == semctl(the_pool.sem_pe, 0, IPC_RMID, dummy) ) {
    perror("semctl IPC RMID sem_pe");
  }

#ifndef __NDEBUG
  printf("spmd close all children\n");
  printf("the_pool.leaders = %p\nthe_pool.thr_tasks = %p,\
  the_pool.thr_leaders = %p, the_pool.slots = %p\n", 
  the_pool.leaders, 
  the_pool.thr_tasks,
  the_pool.thr_leaders,
  the_pool.slots);
#endif

  free(the_pool.leaders);
  free(the_pool.thr_tasks);
  free(the_pool.thr_leaders);
  free(the_pool.slots);
}

int  __spmd_export
spmd_create_warp(int nr, void * fn, unsigned int stk_sz, 
                 void * hook, void * hk_arg)
{
  int i, j, warp_id;
  leader_struct_p cand, ldr;
  warp_struct_p warp;
  int offset;

  for (i=the_pool.nr_pe, offset=0; i>nr && i>=1; 
       offset += the_pool.nr_pe / i, i>>=1);

  if ( i == 0 || (the_pool.nr_pe % nr) != 0 ) 
    return -1; /*ILL nr */

  ldr = NULL;
 pick_leader: cand = the_pool.leaders + offset;

  for (i=0; i < the_pool.nr_pe / nr; ++i, cand++) {
    if ( spinlock_trylock(&(cand->lck)) )  /* don't contend hot lock */
      if ( cand->oc == 0 ) {
	//fprintf(stderr, "ldr oc is 0, okay to obtain this leader\n");
	ldr = cand;
	break;
      }
      else  {
        //fprintf(stderr, "ldr oc is 1, find another leader\n");
	spinlock_unlock(&(cand->lck));
      }
    else {
	//fprintf(stderr, "can not lock on %p\n", cand);
    }
  }

  if ( ldr == NULL ) {
    struct timespec spec = {
      .tv_sec = 0,
      .tv_nsec = 50000 /*50 usec*/
    };
    nanosleep(&spec, NULL);
    goto pick_leader;
  }
  //obtains leader and has locked up
  ldr->oc = 1;
  warp = &(ldr->warp);

  if ( nr != warp->nr ) {
    if ( 0 != semctl(ldr->sem, 0, SETVAL, nr) ) {
      perror("failed set up leader sem");
      exit(EXIT_FAILURE);
    }
  }
  spinlock_unlock(&(ldr->lck));
  //warp->gid = ldr->gid;
  warp->nr = nr;
  warp->fn = fn;
  warp->hook = hook;
  warp->hk_arg = hk_arg;
  //FIXME: does not support customized stack size
  if ( stk_sz != 0 ) return -1;

  unsigned short snapshot[the_pool.nr_pe];
  struct sembuf buf[nr];
  struct timespec ts;
  int eno;

  for (i=0; i<nr; ++i) buf[i].sem_op = -1;
  ts.tv_sec = 0;
  ts.tv_nsec = 2000; /* 2us timeout*/
 retry:  
  semctl(the_pool.sem_pe, 0, GETALL, snapshot);
  for(i=0, j=0; j<nr && i<the_pool.nr_pe; ++i) {
    if ( snapshot[i] == 1 ) {
      buf[j++].sem_num = i;
    }
  }
  if ( j < nr ) {
    //sleep(1);  /*really busy*/
    goto retry;
  }
    
  if ( -1 == (eno=semtimedop(the_pool.sem_pe, buf, nr, &ts)) ) {
    if ( eno == EAGAIN || eno == EINTR) { /* snapshot is old, try again */
      goto retry;
    }
    else {
      perror("semop failed");
      return -1;
    }
  }

  for (i=0; i<nr; ++i) {
    //fprintf(stderr, "selected pe#%2d %d\n", i, buf[i].sem_num);
    ldr->warp.tsks[i].oc = buf[i].sem_num;
  }
  warp_id = (ldr - the_pool.leaders);
  return warp_id;
}  

static void
spmd_runtime_dump()
{
  int i, j, k;
  //FIXME: thread-safe
  assert(spmd_initialized && "spmd rt uninitialized");
  
  printf("spmd thread pool:\n");
  printf("nr_pe|nr_slot|nr_leader|nr_task|nr_thread|\n");
  printf("%5d|%7d|%9d|%7d|%9d|\n", the_pool.nr_pe, the_pool.nr_slot,
	 the_pool.nr_leader, the_pool.nr_task, the_pool.nr_thread);

  for (i=0; i<the_pool.nr_slot; ++i) {
    spmd_thread_slot_p slot = the_pool.slots + i;
    thread_t * ldr = slot->wb_leaders;
    thread_t * tsk = slot->wb_tasks;

    printf("slot %d: %p\n", i, slot);
    printf("width=%d, size=%d\n", slot->warp_width, slot->warp_size);
    
    for(j=0; j<slot->warp_size; ++j) {
      printf("ldr%d=%d:(", j, *ldr++);
      for(k=0; k<slot->warp_width; ++k)
	printf("%d,", *tsk++); 
      printf(")\n");
    }
  }
}

static void
spmd_fire_up(leader_struct_p leader)
{
  int i, eno, last;
  task_struct_p tsk;
  cpu_set_t mask;
  for(i=0; i<leader->nr; ++i) {
    tsk  = &leader->warp.tsks[i];
    CPU_ZERO(&mask);
    CPU_SET(tsk->oc, &mask);
    last = tsk->oc;
    if ( -1 == (eno=sched_setaffinity
		(tsk->tid, sizeof(cpu_set_t), &mask)) ) {
      perror("sched_setaffinity failed");
      //FIXME: unload rt task for gentle exit
    }
  }
/* change myself to that last RT cpu, pervent suspending the signal sending*/
//FIXME: add the following code will induce significant perf retreat.
//I can not understand.
#if 0
  CPU_ZERO(&mask);
  CPU_SET(2, &mask);
  eno = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
  if ( -1 == eno ) {
    perror("bind myself failed\n");
    exit(EXIT_FAILURE);
  }
  else {
    //printf("bind main thread on %d\n", last);
  }
#endif
#ifndef __NDEBUG
  printf("fire up is called\n");
#endif
  unsigned short buf[2];
  if ( -1 == semctl(leader->sem, 0, GETALL, buf) ) {
    fprintf(stderr, "[ERROR] semctl failed: %s\n", strerror(errno)); 
    thread_exit();
  }
  if ( buf[0] != leader->warp.nr || buf[1] != 0 ) {
    fprintf(stderr, "[ERROR] leader->sem: ILL status sem0=%d sem1=%d\n", buf[0], buf[1]);
    thread_exit();
  }

  //fireup
  if ( 0 != semctl(leader->sem, 1, SETVAL, buf[0]) ) {
    fprintf(stderr, "[ERROR] semctl SET value: %s\n", 
	    strerror(errno));
    thread_exit();
  }
#if defined(__TIMELOG) && defined(LINUX)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  fprintf(leader->warp.log_fd, "fire up timestamp: %d:%d\n",
  	ts->tv_sec, ts->tv_nsec);
#endif

#if 0
  for (i=0; i<leader->nr; ++i) {
    tsk = &leader->warp.tsks[i];
#if defined(__TIMELOG) && defined(LINUX)
    struct timespec myts;
    clock_gettime(CLOCK_REALTIME, &myts);
    fprintf(tsk->log_fd, "send signal: %d\ntimestamp %d:%d\n",
       SIGCONT, myts.tv_sec, myts.tv_nsec);
#endif
    if ( -1 == tkill(tsk->tid, SIGCONT) ) {
      fprintf(leader->warp.log_fd,
      	"[ERROR] tkill %d SIGCONT failed: %s\n", 
	strerror(errno));
      thread_exit();
    }
  }/*for*/
#ifdef __TIMELOG
    struct timeval tv;
    gettimeofday(&tv, NULL);
    leader->warp.last_stamp = tv.tv_sec * 1000000 + tv.tv_usec;
#endif

  
  //FIXME: why kill pid does NOT work?
  //kill(leader->gid, SIGCONT);
#endif
}

int __spmd_export
spmd_create_thread(int warp_id, void * self, void * ret, void * arg0, void * arg1)
{
  leader_struct_p ldr;
  int t;

  if ( !( 0 <= warp_id && warp_id < the_pool.nr_leader ) ) 
    return -1; /*ILL warp_id */
 
  ldr = &(the_pool.leaders[warp_id]);
  spinlock_lock(&(ldr->lck));

  if ( ldr->oc != 1 && ldr->warp.fn == NULL ) {
    spinlock_unlock(&(ldr->lck));
    return -1;
  }
   
  if ( ldr->nr >= ldr->warp.nr ) {
    spinlock_unlock(&(ldr->lck));
    return -1; /*more task than warp. already fired*/
  }

  t = ldr->nr++;
#ifndef __NDEBUG
  printf("t = %2d, ldr->warp.nr=%2d ldr->warp.tsk[%d]= %p tid = %d ldr->warp.fn = %p\n", 
	 t, ldr->warp.nr, t, &(ldr->warp.tsks[t]), ldr->warp.tsks[t].tid, ldr->warp.fn);
#endif
  spinlock_unlock(&(ldr->lck));
  
  ldr->warp.tsks[t].fn = ldr->warp.fn;
  ldr->warp.tsks[t].args[0] = self;
  ldr->warp.tsks[t].args[1] = ret;
  ldr->warp.tsks[t].args[2] = arg0;
  ldr->warp.tsks[t].args[3] = arg1;
  
  if ( t == ldr->warp.nr - 1 )
    spmd_fire_up(ldr);

  return t;
}

int __spmd_export
spmd_get_taskid()
{
  return gettid();
}

int __spmd_export
spmd_available_thread()
{
  unsigned short snapshot[the_pool.nr_pe];
  int i;
  int nr = 0;

  if ( -1 == semctl(the_pool.sem_pe, 0, GETALL, snapshot) ) {
    perror("semctl GETALL failed");
    exit(EXIT_FAILURE);
  }
  
  for (i=0; i<the_pool.nr_pe; ++i) {
    if (snapshot[i] == 1 ) nr++;
  }
  return nr;
}

int __spmd_export
spmd_all_complete()
{
  int i;

  for (i=0; i<the_pool.nr_leader; ++i) {
    leader_struct_p ldr = &(the_pool.leaders[i]);
    if ( !spinlock_trylock(&(ldr->lck)) ) {
      return 0;
    }
    else if ( ldr->oc != 0 ) {
      spinlock_unlock(&(ldr->lck));
      return 0;
    }
    else 
      spinlock_unlock(&(ldr->lck));
  }
  return 1;
}

#if 0
void dummy_fn(void * ret, void * arg0, void * arg1)
{
  printf("dummy_fn%2d\n", gettid());
}


int 
main()
{
  int warpid;
  int tid;

  printf("default_task_entry=%p\n", &(default_task_entry));
  assert( probe_nr_processor() == 2);
  assert( spmd_initialize() == 2);
  sleep(1);  
  spmd_runtime_dump();
  
  warpid = spmd_create_warp(2, dummy_fn, 0, NULL);
  if ( warpid == -1 ) {
    fprintf(stderr, "spmd_create_warp failed\n");
    exit(1);n
  }
  else
    printf("created warpid = %2d\n", warpid);

  tid = spmd_create_thread(warpid, NULL, NULL, NULL);
  assert( tid != -1 && "first task failed");
  tid = spmd_create_thread(warpid, NULL, NULL, NULL);
  assert( tid != -1 && "second task failed");
  sleep(1);
  spmd_cleanup();
  return 0;
}
#endif
