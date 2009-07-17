/* tpbench.cc
 *
 * thread benchmark for vina framework
 *
 * author: jwhust@gmail.com
 */

#include "../mtsupport.hpp"
#include "../profiler.hpp"
#include "threadpool.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

using namespace vina;
#define THREAD_NUM 4
#define GROUP_NUM 128
#define WORKER_DELAY -1
#define ENFORCE_YIELD 0

//globals
int nr_thread;
int nr_group;
int wkr_delay;
int enf_yield;
int pol_size;
mt::thread_pool * thr_pool;

void worker(mt::barrier_t barrier,
	    int delay /*sec*/)
{
  if ( delay >= 0 ) sleep(delay);
  else if ( enf_yield )
    sched_yield();

  barrier->wait();
}

void group_without_tp()
{
  mt::barrier_t barrier(new boost::barrier(nr_thread + 1));
  for(int i=0; i<nr_thread; i++)
    {
      mt::thread_t one(bind(worker, barrier, wkr_delay));
      one.detach();
    }
  
  barrier->wait();
}


void group_with_tp()
{
  mt::barrier_t barrier(new boost::barrier(nr_thread + 1));
  for(int i=0; i<nr_thread; i++)
    {
      thr_pool->run(boost::bind(worker, barrier, wkr_delay));
    }
  
  barrier->wait();
}

void print_result(unsigned long t, 
		  const char *s)
{
  float grp;
  
  printf("********** Result of ---%s--- **********\n", s);
  printf("Group Num is: %d\n", nr_group);
  printf("Thread in one group is: %d\n", nr_thread);
  printf("Enforce to yield process: %d\n", enf_yield);
  printf("Poll size is: %d\n", pol_size);

  if ( wkr_delay >= 0 ) 
    printf("Worker delay is %d\n", wkr_delay);
  
  printf("Total time is %u ms\n", t);
  if ( wkr_delay >= 0 ) 
    grp = (float)t / nr_group - (float)wkr_delay * 1000000;
  else {
    grp = (float)t / nr_group;
  }
  printf("One group overhead is %8.2lf ms\n", grp);
  printf("amortized thread cost %8.2lf ms\n", grp/nr_thread);
  printf("\n");
}

void test_function_of(void(*f)(), 
		      const char *s)
{
  Profiler &prof = Profiler::getInstance();
  auto temp0 = prof.eventRegister(s);
  //start
  prof.eventStart(temp0);
  //work load
  for(int i=0; i<nr_group; i++)
    f();

  //end
  prof.eventEnd(temp0);
  print_result(prof.getEvent(temp0)->elapsed(), s);
}

int 
main(int argc, char *argv[])
{
  nr_thread = THREAD_NUM;
  nr_group  = GROUP_NUM;
  wkr_delay = WORKER_DELAY;
  enf_yield = ENFORCE_YIELD;
  int opt;

  while ( (opt=getopt(argc, argv, "yt:i:d:")) 
	  != -1 ) {
    switch(opt) {
    case 't':
      nr_thread = atoi(optarg);
      break;
    case 'i':
      nr_group  = atoi(optarg);
      break;
    case 'd':
      wkr_delay = atoi(optarg);
      break;
    case 'y':
      enf_yield = 1;
      break;
    default:
      fprintf(stderr, "Usage: %s [-t num_of_thread] [-i iteration] [-d delay time] [-y yeild]\n",
	      argv[1]);
      exit(-1);
    }
  }
  //  thr_pool = new mt::thread_pool(nr_thread*2);
  pol_size = nr_thread >= 16 ? nr_thread * 1.6
    : nr_thread * 1.2;
  
  thr_pool = new mt::thread_pool(pol_size);
  test_function_of(group_with_tp, "threadpool");
  test_function_of(group_without_tp, "mt::thread");

  Profiler::getInstance().dump();
  return 0;
}
