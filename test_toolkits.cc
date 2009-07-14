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

  
  printf("pool size_ = %d\n", pool_t.size());
  printf("pool get() = %p\n", pool_t::get());
  printf("pool get() = %p\n", pool_t.get());
#endif

  assert( initialize_ck_burning()
	 && "failed to initialize ck burning");
  Profiler& prof 
    = Profiler::getInstance();
  
  auto timer = prof.eventRegister("test ck burning");
  prof.eventStart(timer);
  burn_usecs(1000000);
  prof.eventEnd(timer);
  auto timer2 = prof.eventRegister("sleep");
  prof.eventStart(timer2);
  sleep(1);
  prof.eventEnd(timer2);
  prof.dump();
}
