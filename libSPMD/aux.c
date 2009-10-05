//History
// Oct. 5, modified wait_for_tg interface. add parameter to perform reset operation.
//         reset value of the semaphore to the number of preset value of warp.
//         Users still have chance to change this value less then @param reset value.

#include "x86/spmd.h"
#include <string.h>

extern struct spmd_thread_pool the_pool;

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
count_keyword_in_line(FILE * f, const char * key)
{
  char * line = NULL;	 
  size_t len;
  ssize_t read;
  int cnt = 0; 
 
  line = NULL;
  while ((read = getline(&line, &len, f)) != -1 ) {  
    if ( NULL != strstr(line, key) ) {
	cnt ++;
    }
  }
  
  if ( line ) 
    free(line);

  return cnt;
}

int 
probe_nr_processor()
{
  FILE * cpuinfo;
  char buffer[80];
  int idx = 0;
  int cores, cores_smp;
  /*one-way processor*/
  cpuinfo = fopen("/proc/cpuinfo", "r");
  if ( cpuinfo == NULL || !read_line_of(cpuinfo, 11/*skip front 11 lines*/)) {
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
#ifdef SMP_SUPPORT
  /* startover, multi-processor machine might contains more processors, e.g.
     2-way processor, has two quadcores processors, which is 2 * 4 cores */
  rewind(cpuinfo);
  cores_smp =  count_keyword_in_line(cpuinfo, "processor"); 
	
  return cores_smp > cores ? cores_smp : cores;
#else
  return cores;
#endif
}

void
spinlock_init(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_init(lck, NULL) ) {
    perror("pthread mutex init failed");
    exit(EXIT_FAILURE);
  }
}

void
spinlock_destroy(spinlock_t * lck)
{
  int ret;

  if ( 0 != (ret=pthread_mutex_destroy(lck)) ) {
    //perror("pthread mutex destroy failed");
    if ( ret == EBUSY ) fprintf(stderr, "EBUSY\n");
    else if ( ret == EINVAL ) fprintf(stderr, "EINVAL\n");
    else if ( ret == EINTR ) 
      fprintf(stderr, "EINTR, These functions shall not return an error code \
of [EINTR]--man pthread_mutex_destroy\n");
    // exit(EXIT_FAILURE);
  }
}

void
spinlock_lock(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_lock(lck) ) {
    perror("pthread mutex lock faied");
    exit(EXIT_FAILURE);
  }
}

void
spinlock_unlock(spinlock_t * lck)
{
  if ( 0 != pthread_mutex_unlock(lck) ) {
    perror("pthread mutex unlock failed");
    exit(EXIT_FAILURE);
  }
}

int 
spinlock_trylock(spinlock_t * lck)
{
  int eno;
  if ( 0 == (eno=pthread_mutex_trylock(lck)) ) 
    return 1;/*return true*/
  else if ( EBUSY == eno ) 
    return 0;/*return false*/
  else { 
    perror("pthread mutex trylock failed");
    exit(EXIT_FAILURE);
  }
}

pid_t gettid(void)
{
  return syscall(SYS_gettid);
}

int tkill(int tid, int sig)
{
  return syscall(SYS_tkill, tid, sig);
}

int thread_exit()
{
  // flush files to make sure errors are avaible 
  int pid, tid, i, j;
  int nr;

  pid = getpid();
  tid = gettid();
  int is_leader = pid == tid;
  leader_struct_p ldr;
  for (i=0; i<the_pool.nr_leader; ++i) {
    ldr = &(the_pool.leaders[i]);
    if ( ldr->gid == pid ) {
      fclose(ldr->warp.log_fd);
      break;
    }
  }
  if ( i == the_pool.nr_leader ) {
    fprintf(stderr, "wrong call of thread_exit outside\
    of libSPMD.\n");
    exit(EXIT_FAILURE);
  }

  for (nr=the_pool.nr_pe, j=0; j < i; 
       j+=the_pool.nr_pe/nr, nr>>=1);

  for (i=0; i<nr; ++i) {
    task_struct_p tsk = &(ldr->warp.tsks[i]);
    fclose(tsk->log_fd); 
  }

  if ( is_leader ) 
    fprintf(stderr, "call thread_exit from leader %d\n", pid);
  else 
    fprintf(stderr, "call thread_exit from task [%d:%d]\n", pid, tid);

  kill(getppid(), SIGKILL);	
}

void wait_for_tg(leader_struct_p leader)
{
  int ret;
  struct sembuf buf[2] = {
    {.sem_num = 0,
     .sem_op  = 0,
     .sem_flg = 0
    },
    {.sem_num = 0, 
     .sem_op = leader->warp.nr,
     .sem_flg = 0
    }
  };
		   

 /*
  *sleep until all of leader managed threads finished
  * Although leader's already masked SIGCONT,
  * this operation still may be interrupted.
  * to tolerate EINTR error,  we just retry semaphore op again.
  */
  do {
    ret = semop(leader->sem, buf, (sizeof(buf)/sizeof(buf[0])));
    fprintf(leader->warp.log_fd, "return from sempo ret = %d\n", ret);
  } while( ret == -1 && errno == EINTR );
  
  if ( ret != 0 ) {
    fprintf(leader->warp.log_fd, "[ERROR] wait_for_tg failed: %s\n", strerror(errno));
    thread_exit();
  }
#ifndef __NDEBUG
  fprintf(stderr, "wait for tg sem=%d, reset=%d pid=%d\n", 
  	sem, reset, getpid());
#endif
}

/**set schduler to rt fifo.
   [prio] -- priority of rt schduler (0-99)
   99 is the highest priority
**/
void 
set_fifo(int pid, int prio)
{
  struct sched_param sp;
  
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = prio;

  if (sched_setscheduler(pid, SCHED_FIFO, &sp) == -1) {
    fprintf(stderr, "sched_setscheduler failed\n");
  }
}

void 
set_normal(int pid)
{
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = 0;
  if (sched_setscheduler(pid, SCHED_OTHER, &sp) == -1) {
    fprintf(stderr, "Weird, could not unset RT scheduling!\n");
  }
}
