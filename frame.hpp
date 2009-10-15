//This file is not supposed to be distributed.
//This file provides TF classes for
// mapseq
// mappar
// mapreduce
// See Sequoia[sc06] for the meanings of these contructs.
//
//History
//July. 20, create file 
//Aug. 13, introduce thread pool
//Sep. 22, introduce libspmd
//Oct. 10, function wrapper
#ifndef VINA_FRAMEWORK
#define VINA_FRAMEWORK
#include "vector.hpp"
#include "mtsupport.hpp"
#include "threadpool.hpp"
#include "profiler.hpp"
#include "toolkits.hpp"
#include "libSPMD/warp.h"

#include <tr1/type_traits>
#include <tr1/functional>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <sstream>


using namespace boost;

namespace vina {

#ifndef __NDEBUG
  static event_id __frm_kernel_seq   = 
    Profiler::getInstance().eventRegister("#frame kernel seq");
  static event_id __frm_kernel_par   = 
    Profiler::getInstance().eventRegister("#frame kernel par");
  static event_id __frm_mem_cost = 
    Profiler::getInstance().eventRegister("#frame mem cost");
  static event_id __frm_thread_cost =
    Profiler::getInstance().eventRegister("#frame thread cost");
  static event_id __frm_thread_creation_cnt = 
    Profiler::getInstance().eventRegister("#thread creation counters", HIT_COUNTER_EVT);
  static event_id __frm_threadpool_enqueue_cnt = 
    Profiler::getInstance().eventRegister("#thread pool enqueue counters", HIT_COUNTER_EVT);
  static event_id __frm_libspmd_thread_cnt = 
    Profiler::getInstance().eventRegister("#libspmd thread creation", HIT_COUNTER_EVT);
#endif

#if defined(__USEPOOL)
  static mt::thread_pool pool(TEST_NR_CPU + 4);
#endif

#define PROF_HIT(EVT) (Profiler::getInstance().eventHit(EVT))

  namespace __aux{
/*
 * To abridge the gap between libvina (modern c++ template library and   \
 * libSPMD (general purpose c library), a couple of function conversion  \  
 * stab is neccessary. two aims of these convertors:   
 * 1) convert from function object to pure function pointer, i.e. any 
 * following functions are reentries.
 * 2) normalize to unary form. 
 */
    struct func_param {
      void * callable; /*this object at least has operator() method*/
      void * arg0;
      void * arg1;
      void * arg2;
    };

    template<class F>
    void wrapper_func_nullary(func_param * p)
    {
      static_cast<F*>(p->callable)->operator()();
    } 
    template <class F>
    void wrapper_func_unary(func_param * p)
    {
      static_cast<F*>(p->callable)->operator()(p->arg0); 
    }
    template <class F>
    void wrapper_func_binary(func_param * p)
    {
      static_cast<F*>(p->callable)->operator()(p->arg0, p->arg1); 
    }
    template <class F>
    void wrapper_func_ternary(func_param * p)
    {
      //printf("ternary\n");
      //static_cast<F*>(p->callable)->operator()(p->arg0, p->arg1, p->arg2);
      mt::thread_t thr(*(static_cast<F*>(p->callable)), p->arg0, p->arg1, p->arg2);
      thr.join();
    }
    template <class F>
    void wrapper_func_ternary_(func_param * p) 
    {
      static_cast<F*>(p->callable)->operator()(p->arg0, p->arg1, p->arg2);
    } 

/* libvina can cut off data regardlss of underlying data-structure. it's desireable to      \
   distint scalar and vector. We use function overload or template specialization to handle \
   them. 
*/
    template <class IN, int size, bool>
    struct subview{
      static ReadView<typename view_trait<IN>::value_type, size> *
      sub_reader(const IN& in, int nth)
      {
	return new ReadView<typename view_trait<IN>::value_type, size>(in.template subRView<size>
	  (nth * size));
      }
      static WriteView<typename view_trait<IN>::value_type, size> * 
      sub_writer(IN& in, int nth)
      {
	return new WriteView<typename view_trait<IN>::value_type, size>( in.template subWView<size>
	  (nth * size));
      }
    };
    //specialization for scalar or same dimention 
    template <class IN, int size>
    struct subview<IN, size, true>
    {
      static IN * 
      sub_reader(const IN& in, int nth __attribute__((unused)) )
      {
	return new IN(in);
      }
      static IN * 
      sub_writer(const IN& in, int nth __attribute__((unused)) )
      {
	return new IN(in);
      }
    };
    template <class IN, int sz_x, int sz_y, bool>
    struct subview2{
      typedef ReadView2<typename view_trait2<IN>::value_type,
		       sz_x, sz_y> subReader_t;

