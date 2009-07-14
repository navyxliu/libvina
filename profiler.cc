#include "profiler.hpp"
#include "cpuid.h"        //for cpuid

#include <errno.h>        //for errno, perror
#include <unistd.h>       //for open, pread
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>   
#include <stdio.h>        //for fprintf, stderr

// model specific register
// Intel developer's manual - system programmer'guide - appendix B

namespace msr{
  union ia32_perfevtsel_msr{
    struct machine_impl{
      unsigned int event_sel   : 8;
      unsigned int unit_mask   : 8;
      bool         user        : 1;
      bool         os          : 1;
      bool         edge_detect : 1;
      bool         pin_control : 1;
      bool         int_enable  : 1;
      bool         anythread   : 1;
      bool         enable      : 1;
      bool         invert      : 1;
      unsigned int counter_mask: 8;
    };
    unsigned long long int opaque_value;
  };
}

#define MSR_IA32_PERFEVT_SEL0                        0x186
#define MSR_IA32_PMC0                                0xc1
// predefine events (version id 1)
// Intel developer's manual - system programmer'guide - 18.13.3
#define PERF_EVT_UNHALTED_CORE_CYCLES_SEL            0x3c
#define PREF_EVT_INSTRUCTION_RETIRED_SEL             0xc0
#define PREF_EVT_UNHALTED_REFERENCE_CYCLES_SEL       0x3c
#define PREF_EVT_LLC_REFERENCE_SEL                   0x2e
#define PREF_EVT_LLC_MISSES_SEL                      0x2e
#define PREF_EVT_BRANCH_INSTRUCTION_RETIRED_SEL      0xc4
#define PREF_EVT_BRANCH_MISSES_RETIRED_SEL           0xc5

#define PREF_EVT_UNHALTED_CORE_CYCLES_UMASK          0x00
#define PREF_EVT_INSTRUCTION_RETIRED_UMASK           0x00 
#define PREF_EVT_UNHALTED_REFERENCE_CYCLES_UMASK     0x01
#define PREF_EVT_LLC_REFERENCE_CYCLES_UMASK          0x4f
#define PREF_EVT_LLC_MISSES_UMASK                    0x41
#define PREF_EVT_BRANCH_INSTRUCTION_RETIRED_UMASK    0x00
#define PREF_EVT_BRANCH_MISSES_RETIRED_UMASK         0x00

// Core Solo and Core Duo non-architectual umask
#define UMASK_ALL_CORES                              0xc0
#define UMASK_THIS_CORE                              0x40

namespace vina {
  Event* Event::createTimerEvent(const string& name)
  {
    return new TimerEvent(name);
  }
  Event* Event::createCounterEvent(const string& name, 
				   event_kind kind)
  {
    assert( kind != GENERAL_TIMER_EVT
	    && "create timer in counter factory");
    
    switch( kind ) {
    case HIT_COUNTER_EVT:
      return new CounterEvent<HIT_COUNTER_EVT>(name);
    case TSC_COUNTER_EVT:
      return new CounterEvent<TSC_COUNTER_EVT>(name);
#ifdef PMC_SUPPORT
    case LL_CACHE_MISS_COUNTER_EVT:
      return new CounterEvent<LL_CACHE_MISS_COUNTER_EVT>(name);
#endif
    default:
      assert(0 && "unknown count kind");
    }
    
    return NULL;
  }
  
  Profiler::Profiler()
  {
    _last_id = 0;
    _running = 0;
    _elapsed = 0;
  };
  Profiler::~Profiler()
  { 
    for(auto i=_table.begin(); i != _table.end(); 
	++i) {
      event_id id = i->second;
      delete _careAbout[id];
    }
  }

  event_id Profiler::eventRegister(const string& name, event_kind kind) 
  {
    if ( _table.find(name) == _table.end() ) {
      event_id id = _last_id++;
      auto result =  _table.insert(std::make_pair(name, id));
      
      if ( result.second ) {
	Event * evt;
	if ( kind == GENERAL_TIMER_EVT ) 
	  evt = Event::createTimerEvent(name);
	else 
	  evt = Event::createCounterEvent(name, kind);
	if ( evt != NULL ) {
	  _careAbout.push_back(evt);
	  return id;
	}
      }
    }
#ifdef __NDEBUG
    // it if an error to re-register the event with same name,
    // however, breakdown need this to cumulate thread-time
    throw profiler_exception();	  
#else
    else {
      return _table[name];
    }
#endif
  }
  void Profiler::eventUnregister(const string& name) 
  {
    typedef std::map<const string, event_id>::iterator 
      Itor;
    Itor p;
    if( (p = _table.find(name)) != _table.end() ) {
      event_id id = _table[name];
      _table.erase(p);
      delete _careAbout[id];
      _careAbout[id] = NULL;
      return;
    }
    throw profiler_exception();
  }
  
