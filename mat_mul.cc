// This file is not supposed to be distributed. 
// History:
// 09, Aug. create file and merge orginal demo here.
// 09, Sep. 25: support libSPMD
// 09, Sep. 28: Add standard routine using mkl.

#include "trait.hpp"
#include "frame.hpp"
#include "matrix2.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"

#include <tr1/functional>
using namespace vina;
#include <pthread.h>
#include <mkl.h>
#include "mkl_cblas.h"

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
pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;

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
  typedef std::tr1::function<void (void *, void *, void *)>
  _Comp;

  static _Comp*
  computation() {
    return  new _Comp(&Func<Result, Arg0, Arg1>::doit_ptr);
  }

  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&)>
  _CompST;
  static _CompST
  computationST()
  {
    return &(Func<Result, Arg0, Arg1>::doit); 
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
  reductionMT() 
  {
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
  
/* reduce function for libspmd*/
  static void
  reduce_ptr(void * arg) {
    auto reduF = reduction(); 
    auto _subs = (SubWView **)arg;
     
   //fprintf(stderr, "in reduce_ptr subs = %p subs[0] = %p\n", arg, _subs[0]);
    for (int s=1; s < K; s<<=1) for (int k=0; k < K; k+=(s<<1)) {
      auto in0 = _subs[k];
      auto in1 = (SubRView2*)_subs[k+s];
      reduF(*in0, /*<--*/*in1);     
    }
   
  }
 /* ydf gave me the idea to implement template static variable by function static var
  * the benefit of this approach is that programmer can avoid the intialization expresson\
  * outside of template class. 
  * our template parameters are  painfully long. 
  * 2009/07, xliu
  */
 /* amazingly ugly. remove the reference of this function from my codebase. because this \
  * trick is still useful in my template library, so leave corpse here. 
  * 2009/09
  */
 #if 0 
 inline static void * 
 localstorage (bool ld/*load or store*/, void * value = NULL )
 {
 /*NOT thread-safe!*/
   static void * ls[K*K];
   static int i;
   static int j;
   void * ret;
   {
   printf("ld = %d, i = %d, j = %d\n", (int)ld, i, j);
   if ( ld ? j >= i : i >= K*K ) {
     fprintf(stderr, "IN DANGER! you are wrongly using this function.");
     _exit(-1);
   }

   if ( !ld ) ret = ls[i++] = value;
   else ret = ls[j++];
   }
   return ret;
 } 
 #endif
};

template <class T, int size_x, int size_y>
struct p_simple{
  const static bool value = size_x <= MM_TEST_GRANULARITY;
};

template <class T>
void 
printm(const T& M)
{
  for (int i=0; i<T::DIM_M; ++i, putc('\n', stdout)) for (int j=0; j<T::DIM_N; ++j) 
    printf("%.4f ", M[i][j]);
}
int main()
{  

  // determine in compile-time, no way to pass it as parameters, 
  // depends on MACROs, see Makefile TEST_INFO
  printf("Matrix Multiplication test program\n\
  MM_TEST_SIZE_N=%4d\n\
  MM_TEST_GRANULARITY=%4d\n\
  MM_TEST_K=%4d\n"
#ifdef __USE_LIBSPMD
"thread: LIBSPMD\n"
#else
"thread: pthread\n"
#endif
  "see Makefile TEST_INFO to set parameters\n", 
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
  auto temp5 = prof.eventRegister("mulmat_smallset");

  NumRandGen<MM_TEST_TYPE> gen(2009);
  typedef TestMatrix::writer_type Writer;
  typedef TestMatrix_v::writer_type Writer_v;

  for (int i=0; i<TestMatrix::DIM_M; ++i) 
    for (int j=0; j<TestMatrix::DIM_N; ++j) 
      x[i][j] = x_v[i][j] = gen(), y[i][j] = y_v[i][j] = gen();
/*
  printf("matrix X:\n");
  printm(x);
  printf("\nmatrix Y:\n");
  printm(y);
  printf("\n");
*/
  double Comp = MM_TEST_SIZE_N * MM_TEST_SIZE_N;
  Comp = Comp * MM_TEST_SIZE_N * 2;

  z.zero();
  z_v.zero();

  STD_result.zero();
  auto result = z.subWView();
  auto result_v = z_v.subWView();
  /*
   * replace my trivial mm with mkl sgemm procedure
   */
  //multiply<MM_TEST_TYPE, MM_TEST_SIZE_N, MM_TEST_SIZE_N, MM_TEST_SIZE_N>
  //(x, y, z);
#ifndef __NDEBUG
  float * std_x, * std_y, * std_z;
  std_x = x.data();
  std_y = y.data();
  std_z = STD_result.data();
  prof.eventStart(temp0);
  {
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, MM_TEST_SIZE_N, MM_TEST_SIZE_N, MM_TEST_SIZE_N,
         1.0f/*alpha*/, std_x, MM_TEST_SIZE_N, std_y, MM_TEST_SIZE_N, 0.0f/*beta*/, std_z, MM_TEST_SIZE_N);
  }
  prof.eventEnd(temp0); 
  printf("STD gflop=%f\n", Gflops(Comp, prof.getEvent(temp0)->elapsed()));
#endif

/*  
  typedef matmul_parallel<Writer, TestMatrix, TestMatrix, 
    matMAddWrapper, matAddWrapper2, p_simple, MM_TEST_K, false> TF; // fransformer :~)
  prof.eventStart(temp1);
  TF::doit(x, y, result);
  prof.eventEnd(temp1);
  CHECK_RESULT(z);
  printf("ST gflop=%f\n", Gflops(Comp, prof.getEvent(temp1)->elapsed()));
*/
/*
  typedef matmul_parallel<Writer_v, TestMatrix_v, TestMatrix_v,
    matMAddWrapper, matAddWrapper2, p_simple, MM_TEST_K, false> TF_SSE;

  prof.eventStart(temp2);
  TF_SSE::doit(x_v, y_v, result_v);
  prof.eventEnd(temp2);
  printf("ST SSE gflop=%f\n", Gflops(Comp, prof.getEvent(temp2)->elapsed()));
  CHECK_RESULT(z_v);
*/ 
/*
  z.zero();
  typedef matmul_parallel<Writer, TestMatrix, TestMatrix,
    matMulWrapper, matAddWrapper2, p_simple, MM_TEST_K> TF_PARALLEL;
  
  prof.eventStart(temp3);
  TF_PARALLEL::doit(x, y, result);
  prof.eventEnd(temp3);
  CHECK_RESULT(z);
  printf("MT gflop=%f\n", Gflops(Comp, prof.getEvent(temp3)->elapsed()));
*/

  //_spmd_initialize(MM_TEST_K);
  spmd_initialize();
  //mkl_set_num_threads(1);
  z_v.zero();
  typedef matmul_parallel<Writer_v, TestMatrix_v, TestMatrix_v,
    matMulWrapper, matAddWrapper2, p_simple, MM_TEST_K> TF_PARALLEL_SSE;

  printf("x_v.data() = %p, y_v.data() = %p result_v = %p\n", 
         x_v.data(), y_v.data(), result_v.data());

  prof.eventStart(temp4);
  TF_PARALLEL_SSE::doit(x_v, y_v, result_v);
  while (!spmd_all_complete());
  prof.eventEnd(temp4);
  CHECK_RESULT(z_v);
  printf("MT SSE gflop=%f\n", Gflops(Comp, prof.getEvent(temp4)->elapsed()));
  spmd_cleanup();
  prof.dump();
}
