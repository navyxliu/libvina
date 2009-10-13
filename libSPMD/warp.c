// This file is not supposed to be distributed.
// History:
// July.13, create file and document design in DESIGN.
// Sep. 12, fix a bug about pointer calculation
// Sep. 22, add file-based execution breakdown and __TIMELOG macro
//          SMP machine support for probe_cpu function
// Sep. 23, replace pause with sigsuspended syscall to solve mysterious hang.
// Oct. 05, rewrite wait_for_tg to add atomatic reset. see aux.c
// Oct. 07, remove all singal stuffs. uses semaphore.
//          bugs fixed. use distribued semaphores for tasks.
// Oct. 10, refactor the project of libSPMD. make it independent from libvina. 
//          test it out by stress.sh and test_sh.sh. 
//
#include "warp.h"
#include "x86/spmd.h"

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
extern void thread_exit(const char *);
extern void wait_for_tg(leader_struct_p ldr);
extern void set_fifo(int pid, int prio);
extern void set_normal(int pid);
extern void pe_snapshot();
extern void exp_backoff();

extern void indent(FILE*, int, char);
extern void indent_v(FILE*, int, int, char);
/*aux func end*/

static void spmd_runtime_dump();
static int init_semaphore(int);
void children_handler(int signum)
{}
void kill_handler(int signum)
{
  thread_exit("interrupt from user");
}

int __task 
default_task_entry(void * arg)
{
  task_struct_p task = (task_struct_p)arg;
  task->tid = gettid();
  struct sembuf sb = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
  semop(task->ldr->warp._init_list.sem_task_prep, &sb, 1);
  int ret;
  set_fifo(task->tid, sched_get_priority_max(SCHED_FIFO));
#ifdef SYNC_SIGNAL 
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCONT); 
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
#endif
  while(1) {

    // open for SIGCONT
#ifdef SYNC_SIGNAL
    sigsuspend(&oldmask); 
    // down semaphore.
#else
    sb.sem_num = task->idx;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    do {
      ret = semop(task->ldr->sem_task, &sb, 1); 
      if ( ret != 0 && !(errno == EAGAIN || errno == EINTR ) ) {
        fprintf(task->log_fd, "[ERROR] semop failed: %s\n", strerror(errno));
        thread_exit("down task semphore failed");
      }
    } while( ret != 0 );
#endif

#ifdef __TIMELOG
    struct timeval fntv_start, fntv_end;
    long execspan;
    
    fprintf(task->log_fd, "task thread counter #%2d\n=================\n", task->counter++);
#if defined(LINUX)
    struct timespec myts;
    clock_gettime(CLOCK_REALTIME, &myts);
    fprintf(task->log_fd, "task start timestamp %d: %d\n", myts.tv_sec, myts.tv_nsec);
#endif
    gettimeofday(&fntv_start, NULL); 
#endif 

    /* usually, the first paramter is pointer of function object */
    if ( (task->ldr->warp.fn) )  {
      (*task->ldr->warp.fn)(task->arg);
    }
#ifdef __TIMELOG
    gettimeofday(&fntv_end, NULL);
    fprintf(task->log_fd, "task execution span %d usec\n", 
    	(execspan = (fntv_end.tv_sec  - fntv_start.tv_sec) * 1000000 
	+ (fntv_end.tv_usec - fntv_start.tv_usec)));
#endif
    /*release physical pe */
    sb.sem_num = task->oc;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    if ( -1 == semop(the_pool.sem_pe, &sb, 1) ){
      fprintf(task->log_fd, "[ERROR] release pe failed: %s\n", strerror(errno));
      thread_exit("failed in semop release pe");
    }
#if !defined(__NDEBUG) ||  defined(__TIMELOG)
    pe_snapshot(task->log_fd, task->oc); 
    int before = semctl(task->ldr->sem, 0, GETVAL);
#endif
    int ret;
    /*notify leader */
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = IPC_NOWAIT;
    do { 
      ret = semop(task->ldr->sem, &sb, 1);  
      if ( -1 == ret && !(errno == EAGAIN || errno == EINTR) ) {
        fprintf(task->log_fd, "[ERROR] notify leader failed: %s\n", strerror(errno));
        thread_exit("failed in semop notify leader");
      }
    } while ( ret != 0 );
#ifdef __TIMELOG
    int semval = semctl(task->ldr->sem, 0, GETVAL);
    fprintf(task->log_fd, "sem=%d, before=%d after = %d\n", 
    	task->ldr->sem, before, semval);
#endif


#ifdef __TIMELOG
#if defined(LINUX)
    long elapsed = myts.tv_sec;
    long elapsed_nsec = myts.tv_nsec;
    clock_gettime(CLOCK_REALTIME, &myts);
    elapsed = ((myts.tv_sec - elapsed) * 1000000000 
    	+ (myts.tv_nsec - elapsed_nsec)) / 1000;
    fprintf(task->log_fd, "task close timestamp %d:%d\ntask active time is %d, effectivity is %.2f\%\n",
    	myts.tv_sec, myts.tv_nsec, elapsed, (100.0f * execspan) / elapsed );
#endif
   fflush(task->log_fd);
#endif

  }/*while*/

  return 0;
}

