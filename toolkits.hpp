// This file is not supposed to be distributed.
#ifndef XLIU_TOOLKITS_HXX
#define XLIU_TOOLKITS_HXX 1

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <tr1/type_traits>
#include <boost/shared_ptr.hpp>

//#include "matrix2.hpp"
#include "profiler.hpp"

#define assert_s(con_expr) \
  BOOST_STATIC_ASSERT((con_expr))

#define verify(T0, T1) \
  assert_s((std::tr1::is_same<T0, T1>::value))

namespace vina {
  /// a very simple random number generator
  template<class T>
  struct NumRandGen {
    explicit NumRandGen(unsigned s){ seed(s); } 
    NumRandGen() { seed((unsigned)time(NULL));}
    void seed(T seed)
    { std::srand((unsigned int)seed); }

    T operator() () { return static_cast<T>(rand()); }
   };
  template<>
  struct NumRandGen<float> {
    explicit NumRandGen(unsigned s){ seed(s);}
    NumRandGen() { seed((unsigned)time(NULL));}
    void seed(unsigned seed)
    { std::srand((unsigned int)seed); }

    float operator() () { return (double)(rand() - RAND_MAX/2) / (double)(RAND_MAX); }
  };
  ///FIXME: Need a thread-safe log stream
  class Log : public std::ostream{

  };

  ///visualize callsites
  class CallsiteOutput {
    struct _dummy_deleter {
      void operator()(void const *) const{}
    };
    CallsiteOutput(boost::shared_ptr<std::ostream> os)
      : os_(os), cnt_(0), grp_(0) {}
  public:
    
    class RAII{
    public:
      RAII(CallsiteOutput * cso) : ptr_(cso) {
	ptr_->enter();
      }
      ~RAII()
      {
	ptr_->leave();
      }
    private:
      CallsiteOutput * ptr_;
    };

    void initializeGraph(const std::string& graphname);
    void finalizeGraph();
    
    void enter();
    void leave();

    void enterMT();
    void leaveMT();

    // call after callsite(D) to link dependences
    // side-effect: clear the dependent cache
    void depend();

    void callsite(const std::string& target);
    // same as callsite, except put node into dependent cache
    void callsiteD(const std::string& target);

    // same as callsite, except parenthesize the node in a standalone
    // cluster, work around the dot, who mistakenly puts root node
    // in first thread region.
    void root(const std::string& root);

    static CallsiteOutput createFileOutput(const char * fname) {
      return CallsiteOutput(boost::shared_ptr<std::ostream>
			    (new std::ofstream(fname)));
    }
    
    // does not own OS, it should be static variables,
    // e.g. &cout
    static CallsiteOutput createFromOStream(std::ostream* os)
    {
      return CallsiteOutput(boost::shared_ptr<std::ostream>
			    (os, _dummy_deleter()));
    }
  private:
    boost::shared_ptr<std::ostream> os_;
    std::stack<std::string>       stack_;
    std::vector<std::string>      dep_;
    std::string                   temp_;
    int                           cnt_, grp_;
  };
  
  inline double Gflops(double scale, double ms)
  {
    return scale * 0.001 /  ms;
  }

  /// return: GHz of processor freqency
  inline float caliberate() {
    unsigned long long int tick_start, tick_end;

    Timer timer;
    volatile int dummy;

    timer.start();
    
    RDTSC(tick_start);

    for (int i=0; i < 1000000; ++i) 
      for (int j=0; j<1000; ++j) 
	dummy += i << 1 + 79 * j;
    
    RDTSC(tick_end);

    timer.stop();
    double ticks = (tick_end - tick_start);
    ticks = ticks / 1000;
    unsigned usec = timer.elapsed();
    return ticks / usec;
  }
  /*
  template<class T, int M, int N>
  inline void dump_m(const Matrix<T, M, N>& m, const std::string& verbose = "")
  {
    if ( verbose.length() != 0 ) {
      std::cout << verbose << std::endl;
    }
    
    auto iter = m.begin();
    for (int i=0, I=M; i != I; ++i, std::cout << std::endl) 
      for (int j=0, J=N; j != J; ++j)
	std::cout << *iter++ << " ";
  }
  */
  template <class T>
  struct Identity{
    typedef T type;
  };
  
} // end of NS
#endif /* XLIU_TOOLKITS_HXX */
