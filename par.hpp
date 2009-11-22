//This file is not supposed to be distributed.
//History:
//Nov.11, 09'  create
//Nov.22, 09'  use forward-recursion algorithm. we parallelized loop by omp.

#ifndef PAR_HPP_
#define PAR_HPP_

#include <vina.hpp>
#include <omp.h>

namespace vina{
  template<class S, uint32_t I, class F>
  struct select_surrounding<VINA_TYPE_PAR, S, I, F> 
  {
     typedef par<S, I, F> type; 
  };

  template <class SURR,      /*surrounding par, i.e. the nested loop. it can NOT be a par_tail, or a usage error. */ 
	    uint32_t ITER = 1,
	    class FUNC = par_func_undefined
            >
  struct par{
    const static unsigned int _itr = ITER;
    const static int _ty = VINA_TYPE_PAR;
    //if FUNC is non-trivia, override surrounding _func
    typedef typename  boost::mpl::if_<std::tr1::is_same<FUNC, par_func_undefined>, 
				      typename SURR::_func,  
				      FUNC>::type
    _surr_func;

    typedef typename select_surrounding<SURR::_ty, typename SURR::_surr, SURR::_itr, _surr_func>::type
    _surr;

    typedef _surr_func _func;

    //entry point:
    static void 
    apply() {
        int i;
        #pragma omp parallel for private(i)
	for (i=0; i<_itr; ++i) {
	  _surr::apply();
	}
    }
  };

  /* A full specialization, indicated the tail of the parallel sequence.
   */
  template<>
  struct par<void, 1, par_func_undefined>{
    const static uint32_t _itr = 1;
    const static int _ty = VINA_TYPE_PAR;
    typedef par_func_undefined _func;
    typedef void _surr;

    // no implementation in purpose. It is erroneous to call this function.
    static void apply();
  };

  typedef par<void, 1, par_func_undefined> par_tail;
  /* A parial specialization, indicate that the last "practical" level of loop"
   */
  template<uint32_t ITER, class FUNC>
  struct par<par_tail, ITER, FUNC>{
    const static uint32_t _itr = ITER;
    const static int _ty = VINA_TYPE_PAR;
    typedef FUNC _func;
    typedef par_tail _surr;

    static void 
    apply() {
      FUNC f;
      int i;
      #pragma omp parallel for private(i)
      for (i=0; i<_itr; ++i) f();
    }
  };
}// end of NS

  /////////////////////////////////////////////////////////////////////
  //run in parallel
  /////////////////////////////////////////////////////////////////////
#if 0
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
#endif
#endif /*PAR_HPP_*/