void __leader
default_leader_entry(leader_struct_p leader)
{

#ifdef __TIMELOG
  fprintf(leader->warp.log_fd, "leader thread pid: %d\n", 
     leader->gid);
#endif

  struct sembuf sb = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
  semop(leader->warp._init_list.sem_task_prep, &sb, 1);

  while (1) {
#ifdef __TIMELOG
    struct timeval tv;
    long elapsed;
    fprintf(leader->warp.log_fd, "leader counter=%2d\n", 
      leader->warp.counter++);
#endif
    
    // reset the last value, otherwise, 
    wait_for_tg(leader);
    
#ifdef __TIMELOG
    struct timeval hook_start, hook_end;
    gettimeofday(&tv, NULL);
    // elapsed time  of waiting
    elapsed = (tv.tv_sec - leader->warp.last_stamp.tv_sec) * 1000000 
    	+ (tv.tv_usec - leader->warp.last_stamp.tv_usec);
    fprintf(leader->warp.log_fd, "leader counter = %d warp num. = %d wait_return=%d\n", 
    	leader->warp.counter, leader->warp.width, elapsed);
    leader->warp.time_in_wait += elapsed; 
    
    pe_snapshot(leader->warp.log_fd, -1);
    gettimeofday(&hook_start, NULL);
#endif

    void * arg = leader->warp.hook_arg;
    if ( leader->warp.hook ) 
      (*leader->warp.hook)(arg);

#ifdef __TIMELOG
    gettimeofday(&hook_end, NULL);
    // elapsed time of hooked function
    elapsed = (hook_end.tv_sec - hook_start.tv_sec) * 1000000
      + (hook_end.tv_usec - hook_start.tv_usec);

    fprintf(leader->warp.log_fd, "hook function consumes %d us\n", elapsed);
    leader->warp.time_in_reduce += elapsed;
#endif

    if ( 0 != semctl(leader->sem, 1, SETVAL, 1) ) {
      thread_exit("failed in release leader");
    }
#ifdef __TIMELOG
    gettimeofday(&tv, NULL);
    elapsed = (tv.tv_sec - leader->warp.last_stamp.tv_sec) * 1000000 
    	+ (tv.tv_usec - leader->warp.last_stamp.tv_usec);
    fprintf(leader->warp.log_fd, "time on fly %d\n", elapsed);
    leader->warp.time_on_fly += elapsed;
    fflush(leader->warp.log_fd);
#endif
  }/*while*/

}

