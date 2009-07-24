// This file is not supposed to be distributed.
#include "warp.h"
#include "x86/spmd.h"

/*globals*/
static struct spmd_thread_pool the_pool;
#if 0
static spinlock_t              the_lock = SPMD_SPINLOCK_INITIALIZER; /* lock to proctect data-structure
									of spmd runtime */
static int                     spmd_initialized;
#endif

/*aux funtions*/
extern int  probe_nr_processor();
extern void spinlock_init(spinlock_t * lck);
extern void spinlock_lock(spinlock_t * lck);
extern void spinlock_unlock(spinlock_t * lck);
extern int  spinlock_trylock(spinlock_t * lck);
extern pid_t gettid(void);
extern int tkill(int tid, int sig);
extern void wait_for_tg(int sem);
extern  void set_fifo(int pid, int prio);
extern  void set_normal(int pid);

int __task 
default_task_entry(void * arg)
{
  fprintf(stderr, "@task created, getppid=%d, getpid=%d gettid=%d, arg=%p\n",
	  getppid(), getpid(), gettid(), arg);
  task_struct_p task = (task_struct_p)arg;
  struct sembuf buf;

  while(1) {
    pause();
    task->fn(task->args[0], task->args[1], task->args[2]);

    fprintf(stderr, "@task %d:%d start working\n", getpid(), gettid());
    /*release physical pe */
    buf.sem_num = task->oc;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(task->sem_pe, &buf, 1);
    /*notify leader */
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    semop(task->sem_ldr, &buf, 1);
  }

  fprintf(stderr, "@got signal, task quit");
  return 0;
}

