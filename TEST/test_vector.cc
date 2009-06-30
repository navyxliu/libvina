// This file is not supposed to be distributed.
#include <memory>
#include <map>
#include <string>
#include <tr1/functional>
#include <tr1/type_traits>

#include "vector.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"
#include "mtsupport.hpp"
using namespace std;

// quite dump impl
template <class Result, class Arg0, class Arg1>
struct algo_vect_add_impl {
  static Result doit(Arg0 arg0, Arg1 arg1)
  {
    typedef typename tr1::remove_reference<
    typename tr1::remove_const<Arg0>::type>::type Vec0;
    typedef typename tr1::remove_reference<
    typename tr1::remove_const<Arg1>::type>::type Vec1;
    
    typedef xliu::Vector<typename Result::value_type, 
      Result::DIM_N/2> SubVec;					       
    
    SubVec loArg0(arg0.begin(), arg0.begin() + (Vec0::DIM_N/2));
    SubVec hiArg0(arg0.begin() + (Vec0::DIM_N/2), arg0.end());

    SubVec loArg1(arg1.begin(), arg1.begin() + (Vec1::DIM_N/2));
    SubVec hiArg1(arg1.begin() + (Vec1::DIM_N/2), arg1.end());
    
    SubVec loResult, hiResult;

    typedef tr1::function
      <SubVec (const SubVec&, const SubVec&)> 
      subComp;
    
     subComp subF
      = &(xliu::operator+<typename Result::value_type, (Result::DIM_N)/2>);
     
     loResult =::bind(subF, loArg0, loArg1)();
     hiResult = tr1::bind(subF, hiArg0, hiArg1)();

     Result result;

     for (int i=0; i < Result::DIM_N/2; ++i){
       result[i] = loResult[i];
       result[Result::DIM_N/2 + i] = hiResult[i];
     }

#ifndef __NDEBUG          
     // using types of arguments, we can figure out
     // which specialization of template function.
     tr::function<Result (Arg0, Arg1)> func
       = &(xliu::operator+<typename Result::value_type, Result::DIM_N>);

     Result result2 = tr1::bind(func, arg0, arg1)();
     assert( result == result2);
#endif
     return result;
  }
};

// class as pattern impl.
template <class RESULT, class Arg0, class Arg1>
struct algo_vect_add_impl2 {
  
  typedef xliu::view_trait<Arg0> trait0;
  typedef xliu::view_trait<Arg1> trait1;
  typedef xliu::view_trait<RESULT> trait_result;

  typedef typename trait0::vector_type::
  template ReadView<trait0::READER_SIZE/2>
  SubRView0;
  
  typedef typename trait1::vector_type::
  template ReadView<trait0::READER_SIZE/2>
  SubRView1;

  typedef typename trait_result::vector_type::
  template WriteView<trait_result::WRITER_SIZE/2>
  SubWView;
  
  static void doit(const Arg0& arg0, const Arg1& arg1, RESULT& result)
  {

    auto loArg0 = arg0.template subRView<SubRView0::VIEW_SIZE>(0);
    auto hiArg0 = arg0.template subRView<SubRView0::VIEW_SIZE>(SubRView0::VIEW_SIZE);

    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(loArg0), SubRView0>::value ));
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(hiArg0), SubRView0>::value ));
     
    auto loArg1 = arg1.template subRView<SubRView1::VIEW_SIZE>(0);
    auto hiArg1 = arg1.template subRView<SubRView1::VIEW_SIZE>(SubRView1::VIEW_SIZE);
    
    auto loResult = result.template subWView<SubWView::VIEW_SIZE>(0);
    auto hiResult = result.template subWView<SubWView::VIEW_SIZE>(RESULT::VIEW_SIZE/2);
    
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(loResult), SubWView>::value ));
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(hiResult), SubWView>::value ));
    
    
    typedef tr1::function
      <void (const SubRView0&, const SubRView1&, SubWView&)> 
      subComp;
    
     subComp subF
       = &(xliu::vecAdd<SubWView, SubRView0, SubRView1>);

     tr1::bind(subF, loArg0, loArg1, loResult)();
     tr1::bind(subF, hiArg0, hiArg1, hiResult)();
  }
};

// predicator, its purpose is end recursive instantiations
template <int SZ>
struct p_algo_vect_add
{
  const static bool value = SZ < 500000;
};

// implement reduction
template <class RESULT, class Arg0, class Arg1, template<int> class Pred, 
	  bool __SENTINEL__ /*never use it*/ = Pred< xliu::view_trait<Arg0>::READER_SIZE>::value >
struct algo_vect_add_impl3 {
  typedef xliu::view_trait<Arg0> trait0;
  typedef xliu::view_trait<Arg1> trait1;
  typedef xliu::view_trait<RESULT> trait_result;

  typedef typename trait0::vector_type::
  template ReadView<trait0::READER_SIZE/2>
  SubRView0;
  
  typedef typename trait1::vector_type::
  template ReadView<trait0::READER_SIZE/2>
  SubRView1;

  typedef typename trait_result::vector_type::
  template WriteView<trait_result::WRITER_SIZE/2>
  SubWView;
  
