#include "toolkits.hpp"
#include "trait.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset

#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#ifdef LINUX
#include <time.h> // for clock_gettime
#include <sched.h>
#endif

namespace vina {
  unsigned long loops_per_msec;
  
  void CallsiteOutput::initializeGraph(const string& graphname)
    {
      (*os_) << "digraph " << graphname << "{\n"
	     << "label=\"CallSite Graph\"\n"
	     << "edge [fontsize=8, fontname=\"Times-Italic\"]\n";
    }
  void CallsiteOutput::finalizeGraph()
  {
    (*os_) << "}"
	   << "#end of file";
  }
  void CallsiteOutput::enter(){
    stack_.push(temp_);
  }
  void CallsiteOutput::enterMT() {
    std::ostringstream oss;
    int gid = grp_++;
    oss << "cluster_" << gid;
    std::string sub = oss.str();
    
    (*os_) << "subgraph " << sub
	   << "{\n"
	   << "style=filled\n"
	   << "color=lightgray\n"
	   << "label=\"\"";
    }
  void CallsiteOutput::leave(){
    stack_.pop();
  }
  void CallsiteOutput::leaveMT() {
    (*os_) << "}\n";
  }
  // link last node to all dependences
  // side-effect: clear out dep_
  void CallsiteOutput::linkdep() {
    for (auto i=dep_.begin(); i != dep_.end(); ++i)
      {
	(*os_) << *i << " -> " << temp_
	       << "[style=dotted]\n";
      }
    dep_.clear();
  }
  void CallsiteOutput::callsite(const string& target)
  {
    this->callsiteN(target);

    if ( stack_.size() != 0 ){
      (*os_) << stack_.top() << " -> " << temp_ << "\n";
    }
  }
  void CallsiteOutput::callsiteN(const string& target)
  {
    std::ostringstream oss;
    std::string tar;
    
    int tid = cnt_++; // get a id, invisible for dot graph
    oss << target << "_" << tid;
    tar = oss.str();
    
    (*os_) << tar << "[label=\""
	   << target << "\"]\n";
 
    temp_ = tar;
  }
  void CallsiteOutput::cluster(const string& tag)
  {
    std::ostringstream oss;

    oss << "\nsubgraph cluster_" << tag << cnt_ <<  "{\nlabel=\"\"";
    oss << "\nstyle=invis\n";
    (*os_) << oss.str();
    callsite(tag);
    (*os_) << "}\n";
  }  

#ifdef LINUX
  unsigned long long get_nsecs(struct timespec *myts)
  {
    if (clock_gettime(CLOCK_REALTIME, myts))
      printf("clock_gettime failed\n");
    return (myts->tv_sec * 1000000000 + myts->tv_nsec );
  }
  
