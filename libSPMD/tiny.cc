#include <stdio.h>
#include <assert.h>
#include "../profiler.hpp"
#include "warp.h"

void 
dummy(void * ret, 
      void * arg0, 
      void * arg1)
{
  //printf("i am %d\n", spmd_get_taskid());
}

int
main(int argc, char * argv[]) 
{
  int nr = atoi(argv[1]);
  int pe = spmd_initialize();
  int i;

  printf("spmd rts pe = %d\n", pe);
  int id = spmd_create_warp(nr, (void *)&dummy, 0, NULL);
  assert( id != -1 && "create warp failed");
  sleep(5);

  vina::Profiler& prof = vina::Profiler::getInstance();
  vina::event_id evt = prof.eventRegister("timer");
  prof.eventStart(evt);

  for (i=0; i<nr; ++i) 
    spmd_create_thread(id, NULL, NULL, NULL);
  prof.eventEnd(evt);

  spmd_cleanup();
  prof.dump();
}
