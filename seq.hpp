//This file is not supposed to be distributed.
//History:
//Nov.11. 09'  create

#ifndef SEQ_HPP_
#define SEQ_HPP_
#include <vina.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <tr1/type_traits>

namespace vina{
  // no implementation in purpose
  struct seq_func_undefined;

  template <class SURR,      /*surrounding seq*/ 
	    class FUNC,
	    uint32_t ITER = 1
            >
  struct seq{
    const static unsigned int _itr = ITER;
    typedef SURR _surr;
    
    // iterator for directly surrounding loop
    typedef boost::mpl::int_<SURR::_itr> ITER_SURR;
    const static uint32_t _itr_j = ITER_SURR::value;

    // iterator for surrounding's surrounding' loop
    typedef typename boost::mpl::if_<std::tr1::is_same<SURR, seq<void, seq_func_undefined> >,
				     seq<void, seq_func_undefined>,
				     typename SURR::_surr>::type
    SURR_SURR;
    const static uint32_t _itr_i = SURR_SURR::_itr;

    /*if FUNC is non-trivia, override surrounding _func
     */
    typedef typename  boost::mpl::if_<std::tr1::is_same<FUNC, seq_func_undefined>, 
				      typename SURR::_func,  
				      FUNC>::type
    _func;

    /*entry point:
     *currently, we only implement 3-level nested loop
     */
    static void 
    apply() {
      for (int i=0; i<_itr_i; ++i) for (int j=0; j<_itr_j; ++j)
	for (int k=0; k<ITER; ++k) {
	  _func()();
	}
    }
  };

  template<>
  struct seq<void, seq_func_undefined>{
    const static uint32_t _itr = 1;
    typedef seq_func_undefined _func;
    typedef void _surr;
  };

  typedef seq<void, seq_func_undefined> seq_init;

  /* seq_handler functions are used to bind loop-variables(LV).
   * becuse \emph{seq} is hierarchy-awareless, we need to reason 
   * LV by handler.
   * gcc supports C++0x lambda since 4.5.0, it is possible to replace handler
   * functions with lambda expression.
   */
  template<class F>
  struct seq_handler_f{
    void
    operator() () {
      F()(cnt_);

      ++ cnt_;
    }
    static  int cnt_;
  };

  /*this function objection is used in two-level iteration, which is 
   *for (i=0; i<I; ++i) for(j=0; j<J; ++j) ..
   *invariance: i * J  + j == cnt;
   */
  template <class F, int J>
  struct seq_handler_g{
    static int cnt_;
    
    void 
    operator() () {
      int j = cnt_ % J;
      int i = (cnt_ - j) / J ;
      // increament iteration counter; 
      // the most-nested iterator is the fastest one
      F()(i, j);
      ++ cnt_; 

    }
  };

  /*3-level iteration
   *invariance: i * (K*J) + j * K + k == cnt
   */
  template<class F, int K, int J>
  struct seq_handler_h{
    static int cnt_;
    
    void 
    operator() () {
      int k = cnt_ % K;
      int j = (cnt_ % (K * J) - k) / K;
      int i = (cnt_ - j * K - k) / (K*J);

      F()(i, j, k);

      // increament iteration counter; 
      // the most-nested iterator is the fastest one
      ++ cnt_; 
    }
  };
  template <class F>
  int seq_handler_f<F>::cnt_ = 0;
  
  template <class F, int J>
  int seq_handler_g<F, J>::cnt_ = 0;

  template <class F, int K, int J>
  int seq_handler_h<F, K, J>::cnt_ = 0;

  /////////////////////////////////////////////////////////////////////
  //run in Sequence 
  /////////////////////////////////////////////////////////////////////

  
  template <int L, template<int> class Pred, 
	    template<int> class Func,
	    bool __SENTINEL__/*never used*/ = Pred<L>::value >
  struct Seq
  {
    static void apply() {
      Func<L>() ();
      Seq<L+1, Pred, Func/*, Pred<L+1>::value*/>::apply(); 
    }
  };
  //tail do 
  template<int L, template<int> class Pred, template<int> class Func>
  struct Seq<L, Pred, Func, true>
    : _Trail {};
}//end of NS
#endif //SEQ_HPP_