int __leader 
default_creator_entry(void *arg)
{
  leader_struct_p ldr = (leader_struct_p)arg;
  warp_struct_p warp = &(ldr->warp);
  int nr = ldr->warp.width;
  void * stacks[nr]; // gcc ext.
  int i, tid;
  unsigned short array[nr];

  ldr->gid = getpid();

  // lock up semaphore of leader
  if ( 0 != semctl(ldr->sem, 0, SETVAL, nr) ) {
    fprintf(ldr->warp.log_fd, "[ERROR] failed set up leader sem: %s\n", strerror(errno));
    thread_exit("failed in initialization of leader sem");
  }
  if ( 0 != semctl(ldr->sem, 1, SETVAL, 1) ) {
    fprintf(ldr->warp.log_fd, "[ERROR] failed set up leader sem: %s\n", strerror(errno));
    thread_exit("failed in initialization of leader sem");
  } 

#ifndef SYNC_SIGNAL
  //block tasks
  memset(array, 0, sizeof(array));
  if ( 0 != semctl(ldr->sem_task, 0, SETALL, array) ) {
    fprintf(ldr->warp.log_fd, "[ERROR] failed set up sem_task: %s\n", strerror(errno));
    thread_exit("setup sem_task failed");
  }
#endif

  for(i=0; i<nr; ++i) {
    task_struct_p task = &(warp->tsks[i]);
    task->ldr = ldr; 
    task->idx = i;
    task->counter = 0;

    stacks[i] = warp->_init_list.stks[i];
    if ( -1 == (tid = clone(&default_task_entry, stacks[i], 
			CLONE_THREAD | CLONE_SIGHAND  | \
			CLONE_VM | CLONE_FS			       \
			| SIGCHLD/*signal sent to parent when the child dies*/,
		    task/*arg*/)) )  {
	fprintf(ldr->warp.log_fd, "[ERROR] clone task failed: %s\n", strerror(errno));
	thread_exit("failed in cloning task");
    }
    else {
       *((warp->_init_list).wb_tsks + i) = tid;
    }
  }/*for*/

  // set handler of this thread group for SIGINT.
  // This maybe helpful to obtains some information when the library gets stuck
  signal(SIGINT, kill_handler);
#ifdef SYNC_SIGNAL
  signal(SIGCONT, children_handler);
#endif
  
  // infinite loop to serve as a leader.
  default_leader_entry(ldr);
  return 0;
}

static int
init_semaphore(int nr)
{
  key_t key;
  int semid, i;
  unsigned short array[nr];

  key = ftok(SPMD_SEM_KEY, 0xff);
  if ( key == -1 ) {
    perror("ftok failed in init");
    fprintf(stderr, "ftok pathname %s\n", SPMD_SEM_KEY);
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

  char log[SPMD_MAXIMAL_FILE_LENGTH];

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
      }
    }
    /* postpone gid to creator function */
    ldrs[i].nr = 0;   
    ldrs[i].warp.width = width;
    ldrs[i].warp.fn = NULL;
    ldrs[i].warp.hook = NULL;
    ldrs[i].warp.tsks = malloc(sizeof(struct task_struct) * width);

    if ( ldrs[i].warp.tsks == NULL ) {
      perror("malloc failed");
      exit(EXIT_FAILURE);
    }

    ldrs[i].warp._init_list.stk_sz = SPMD_TASK_STACK_SIZE;
    ldrs[i].warp._init_list.stks   = slot->children_stacks[i];
    ldrs[i].warp._init_list.wb_tsks = slot->wb_tasks + i * width;
    semctl(ldrs[i].warp._init_list.sem_task_prep, 0, SETVAL, 0);

