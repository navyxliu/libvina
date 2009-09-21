// This file is not supposed to be distributed.
//
/// HISTORY:
// 
// Jun.08.09' -- class hierarchy overhaul
//   Change event (un)register to string query and return
//   a handle. Online querying event position with name may
//   hurt perf.
//   I intend to keep capability of using all the events
//   standalone. However, the dual inheritances bring another
//   burden of function forward. Wish compiler inline them.
// May.30.09' -- file creation, only support timer

#ifndef XLIU_PROFILER_HXX
#define XLIU_PROFILER_HXX 1

#include <string>
#include <utility>
#include <vector>
#include <map>

#include <stdio.h>    // for sprintf, putchar
#include <assert.h>
#include <sys/time.h> // for gettimeofdate

/// for Intel 64 processors
// warning: I never test it on x86_32
//opcode: 0x0f31
#define RDTSC(ullval) ({ unsigned a, d;			\
	__asm__ __volatile__ (				\
			      "rdtsc"			\
			    : "=a" (a), "=d" (d)	\
			     );	                        \
        ullval = d; ullval<<=32; ullval |= a; })

namespace vina {
  using std::string;
  
  typedef unsigned int event_id;
  typedef unsigned long long int ull_t;

  class Timer 
  {
    timeval _start, _end;
  public:
    void start() { gettimeofday(&_start, NULL); }
    void stop() { gettimeofday(&_end, NULL); }
    
    // return: microseconds
    unsigned elapsed() const 
    { 
      return 1000000 * (_end.tv_sec - _start.tv_sec) 
      + (_end.tv_usec -  _start.tv_usec); 
    }
    const char * elapsedToStr() const
    {
      static char buf[80];
      int s = (_end.tv_sec - _start.tv_sec);
      int ms = (_end.tv_usec - _start.tv_usec);
      
      if ( ms < 0 ) s--, ms = ms + 1000000;
      sprintf(buf, "%d.%06d", s, ms);
      
      return buf;
    }
  };


  enum event_kind { 
    GENERAL_TIMER_EVT,                   /*general time span event    */
    HIT_COUNTER_EVT,                     /*hit counter event          */
    TSC_COUNTER_EVT,                     /*timestamp counter          */
    LL_CACHE_MISS_COUNTER_EVT,           /*lowest-level cache misses  */
    NUM_OF_EVT
  };

  // undefine counter
  template<event_kind>
  class Counter;

  template<>
  class Counter<HIT_COUNTER_EVT> {
    unsigned long long hit_;
  public:
    Counter() : hit_(0) {}
    void start() { hit_ = 0; }
    void stop() {}
    unsigned long long elapsed() const{ return hit_; }
    void hit() { ++hit_; }
  };
  template<>
  class Counter<TSC_COUNTER_EVT> {
    ull_t beg_, end_;
  public:
    void start(){
      RDTSC(beg_);
    }
    void stop() {
      RDTSC(end_);
    }
    unsigned elapsed() const {
      return (end_ - beg_);
    }
  };

#ifdef PMC_SUPPORT
  template<>
  class Counter<LL_CACHE_MISS_COUNTER_EVT>
  {
    ull_t beg_, end_;
    Counter(const Counter<LL_CACHE_MISS_COUNTER_EVT>&);                               // no impl.
    Counter<LL_CACHE_MISS_COUNTER_EVT>& operator=(const Counter<LL_CACHE_MISS_COUNTER_EVT>&); // no impl.
  public:
    Counter() : msr_fd_(-1) {
      perf_evt_initialize();
    }
    ~Counter();
    void start() {
      beg_ = read_pmc();
    }
    void stop() {
      end_ = read_pmc();
    }
    unsigned elapsed() const {
      return (unsigned)(end_ - beg_);
    }
  private:
    ull_t read_pmc();
    void perf_evt_initialize();
    int msr_fd_;                  // file descriptor of msr          
    unsigned pmc_;                // pmc no.
  };
#endif /* PMC_SUPPORT */ 

