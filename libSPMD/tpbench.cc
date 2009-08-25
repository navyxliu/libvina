/* tpbench.cc
 *
 * thread benchmark for vina framework
 *
 * author: jwhust@gmail.com
 */

#include "../mtsupport.hpp"
#include "../profiler.hpp"
#include "../toolkits.hpp"
#include "threadpool.hpp"
#include "warp.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

using namespace vina;
#define THREAD_NUM 2
#define GROUP_NUM 128
#define WORKER_DELAY -1
#define ENFORCE_YIELD 0
#define MASK_MT 1
#define MASK_TP 2
#define MASK_WP 4

//globals
int nr_thread;
int nr_group;
int wkr_delay;
int enf_yield;
int pol_size;
int avl_pe;
int msk_bench;
int cont_level; // content level 0~10, 0 is the most peaceful condition
mt::thread_pool * thr_pool;

void worker(mt::barrier_t barrier,
	    int delay /*us*/)
{
<<<<<<< local
  if ( delay >= 0 ) 
=======
  if ( likely(delay >= 0) ) 
>>>>>>> other
    burn_usecs(delay);
<<<<<<< local
 
=======
>>>>>>> other
  else if ( enf_yield )
    sched_yield();

  if ( barrier != mt::null_barrier ) 
    barrier->wait();
}
void worker2(void * ret, void * arg0, void * arg1)
{
  int delay = (*((int *)(arg0)));
  //fprintf(stderr, "work2 delay = %d\n", delay);
  if ( delay >= 0 ) 
    burn_usecs(delay);  
}

void group_without_tp()
{
  mt::barrier_t barrier(new boost::barrier(nr_thread + 1));
  for(int i=0; i<nr_thread; i++)  {
    mt::thread_t one(bind(worker, barrier, wkr_delay));
    one.detach();
  }
  
  barrier->wait();
}


void group_with_tp()
{
  mt::barrier_t barrier(new boost::barrier(nr_thread + 1));
  for(int i=0; i<nr_thread; i++) {
    thr_pool->run(boost::bind(worker, barrier, wkr_delay));
  }
  
  barrier->wait();
}
void group_with_warp()
{
  static int inner_loop;
  static int nr;
  int cnt = 0;

  // to reduce the cost of management, use static variable here.
  if ( nr == 0) {
    if ( nr_thread > avl_pe ) {
      assert( (nr_thread % avl_pe) == 0 && "nr_thread is invalid");
      nr = avl_pe;
      inner_loop = nr_thread / avl_pe;
    }
    else {
      inner_loop = 1;
      nr = nr_thread;
    }
  }
  /*
  fprintf(stderr, "libspmd nr = %d, avl_pe= %d, inner_loop=%d\n", 
	  nr, avl_pe, inner_loop);
  */
  for (int i=0; i<inner_loop; ++i) {
    int id = spmd_create_warp(nr, (void *)worker2, 0, NULL);
    assert ( id != -1 && "create warp failed");
    for (int j=0; j<nr; ++j, cnt++)
      assert ( -1 != spmd_create_thread(id, NULL, &(wkr_delay), NULL)
	       && "spmd_create thread failed");
    
  }
  assert( cnt == nr_thread  && "failed of counter in libspmd");
}

void print_result(unsigned long t,/*in micro sec, us*/ 
		  double exp, double var, 
		  const char *s)
{
  float grp;
  
  printf("********** Result of ---%s--- **********\n", s);
  printf("Group Num is: %d\n", nr_group);
  printf("Thread in one group is: %d\n", nr_thread);
  printf("Enforce to yield process: %d\n", enf_yield);
  printf("Poll size is: %d\n", pol_size);
  printf("content level: %d\n", cont_level);
  if ( wkr_delay >= 0 ) 
    printf("Worker delay is %d us using CK-Burning\n", wkr_delay);
  
  printf("Total time is %ul us\n", t);

<<<<<<< local
  printf("group overhead Expect is %8.2lf us\n", exp);
  printf("group overhead Variance is %8.2lf\n", var);
  printf("group overhead Standard Error is %8.2lf\n", sqrt(var));
=======
  if ( wkr_delay >= 0 ) 
    printf("Worker delay is %d usec using CK-Burning\n", wkr_delay);
  
  printf("Total time is %u ms\n", t);
  if ( wkr_delay >= 0 ) 
    grp = (float)t / nr_group - (float)wkr_delay;
  else {
    grp = (float)t / nr_group;
  }
printf("para 1 is %lf\n", (float)t / nr_group);
printf("para 2 is %lf\n", (float)wkr_delay);
  printf("One group overhead is %8.2lf ms\n", grp);
  printf("amortized thread cost %8.2lf ms\n", grp/nr_thread);
>>>>>>> other
  printf("\n");
}

