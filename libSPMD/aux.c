#include "x86/spmd.h"

static int
read_line_of(FILE * f, int n/*lines*/)
{
  int ch;
  for (; n-- > 0; ) {
    while( (ch=fgetc(f)) != '\n' && (ch != EOF) );
  }
  return ch != EOF;
}

int 
probe_nr_processor()
{
  FILE * cpuinfo;
  char buffer[80];
  int idx = 0;
  int cores;

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

  return cores;
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
