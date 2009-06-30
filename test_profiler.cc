#include "profiler.hpp"
#include "toolkits.hpp"
#include <math.h>
using namespace vina;

int main()
{
  volatile int dummy;
  volatile float dummyf;

  Profiler &prof = Profiler::getInstance();

  auto pc  = prof.eventRegister("i am a pc");
  auto mac = prof.eventRegister("i am a mac", GENERAL_TIMER_EVT);
  auto hit = prof.eventRegister("i am a HIT counter", HIT_COUNTER_EVT);
  auto tsc = prof.eventRegister("i am a TSC counter", TSC_COUNTER_EVT);
#ifdef PMC_SUPPORT
  auto llc = prof.eventRegister("i am a llc miss counter", LL_CACHE_MISS_COUNTER_EVT);
#endif  
  prof.eventStart(pc);
  prof.eventStart(tsc);
#ifdef PMC_SUPPORT
  prof.eventStart(llc);
#endif
  for (int i=0; i<1000000; ++i)
    //    {  dummy = ~(2 * i << 3); }
    for(int j=0; j<1000; ++j)
      2.0*3.48+ 1223;
#ifdef PMC_SUPPORT
  prof.eventEnd(llc);
#endif
  prof.eventEnd(tsc);
  prof.eventEnd(pc);
  printf("gflop=%f\n", Gflops(1000000 * 1000.0 * 2.0, prof.getEvent(pc)->elapsed()));
  prof.eventStart(hit);
  prof.eventStart(mac);
  for (int i=0; i<1000000; ++i)
    { 
      dummyf = sin(1.2 * dummy) * 3.14 - 0.12; 
      prof.eventHit(hit);
    }
  prof.eventEnd(mac);
  prof.eventEnd(hit);
  
  prof.dump();
#ifdef PMC_SUPPORT
  prof.eventStart(llc);
  prof.eventEnd(llc);
  prof.eventFold(pc, mac+1);
  prof.dump();  
#endif

  auto timer0 = prof.eventRegister("timer0");
  auto timer1 = prof.eventRegister("timer1");
  auto timer2 = prof.eventRegister("timer2");
  auto timer3 = prof.eventRegister("timer3");
  auto timer4 = prof.eventRegister("timer4");
  printf("timer0_id %d\n", timer0);
  printf("timer1_id %d\n", timer1);
  printf("timer2_id %d\n", timer2);
  printf("timer3_id %d\n", timer3);
  printf("timer4_id %d\n", timer4);

  prof.eventStart(timer1);
  sleep(3);
  prof.eventEnd(timer1);

  printf("timer0 elapsed %f\n", (float)prof.getEvent(timer1)->elapsed());
  prof.eventStart(timer3);
  sleep(10);
  prof.eventEnd(timer3);
  printf("timer1 elapsed %f\n", (float)prof.getEvent(timer3)->elapsed());
  
  prof.eventStart(timer0);
  prof.eventStart(timer1);
  sleep(1);
  prof.eventEnd(timer1);
  prof.eventEnd(timer0);

  printf("timer0 elapsed %f\n", (float)prof.getEvent(timer0)->elapsed());
  prof.eventStart(timer2);
  sleep(1);
  prof.eventEnd(timer2);

  prof.dump();
}
