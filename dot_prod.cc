// This file is not supposed to be distributed.
#include "vector.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"
#include "frame.hpp"

#include <stdio.h>
#include <tr1/functional>

#ifdef MKL
#include <mkl.h>
#include "mkl_cblas.h"
#endif

using namespace vina;

#ifdef __NDEBUG 
#define CHECK_RESULT(dummy)
#endif

#ifndef CHECK_RESULT
#define CHECK_RESULT(V_)  if ( fabs(V_-STD_result) > 1.0) {	\
    printf("result= %f WA, correct is %f\n",	\
	   V_, STD_result);			\
    exit(1);					\
  }
#endif
 /*
#define CHECK_RESULT(V_) if ( V_ != STD_result ) {	\
    printf("result= %d WA, correct is %d\n",	\
	   V_, STD_result);			\
    exit(1);					\
  }   
#endif
*/
template <class RESULT,  class ARG0, class ARG1,
	  template <typename, typename, typename> class Func,
	  template <typename> class Redu,
	  template <typename, int> class Pred,
	  int K = 2, bool IsMT=false>
struct dotprod{
  typedef mapreduce<dotprod, K, IsMT,
		    Pred<RESULT, view_trait<ARG0>::READER_SIZE>::value
		 > Map;
  typedef RESULT Result;
  typedef ARG0   Arg0;
  typedef ARG1   Arg1;
  typedef RESULT T;

  typedef view_trait<Arg0> trait0;
  typedef view_trait<Arg1> trait1;

  typedef ReadView<T, trait0::READER_SIZE/K>
  SubRView0;

  typedef ReadView<T, trait1::READER_SIZE/K>
  SubRView1;

  const static bool _pred = Pred<T, view_trait<SubRView0>::READER_SIZE>::value;
  
  typedef dotprod<T, SubRView0, SubRView1, Func, Redu, Pred, K> SubTask;
 
  static void 
  doit(const Arg0& arg0, const Arg1& arg1, Result& result)
  {
    Map::doit(arg0, arg1, result);
  }

  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&)>
  _Comp;

  static _Comp
  computation() {
    return &(Func<Result, Arg0, Arg1>::doit);
  }

  typedef std::tr1::function<void (void *, void *, void *)>
  _Comp_ptr;

  static _Comp_ptr *
  computation_ptr () {
    return new _Comp_ptr(&(Func<Result, Arg0, Arg1>::doit_ptr));
  }

  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&, mt::barrier_t)>
  _CompMT;
  static _CompMT
  computationMT() {
    return &(Func<Result, Arg0, Arg1>::doitMT);
  }

  typedef std::tr1::function<void (T&, const T&)>
  _Redu;

  static _Redu 
  reduction() 
  {
    return &(Redu<T>::doit);
  }

  static void 
  reduce(T& lhs, const T& rhs)
  {
    Redu<T>::doit(lhs, rhs);
  }

  static void
  reduce2(T * R) 
  {	
     for (int s=1; s < K; s<<=1) for (int k=0; k < K; k+=(s<<1)) {
	 *(R+k) += *(R+k+s);
     }
  }
};
template <class T, int DIM>
struct p_simple{
  const static bool value = DIM <= VEC_TEST_GRANULARITY;
};

template <class T>
struct reduce_add{
  static void 
  doit(T& lhs, const T& rhs)
  {
    lhs += rhs;
  }
};

int main() 
{
  printf("Vector DotProduction program\n\
VEC_TEST_SIZE_N=%4d\n\
VEC_TEST_GRANULARITY=%4d\n\
VEC_TEST_K=%d\n"
#ifdef __USE_LIBSPMD
"thread: libspmd"
#else
"thread:pthread"
#endif
"See Makefile TEST_INFO to set parameters\n",
 VEC_TEST_SIZE_N, VEC_TEST_GRANULARITY, VEC_TEST_K);

  typedef Vector<VEC_TEST_TYPE, VEC_TEST_SIZE_N> TestVector;
  typedef Vector<vector_type<VEC_TEST_TYPE>, VEC_TEST_SIZE_N> TestVector_v;
  typedef view_trait<TestVector>::reader_type Reader;
  typedef view_trait<TestVector_v>::reader_type Reader_v;
  TestVector *px = new TestVector;
  TestVector *py = new TestVector;
  TestVector_v *px_v = new TestVector_v;
  TestVector_v *py_v = new TestVector_v;
  
  TestVector &x = *px, &y = *py;
  TestVector_v  &x_v = *px_v, &y_v = *py_v;

  VEC_TEST_TYPE z, z_v, STD_result;

  NumRandGen<VEC_TEST_TYPE> gen(2009);
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    x[i] = x_v[i] = gen(), y[i] = y_v[i] = gen();

  double Comp = VEC_TEST_SIZE_N * 2.0;
  Profiler & prof = Profiler::getInstance();

  auto temp0 = prof.eventRegister("STD");
  auto temp1 = prof.eventRegister("ST");
  auto temp2 = prof.eventRegister("MT");

#ifndef __NDEBUG
  /*  
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    printf("%4d", x[i]);
  printf("\n");
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    printf("%4d", y[i]);
  printf("\n");
  */
  const float * data_x = x.data();
  const float * data_y = y.data();
  prof.eventStart(temp0);
#ifndef MKL
  STD_result = dotproduct<VEC_TEST_TYPE, VEC_TEST_SIZE_N>(x, y);
#else 
  STD_result = cblas_sdot(VEC_TEST_SIZE_N, data_x, 1, data_y, 1);
#endif
  prof.eventEnd(temp0);  

  printf("elapsed=%d\n", prof.getEvent(temp0)->elapsed());
  printf("STD gflop=%f\n", Gflops(Comp, prof.getEvent(temp0)->elapsed()));
#endif 
 /* 
  typedef dotprod<VEC_TEST_TYPE, TestVector, TestVector, 
    vecDotProdWrapper, reduce_add, p_simple,VEC_TEST_K> TF;

  z = 0;
  prof.eventStart(temp1);
  TF::doit(x, y, z);
  prof.eventEnd(temp1);
 
  CHECK_RESULT(z);
  printf("ST gflop=%f\n", Gflops(Comp, prof.getEvent(temp1)->elapsed()));
  fflush(stdout);
*/

#ifdef __USE_LIBSPMD
  spmd_initialize();
#endif
  typedef dotprod<VEC_TEST_TYPE, TestVector, TestVector,
    vecDotProdWrapper, reduce_add, p_simple, VEC_TEST_K, true> TF_MT;

  z = 0;
  prof.eventStart(temp2);
  TF_MT::doit(x, y, z);
  prof.eventEnd(temp2);

#ifdef __USE_LIBSPMD
  spmd_cleanup();
#endif

  CHECK_RESULT(z);
  printf("elapsed=%d\n", prof.getEvent(temp2)->elapsed());
  printf("MT gflop=%f\n", Gflops(Comp, prof.getEvent(temp2)->elapsed()));
  fflush(stdout);
  
  prof.dump();
}