void test_function_of(void(*f)(), 
		      const char *s)
{
  Profiler &prof = Profiler::getInstance();
  auto temp0 = prof.eventRegister(s);
  vina::Timer tmr;
  int skip = 3;
  float v[nr_group];
  double exp, var;

  // skip warm-up
  if ( f == group_with_tp ) {
    skip = pol_size;
  }
  //work load
  for(int i=0; i<nr_group; i++) {
    if ( (rand()%10) >= cont_level ) 
      sleep(1);
    tmr.start();
    if ( likely(i >= skip) ) prof.eventStart(temp0);
    f();
    if ( likely(i >= skip) ) prof.eventEnd(temp0);
    tmr.stop();
    v[i] = tmr.elapsed();
    printf("test #%d: %.0f\n", i, v[i]);
    //fprintf(stderr, "test #%d: %s\n", i , tmr.elapsedToStr());
  }
  exp = prof.getEvent(temp0)->elapsed() / (nr_group - skip);
  var = 0.0;
  for (int i=skip; i<nr_group; ++i) var += (v[i] - exp) * (v[i] - exp);
  var /= (nr_group - skip);
  print_result(prof.getEvent(temp0)->elapsed(), exp, var, s);
}

void usage(char * n)
{
   fprintf(stderr, "Usage: %s [-t num_of_thread] [-i iteration] [-d delay(us)] [-y yeild] [-m mask]i [-c content-level(0~10)]\n",
	      n); 
}

int 
main(int argc, char *argv[])
{
  nr_thread = THREAD_NUM;
  nr_group  = GROUP_NUM;
  wkr_delay = WORKER_DELAY;
  enf_yield = ENFORCE_YIELD;
  int opt;

  while ( (opt=getopt(argc, argv, "hyt:i:d:m:c:")) 
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
    case 'm':
      msk_bench = atoi(optarg);
      break;
    case 'c':
	cont_level = atoi(optarg);
	if ( 0<= cont_level && cont_level <= 10) 
		break;
	//else fall
    default:
    	usage(argv[0]);
      exit(-1);
    }
  }

  // init ck_burning
  assert( initialize_ck_burning()
    && "failed to initialize ck burning");

  Profiler &prof = Profiler::getInstance();
  auto chker = prof.eventRegister("ck-burning checker");
  prof.eventStart(chker);
  burn_usecs(wkr_delay);  
  prof.eventEnd(chker);

  pol_size = nr_thread >= 16 ? nr_thread * 1.6
    : nr_thread * 1.2;
  
<<<<<<< local
  if ( msk_bench & MASK_TP ) {
    thr_pool = new mt::thread_pool(pol_size);
    test_function_of(group_with_tp, "threadpool");
  }
=======
  thr_pool = new mt::thread_pool(pol_size);

  // init ck_burning
  assert( initialize_ck_burning()
    && "failed to initialize ck burning");

  // start test
  test_function_of(group_with_tp, "threadpool");
  test_function_of(group_without_tp, "mt::thread");
>>>>>>> other

  if ( msk_bench & MASK_MT ) 
    test_function_of(group_without_tp, "mt::thread");

  if ( msk_bench & MASK_WP ) {
    avl_pe = spmd_initialize();
    assert( avl_pe != -1 && "failed to initialize libspmd runtime");
    sleep(1);

    test_function_of(group_with_warp, "libspmd");
    //sleep(5);
    spmd_cleanup();
  }
  if ( msk_bench ) 
    Profiler::getInstance().dump();

  auto std = prof.eventRegister("seq");

  for (int i=0; i<nr_thread; ++i){
    prof.eventStart(std);
    burn_usecs(wkr_delay);  
    prof.eventEnd(std);
    sleep(1); //prohibit H/W opt.
  }

  printf("*run in sequence %d\n", prof.getEvent(std)->elapsed());

  return 0;
}