void __leader
default_leader_entry(leader_struct_p leader)
{
  while (1) {
    leader->nr = 0;
    if ( 0 != semctl(leader->sem, 0, SETVAL, leader->warp.nr) ) {
      printf("leader->sem %d\n", leader->sem);
      perror("failed set up leader sem");
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "#waiting for thread group\n");
    
    wait_for_tg(leader->sem);
    
    fprintf(stderr, "#wait returned, reduce...\n");

    if ( NULL != leader->warp.hook ) 
      (leader->warp.hook)();

    spinlock_lock(&(leader->lck));
    leader->oc = 0;
    spinlock_unlock(&(leader->lck));
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

  printf("leader created, getppid=%d, getpid=%d\n",
	 getppid(), getpid());
  
  warp->gid = getpid();


  for(i=0; i<nr; ++i) {
    task_struct_p task = &(warp->tsks[i]);
    task->gid = warp->gid;
    task->sem_pe = the_pool.sem_pe;
    task->sem_ldr = warp->sem;
    task->fn = warp->fn;
    stacks[i] = warp->init_list.stks[i];
    //printf("thread %d stack =%p\n", i, stacks[i]);
    if ( -1 == (tid = clone(&default_task_entry, stacks[i], 
			   CLONE_THREAD | CLONE_SIGHAND | CLONE_SYSVSEM | CLONE_VM | CLONE_FS \
			   | SIGCHLD/*signal sent to parent when the child dies*/,
			   task/*arg*/)) )  {
	perror("clone task failed");
	exit(EXIT_FAILURE);
      }
    else {
       printf("task is created = %d\n", tid);
       *((warp->init_list).wb_tsks + i) = task->tid = tid;
    }
    set_fifo(tid, sched_get_priority_max(SCHED_FIFO));
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
  semid = semget(key, nr, 0666 | IPC_CREAT);
  if ( semid == -1 ) {
    perror("semget failed");
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
  /*
  printf("allocate_slot width=%d, size=%d, slot=%p, ldrs=%p\n",
	 width, size, slot, ldrs);
  */
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
      printf("create leader %d\n", pid);	
      *(slot->wb_leaders + i) = pid; /*writeback*/
    }
  }// for   
}
//FIXME: I doubt it is desirable to make this function thread-safe.
//this function should be called in primary thread/process, otherwise is error-prone.
int __spmd_export
spmd_initialize()
{
  int nr_thr, nr_slot, nr_leader=0;
  int nr_pe = probe_nr_processor();
  int i, j;	
  int semid;
  key_t key;
  //FIXME: lock protection or re-entrance

  /*also works for systems with non-exponential cores
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

  the_pool.leaders = (struct leader_struct *)malloc
    (nr_leader * sizeof(struct leader_struct));
  if ( the_pool.leaders == NULL ) 
    goto err_happened2;
  
  for (i=0; i<nr_leader; ++i) {
    key = ftok(SPMD_SEM_KEY, 'a' + i);
    
    if ( key == -1 ) {
      perror("ftok failed");
      goto err_happened3;
    }
    
    the_pool.leaders[i].sem = 
      semget(key, 1, 0666 | IPC_CREAT);
    //    printf("sem leader[%1d].sem = %d\n", i, the_pool.leaders[i].sem);
  }

  thread_t * ldr = the_pool.thr_leaders;
  thread_t * tsk = the_pool.thr_tasks;
  leader_struct_p leaders = the_pool.leaders;

  for(i=0, j=nr_pe; i<nr_slot; ++i, j>>=1) {
    spmd_thread_slot_p slot = the_pool.slots + i;

    slot->wb_leaders = ldr;
    slot->wb_tasks = tsk;
    allocate_slot(j, nr_pe/j, slot, leaders);
    ldr = ldr + j;
    tsk = tsk + nr_pe;
    leaders += j;
 }
        
 ldr = the_pool.thr_leaders;
 for (i=0; i<nr_leader; ++i) {
   spinlock_init(&(the_pool.leaders[i].lck));
   the_pool.leaders[i].oc = 0; 
   the_pool.leaders[i].gid = *(ldr++);
 }

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

int  __spmd_export
smpd_create_warp(int nr, void * fn, unsigned int stk_sz, void * hook)
{
  int offset, i, j, warp_id;
  leader_struct_p cand, ldr;
  warp_struct_p warp;

  for (offset=0, i=the_pool.nr_pe; i>nr && i>=1; 
       offset += i, i <<= 1);
  if ( i == 0 || (the_pool.nr_pe % nr) != 0 ) 
    return -1; /*ILL nr */

  ldr = NULL;
  cand = the_pool.leaders + offset;
 pick_leader:
  for (i=0; i < the_pool.nr_pe / nr; ++i, cand++) {
    if ( spinlock_trylock(&(cand->lck)) ) { /* don't contend hot lock */
      if ( cand->oc == 0 )
	ldr = cand;
	break;
      }
      else 
	spinlock_unlock(&(cand->lck));
  }

  if ( ldr == NULL ) {
    sleep(1); /*gentlely re-try*/
    goto pick_leader;
  }

  ldr->oc = 1;
  warp = &(ldr->warp);
  spinlock_unlock(&(ldr->lck));
    
  warp->gid = ldr->gid;
  warp->nr = nr;
  warp->fn = fn;
  warp->hook = hook;

  //FIXME: does not support customized stack size
  if ( stk_sz != 0 ) return -1;

  unsigned short snapshot[the_pool.nr_pe];
  struct sembuf buf[nr];
  struct timespec ts;
  int eno;

  for (i=0; i<nr; ++i) buf[i].sem_op = -1;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000; /* 100 ms timeout*/
 retry:  
  semctl(the_pool.sem_pe, 0, GETALL, snapshot);
  for(i=0, j=0; j<nr && i<the_pool.nr_pe; ++i) {
    if ( snapshot[i] == 1 ) {
      buf[j++].sem_num = i;
    }
  }
  if ( j < nr ) {
    sleep(1);  /*really busy*/
    goto retry;
  }
    
  if ( -1 == (eno=semtimedop(the_pool.sem_pe, buf, nr, &ts)) ) {
    if ( eno == EAGAIN ) { /* snapshot is old, try again */
      goto retry;
    }
    perror("semop failed");
    return -1;
  }

  for (i=0; i<nr; ++i)
    ldr->warp.tsks[i].oc = buf[i].sem_num;

  warp_id = (ldr - the_pool.leaders) / sizeof(leader_struct_p);
  return warp_id;
}  

static void
spmd_runtime_dump()
{
  int i, j, k;

  //  spinlock_lock(&the_lock);
  //  assert(spmd_initialized && "spmd rt uninitialized");
  
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
  int i, eno;
  task_struct_p tsk;

  for(i=0; i<leader->nr; ++i) {
    cpu_set_t mask;
    tsk  = &leader->warp.tsks[i];
    CPU_ZERO(&mask);
    CPU_SET(tsk->oc, &mask);
    if ( -1 == (eno=sched_setaffinity(tsk->tid, sizeof(cpu_set_t), &mask)) ) {
      perror("sched_setaffinity");
      //FIXME: unload rt task for gentle quit
      exit(EXIT_FAILURE);
    }
  }
  
  for (i=0; i<leader->nr; ++i)
    tkill(tsk->tid, SIGCONT);
}

int __spmd_export
spmd_create_thread(int warp_id, void * ret, void * arg0, void * arg1)
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
    return -1; /*more task then warp. already fired*/
  }

  t = ldr->nr++;
  spinlock_unlock(&(ldr->lck));
  
  ldr->warp.tsks[t].args[0] = ret;
  ldr->warp.tsks[t].args[1] = arg0;
  ldr->warp.tsks[t].args[2] = arg1;

  if ( t == ldr->warp.nr )
    spmd_fire_up(ldr);

  return t;
}

#ifndef __NDEBUG
int 
main()
{

  printf("default_task_entry=%p\n", &(default_task_entry));
  assert( probe_nr_processor() == 2);
  assert( spmd_initialize() == 2);
  sleep(1);  
  spmd_runtime_dump();
  sleep(60);
  return 0;
}
#endif