      static subReader_t * 
      sub_reader(const IN& in, int nth_x, int nth_y)
      {
	return new subReader_t( in.template subRView<sz_x, sz_y>
	  (nth_x * sz_x, nth_y * sz_y) );
      }
    };
    //specialization for ...
    template <class IN, int sz_x, int sz_y>
    struct subview2<IN, sz_x, sz_y, true>{
      static IN * 
      sub_reader(const IN& in, 
		 int nth_x __attribute__((unused)),
		 int nth_y __attribute__((unused)))
      {
	return new IN(in);
      }
    };

    template <class OUT>
    OUT& ref(OUT& o, mpl::false_)
    {
      return o;
    }
    
    template <class OUT>
    reference_wrapper<OUT> 
    ref(OUT& o, mpl::true_/*scalar*/)
    {
      return boost::ref(o);
    }

    template <class T>
    struct viewdim{
      typedef mpl::int_<T::VIEW_SIZE>
      type;
    };
    template <class T>
    struct viewdim2{
      typedef mpl::int_<T::VIEW_SIZE_X>
      view_x;
      typedef mpl::int_<T::VIEW_SIZE_Y>
      view_y;

      typedef viewdim2<T>
      type;
    };

    template<>
    struct viewdim2<mpl::int_<0>>{
      typedef mpl::int_<0>
	view_x;
      typedef mpl::int_<0>
	view_y;
      
      typedef viewdim2<mpl::int_<0>> 
	type;
    };
  }//end of NS
  //==============================================//
  //~~              MAPPAR                      ~~//
  //==============================================//
  template <class Instance, 
	    int  _K,
	    bool _IsMT,
	    bool __SENTINEL__
	    >
  struct mappar{
    typedef mappar<typename Instance::SubTask, _K, _IsMT, 
		Instance::SubTask::_pred> _Tail;

    typedef mpl::bool_<std::tr1::is_arithmetic<typename Instance::Arg0>::value>
    arg0_arithm;
    typedef mpl::bool_<std::tr1::is_arithmetic<typename Instance::Arg1>::value> 
    arg1_arithm;
    typedef mpl::bool_<std::tr1::is_arithmetic<typename Instance::Result>::value> 
    ret_arithm;
    
    typedef typename mpl::eval_if<arg0_arithm,
				  mpl::int_<0>, 
				  __aux::viewdim<typename Instance::SubTask::Arg0>
		          >::type arg0_dim;

    typedef typename mpl::eval_if<arg1_arithm,
				  mpl::int_<0>, 
				  __aux::viewdim<typename Instance::SubTask::Arg1>
    			  >::type arg1_dim;

    typedef typename mpl::eval_if<ret_arithm,
				  mpl::int_<0>, 
				  __aux::viewdim<typename Instance::SubTask::Result>
    			  >::type ret_dim;

