// This file is not supposed to be distributed.
#ifndef VINA_FRAMEWORK
#define VINA_FRAMEWORK
#include "vector.hpp"
#include "mtsupport.hpp"
#include "profiler.hpp"
#include <tr1/type_traits>
#include <tr1/functional>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <sstream>

using namespace boost;

namespace vina {
  static event_id __frm_kernel_seq   = 
    Profiler::getInstance().eventRegister("#frame kernel seq");
  static event_id __frm_kernel_par   = 
    Profiler::getInstance().eventRegister("#frame kernel par");
  static event_id __frm_mem_cost = 
    Profiler::getInstance().eventRegister("#frame mem cost");
  static event_id __frm_thread_cost =
    Profiler::getInstance().eventRegister("#frame thread cost");

  struct sleeper{
    void operator()() {
      sleep(3);
    }
  };
  struct worker {
    void operator()()
    {
      float c = 0.0;
      for (int i=0; i<VEC_TEST_GRANULARITY; ++i)
	c+=3.14 * 0.85;
    }
  };
  namespace __aux{
    template <class IN, int size, bool>
    struct subview{
      static ReadView<typename view_trait<IN>::value_type, size>
      sub_reader(const IN& in, int nth)
      {
	return in.template subRView<size>
	  (nth * size);
      }
    };
    //scalar specialization
    template <class IN, int size>
    struct subview<IN, size, true>
    {
      static const IN&
      sub_reader(const IN& in, int nth __attribute__((unused)) )
      {
	return in;
      }
    };

    template <class T>
    struct viewdim{
      typedef mpl::int_<T::VIEW_SIZE>
      type;
    };
  }
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
    
    typedef typename mpl::eval_if<arg0_arithm,
				  mpl::int_<0>, 
				  __aux::viewdim<typename Instance::SubTask::Arg0>
				  >::type arg0_dim;

    typedef typename mpl::eval_if<arg1_arithm,
				  mpl::int_<0>, 
				  __aux::viewdim<typename Instance::SubTask::Arg1>
    			  >::type arg1_dim;