#ifdef __TIMELOG
    sprintf(log, "timelog-%d-%d.ldr", width, i);
    FILE * fh = fopen(log, "w");
    if ( fh == NULL ) {
      fprintf(stderr, "[ERROR] fopen failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    ldrs[i].warp.log_fd = fh; 
#else 
    ldrs[i].warp.log_fd = stderr;
#endif

    for (j=0; j<width; ++j) { 
      task_struct_p task = &(ldrs[i].warp.tsks[j]);

#ifdef __TIMELOG
      sprintf(log, "timelog-%d-%d-%d.tsk", width, i, j);
      fh = fopen(log, "w");
      if ( fh == NULL ) {
        fprintf(stderr, "[ERROR] fopen failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      task->log_fd = fh;
#else
      task->log_fd = stderr;
#endif
    }/*for*/ 
    
    ldrs[i].warp.time_on_fly = 0;
    ldrs[i].warp.time_in_reduce = 0;
    ldrs[i].warp.time_in_wait = 0;
    ldrs[i].warp.counter = 0;
 
    if (-1 == (pid = clone(&default_creator_entry, slot->leaders_stacks[i], 
			   CLONE_VM | CLONE_FS | SIGCHLD, ldrs+i)) ) {
      perror("clone leader failed");
      exit(EXIT_FAILURE);
    }
    else {
      *(slot->wb_leaders + i) = pid; /*writeback*/
    }/*if*/

    // wait for all tasks and leader have started up
    struct sembuf sb =  {
       .sem_num = 0, 
       .sem_op = -width-1, 
       .sem_flg = 0
    };
    semop(ldrs[i].warp._init_list.sem_task_prep, &sb, 1);
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

  if ( -1 == (semid=init_semaphore(nr_pe)) )
    return -1; /* fail to obtain sem set */
  else 
    the_pool.sem_pe = semid;

  /*reallocate resouce*/
  the_pool.nr_pe    = nr_pe;
  the_pool.nr_slot  = nr_slot;
  the_pool.nr_thread = nr_thr;
  the_pool.nr_leader = nr_leader;
  the_pool.nr_task   = nr_thr - nr_leader;

  /*too many leaders. currently, we use semaphore to simulates signal wait_for_t
   *SEMs are identified by file. see ftok(3). 0xff has already been associated with sem_pe.
   *For the maxinal num. of SEMs set, refer to /proc/sys/kernel/sem 4th col.
   */
  if ( 2*nr_leader > 0xfe ) { 
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
  }

  thread_t * ldr = the_pool.thr_leaders;
  thread_t * tsk = the_pool.thr_tasks;
  leader_struct_p leaders = the_pool.leaders;
  int k, nr;

  for (i=0, nr=nr_pe, k=0; i<nr_slot; ++i, nr>>=1) {
    for (j=0; j<(nr_pe/nr); ++j) {
      int key = ftok(SPMD_SEM_KEY, nr_leader + k);
      int key2 = ftok(SPMD_SEM_KEY, nr_leader*2 + k);

      if ( key == -1 ) {
        perror("ftok failed");
        goto err_happened3;
      }
    
      if ( -1 == (the_pool.leaders[k].sem_task = 
      		semget(key, nr, 0666 | IPC_CREAT)) ) {
        perror("semget fail");
        goto err_happened3;
      }
      if ( -1 == (the_pool.leaders[k].warp._init_list.sem_task_prep = 
      		semget(key2, 1, 0666 | IPC_CREAT)) ) {
        perror("semget fail");
        goto err_happened3;
      }
      ++k;
    } 
  }/*for*/

  for (i=0, j=nr_pe; i<nr_slot; ++i, j>>=1) {
    spmd_thread_slot_p slot = the_pool.slots + i;

    slot->wb_leaders = ldr;
    slot->wb_tasks = tsk;
    allocate_slot(j/*width*/, nr_pe/j/*size*/, slot, leaders);
    ldr = ldr + (nr_pe/j);
    tsk = tsk + nr_pe;
    leaders += (nr_pe/j);
 }

 //spmd_runtime_dump();

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
  int i, status;

  while ( !spmd_all_complete() ) {
    exp_backoff();
  }

  for ( i=0; i<the_pool.nr_leader; ++i) {
    leader_struct_p ldr = &(the_pool.leaders[i]);  

    assert( 0 == kill(ldr->gid, SIGTERM) 
    	   && "kill function failed.");

    if ( -1 == waitpid(ldr->gid, &status, 0) ) {
       fprintf(stderr, "[ERROR] returned value from waitpid: %s\n", 
     		strerror(errno)); 
    }

    if ( -1 == semctl(ldr->sem, 0, IPC_RMID) ) {
      perror("semctl IPC RMID sem");
    }

    if ( -1 == semctl(ldr->sem_task, 0, IPC_RMID) ) {
      perror("semctl IPC RMID sem_task");
    } 
    if ( -1 == semctl(ldr->warp._init_list.sem_task_prep, 0, IPC_RMID) ) {
      perror("semctl IPC RMID sem_task_prep");
    }
    fprintf(ldr->warp.log_fd, "time_on_fly = %2d\ntime_in_reduce = %2d\ntime_in_wait = %2d\n", 
    	ldr->warp.time_on_fly, 
        ldr->warp.time_in_reduce, 
	ldr->warp.time_in_wait);
    fclose(ldr->warp.log_fd);
  }/*for*/

  if ( -1 == semctl(the_pool.sem_pe, 0, IPC_RMID) ) {
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
  struct timespec spec = {.tv_sec = 0, .tv_nsec = 25000}; 
  struct sembuf sb = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};
  for (i=the_pool.nr_pe, offset=0; i>nr && i>=1; 
       offset += the_pool.nr_pe / i, i>>=1);

  if ( i == 0 || (the_pool.nr_pe % nr) != 0 ) 
    return -1; /*ILL nr */
  
  //fprintf(stderr, "create warp\n");
  ldr = NULL;
 pick_leader: cand = the_pool.leaders + offset;
  for (i=0; i < the_pool.nr_pe / nr; ++i, cand++) {
    if ( 0 == semtimedop(cand->sem, &sb, 1, &spec) ) {
      ldr = cand;
      break; 
    }
    else {
      if ( errno != EAGAIN && errno != EINTR ){
      fprintf(stderr, "[ERROR] seek leader: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
      }
    }
  }

  if ( ldr == NULL ) {
    goto pick_leader;
  }

  warp = &(ldr->warp);
  if ( nr != warp->width ) {
    if ( 0 != semctl(ldr->sem, 0, SETVAL, nr) ) {
       perror("failed set up leader sem");
       exit(EXIT_FAILURE);
    } 
    warp->width = nr;
  }
  
  warp->fn = fn;
  warp->hook = hook;
  warp->hook_arg = hk_arg;

  //FIXME: does not support customized stack size
  if ( stk_sz != 0 ) return -1;

  //select pe
  unsigned short snapshot[the_pool.nr_pe];
  struct sembuf buf[nr];
  int eno;

  for (i=0; i<nr; ++i) buf[i].sem_op = -1;
 retry:  
  semctl(the_pool.sem_pe, 0, GETALL, snapshot);
  for(i=0, j=0; j<nr && i<the_pool.nr_pe; ++i) {
    if ( snapshot[i] == 1 ) 
      buf[j++].sem_num = i;
  }
  if ( j < nr ) {
 #ifndef __NDEBUG
    fprintf(stderr, "j < nr failed, j=%d, nr=%d\n", j, nr); 
    int remained = semctl(ldr->sem, 0, GETVAL);
    int fired = semctl(ldr->sem, 1, GETVAL);
    fprintf(stderr, "remained=%d, fired=%d\n", remained, fired);
 #endif
    exp_backoff();
    goto retry;
  }
  if ( -1 == (eno=semtimedop(the_pool.sem_pe, buf, nr, &spec)) ) {
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
  //fprintf(stderr, "warpid=%d\n", warp_id);
  return warp_id;
}  

static void
spmd_runtime_dump()
{
  int i, j, k;

  //FIXME: thread-safe
  assert(spmd_initialized && "spmd rt uninitialized");

  //sleep(1); 
  fprintf(stderr, "spmd thread pool:\n");
  fprintf(stderr, "nr_pe|nr_slot|nr_leader|nr_task|nr_thread|\n");
  fprintf(stderr, "%5d|%7d|%9d|%7d|%9d|\n", the_pool.nr_pe, the_pool.nr_slot,
	 the_pool.nr_leader, the_pool.nr_task, the_pool.nr_thread);

  for (i=0; i<the_pool.nr_slot; ++i) {
    spmd_thread_slot_p slot = the_pool.slots + i;
    thread_t * ldr = slot->wb_leaders;
    thread_t * tsk = slot->wb_tasks;

    fprintf(stderr, "slot %d: %p\n", i, slot);
    fprintf(stderr, "width=%d, size=%d\n", slot->warp_width, slot->warp_size);
    
    for(j=0; j<slot->warp_size; ++j) {
      fprintf(stderr, "ldr%d=%d:(", j, *ldr++);
      for(k=0; k<slot->warp_width; ++k)
	fprintf(stderr, "%d,", *tsk++); 
      fprintf(stderr, ")\n");
    }
  }

  for (i=0; i<the_pool.nr_leader; ++i) {
     leader_struct_p ldr = &(the_pool.leaders[i]);
     fprintf(stderr, "Leader[%d] pid: %2d width = %2d\n", i, ldr->gid,
     	ldr->warp.width);
     for (j=0; j<ldr->warp.width; ++j) {
       task_struct_p tsk = &(ldr->warp.tsks[j]);
       indent(stderr, 4, ' ');
       fprintf(stderr, "%d\n", tsk->tid);
     }
  }
}

void
spmd_fire_up(leader_struct_p leader)
{
  int i, eno, last;
  task_struct_p tsk;
  cpu_set_t mask;
  unsigned short array[leader->warp.width];
  int semval;

  //fprintf(stderr,"fire up\n");
  //
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

/*change myself to that last RT cpu, pervent suspending the signal sending*/
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

#ifndef SYNC_SIGNAL
  if ( -1 == (semval=semctl(leader->sem, 0, GETVAL)) ) {
    fprintf(stderr, "[ERROR] semctl failed: %s\n", strerror(errno)); 
    exit(EXIT_FAILURE);
  }
  if ( semval != leader->warp.width ) {
    fprintf(stderr, "[ERROR] leader->sem: ILL status semval=%d\n", semval);
    exit(EXIT_FAILURE);
  }
#endif

#ifndef __NDEBUG
  semctl(leader->sem_task, 0, GETALL, array);
  for(semval=0, i=0; i<leader->warp.width; ++i) semval+=array[i];
  if ( semval ) {
    fprintf(stderr, "[ERROR] leader->sem: ILL status BEFOR Esemval=%d\n", semval);
    exit(EXIT_FAILURE);
  }
#endif

#ifdef __TIMELOG
#if defined(LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    fprintf(leader->warp.log_fd, "fire up timestamp: %d:%d\n", ts.tv_sec, 
    	ts.tv_nsec);
#endif
    gettimeofday(&(leader->warp.last_stamp), NULL);
#endif

#ifndef SYNC_SIGNAL
  //fireup
  for(i=0; i<leader->warp.width; ++i) array[i] = 1;
  if ( 0 != semctl(leader->sem_task, 0, SETALL, array) ) {
    fprintf(stderr, "[ERROR] raise leader->sem_task failed: %s\n", strerror(errno));  
    exit(EXIT_FAILURE);
  }
#else
  for (i=0; i<leader->nr; ++i) {
    tsk = &leader->warp.tsks[i];
    //fprintf("tkill tid = %d\n", tsk->tid);
    if ( -1 == tkill(tsk->tid, SIGCONT) ) {
      fprintf(stderr, "[ERROR] tkill %d SIGCONT failed: %s\n", 
        tsk->tid, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }/*for*/
#endif
  leader->nr = 0;
}

int __spmd_export
spmd_create_thread(int warp_id, void * arg) 
{
  leader_struct_p ldr;
  int t;
   //printf("warp_id = %d\n", warp_id); 
   //FIXME RESET
  if ( !( 0 <= warp_id && warp_id < the_pool.nr_leader ) ) 
    return -1; /*ILL warp_id */

  ldr = &(the_pool.leaders[warp_id]);
  if ( ldr->nr >= ldr->warp.width ) {
    return -1; /*more task than warp. already fired*/
  }

  t = ldr->nr++;

  //load function and its arguments 
  ldr->warp.tsks[t].arg = arg;
  //fprintf(stderr, "task %d arg = %p\n", t, arg);
  // variable t is local. it guarantees fireup once.
  if ( t+1 == ldr->warp.width )
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
  int i, nr = 0;

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
    //contend
    if ( 0 == semctl(ldr->sem, 1, GETVAL) ) {
      return 0;
    }
  }
  return 1;
}
