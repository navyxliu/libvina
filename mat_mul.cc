#include "trait.hpp"
#include "frame.hpp"
#include "matrix2.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"

#include <tr1/functional>
using namespace vina;

#ifdef __NDEBUG
#define CHECK_RESULT(X) 
#endif

#ifndef CHECK_RESULT 
#define CHECK_RESULT(_R_) for (int i=0; i<TestMatrix::DIM_M; ++i)	\
    for (int j=0; j<TestMatrix::DIM_N; ++j)				\
      {									\
	if( fabs(_R_[i][j] - STD_result[i][j]) > 1e-3) {		\
	  printf("z[%2d][%2d]= %f WA, correct is %f\n",			\
		 i,j, _R_[i][j], STD_result[i][j]);			\
	  exit(1);							\
	}								\
      }									
#endif

template <class RESULT, class ARG0, class ARG1,
	  template <typename, typename, typename> class Func,
	  template <typename, typename>           class Redu,
	  template <typename, int, int>           class Pred,
	  int K=2, bool IsMT = true>
struct matmul_parallel
{
  typedef RESULT                Result;
  typedef ARG0                  Arg0;
  typedef ARG1                  Arg1;
  typedef mapreduce2< matmul_parallel, K, IsMT,
		      Pred<typename view_trait2<RESULT>::value_type,
			view_trait2<RESULT>::WRITER_SIZE_X,
			view_trait2<RESULT>::WRITER_SIZE_Y>::value
			> Map;

  typedef view_trait2<Arg0>    trait0;
  typedef view_trait2<Arg1>    trait1;
  typedef view_trait2<Result>  trait_result;
  typedef typename trait_result::value_type T;  
 
  // divide method
  typedef ReadView2<T, trait0::READER_SIZE_X/K, 
		       trait0::READER_SIZE_Y/K> 
  SubRView0;
  
  typedef ReadView2<T, trait1::READER_SIZE_X/K, 
		       trait1::READER_SIZE_Y/K> 
  SubRView1;
  
  typedef ReadView2<T, trait_result::WRITER_SIZE_X / K, 
                       trait_result::WRITER_SIZE_Y / K>
  SubRView2;
  
  typedef WriteView2<T, trait_result::WRITER_SIZE_X / K,
		        trait_result::WRITER_SIZE_Y / K>
  SubWView;


  const static bool _pred = Pred<T, view_trait2<RESULT>::WRITER_SIZE_X,
				 view_trait2<RESULT>::WRITER_SIZE_Y>::value;
  /*
  typedef memory_pool<typename trait_result::container_type, 
		      K, _pred> pool;
  */
  typedef matmul_parallel<SubWView, SubRView0, SubRView1,
			  Func, Redu, Pred, K, IsMT> SubTask;

  //interface to user, isomorphic transformation
  static void 
  doit(const Arg0& arg0, 
       const Arg1& arg1,
       Result& result){

    printf("_pred=%d SubTask::_pred=%d\n", _pred, SubTask::_pred);
    printf("SubTask::RESULT::WRITER_SIZE_X=%d\n", 
	   view_trait2<typename SubTask::Result>::WRITER_SIZE_X);

    Map::doit(arg0, arg1, result);
  };

  //leaf node concret function
  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&)>
  _Comp;

  static _Comp
  computation() {
    return  &(Func<Result, Arg0, Arg1>::doit);
  }

#ifndef __NDEBUG
  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&, mt::barrier_t, event_id)>
  _CompMT_t;
  static _CompMT_t
  computationMT_t() {
    return &(Func<Result, Arg0, Arg1>::doitMT_t);
  }
#endif
  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&, mt::barrier_t)>
  _CompMT;

  static _CompMT
  computationMT() {
    return &(Func<Result, Arg0, Arg1>::doitMT);
  }


  //reduce function
  typedef std::tr1::function<void (SubWView&, const SubRView2&)>
  _Redu;
  
  static _Redu
  reduction() {
    return &(Redu<SubWView, SubRView2>::doit);
  }
  
  typedef std::tr1::function<void (SubWView&, const SubRView2&, mt::barrier_t)>
  _ReduMT;

  static _ReduMT
  reductionMT() {
    return &(Redu<SubWView, SubRView2>::doitMT);
  }

  static void 
  reduce(Result& result, int i, int j, const SubRView2& RHS)
  {
    auto r = result.template subWView
      <SubWView::VIEW_SIZE_X, SubWView::VIEW_SIZE_Y>
  (i * (SubWView::VIEW_SIZE_X), j * (SubWView::VIEW_SIZE_Y));
    
    Redu<SubWView, SubRView2>::doit(r, RHS);
  }
};


template <class T, int size_x, int size_y>
struct p_simple{
  const static bool value = size_x <= MM_TEST_GRANULARITY;
};