  unsigned long get_usecs(struct timespec *myts)
  {
    if (clock_gettime(CLOCK_REALTIME, myts))
      printf("clock_gettime failed\n");
    return (myts->tv_sec * 1000000 + myts->tv_nsec / 1000 );
  }
#else 
  unsigned long long 
  get_nsecs(struct timeval * myts)
  {
    if (gettimeofday(myts, NULL))
      printf("gettimeofday failed\n");
    return (myts->tv_sec * 1000000000 + myts->tv_usec *1000);
  }
  unsigned long 
  get_usecs(struct timeval * myts)
  {
    if (gettimeofday(myts, NULL))
      printf("gettimeofday failed\n");
    return (myts->tv_sec * 1000000 + myts->tv_usec);
  }
#endif
  

static void 
burn_loops(unsigned long loops)
{
        unsigned long i;

        /*
         * We need some magic here to prevent the compiler from optimising
         * this loop away. Otherwise trying to emulate a fixed cpu load
         * with this loop will not work.
         */
        for (i = 0 ; i < loops ; i++)
             asm volatile("" : : : "memory");
}

/*
 * In an unoptimised loop we try to benchmark how many meaningless loops
 * per second we can perform on this hardware to fairly accurately
 * reproduce certain percentage cpu usage
 */
static unsigned long 
calibrate_loop(void)
{
        unsigned long long start_time, run_time = 0;
	unsigned long long bogus_loop;
        unsigned long loops;
#ifdef LINUX
        struct timespec myts;
#else 
	struct timeval myts;
#endif
        bogus_loop = 100000;
redo:
        /* Calibrate to within 1% accuracy */
        while (run_time > 1010000 || run_time < 990000) {
	  loops = bogus_loop;
	  start_time = get_nsecs(&myts);
	  burn_loops(loops);
	  run_time = get_nsecs(&myts) - start_time;
	  bogus_loop = (1000000 * bogus_loop / run_time ? :
                        bogus_loop);
        }

        /* Rechecking after a pause increases reproducibility */
        sleep(1);
        loops = bogus_loop;
        start_time = get_nsecs(&myts);
        burn_loops(loops);
        run_time = get_nsecs(&myts) - start_time;

        /* Tolerate 5% difference on checking */
        if (run_time > 1050000 || run_time < 950000)
                goto redo;

        return bogus_loop;
}

/*
 * This function measures the loops per msec
 * and write the result to file vina.loops_per_ms
 * so you can directly use the result if running on a same machine
 * by fp = fopen(fname, "r"); fscanf(fp, "%lu", &ud.loops_per_ms);
 */
static void 
calc_loop()
{
	FILE *fp;
        /* 
         * This file stores the loops_per_ms to be reused in a filename that
         * can't be confused
         */
        char *fname = "vina.loops_per_ms";

	printf("Start to benchmark loops_per_ms...\n");
#ifdef LINUX
	set_fifo(0, 99);
	loops_per_msec = calibrate_loop();
	printf("loop per msec: %lu\n", loops_per_msec);
	set_normal(0);
#else
	loops_per_msec = calibrate_loop();
#endif
#ifdef LINUX 
	// LINUX system is relatively acurate,
	// intend to trust the figure "re-usable"
	//write loops_per_msec to file
	//so you can use it directly without calc loops_per_msec again
        if (!(fp = fopen(fname, "w")))
                fprintf(stderr, "Unable to write to file vina.loops_per_ms\n");
        
        fprintf(fp, "%lu", loops_per_msec);
        fprintf(stderr, "%lu loops_per_ms saved to file vina.loops_per_ms\n",
                loops_per_msec);
        if (fclose(fp) == -1)
	  printf("fclose failed\n");
#endif
}
  bool
  initialize_ck_burning()
  {
    unsigned long long start_time, run_time = 0;
    unsigned long loops;
#ifdef LINUX
    struct timespec myts;
#else 
    struct timeval myts;
#endif
#ifdef LINUX
    if ( 0 == loops_per_msec ) {
      FILE * bogus = fopen("vina.loops_per_ms", "r");
      if (bogus != NULL){
      fscanf(bogus, "%lu", &loops_per_msec);
      fclose(bogus);
      }
    }
#endif
    if ( 0 == loops_per_msec )
      calc_loop();
    
    // last-minute verification
    sleep(1);
    loops = loops_per_msec;
    start_time = get_nsecs(&myts);
    burn_loops(loops);
    run_time = get_nsecs(&myts) - start_time;
    
    /* Tolerate 5% difference on checking */
    return  (950000 < run_time && run_time < 1050000);
  }
  void 
  burn_usecs(unsigned long usecs)
  {
    unsigned long ms_loops;
    
    ms_loops = loops_per_msec / 1000 * usecs;
    burn_loops(ms_loops);
  }
#ifdef LINUX
  void 
  set_fifo(int pid, int prio)
  {
    struct sched_param sp;
    
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio;
    if (sched_setscheduler(pid, SCHED_FIFO, &sp) == -1) {
      if (errno != EPERM)
	printf("sched_setscheduler failed\n");
    }
  }
  
  void 
  set_normal(int pid)
  {
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 0;
    if (sched_setscheduler(pid, SCHED_OTHER, &sp) == -1) {
      fprintf(stderr, "Weird, could not unset RT scheduling!\n");
    }
  }
#endif
}//endof NS