    const static int lookahead = 1 + _Tail::lookahead;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result, 
		     mt::barrier_t dummy = mt::null_barrier /*__attribute__((unused))*/ )
    {
#if !defined( __USE_LIBSPMD)
      	mt::barrier_t barrier(new boost::barrier(_K+1));
#ifndef __NDEBUG 
	event_id timers[_K];
	if ( _IsMT && lookahead == 1){
	  for(int i=0; i<_K; ++i)
	  {
	      std::ostringstream oss;
	      oss << "thread" << i << " timer";
	      timers[i] = Profiler::getInstance().eventRegister(oss.str());
	  }
	  Profiler::getInstance().eventStart(__frm_kernel_par);
	}
#endif
#else
        // libSPMD has implicit barrier
	// furthermore, it has independent breakdown profiler based on files
        int wid;
        if ( _IsMT && lookahead == 1 ) {
	  auto compF = Instance::SubTask::computation();
	  typedef void (*func_t)(__aux::func_param *); 
	  func_t task = &__aux::wrapper_func_ternary_<typename Instance::SubTask::_Comp>;

	  wid = spmd_create_warp(_K, (void *)task, 0, 0, 0);
	  assert( wid != -1 && "spmd creation faied");
	  //printf("create warp id %d\n", wid);
        }
#endif
	for (int k=0; k < _K; ++k) {
	  auto subArg0   = __aux::subview<typename Instance::Arg0, 
	    arg0_dim::value, arg0_arithm::value>::sub_reader(arg0, k);
	  
	  auto subArg1   = __aux::subview<typename Instance::Arg1,
	    arg1_dim::value, arg1_arithm::value>::sub_reader(arg1, k); 
	  
	  auto subResult = __aux::subview<typename Instance::Result,
	    ret_dim::value, ret_arithm::value>::sub_writer(result, k);
	  //printf("dim%d\n",  ret_dim::value);

	  if ( _IsMT && lookahead == 1) {// leaf node
#ifndef __NDEBUG
#ifdef __USE_LIBSPMD
            auto compF = Instance::SubTask::computation();
	    __aux::func_param * arg = new __aux::func_param;
            arg->callable = compF;
            arg->arg0 = (void *)subArg0; 
	    arg->arg1 = (void *)subArg1;
	    arg->arg2 = (void *)subResult;

	    int tid =  spmd_create_thread(wid, arg);
	   // printf("tid = %d compF=%p\n", tid, compF);
	    if ( -1 == tid ) {
		fprintf(stderr, "failed to create thread\n");
		exit(-1);
	    }

	    PROF_HIT(__frm_libspmd_thread_cnt);
#elif defined( __USEPOOL) // thread pool
            auto compF = Instance::SubTask::computationMT_t();
            pool.run(boost::bind(compF, *subArg0, *subArg1, *subResult, barrier, timers[k]));
            PROF_HIT(__frm_threadpool_enqueue_cnt);
#else
            auto compF = Instance::SubTask::computationMT_t();
	    mt::thread_t leaf(compF, subArg0, subArg1, subResult, barrier ,timers[k]);
	    leaf.detach();
	    //std::cout << "thread:" << leaf.get_id() <<"\n";
	    PROF_HIT(__frm_thread_creation_cnt);
#endif

#else
#ifdef __USE_LIBSPMD
	    auto compF = Instance::SubTask::computation();
	    __aux::func_param * arg = new __aux::func_param;
            arg->callable = (void*)compF;
            arg->arg0 = (void *)subArg0; 
	    arg->arg1 = (void *)subArg1;
	    arg->arg2 = (void *)subResult;

            spmd_create_thread(wid, arg);
#else
	    _Tail::doit(*subArg0, *subArg1, *subResult, barrier);
#endif
#endif //__NDEBUG
	  }
	  else //recursion
	    _Tail::doit(*subArg0, *subArg1, *subResult);
	}//end for

#ifndef __USE_LIBSPMD	
	if ( _IsMT && lookahead == 1 )	
	    barrier->wait();
#endif

#if !defined(__NDEBUG) && !defined(__USE_LIBSPMD)
	Profiler::getInstance().eventEnd(__frm_kernel_par);
#endif
      }
 };
  template<class Instance, int _K>
  struct mappar<Instance, _K, false, true>
  {
    static const int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t dummy = mt::null_barrier)
    {
      auto compF = Instance::computation();
#ifndef __NDEBUG
      Profiler::getInstance().eventStart(__frm_kernel_seq);
#endif
      (*compF)(arg0, arg1, result);
#ifndef __NDEBUG
      Profiler::getInstance().eventEnd(__frm_kernel_seq);
#endif
    }
  };
           
  template<class Instance, int _K>
  struct mappar<Instance, _K, true, true>
  {
    static const int lookahead = 0;
    typedef mpl::bool_<std::tr1::is_arithmetic<
			 typename Instance::Result
			 >::value>
    result_arithm;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::trivial_barrier)
    {
      auto compF = Instance::computationMT();
#ifndef __USEPOOL
      mt::thread_t leaf(compF, arg0, arg1, 
      	__aux::ref(result, result_arithm()), 
      		barrier);
      leaf.detach();
#else
      pool.run(bind(compF, arg0, arg1, __aux::ref(result, result_arithm()), 
	    barrier));
#endif
    }
  };

  template <class Instance,
	    int  _K,
	    bool _IsMT,         /*enable multithread*/
	    bool __SENTINEL__   /*never use it explicitly*/ 
	    >
  struct mappar2{

    typedef mpl::bool_<std::tr1::is_arithmetic<typename Instance::Arg0>::value>
    arg0_arithm;

    typedef typename mpl::or_<mpl::bool_<std::tr1::is_arithmetic<typename Instance::Arg1>::value>,
			      mpl::bool_<std::tr1::is_same<typename Instance::Arg1, 
							   typename Instance::SubTask::Arg1>
					 ::value>
			      >::type
    arg1_isomorph;
    
    typedef typename mpl::eval_if<arg0_arithm,
				  __aux::viewdim2<mpl::int_<0>>,                    // then
				  __aux::viewdim2<typename Instance::SubTask::Arg0>  // else
				  >::type arg0_dim;

    typedef typename mpl::eval_if<arg1_isomorph,
				  __aux::viewdim2<mpl::int_<0>>, 
				  __aux::viewdim2<typename Instance::SubTask::Arg1>
    			  >::type arg1_dim;

    typedef mappar2<typename Instance::SubTask, _K, _IsMT, Instance::SubTask::_pred> _Tail;
    const static int lookahead = 1 + _Tail::lookahead;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result)
    {

      for(int i=0; i<_K; ++i) {
#ifdef __USE_LIBSPMD
        int wid;
        if ( lookahead == 1 && _IsMT ) {
	typedef void (*func_t)(__aux::func_param *);
	func_t task = &(__aux::wrapper_func_ternary_<typename Instance::SubTask::_Comp_void>);
	wid = spmd_create_warp(_K, (void *)task, 0, 0, 0);
	assert( wid != -1 && "failed in create warp");
	}
#endif
        for(int j=0; j<_K; ++j) {
	  auto subResult = result.template subWView
	    <Instance::SubWView::VIEW_SIZE_X, Instance::SubWView::VIEW_SIZE_Y>
	    (i * (Instance::SubWView::VIEW_SIZE_X), j * Instance::SubWView::VIEW_SIZE_Y);
	  
	  auto subArg0   = arg0.template subRView
	    <Instance::SubRView0::VIEW_SIZE_X, Instance::SubRView0::VIEW_SIZE_Y>
	    (i * (Instance::SubRView0::VIEW_SIZE_X), (j * Instance::SubRView0::VIEW_SIZE_Y));
	  
	  auto subArg1   = __aux::subview2<typename Instance::Arg1,
				  arg1_dim::view_x::value, arg1_dim::view_y::value, 
				  arg1_isomorph::value>::sub_reader(arg1, i, j); 
#ifndef __USE_LIBSPMD	  
	  _Tail::doit(subArg0, *subArg1, subResult);
#else
          if ( _IsMT && 1 == lookahead ) {
          auto compF = Instance::SubTask::computation_ptr();
          __aux::func_param * arg = new __aux::func_param;
          arg->callable = compF;
          arg->arg0 = new typename Instance::SubTask::Arg0(subArg0);
          arg->arg1 = subArg1; 
          arg->arg2 = new typename Instance::SubTask::Result(subResult);
          int tid = spmd_create_thread(wid, arg);
	  //printf("tid = %d\n", tid);
	  assert( tid != -1 && "failed in creating task");
	  }
	  else 
	    _Tail::doit(subArg0, *subArg1, subResult);
#endif
	}/*for*/
     }/*for*/  
   }
};

  template<class Instance, int _K>
  struct mappar2<Instance, _K, false, true>
  {
    const static int lookahead = 0;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result)
    {
      auto compF = Instance::computation();
      compF(arg0, arg1, boost::ref(result));
    }
  };
           
  template<class Instance, int _K>
  struct mappar2<Instance, _K, true, true>{
    typedef mpl::bool_<std::tr1::is_arithmetic<
			 typename Instance::Result
			 >::value>
    result_arithm;
    const static int lookahead = 0;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result)
    {
#ifndef __USE_LIBSPMD
      auto compF = Instance::computation();
      
      mt::thread_t leaf(compF, arg0, arg1, 
			__aux::ref(result, result_arithm()));

#ifndef __NDEBUG 
      std::cout << "thread:" << leaf.get_id() <<"\n";
#endif

      leaf.detach();
#else
     assert( 0 && "wrong code");
#endif
    }
  };

  //==============================================//
  //~~              MAPREDUCE                   ~~//
  //==============================================//
