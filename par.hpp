#ifndef PAR_HPP_
#define PAR_HPP_
#include <vina.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
namespace vina{
  template <int L, template<int> class Pred, 
	    template<int> class Func,
	    bool __SENTINEL__/*never used*/ = Pred<L>::value >
  struct Par
  {
    static void apply() {
      Par<L+1, Pred, Func>::apply(); 
      boost::function<void (void)> func = &(Func<L>::g);
      boost::thread thr(func);
     }
  };
  //tail do 
  template<int L, template<int> class Pred, template<int> class Func>
  struct Par<L, Pred, Func, true>
    : _Trail {};

}// end of NS
#endif /*PAR_HPP_*/