    const static int lookahead = 1 + _Tail::lookahead;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result)
    {
      if ( !_IsMT || lookahead > 1){
	  for (int k=0; k < _K; ++k) {
	    auto subArg0   = __aux::subview<typename Instance::Arg0, 
	      arg0_dim::value, arg0_arithm::value>::sub_reader(arg0, k);
	    
	    auto subArg1   = __aux::subview<typename Instance::Arg1,
	      arg1_dim::value, arg1_arithm::value>::sub_reader(arg1, k); 
	    
	    auto subResult = result.template subWView
	      <Instance::SubWView::VIEW_SIZE> (k * (Instance::SubWView::VIEW_SIZE));
	    _Tail::doit(subArg0, subArg1, subResult); 		
	  }
      }
      else {
	Profiler::getInstance().eventStart(__frm_kernel_par);
	//mt::barrier_t barrier(new boost::barrier(_K+1));
	mt::thread_t * handles[_K];
	event_id timers[_K];
	for(int i=0; i<_K; ++i)
	  {
	    std::ostringstream oss;
	    oss << "thread" << i << " timer";
	    timers[i] = Profiler::getInstance().eventRegister(oss.str());
	  }
	for (int k=0; k < _K; ++k) {
	  auto subArg0   = __aux::subview<typename Instance::Arg0, 
	    arg0_dim::value, arg0_arithm::value>::sub_reader(arg0, k);
	  
	  auto subArg1   = __aux::subview<typename Instance::Arg1,
	    arg1_dim::value, arg1_arithm::value>::sub_reader(arg1, k); 
	  
	  auto subResult = result.template subWView
	    <Instance::SubWView::VIEW_SIZE> (k * (Instance::SubWView::VIEW_SIZE));

	  auto compF = Instance::SubTask::computation();
	  //	  mt::thread_t leaf(bind(compF, subArg0, subArg1, subResult, barrier));
	  
	  //handles[k] = new boost::thread(compF, subArg0, subArg1, subResult, timers[k]);
	  handles[k] = new boost::thread(worker());
	  std::cout << "thread:" << handles[k]->get_id() <<"\n";
      

	  //leaf.detach();

	  //_Tail::doit(subArg0, subArg1, subResult, barrier);
	}
	//	barrier->wait();
	for(int k=0; k<_K; ++k)
	  handles[k]->join();
	
	Profiler::getInstance().eventEnd(__frm_kernel_par);
      }
    }
  };
  template<class Instance, int _K>
  struct mappar<Instance, _K, false, true>
  {
    static const int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      
      Profiler::getInstance().eventStart(__frm_kernel_seq);
      /*
      auto compF = Instance::computation();
      compF(arg0, arg1, result, -1);
      */
      worker w;
      w();
      Profiler::getInstance().eventEnd(__frm_kernel_seq);
    }
  };
           
  template<class Instance, int _K>
  struct mappar<Instance, _K, true, true>
  {
    static const int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(bind(compF, arg0, arg1, result, barrier));

#ifndef __NDEBUG 
      std::cout << "thread:" << leaf.get_id() <<"\n";
#endif
      leaf.detach();
    }
  };

  template <class Instance,
	    int  _K,
	    bool _IsMT,         /*enable multithread*/
	    bool __SENTINEL__   /*never use it explicitly*/ 
	    >
  struct mappar2{
    
    typedef mappar2<typename Instance::SubTask, _K, _IsMT, 
		Instance::SubTask::_pred> _Tail;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result)
    {
      for(int i=0; i<_K; ++i) for(int j=0; j<_K; ++j) {
	  auto subResult = result.template subWView
	    <Instance::SubWView::VIEW_SIZE_X, Instance::SubWView::VIEW_SIZE_Y>
	    (i * (Instance::SubWView::VIEW_SIZE_X), j * Instance::SubWView::VIEW_SIZE_Y);
	  
	  for (int k=0; k < _K; ++k) {
	    auto subArg0   = arg0.template subRView
	      <Instance::SubRView0::VIEW_SIZE_X, Instance::SubRView0::VIEW_SIZE_Y>
	      (i * (Instance::SubRView0::VIEW_SIZE_X), (k * Instance::SubRView0::VIEW_SIZE_Y));
	    auto subArg1   = arg1.template subRView
	      <Instance::SubRView1::VIEW_SIZE_X, Instance::SubRView1::VIEW_SIZE_Y>
	      (k * (Instance::SubRView1::VIEW_SIZE_X), (j * Instance::SubRView1::VIEW_SIZE_Y));
	    
	    _Tail::doit(subArg0, subArg1, subResult); 
	  }
      }
    }
  };

  template<class Instance, int _K>
  struct mappar2<Instance, _K, false, true>
  {
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result)
    {
      auto compF = Instance::computation();
      compF(arg0, arg1, result);
    }
  };
           
  template<class Instance, int _K>
  struct mappar2<Instance, _K, true, true>
  {
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(bind(compF, arg0, arg1, result, barrier));
      leaf.detach();
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

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      if (_IsMT && lookahead == 1) { // leaf
	mt::barrier_t barrier(new boost::barrier(_K+1));
	typedef typename Instance::SubTask::Result
	  SubResultType;
	auto submats = new SubResultType[_K];
	std::fill(submats, submats+_K, 0);

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
	delete [] submats;
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
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computation();
      compF(arg0, arg1, result);
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce<Instance, _K, true, true>
  {
    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(bind(compF, arg0, arg1, boost::ref(result), barrier));
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
    const static int lookahead = 1 + _Tail::lookahead;

    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result, 
		     mt::barrier_t barrier = mt::null_barrier)
    {
      for(int i=0; i<_K; ++i) for(int j=0; j<_K; ++j) {
	  if (_IsMT && lookahead == 1) { // leaf
	    typedef typename view_trait2<typename Instance::SubTask::Result>
	      ::container_type SubResultType;
	    typedef typename view_trait2<SubResultType>::writer_type SubWViewTemp;
	    typedef typename view_trait2<SubResultType>::reader_type SubRViewTemp;
	    auto submats = new SubResultType[_K];
	    SubWViewTemp subResults[_K];

	    for (int k=0; k<_K; ++k) 
	      subResults[k] = submats[k].subWView();

	    mt::barrier_t barrier(new boost::barrier(_K+1));
	    for (int k=0; k<_K; ++k) {
	      auto subArg0   = arg0.template subRView<Instance::SubRView0::VIEW_SIZE_X, 
		Instance::SubRView0::VIEW_SIZE_Y>
		(i * (Instance::SubRView0::VIEW_SIZE_X), k * (Instance::SubRView0::VIEW_SIZE_Y));
	      auto subArg1   = arg1.template subRView<Instance::SubRView1::VIEW_SIZE_X, 
		Instance::SubRView1::VIEW_SIZE_Y>
		(k * (Instance::SubRView1::VIEW_SIZE_X), j * (Instance::SubRView1::VIEW_SIZE_Y));
	      
	      _Tail::doit(subArg0, subArg1, subResults[k], barrier);
	    }
	  
	    barrier->wait();

	    //reduce
	    auto reduF = Instance::reduction();
	    for (int s=1; s < _K; s<<=1) for (int k=0; k < _K; k+=(s<<1)) {
		auto in0 = subResults[k];
		auto in1 = subResults[k+s].subRView();
		reduF(in0, /*<--*/in1);
	    }
	    Instance::reduce(result, i, j, subResults[0]);
	    delete [] submats;
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
    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computation();
      compF(arg0, arg1, result);
    }
  };
  
  template<class Instance, int _K>
  struct mapreduce2<Instance, _K, true, true>
  {
    const static int lookahead = 0;
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1,
		     typename Instance::Result& result,
		     mt::barrier_t barrier = mt::null_barrier)
    {
      auto compF = Instance::computationMT();
      mt::thread_t leaf(bind(compF, arg0, arg1, boost::ref(result), barrier));
      leaf.detach();
    }
  };
  
}
#endif
