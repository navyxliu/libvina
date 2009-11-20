// This file is not supposed to be distributed.
/// Multi-thread support
// History
// Jun.10.2009' -- file creation

#ifndef XLIU_MTSUPPORT_HXX
#define XLIU_MTSUPPORT_HXX
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
namespace vina {
  namespace mt {
    typedef boost::thread                     thread_t;
    typedef boost::mutex                      mutex_t;
    typedef boost::shared_ptr<boost::barrier> barrier_t; 
    typedef boost::shared_ptr<boost::condition_variable> cond_variable_t;
  
    extern barrier_t null_barrier;      // equal to null pointer
    extern barrier_t trivial_barrier;   // barrier with one notifier. use as a harmless placeholder.

    /// RATIONALE:
    // Traditional lock hurts high performance parallel program.
    // We intend to support stream-computing, ie. in bulk flavour.
    // To abstract that, we use "view" to represent subset of processing data.
    // However, ReadView and WriteView can only work in Sequenial environment.
    // In order to implement complex parallel pattern(such as pipeline),signal is designed 
    // to pass ownership from one computational entity to others. The corresponding concept is
    // signal in CellBE or other distributed systems. Views which risk harzard should cast to
    // ViewMT before pass to other thread spaces. up-stream thread holds the ownership until
    // completes its task, then sets val_ to true in atomic. Down-stream entities(it is possible
    // for more than one reader) wait for val_ flipping in platform effective ways, only guarantee
    // it is thread-safe and blocking op. Condition_variable is trivial solution for SMP arch and
    // pthread environment. For CellBE or others which already have mechanism similar to our design,
    // more effecient ways are possible.
    // 
    class signal {
    public:
      signal();
      //intnded to use by down stream threads
      //blocking op
      void wait();
      //intended to use by up stread thread
      void set();

    private: 
      class impl;
      boost::shared_ptr<impl> pimpl_;
    };
      
    typedef signal signal_t;
    
    //convert boost::function to raw function.
    //seriously forbid pass returned value of boost::bind.
    //if error occurs, ruturn NULL.
    struct function_helper{
      typedef void (*fn_nullary)(void);
      typedef void (*fn_unary) (void * arg/*in-out*/);
      typedef void (*fn_binary)(void * ret, void *arg);
      typedef void (*fn_ternary)(void * ret, void * arg0, void * arg1);
      template <class F>  
      static fn_nullary * to_nullary(F f)
      {
	return f.template target<fn_nullary>();
      }

      template <class U>
      static fn_unary * to_unary(U f)
      {
	return f.template target<fn_unary>();
      }

      template <class F>
      static fn_ternary to_ternary(F f)
      {
	return f.template target<fn_ternary>();
      }
    };
  }//mt
}//endof NS
#endif /*XLIU_MTSUPPORT_HXX*/