  // forward decl
  class TimerEvent;
  template<event_kind>  class CounterEvent;

  class Event {
  public:
    mutable unsigned sum_;
    mutable unsigned counter_;
    /*may change name after folding*/
    string name_; 
    /*user add comments*/
    string extra_; 
  protected:
    Event(const string& n, unsigned sum, unsigned counter)
      : name_(n), sum_(sum), counter_(counter) {}
    Event(const Event&);
  public:
    static Event* createTimerEvent(const string& name);
    // resolve the concrete type of counter
    static Event* createCounterEvent(const string& name, event_kind kind);

    virtual void dump() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void hit() {}
    
    virtual ~Event(){}
    unsigned elapsed() const { return sum_; }
    void     reset() const { sum_ = counter_ = 0; }
  };
  
  
  // subclass events are designed to company with Profiler,
  // who acts as a manager, or use standalone.
  class TimerEvent : public Timer,  public Event{
  public:
    TimerEvent(const string& n) 
      : Event(n, 0, 0) {}
    TimerEvent(const TimerEvent&);
    inline void start() {
      Timer::start();
    }
    
    inline void stop() {
      Timer::stop();
      counter_ += 1;
      sum_ += Timer::elapsed();
    }

    void dump() const {
      printf("%24s %4d %8dusec %10s", name_.c_str(), counter_, 
	     sum_, extra_.c_str());
    }
  };

  template <event_kind kind>
  class CounterEvent : public Counter<kind>, public Event {
  public: 
    CounterEvent(const string& n) 
      : Event(n, 0, 0) {}
    inline void start() {
      Counter<kind>::start();
    }
    inline void stop() {
      Counter<kind>::stop();
      counter_ += Counter<kind>::elapsed();
    }
    inline void hit() {
      //just count number of hits, do worry about speed. 
      if( auto h = dynamic_cast<Counter<HIT_COUNTER_EVT>*>(this) ) {
	h->hit();
      }
      // if not, skip harmlessly
    }
    void dump() const {
      if ( auto h = dynamic_cast<const Counter<HIT_COUNTER_EVT>*>(this) ) {
	printf("%18s %4d           %10s", name_.c_str(), h->elapsed(),
	     extra_.c_str());
      }
      else 
	printf("%18s %4d           %10s", name_.c_str(), counter_, 
	     extra_.c_str());
    }
  };

  /// a event-based profiler. 
  class Profiler 
    {
      mutable Timer     _timer;
      mutable int       _running;
      mutable unsigned  _elapsed;
      Profiler();
      ~Profiler();
    public:
      struct profiler_exception {};
      
      static Profiler& getInstance() { 
	static Profiler _instance;
	return _instance; 
      }
      event_id eventRegister(const string& name, 
			     event_kind kind = GENERAL_TIMER_EVT);
      void eventUnregister(const string& name);      

      // For performance consideration, the statistic is neccesarily
      // non-thread-safe. therefore, it is intended to use separated slots
      // for individual threads, then fold those event into one
      void eventFold(event_id beg, event_id end);
      
      void eventStart(event_id k) { 
	if ( _running++ == 0 ) _timer.start();

	assert(k < _last_id && _careAbout[k] != NULL); 
	_careAbout[k]->start(); 
      }
      void eventEnd(event_id k) { 
	if ( --_running == 0 ) {
	  _timer.stop();
	  _elapsed += _timer.elapsed();
	}
	assert(k < _last_id && _careAbout[k] != NULL); 
	_careAbout[k]->stop(); 
      }
      void eventHit(event_id k) {
	assert(k < _last_id && _careAbout[k] != NULL); 
	_careAbout[k]->hit();
      }
      void eventSetExtra(event_id k, const string& extra);

      const Event * getEvent(event_id k) const {
	return _careAbout[k];
      }
      void dump() const;
    private:
      std::map<const string, event_id>     _table;
      std::vector<Event *>                 _careAbout;
      event_id                             _last_id;
    };

} //end of NS

#endif // XLIU_PROFILER_HXX
