#include "toolkits.hpp"
#include "profiler.hpp"

#include <stdio.h>
#include <assert.h>
using namespace vina;

int 
main()
{
  float ghz = caliberate();
  printf("the freqency of processor is %.2fGhz\n", ghz);
#ifdef __USEPOOL
  typedef memory_pool<Matrix<float, 12, 12>, 3, true> pool_t;
  memory_pool<Matrix<float, 12, 12>, 3, true> pool;
  
  printf("sizeof(pool)=%d\n", sizeof(pool));

  
  printf("pool size_ = %d\n", pool_t::size());
  printf("pool get() = %p\n", pool_t::get());
  printf("pool get() = %p\n", pool_t::get());
#endif

  assert( initialize_ck_burning()
	 && "failed to initialize ck burning");
  Profiler& prof 
    = Profiler::getInstance();
  
  auto timer = prof.eventRegister("test ck -- 1 msec");
  prof.eventStart(timer);
  burn_usecs(1000);
  prof.eventEnd(timer);
  
  auto timer2 = prof.eventRegister("test ck -- 50 usec");
  prof.eventStart(timer2);
  burn_usecs(50);
  prof.eventEnd(timer2);
  
  auto timer3 = prof.eventRegister("test ck -- 1 sec");
  //xliu: ck-burning is broken for secends delay in (fedora7, duo core2).
  // however, jw reported it is relative accurate in (RHEL, nehalem)
  // we strongly encourage users to use sleep(2) for large time-span.
  // hot burning approach serve for short delay.
  set_fifo(0, 99);
  {
    prof.eventStart(timer3);
    burn_usecs(1000000);
    prof.eventEnd(timer3);
  }
  set_normal(0);

  auto timer4 = prof.eventRegister("sleep 1sec");
  prof.eventStart(timer4);
  sleep(1);
  prof.eventEnd(timer4);

  prof.dump();
}