template <class T, int SIZE_X, int SIZE_Y, int SIZE_Z>
struct p_lt_cache_ll : boost::mpl::bool_<((SIZE_A * SIZE_B + SIZE_A * SIZE_C + SIZE_B * SIZE_C) 
* sizeof(T) <= CACHE_LL_SIZE>
{};
int main()
{  

  // determine in compile-time, no way to pass it as parameters, 
  // depends on MACROs, see Makefile TEST_INFO
  printf("Matrix Multiplication test program\n\
  MM_TEST_SIZE_N=%4d\n\
  MM_TEST_GRANULARITY=%4d\n\
  MM_TEST_K=%4d\n\
  see Makefile TEST_INFO to set parameters\n", 
	 MM_TEST_SIZE_N, MM_TEST_GRANULARITY, MM_TEST_K);

  typedef Matrix<MM_TEST_TYPE, MM_TEST_SIZE_N, MM_TEST_SIZE_N> TestMatrix;
  typedef Matrix<vector_type<MM_TEST_TYPE>, MM_TEST_SIZE_N, MM_TEST_SIZE_N> TestMatrix_v;
  static TestMatrix x, y, z, STD_result;
  static TestMatrix_v x_v, y_v, z_v;

  Profiler &prof = Profiler::getInstance();
  //local
  auto temp0 = prof.eventRegister("STD multiply");
  auto temp1 = prof.eventRegister("Single-threaded");
  auto temp2 = prof.eventRegister("ST SSE");
  auto temp3 = prof.eventRegister("mulmat_parallel");
  auto temp4 = prof.eventRegister("mulmat_parallel SSE");

  NumRandGen<MM_TEST_TYPE> gen(2009);
  typedef TestMatrix::writer_type Writer;
  typedef TestMatrix_v::writer_type Writer_v;

  for (int i=0; i<TestMatrix::DIM_M; ++i) 
    for (int j=0; j<TestMatrix::DIM_N; ++j) 
      x[i][j] = x_v[i][j] = gen(), y[i][j] = y_v[i][j] = gen();

  double Comp = MM_TEST_SIZE_N * MM_TEST_SIZE_N;
  Comp = Comp *MM_TEST_SIZE_N * 2;

  z.zero();
  z_v.zero();
  STD_result.zero();
  auto result = z.subWView();
  auto result_v = z_v.subWView();

#ifndef __NDEBUG 
  prof.eventStart(temp0);
  
  multiply<MM_TEST_TYPE, MM_TEST_SIZE_N, MM_TEST_SIZE_N, MM_TEST_SIZE_N>
    (x, y, STD_result);

  prof.eventEnd(temp0);  printf("STD gflop=%f\n", Gflops(Comp, prof.getEvent(temp0)->elapsed()));
#endif 
  
  typedef matmul_parallel<Writer, TestMatrix, TestMatrix, 
    matMAddWrapper, matAddWrapper2, p_simple, MM_TEST_K, false> TF; // fransformer :~)

  prof.eventStart(temp1);
  TF::doit(x, y, result);
  prof.eventEnd(temp1);
  CHECK_RESULT(z);
  printf("ST gflop=%f\n", Gflops(Comp, prof.getEvent(temp1)->elapsed()));

  typedef matmul_parallel<Writer_v, TestMatrix_v, TestMatrix_v,
    matMAddWrapper, matAddWrapper2, p_simple, MM_TEST_K, false> TF_SSE;

  prof.eventStart(temp2);
  TF_SSE::doit(x_v, y_v, result_v);
  prof.eventEnd(temp2);
  printf("ST SSE gflop=%f\n", Gflops(Comp, prof.getEvent(temp2)->elapsed()));
  CHECK_RESULT(z_v);

  z.zero();
  typedef matmul_parallel<Writer, TestMatrix, TestMatrix,
    matMulWrapper, matAddWrapper2, p_simple, MM_TEST_K> TF_PARALLEL;
  
  prof.eventStart(temp3);
  TF_PARALLEL::doit(x, y, result);
  prof.eventEnd(temp3);
  CHECK_RESULT(z);
  printf("MT gflop=%f\n", Gflops(Comp, prof.getEvent(temp3)->elapsed()));

  z_v.zero();
  typedef matmul_parallel<Writer_v, TestMatrix_v, TestMatrix_v,
    matMulWrapper, matAddWrapper2, p_simple, MM_TEST_K> TF_PARALLEL_SSE;
 
  prof.eventStart(temp4);
  TF_PARALLEL_SSE::doit(x_v, y_v, result_v);
  prof.eventEnd(temp4);
  CHECK_RESULT(z_v);
  printf("MT SSE gflop=%f\n", Gflops(Comp, prof.getEvent(temp4)->elapsed()));

  prof.dump();
}