  static void doit(const Arg0& arg0, const Arg1& arg1, RESULT& result)
  {
    auto loArg0 = arg0.template subRView<SubRView0::VIEW_SIZE>(0);
    auto hiArg0 = arg0.template subRView<SubRView0::VIEW_SIZE>(SubRView0::VIEW_SIZE);

    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(loArg0), SubRView0>::value ));
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(hiArg0), SubRView0>::value ));
     
    auto loArg1 = arg1.template subRView<SubRView1::VIEW_SIZE>(0);
    auto hiArg1 = arg1.template subRView<SubRView1::VIEW_SIZE>(SubRView1::VIEW_SIZE);
    
    auto loResult = result.template subWView<SubWView::VIEW_SIZE>(0);
    auto hiResult = result.template subWView<SubWView::VIEW_SIZE>(RESULT::VIEW_SIZE/2);
    
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(loResult), SubWView>::value ));
    BOOST_STATIC_ASSERT(( tr1::is_same<decltype(hiResult), SubWView>::value ));

    void (*pf) (const SubRView0&, const SubRView1&, SubWView&);
    
    pf = &algo_vect_add_impl3<SubWView, SubRView0, SubRView1, 
      Pred>::doit;
    
#ifdef  FORK_AT_CALLSITE 
    xliu::mt::thread_t t0(pf, loArg0, loArg1, loResult);
    xliu::mt::thread_t t1(pf, hiArg0, hiArg1, hiResult);
    t0.detach();
    t1.detach();
#else // otherwise, initialize thead at leaf
    algo_vect_add_impl3<SubWView, SubRView0, SubRView1, 
      Pred>::doit(loArg0, loArg1, loResult);
    
    algo_vect_add_impl3<SubWView, SubRView0, SubRView1, 
      Pred>::doit(hiArg0, hiArg1, hiResult);
#endif
  }
};
// this is the tail of algo
template <class RESULT, class Arg0, class Arg1, template<int> class Pred>
struct algo_vect_add_impl3<RESULT, Arg0, Arg1, Pred,  true>
{
  
  static void doit(const Arg0& arg0, const Arg1& arg1, RESULT& result)
  {
    /*
#ifndef __NDEBUG
    std::cout << "execution instance: " << RESULT::VIEW_SIZE << "size"
	 << "\tthread id: " << boost::this_thread::get_id()
	 << std::endl;
#endif
    */
#ifdef  FORK_AT_CALLSITE  // call calculation directly
    xliu::vecAdd(arg0, arg1, result);
#else
    typedef tr1::function
      <void (const Arg0&, const Arg1&, RESULT&)> 
      Comp;
    
    Comp F = &(xliu::vecAdd<RESULT, Arg0, Arg1>);
    
    auto task = tr1::bind(F, arg0, arg1, result);

    xliu::mt::thread_t t(task);
    t.detach();
#endif
    
  }
};

int 
main()
{
  xliu::NumRandGen<float> gen;

  static xliu::Vector<float, 1000000> A, B, C, D;

  for (int i=0; i < 1000000; ++i)
    A[i] = gen();
  for (int i=0; i < 1000000; ++i)
    B[i] = gen();
  
  using xliu::Profiler;

  Profiler& prof = Profiler::getInstance();
  auto temp0 = prof.eventRegister("canonical plus");
  auto temp1 = prof.eventRegister("view plus");
  auto temp2 = prof.eventRegister("recursive plus");
  prof.eventStart(temp0);
  
  C = A + B;

  prof.eventEnd(temp0);
  prof.eventStart(temp1);
   
  auto result = C.subWView();
  //vecAdd(A.subRView(), B.subRView(), result);

  typedef algo_vect_add_impl2<xliu::Vector<float, 1000000>::WriteView<1000000>,
    xliu::Vector<float, 1000000>,
    xliu::Vector<float, 1000000>
    > R;
  
  BOOST_STATIC_ASSERT((boost::is_same<R::SubRView0, xliu::Vector<float, 1000000>::ReadView<500000> >::value));
  BOOST_STATIC_ASSERT((boost::is_same<R::SubRView1, xliu::Vector<float, 1000000>::ReadView<500000> >::value));
  BOOST_STATIC_ASSERT((boost::is_same<R::SubWView, xliu::Vector<float, 1000000>::WriteView<500000> >::value));

  R::doit(A, B, result);

  prof.eventEnd(temp1);
  
  prof.eventStart(temp2);
  typedef algo_vect_add_impl3<xliu::Vector<float, 1000000>::WriteView<1000000>,
    xliu::Vector<float, 1000000>,
    xliu::Vector<float, 1000000>,
    p_algo_vect_add>  R2;
  
  R2::doit(A, B, result);
  
  prof.eventEnd(temp2);
  
  
#ifndef __NDEBUG 
  auto lhs = result.subRView();
  auto rhs = C.subRView();
  BOOST_STATIC_ASSERT(( boost::is_same<decltype(lhs), decltype(rhs)>::value ));
  // the comparison of rview can not match function call by their own. 
  // check manually.
  auto res_pair = std::mismatch(lhs.begin(), lhs.end(), rhs.begin());
  assert( res_pair.first == lhs.end() 
	  && res_pair.second == rhs.end());
#endif

  prof.dump();
}
