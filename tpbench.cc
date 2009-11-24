/* tpbench.cc
 *
 * thread benchmark for vina framework
 *
 * author: jwhust@gmail.com
 * navy.xliu@gmail.com
 */

#include "mtsupport.hpp"
#include "profiler.hpp"
#include "toolkits.hpp"
#include "threadpool.hpp"
#include "libSPMD/warp.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace vina;
#define THREAD_NUM 2
#define GROUP_NUM 128
#define WORKER_DELAY -1
#define ENFORCE_YIELD 0
#define MASK_MT 1
#define MASK_TP 2
#define MASK_WP 4

#ifdef __TIMELOG
  pthread_mutex_t dbg_lock = PTHREAD_MUTEX_INITIALIZER;
  static int dbg_task_counter;
#endif 

// T-distribution, two-tail alpha is 0.05
float T_alpha_5[] = {2.228, /*start at N=10*/
		     2.201,
		     2.179,
		     2.160,
		     2.145,
		     2.131, /* N=15 */
		     2.120,
		     2.110,
		     2.101,
		     2.093,
		     2.086, /* N=20 */
		     2.080,
		     2.074,
		     2.069,
		     2.064,
		     2.060, /* N=25 */
		     2.056,
		     2.052,
		     2.048,
		     2.045,
		     2.042, /*N=30*/
};

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
  if ( delay >= 0 ) 
    burn_usecs(delay);
 
  else if ( enf_yield )
    sched_yield();

  if ( barrier != mt::null_barrier ) 
    barrier->wait();
}
void worker2(void *arg)
{
  int delay = (*((int *)(arg)));

  if ( delay >= 0 ) 
    burn_usecs(delay);  
}
void reduce(void * arg __attribute__((unused)) )
{
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
    int id = spmd_create_warp(nr, (void *)worker2, 0, (void *)reduce, NULL);
    assert ( id != -1 && "create warp failed");
    for (int j=0; j<nr; ++j, cnt++)
      assert ( -1 != spmd_create_thread(id, &(wkr_delay))
	       && "spmd_create thread failed");
    while ( !spmd_all_complete() );
#ifdef __TIMELOG
/*
    pthread_mutex_lock(&dbg_lock);
    printf("dbg_task_counter = %d\n", dbg_task_counter);
    pthread_mutex_unlock(&dbg_lock);
*/
#endif
    
  }
  //assert( cnt == nr_thread  && "failed of counter in libspmd");
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

  printf("group overhead Expect is %8.2lf us\n", exp);
  printf("group overhead Variance is %8.2lf\n", var);
  printf("group overhead Standard Error is %8.2lf\n", sqrt(var));
  printf("\n");
}

void test_function_of(void(*f)(), 
		      const char *s)
{
  Profiler &prof = Profiler::getInstance();
  auto temp0 = prof.eventRegister(s);
  vina::Timer tmr;

  int skip = 3;        /* warmup iterations. for tp case, it's a little tricky.
			  we used the size of pool to eliminate the affact of
			  pool initialization */
  int NR;              /* the total number of experiment */
  int n;               /* the size of sample */
  float v[nr_group];   
  float t;             /* t value for alpha 0.05 */
  double exp, var;     /* Expect and Variance for Sample */
  double null;         /* null hypothese */
  double se;           /* Standard Error */
  // skip warm-up
  if ( f == group_with_tp ) {
    skip = pol_size;
  }
  NR = (nr_group - skip);
 do_sampling:
  // work load
  n = (nr_group - skip);
  printf("do_sampling\n");
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
  //sleep(1);
  null = 0.6 * tmr.elapsed() + 0.4 * prof.getEvent(temp0)->elapsed() / NR;
  exp = 0.0;
  for (int i=skip; i<nr_group; ++i) exp += (v[i]);
  exp = exp / n;
  var = 0.0;
  for (int i=skip; i<nr_group; ++i) var += (v[i] - exp) * (v[i] - exp);
  var = var / ( n - 1 );
  t = T_alpha_5[n - 11];
  se = t * sqrt(var/n);
  printf("null=%.2lf, NR=%d, exp=%.2lf sd =%.2lf, se = %.2lf, CI (%lf-%lf) p-value 0.05\n",
	 null, NR, exp, sqrt(var/n), se, (exp-se), (exp + se));

  if ( null < (exp - se)
       || null > (exp + se) )  {
    printf("refuse test\n");
    // have to repeat time
    NR += n;
    goto do_sampling;
  }
  print_result(prof.getEvent(temp0)->elapsed(), null, var, s);
}

void usage(char * n)
{
   fprintf(stderr, "Usage: %s [-t num_of_thread] [-i iteration(10-30)] [-d delay(us)] [-y yeild] [-m mask]i [-c content-level(0~10)]\n",
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
      if ( nr_group < 11 || nr_group > 31 ) {
	usage(argv[0]);
	exit(EXIT_FAILURE);
      }
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
  //fprintf(stderr, "preset wkr_delay = %d\n", wkr_delay);

  Profiler &prof = Profiler::getInstance();
  if ( wkr_delay >= 0 ) {
  auto chker = prof.eventRegister("ck-burning checker");
  prof.eventStart(chker);
  burn_usecs(wkr_delay);  
  prof.eventEnd(chker);
  }
  pol_size = nr_thread >= 16 ? nr_thread * 1.6
    : nr_thread * 1.2;
  
  if ( msk_bench & MASK_TP ) {
    thr_pool = new mt::thread_pool(pol_size);
    test_function_of(group_with_tp, "threadpool");
  }

  if ( msk_bench & MASK_MT ) 
    test_function_of(group_without_tp, "mt::thread");

  if ( msk_bench & MASK_WP ) {
    avl_pe = spmd_initialize();
    printf("avl_pe is %4d\n", avl_pe);
    assert( avl_pe != -1 && "failed to initialize libspmd runtime");
    //sleep(1);

    test_function_of(group_with_warp, "libspmd");
    while ( !spmd_all_complete() ) ;
    //sleep(1);
    spmd_cleanup();
  }
  if ( msk_bench ) 
    Profiler::getInstance().dump();
  if ( wkr_delay >= 0 ) {
  auto std = prof.eventRegister("seq");

#ifndef __NDEBUG 
  for (int i=0; i<nr_thread; ++i){
    prof.eventStart(std);
    burn_usecs(wkr_delay);  
    prof.eventEnd(std);
    sleep(1); //prohibit H/W opt.
  }
  printf("*run in sequence %d\n", prof.getEvent(std)->elapsed());
#endif
  }
  return 0;
}
