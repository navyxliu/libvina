// This file is not supposed to be distributed.
#include "vector.hpp"
#include "frame.hpp"
#include "toolkits.hpp"
#include <tr1/functional>
#include <boost/mpl/bool.hpp>

//#include <omp.h>
using namespace vina;
#define ITERS 100

#ifdef __NDEBUG 
#define CHECK_RESULT(dummy)
#endif

#ifndef CHECK_RESULT
#define CHECK_RESULT(V_) for(int i=0; i<VEC_TEST_SIZE_N; ++i){		\
    if ( fabs(V_[i] - STD_result[i]) > 1e-3 ) {				\
      printf("result= %f WA, correct is %f\n",				\
	     V_[i], STD_result[i]);					\
      exit(1);								\
    }  }
#endif


template <class RESULT, class T, class RHS, 
	  template <typename, typename> class Func,
	  template <typename, int>      class Pred,
	  int K = 2 , bool IsMT = false>
struct saxpy{
  typedef view_trait<RHS> trait1;
  typedef view_trait<RESULT> trait_result;
  
  typedef typename trait_result::writer_type Result;
  typedef T                                  Arg0;
  typedef typename trait1::reader_type       Arg1;

  typedef mappar<saxpy, K, IsMT,
		 Pred<T, view_trait<RHS>::READER_SIZE>::value
 		 > Map;

  typedef ReadView<T, trait1::READER_SIZE/K>
  SubRView1;
  typedef WriteView<T, trait_result::WRITER_SIZE/K>
  SubWView;

  const static bool _pred 
  = Pred<T, view_trait<RHS>::READER_SIZE>::value;

  typedef saxpy<SubWView, T, SubRView1, Func, Pred, K, IsMT> SubTask;
  
  static void 
  doit(const T& alpha, const Arg1& lhs, 
       Result& rhs)
  {
//  printf("_pred=%d, SubTask::_pred=%d\n", 
//	   _pred, SubTask::_pred);

    Map::doit(alpha, lhs, rhs);
  }
  
  typedef std::tr1::function<void (void*, void*, void*)> 
  _Comp;
  static _Comp
  computation()
  {
    return &(Func<Result, Arg1>::doit_ptr);
  }
  typedef std::tr1::function<void (const T&, const Arg1&, Result&, mt::barrier_t)>
  _CompMT;
  static _CompMT
  computationMT()
  {
    return &(Func<Result, Arg1>::doitMT);
  }
#ifndef __NDEBUG
  typedef std::tr1::function<void (const T&, const Arg1&, Result&, mt::barrier_t, event_id)>
  _CompMT_t;
  static _CompMT_t
  computationMT_t() {
    return &(Func<Result, Arg1>::doitMT_t);
  }
#endif

};
template <class T, int DIM>
struct p_simple: mpl::bool_<DIM <= VEC_TEST_GRANULARITY>
{};


int 
main(int argc, char * argv[])
{
  printf("SAXPY Program in Blas\n\
VEC_TEST_SIZE_N=%d\n\
VEC_TEST_GRANULARITY=%4d\n\
VEC_TEST_K=%d\n\
See Makefile TEST_INFO to set parameters\n",
	 VEC_TEST_SIZE_N, VEC_TEST_GRANULARITY, VEC_TEST_K);
  typedef Vector<VEC_TEST_TYPE, VEC_TEST_SIZE_N> TestVector;
  typedef Vector<vector_type<VEC_TEST_TYPE>, VEC_TEST_SIZE_N> TestVector_v;
  typedef view_trait<TestVector>::reader_type Reader;
  typedef view_trait<TestVector_v>::reader_type Reader_v;
  typedef view_trait<TestVector>::writer_type Writer;
  typedef view_trait<TestVector_v>::writer_type Writer_v;


  static TestVector x, y, STD_result;
  static TestVector_v  x_v, y_v;
  Writer result = y.subWView();
  Writer_v result_v = y_v.subWView();

  NumRandGen<VEC_TEST_TYPE> gen(2009);
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    x[i] = x_v[i] = gen();

  double Comp = VEC_TEST_SIZE_N * 2;
  Profiler & prof = Profiler::getInstance();
  

  auto temp0 = prof.eventRegister("STD");
  auto temp1 = prof.eventRegister("ST");
  auto temp2 = prof.eventRegister("MT");
  //auto temp3 = prof.eventRegister("OPENMP");
#ifndef __NDEBUG
  /*  
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    printf("%4d", x[i]);
  printf("\n");
  for (int i=0; i<VEC_TEST_SIZE_N; ++i)
    printf("%4d", y[i]);
  printf("\n");
  */
  STD_result.zero();
  float * data_x = x.data();
  float * data_y = STD_result.data();

  prof.eventStart(temp0);
  for(int i=0; i<ITERS; ++i)
  //vina::saxpy<VEC_TEST_TYPE, VEC_TEST_SIZE_N>(7, x, STD_result);
  cblas_saxpy(VEC_TEST_SIZE_N, 7.0f, data_x, 1, data_y, 1);
  prof.eventEnd(temp0);  

  printf("elapsed=%d\n", prof.getEvent(temp0)->elapsed());
  printf("STD gflop=%f\n", Gflops(Comp, prof.getEvent(temp0)->elapsed()/ITERS));
#endif 
  
/*    
  y.zero();
  typedef ::saxpy<Writer, VEC_TEST_TYPE,  TestVector, vecMAddWrapper, p_simple, VEC_TEST_K>
    TF;
  prof.eventStart(temp1);
  for (int i=0; i<ITERS; ++i) {
	  TF::doit(7, x, result);
  }
  prof.eventEnd(temp1);
  CHECK_RESULT(y);
  printf("ST gflop=%f\n", Gflops(Comp, prof.getEvent(temp1)->elapsed() / ITERS));
 */ 
  y.zero();
  typedef ::saxpy<Writer, VEC_TEST_TYPE, TestVector, vecMAddWrapper, p_simple, VEC_TEST_K, true>
    TF_MT;

  int nr_pe = spmd_initialize();
  assert( nr_pe != -1 && "failed to initialize libSPMD runtime");
  printf("startup libspmd runtime: %d pe is detected\n", nr_pe);


  prof.eventStart(temp2);
  for (int i=0; i<ITERS; ++i) {
    TF_MT::doit(7.0f, x, result);
    //printf("iter#%2d: elasped %d\n", i, prof.getEvent(temp3)->elapsed());  
  }
  while ( !spmd_all_complete() );
  prof.eventEnd(temp2);
  CHECK_RESULT(y);

  printf("elapsed=%d\n", prof.getEvent(temp2)->elapsed());  
  printf("MT gflop=%f\n", Gflops(Comp, prof.getEvent(temp2)->elapsed() / ITERS));

  spmd_cleanup();

#if 0  
  prof.eventStart(temp3);
  for (int i=0; i<ITERS; ++i) {
  float * x_ptr = x.data();
  float * result_ptr = result.data(); 
  #pragma omp parallel for 
  for(int i=0; i<VEC_TEST_K; ++i)
    {
      //result[i] = 7 * x[i];    
      *(result_ptr++) = 7.0 * *x_ptr++;
    }
  }
  prof.eventEnd(temp3);
  printf("OMP gflop=%f\n", Gflops(Comp, prof.getEvent(temp3)->elapsed()/ ITERS));
#endif 
  prof.dump();
}
