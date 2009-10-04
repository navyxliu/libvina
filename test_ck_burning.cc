#include "toolkits.hpp"
#include "profiler.hpp"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
using namespace vina;

int
main(int argc, char *argv[])
{
  int t;
  double diff;
  long elapsed;
  t = atoi(argv[1]);
  assert( initialize_ck_burning()
         && "failed to initialize ck burning");
  Profiler& prof
    = Profiler::getInstance();

  auto timer = prof.eventRegister("test ck");
  prof.eventStart(timer);
  burn_usecs(t);
  prof.eventEnd(timer);

  elapsed = prof.getEvent(timer)->elapsed();

  diff=fabs(100.0*(t-elapsed)/t);
  printf("result:\t%d\t%lu\t%lf%%\n", t, elapsed, diff);
}
