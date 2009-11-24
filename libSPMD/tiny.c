#include <stdio.h>
#include <assert.h>
#include "warp.h"
#define ITERATION 100
void 
dummy(void * arg)
{
  //printf("i am %d\n", spmd_get_taskid());
}

void 
reduce(void * arg)
{
  //printf("reduction %d\n", getpid());
}

int
main(int argc, char * argv[]) 
{
  int nr = atoi(argv[1]);
  int pe = spmd_initialize();
  int k, i;

  printf("spmd rts pe = %d\n", pe);

  assert ( pe == spmd_available_thread() 
	&& "wrong num. of available thread");
  for ( k=0; k<ITERATION; ++k) {
  //fprintf(stderr, "%d\n", k);

  int id = spmd_create_warp(nr, (void *)&dummy, 0, (void*)&reduce, NULL);
  assert( id != -1 && "create warp failed");

  for (i=0; i<nr; ++i) 
    assert ( -1 != spmd_create_thread(id, NULL) );
	
  }
  spmd_cleanup();
  return 0;
}