template <class Instance, 
	    int  _K, 
	    bool _IsMT, 
	    bool __SENTINEL__
	    >
struct mapreduce {
  typedef mapreduce <typename Instance::SubTask, _K, _IsMT, 
		     Instance::SubTask::_pred> _Tail;
  const static int lookahead = 1 + _Tail::lookahead;

  typedef typename Instance::SubTask::Result
	  SubResultType;

  static SubResultType * 
  get_result() 
  {
    static SubResultType results[_K];
    //memset(results, 0, sizeof(results));
    return results;
  }
  static void doit(const typename Instance::Arg0& arg0, 
		   const typename Instance::Arg1& arg1, 
		   typename Instance::Result& result,
		   mt::barrier_t barrier = mt::trivial_barrier)
  {
    if (_IsMT && lookahead == 1) { // leaf
/*
	typedef typename Instance::SubTask::Result
	  SubResultType;
	auto submats = new SubResultType[_K];
	std::fill(submats, submats+_K, 0);
*/
        auto submats = get_result();
#ifdef __USE_LIBSPMD
        typedef void (*func_t)( __aux::func_param * );
	func_t task = &__aux::wrapper_func_ternary_<typename Instance::SubTask::_Comp_ptr>;
        typedef void (*redu_t)( SubResultType * ); 
        redu_t redu = &Instance::reduce2;

        int wid = spmd_create_warp(_K, (void*)task, 0, (void*)redu, submats);         
	assert ( wid != -1 && "failed in created warp");

	for(int k=0; k<_K; ++k) {
	  auto subArg0   = arg0.template subRView
	    <Instance::SubRView0::VIEW_SIZE>(k * (Instance::SubRView0::VIEW_SIZE));
	  auto subArg1   = arg1.template subRView
	    <Instance::SubRView1::VIEW_SIZE>(k * (Instance::SubRView1::VIEW_SIZE));

	  auto Comp = Instance::SubTask::computation_ptr();
	  __aux::func_param * arg = new __aux::func_param;

	  arg->callable = Comp;
	  arg->arg0     = new typename Instance::SubTask::Arg0(subArg0);
	  arg->arg1     = new typename Instance::SubTask::Arg1(subArg1);
	  arg->arg2     = &(submats[k]);
	  int tid = spmd_create_thread(wid, arg);
	  assert( tid != -1 && "failed in created task");
	}
        while (!spmd_all_complete()); 
	result = submats[0];
#else
	mt::barrier_t barrier(new boost::barrier(_K+1));
	for(int k=0; k<_K; ++k) {
	  auto subArg0   = arg0.template subRView
	    <Instance::SubRView0::VIEW_SIZE>(k * (Instance::SubRView0::VIEW_SIZE));
	  auto subArg1   = arg1.template subRView
	    <Instance::SubRView1::VIEW_SIZE>(k * (Instance::SubRView1::VIEW_SIZE));
	  _Tail::doit(subArg0, subArg1, submats[k], barrier);
	}
	barrier->wait();

	auto reduF = Instance::reduction();
	for (int s=1; s < _K; s<<=1) for (int k=0; k < _K; k+=(s<<1)) {
	    SubResultType &in0 = submats[k];
	    SubResultType &in1 = submats[k+s];
	    reduF(in0, in1);
	}
	Instance::reduce(result,/*<--*/ submats[0]);
#endif
      }

      else {
	  for (int k=0; k < _K; ++k) {
	    auto subArg0   = arg0.template subRView
	      <Instance::SubRView0::VIEW_SIZE>(k * (Instance::SubRView0::VIEW_SIZE));
	    auto subArg1   = arg1.template subRView
	      <Instance::SubRView1::VIEW_SIZE>(k * (Instance::SubRView1::VIEW_SIZE));
	    _Tail::doit(subArg0, subArg1, result); 
	  }
      }
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce<Instance, _K, false, true>
  {
    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t dummy = mt::null_barrier)
    {
      auto compF = Instance::computation();
      compF(arg0, arg1, boost::ref(result));
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce<Instance, _K, true, true>
  {
    const static int lookahead = 0;
    typedef mpl::bool_<std::tr1::is_arithmetic<
			 typename Instance::Result
			 >::value>
    result_arithm;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::trivial_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(compF, arg0, arg1, 
			__aux::ref(result, result_arithm()),
			barrier);
#ifndef __NDEBUG 
      std::cout << "thread:" << leaf.get_id() <<"\n";
#endif
      leaf.detach();
    }
  };
  
  template <class Instance, 
	    int  _K, 
	    bool _IsMT, 
	    bool __SENTINEL__
	    >
  struct mapreduce2 {

    typedef mapreduce2 <typename Instance::SubTask, _K, _IsMT, 
			Instance::SubTask::_pred> _Tail;

    typedef mapreduce2 <Instance, _K, _IsMT, Instance::_pred> _Self;

    typedef view_trait2<typename Instance::SubTask::Result>  trait_result;

#ifdef __USEPOOL
    typedef memory_pool<typename trait_result::container_type,
			_K, Instance::SubTask::_pred> mem_pool;
#endif    
    const static int lookahead = 1 + _Tail::lookahead;
    
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result, 
		     mt::barrier_t dummy = mt::null_barrier)
    {

      for(int i=0; i<_K; ++i) for(int j=0; j<_K; ++j) {
	  if (_IsMT && lookahead == 1) { // leaf
	    typedef typename view_trait2<typename Instance::SubTask::Result>
	      ::container_type SubResultType;
	    typedef typename view_trait2<SubResultType>::writer_type SubWViewTemp;
	    typedef typename view_trait2<SubResultType>::reader_type SubRViewTemp;
	    
#ifndef __USEPOOL
	    auto submats = new SubResultType[_K];
#else 
	    auto submats = new(mapreduce2::mem_pool::get()) SubResultType[_K];
#endif
	    typedef SubWViewTemp * SubWViewTempPtr_t;

	    SubWViewTemp ** subResults = new SubWViewTempPtr_t[_K];

	    for (int k=1; k<_K; ++k) 
	      subResults[k] = new SubWViewTemp(submats[k].subWView());

            subResults[0] = new SubWViewTemp(result.template subWView
	    	<Instance::SubWView::VIEW_SIZE_X,
                 Instance::SubWView::VIEW_SIZE_Y>(i * (Instance::SubWView::VIEW_SIZE_X), 
                                                  j * (Instance::SubWView::VIEW_SIZE_Y)));    
            //printf("subResult[0]->data() = %p\n", subResults[0]->data());
#if !defined(__NDEBUG) && !defined(__USE_LIBSPMD)
              event_id timers[_K];
	      for (int _i=0; _i<_K; ++_i) {
	        std::ostringstream oss;
	        oss << "mat thread" << _i << " timer";
	        timers[_i] = Profiler::getInstance().eventRegister(oss.str());
	      }

	    Profiler::getInstance().eventStart(__frm_kernel_par);
#endif

#ifndef __USE_LIBSPMD
	mt::barrier_t barrier(new boost::barrier(_K+1));
#else
        int wid;
        auto compF = Instance::SubTask::computation();
	typedef void (* func_t) (__aux::func_param *);
	func_t task = &__aux::wrapper_func_ternary<typename Instance::SubTask::_Comp>;

	wid = spmd_create_warp (_K, (void *)task, 0, 
	                       (void *)&Instance::reduce_ptr, (void *)subResults);
	assert( wid != -1 && "spmd creation faied");
	//printf("warp %d is created\n", wid);
#endif
	for (int k=0; k<_K; ++k) {
	    auto subArg0   = __aux::subview2<typename Instance::Arg0, Instance::SubRView0::VIEW_SIZE_X, 
  	                              Instance::SubRView0::VIEW_SIZE_Y, false>::sub_reader(arg0, i, k);
	    auto subArg1   = __aux::subview2<typename Instance::Arg1, Instance::SubRView1::VIEW_SIZE_X, 
		                      Instance::SubRView1::VIEW_SIZE_Y, false>::sub_reader(arg1, k, j);

#if defined(__USE_LIBSPMD)
	    __aux::func_param * param = new __aux::func_param;
	    param->callable = Instance::SubTask::computation();
	    param->arg0 = subArg0;
	    param->arg1 = subArg1;
	    param->arg2 = subResults[k];

            //printf("arg0->data %p\n", subArg0->data());
	    //printf("arg1->data %p\n", subArg1->data());
            int tid = spmd_create_thread(wid, param);
	    //printf("tid=%d\n", tid);
#ifndef __NDEBUG
            if ( -1 == tid ) {
              fprintf(stderr, "failed to create thread\n");
	      exit(-1);
	    }
            PROF_HIT(__frm_libspmd_thread_cnt);
#endif

#elif defined(__USE_POOL)
#ifndef __NDEBUG
            auto Comp = Instance::SubTask::computationMT_t();
	    pool.run(boost::bind(Comp, *subArg0, *subArg1, *subResults[k],
		    barrier, timers[k]))
	    PROF_HIT(__frm_threadpool_enqueue_cnt);
#else
            auto Comp = Instance::SubTask::computationMT();
	    pool.run(boost::bind(Comp, *subArg0, *subArg1, *subResults[k], 
	                            barrier);
#endif
#else  //normal pthread 
#ifndef __NDEBUG
            auto Comp = Instance::SubTask::computationMT_t();
            boost::thread leaf(Comp, *subArg0, *subArg1, *subResults[k], 
				 barrier, timers[k]);
	    leaf.detach();
            //std::cout << "thread " << leaf.get_id() << "\n";
            PROF_HIT(__frm_thread_creation_cnt);
#else
            auto Comp = Instance::SubTask::computationMT();
	    boost::thread leaf(Comp, *subArg0, *subArg1, *subResults[k], 
	                         barrier);
	    leaf.detach();
#endif            
#endif
	    }
#ifndef __USE_LIBSPMD
	    barrier->wait();
#else
            //while (!spmd_all_complete());
/*
            printf("i = %d, j = %d\n", i, j);
	    printf("data %p\n", &(result[i*2][j*2]));
	    for (int ii=0; ii<2; ++ii, putc('\n', stdout))
	      for(int jj=0; jj<2; ++jj)
	        printf("%.4f", result[i*2 + ii][j*2+jj]);
            printf("\n");
*/

#endif

#if !defined(__NDEBUG) && !defined(__USE_LIBSPMD)
	    Profiler::getInstance().eventEnd(__frm_kernel_par);
#endif
#ifndef __USE_LIBSPMD
	    //reduce
	    auto reduF = Instance::reduction();
	    for (int s=1; s < _K; s<<=1) for (int k=0; k < _K; k+=(s<<1)) {
		auto in0 = subResults[k];
		auto in1 = new SubRViewTemp(subResults[k+s]->subRView());
		reduF(*in0, /*<--*/*in1);
	    }
	    //Instance::reduce(result, i, j, subResults[0]);
#endif
#ifndef __USEPOOL
            //unsafe delete here. 
	    //
	    //delete [] submats;
#endif
	  }
	  else{
	    auto subResult = result.template subWView
	      <Instance::SubWView::VIEW_SIZE_X, Instance::SubWView::VIEW_SIZE_Y>
	      (i * (Instance::SubWView::VIEW_SIZE_X), j * (Instance::SubWView::VIEW_SIZE_Y));
	    
	    for (int k=0; k < _K; ++k) {
	      auto subArg0   = arg0.template subRView
		<Instance::SubRView0::VIEW_SIZE_X, Instance::SubRView0::VIEW_SIZE_Y>
		(i * (Instance::SubRView0::VIEW_SIZE_X), k * (Instance::SubRView0::VIEW_SIZE_Y));
	      auto subArg1   = arg1.template subRView
		<Instance::SubRView1::VIEW_SIZE_X, Instance::SubRView1::VIEW_SIZE_Y>
		(k * (Instance::SubRView1::VIEW_SIZE_X), j * (Instance::SubRView1::VIEW_SIZE_Y));
	      
	      _Tail::doit(subArg0, subArg1, subResult); 
	    }
	  }
	}//end of loops
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce2<Instance, _K, false, true>
  {
    typedef mpl::false_ mem_pool;
    //    static bool thr_pool;

    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t dummy = mt::null_barrier)
    {
      auto compF = Instance::computationST();
#ifndef __NDEBUG 
      Profiler::getInstance().eventStart(__frm_kernel_seq);
#endif
      compF(arg0, arg1, result);
#ifndef __NDEBUG
      Profiler::getInstance().eventEnd(__frm_kernel_seq);
#endif
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce2<Instance, _K, true, true>
  {
    typedef mpl::false_ mem_pool;
    //    static bool thr_pool;

    typedef mpl::bool_<std::tr1::is_arithmetic<
			 typename Instance::Result
			 >::value>
    result_arithm;

    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::trivial_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(compF, arg0, arg1, __aux::ref(result, result_arithm()),
			barrier);
      leaf.detach();
    }
  };  
}
#endif