  void Profiler::dump() const
  {
    printf("Profile info\n");
    for (int i=0; i<70; ++i) putchar('=');
    putchar('\n');

    for (int i=0, I=_careAbout.size(); i != I; ++i) {
      if( _careAbout[i] ) { // skip holes
	Event *evt = _careAbout[i];
	printf("#%2d ", i);
	evt->dump();

	if( dynamic_cast<TimerEvent*>(evt) )
	  printf(" %2.1f%%\n", (evt->sum_*1.0f)/_elapsed*100.0);
	else 
	  printf("      \n");
      }
    }
  }

  void Profiler::eventFold(event_id start, event_id end)
  {
    Event *pEvt = _careAbout[start];
    assert ( pEvt != NULL && dynamic_cast<TimerEvent*>(pEvt) != 0 
	     && "invalid fold");
    
    for (int i=start+1; i < end; ++i){
      Event * evt = _careAbout[i];
      assert( evt != NULL && dynamic_cast<TimerEvent*>(evt) != 0 
	      && "invalid fold");

      pEvt->sum_     += evt->sum_;
      pEvt->counter_ += evt->counter_;
      eventUnregister(evt->name_);      
    }
    
    pEvt->name_ = pEvt->name_ + "(folded)";    
  }
  void Profiler::eventSetExtra(event_id k, const string& extra)
  {
    assert( k < _last_id && _careAbout[k] != NULL 
	    && "invalid setExtra");
    _careAbout[k]->extra_ = extra;
  }
#ifdef PMC_SUPPORT  
  //==============================================//
  //~~       LL_CACHE_MISSES COUNTER            ~~//
  //==============================================//
  Counter<LL_CACHE_MISS_COUNTER_EVT>::~Counter()
  {
    if ( msr_fd_ >= 0 ) {
      close(msr_fd_);
    }
  }
  void Counter<LL_CACHE_MISS_COUNTER_EVT>::perf_evt_initialize() 
  {
    char msr_file_name[64];
    ull_t data;
    unsigned EAX, EBX, ECX, EDX;
    int nu_pmc;

    EAX = 0xa;
    CPUID(EAX, EBX, ECX, EDX);
    if ( (EAX & PERF_MONITOR_VERSIONID_MASK) == 0 ) {
      fprintf(stderr, "processor does not support performance counter\n");
      exit(1);
    }
    nu_pmc = (EAX & NUM_OF_GP_COUNTER_MASK) >> 8;

    sprintf(msr_file_name, "/dev/cpu/0/msr");
    msr_fd_ = open(msr_file_name, O_RDONLY);
    if (msr_fd_ < 0) {
      if (errno == ENXIO) {
	fprintf(stderr, "rdmsr: No CPU 0\n");
	exit(2);
      } else if (errno == EIO) {
	fprintf(stderr, "rdmsr: CPU 0 doesn't support MSRs\n");
	exit(3);
      } else {
	perror("rdmsr: open");
	exit(127);
      }
    }
    unsigned llc_miss_evt =  (0x41<<16)   /*counter enable and working in user mode*/ 
      | (PREF_EVT_LLC_MISSES_UMASK<<8) 
      | PREF_EVT_LLC_MISSES_SEL;

    for (int i=0; i<nu_pmc; ++i) {
      if (pread(msr_fd_, &data, sizeof data, (MSR_IA32_PERFEVT_SEL0+i)) 
	  == sizeof data) {
	if ( (data & llc_miss_evt) == llc_miss_evt) {
	  pmc_ = i;
	  return;
	}
      }
    }
    if (errno == EIO) {
      fprintf(stderr, "rdmsr: CPU 0 cannot read "
	      "MSR");
      exit(4);
    } else {
      perror("rdmsr: pread");
      exit(127);
    } 
  }
  
  ull_t Counter<LL_CACHE_MISS_COUNTER_EVT>::read_pmc()
  {
    ull_t data;

    if (pread(msr_fd_, &data, sizeof data, (pmc_ + MSR_IA32_PMC0)) 
	!= sizeof data ) { 
      if (errno == EIO) {
	fprintf(stderr, "rdmsr: CPU 0 cannot read "
		"MSR 0x%08\n", (pmc_ + MSR_IA32_PMC0));
	exit(4);
      } else {
	perror("rdmsr: pread");
	exit(127);
      }
    }
    return data;
  }
#endif
} // end of namespace xliu
