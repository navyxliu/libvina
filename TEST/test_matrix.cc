#include "matrix2.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"
#include "mtsupport.hpp"

#include <tr1/type_traits>
#include <tr1/functional>

#include <iostream>
#include <string>

using namespace xliu;

using std::cout;
using std::string;
using std::tr1::function;
using std::tr1::is_same;
using std::tr1::bind;

event_id prof_time_kernel;
event_id prof_cnt_cache0;
event_id prof_cnt_cache1;
event_id prof_cnt_cache2;

// toy predicator to debug
template <class T, int SIZE_A, int SIZE_B, int SIZE_C>
struct p_very_simple {
  const static bool value = SIZE_A <= 2;
};

template <class T, int SIZE_A, int SIZE_B, int SIZE_C>
struct p_lt_cache_ll {
  enum {CACHE_L1_SIZE = 4096*1024};
  const static bool value = ((SIZE_A * SIZE_B + SIZE_A * SIZE_C + SIZE_B * SIZE_C) 
			     * sizeof(T) ) <= CACHE_L1_SIZE;
};

/// Template Parameters: 
// F: see [matrix2.hpp] L404
template<class RESULT, class Arg0, class Arg1, 
	 template <typename, typename, typename> class F, /*generalized functor type*/
	 template<typename, int, int, int> class Pred, int K = 2, bool IsMT = false,/*is multithreaded*/
	 bool __SENTINEL__ /*never use it explicitly*/ 
	 = Pred<typename RESULT::value_type, 
		view_trait2<Arg0>::READER_SIZE_X,
		view_trait2<Arg1>::READER_SIZE_Y,
		view_trait2<RESULT>::WRITER_SIZE_Y>::value>
struct algo_matr_impl
{
  typedef view_trait2<Arg0>    trait0;
  typedef view_trait2<Arg1>    trait1;
  typedef view_trait2<RESULT>  trait_result;
  
  typedef typename trait0::matrix_type::
  template ReadView<trait0::READER_SIZE_X/K, 
		    trait0::READER_SIZE_Y/K> 
  SubRView0;
  
  typedef typename trait1::matrix_type::
  template ReadView<trait1::READER_SIZE_X/K, 
		    trait1::READER_SIZE_Y/K> 
  SubRView1;
  
  typedef typename trait_result::matrix_type::
  template ReadView<trait_result::READER_SIZE_X / K, 
		    trait_result::WRITER_SIZE_Y / K>
  SubRView2;
  
  typedef typename trait_result::matrix_type::
  template WriteView<trait_result::WRITER_SIZE_X / K,
		     trait_result::WRITER_SIZE_Y / K>
  SubWView;
  
  typedef Matrix<typename RESULT::value_type, 
		 SubWView::VIEW_SIZE_X, SubWView::VIEW_SIZE_Y> 
  SubMatrix;

  typedef algo_matr_impl<SubWView, SubRView0, SubRView1, F, Pred, 
			 K, IsMT> _Tail;
  
  const static int lookahead = 1 + _Tail::lookahead;

  static void checkit() {
    static_assert(trait0::READER_SIZE_X % K == 0, 
		  "invalid K SubRView0");
    static_assert(trait0::READER_SIZE_Y % K == 0, 
		  "invalid K SubRView0");
    static_assert(trait1::READER_SIZE_X % K == 0,
		  "invalid K SubRView1");
    static_assert(trait1::READER_SIZE_Y % K == 0,
		  "invalid K SubRView1");
    static_assert(trait_result::WRITER_SIZE_X % K == 0,
		  "invalid K SubWView");
    static_assert(trait_result::WRITER_SIZE_Y % K == 0, 
		  "invalid K SubWView");
  }
  //entry point
  static void doit(const Arg0& arg0, const Arg1& arg1, RESULT& result
		   ,CallsiteOutput * gout = NULL)
  {
    checkit();
#ifdef VIZ_CALLSITE
    if ( gout ) gout->enter();
#endif
    // care-free split
    for (int i=0; i < K; ++i) {
      for (int j=0; j < K; ++j) {

	if (IsMT && lookahead == 1) {	  // time to fork
	  typedef typename view_trait2<SubMatrix>::writer_type SubWViewTemp;
	  typedef typename view_trait2<SubMatrix>::reader_type SubRViewTemp;
	  SubWViewTemp subResults[K];
	  
	  auto submats = new SubMatrix[K];
	  for (int k=0; k<K; ++k) {
	    subResults[k] = submats[k].subWView();
	  }
#ifdef VIZ_CALLSITE
	  if (gout) gout->enterMT();
#endif

	  mt::barrier_t barrier(new boost::barrier(K+1));
	  for (int k=0; k<K; ++k) {
	    auto subArg0   = arg0.template subRView<SubRView0::VIEW_SIZE_X, SubRView0::VIEW_SIZE_Y>
	      (i * (SubRView0::VIEW_SIZE_X), (k * SubRView0::VIEW_SIZE_Y));
	    auto subArg1   = arg1.template subRView<SubRView1::VIEW_SIZE_X, SubRView1::VIEW_SIZE_Y>
	      (k * (SubRView1::VIEW_SIZE_X), (j * SubRView1::VIEW_SIZE_Y));
#ifdef VIZ_CALLSITE
	    if (gout) gout->callsiteD("doitMT");
#endif
	    algo_matr_impl<SubWViewTemp, SubRView0, SubRView1, matMulWraper, Pred, K, true, true>
	      ::doitMT(subArg0, subArg1, subResults[k], barrier, gout);
	  }
	  
	  barrier->wait();

#ifdef VIZ_CALLSITE
	  if (gout) gout->leaveMT();
#endif

#ifndef __NDEBUG
	  for (int k=0; k<K; ++k){
	    cout << "Result" << k << "\n";
	    for (int ii=0; ii<SubWViewTemp::VIEW_SIZE_X; ++ii, cout << "\n")
	      for (int jj=0; jj<SubWViewTemp::VIEW_SIZE_Y; ++jj)
		{
		  cout << subResults[k][ii][jj] << "  ";
		}
	  }
#endif
	  
	  //reduce
	  for (int s=1; s < K; s<<=1)
	    for (int k=0; k < K; k+=(s<<1)) {
	      auto in0 = subResults[k].subRView();
	      auto in1 = subResults[k+s].subRView();
	      BOOST_STATIC_ASSERT(( is_same<decltype(in0), SubRViewTemp>::value ));
	      BOOST_STATIC_ASSERT(( is_same<decltype(in1), SubRViewTemp>::value ));
	      // gcc bug?
	      //BOOST_STATIC_ASSERT(( boost::is_same<decltype(subResults[k]), SubWViewTemp>::value ));
	      matAddWraper<SubWViewTemp, SubRViewTemp, SubRViewTemp>::doit(in0, in1, subResults[k]);
	    }
	  
	  auto r = result.template subWView
	    <SubWView::VIEW_SIZE_X, SubWView::VIEW_SIZE_Y>
	    (i * (SubWView::VIEW_SIZE_X), j * SubWView::VIEW_SIZE_Y);

#ifdef VIZ_CALLSITE
	  if ( gout ) {
	    gout->callsite("reduce");
	    gout->depend();
	  }	  
#endif
	  for (int ii=0; ii<SubWView::VIEW_SIZE_X; ++ii)
	    for (int jj=0; jj<SubWView::VIEW_SIZE_Y; ++jj)
	      {
		r[ii][jj] += submats[0][ii][jj];
	      }

#ifndef __NDEBUG 
	  cout << "after reduce\n";
	  for (int ii=0; ii<SubWView::VIEW_SIZE_X; ++ii, cout <<"\n")
	    for (int jj=0; jj<SubWView::VIEW_SIZE_Y; ++jj)
	      cout << r[ii][jj] << "  ";

#endif
	  //delete [] submats;
	}
	else {
#ifndef __NDEBUG
	  cout << "LOOKAHEAD" << lookahead 
	       << "IsMT" << IsMT << std::endl;
#endif
	  auto subResult = result.template subWView
	      <SubWView::VIEW_SIZE_X, SubWView::VIEW_SIZE_Y>
	      (i * (SubWView::VIEW_SIZE_X), j * SubWView::VIEW_SIZE_Y);
	    
	    for (int k=0; k < K; ++k) {
	      auto subArg0   = arg0.template subRView<SubRView0::VIEW_SIZE_X, SubRView0::VIEW_SIZE_Y>
		(i * (SubRView0::VIEW_SIZE_X), (k * SubRView0::VIEW_SIZE_Y));
	      auto subArg1   = arg1.template subRView<SubRView1::VIEW_SIZE_X, SubRView1::VIEW_SIZE_Y>
		(k * (SubRView1::VIEW_SIZE_X), (j * SubRView1::VIEW_SIZE_Y));
#ifdef VIZ_CALLSITE
	      if ( gout ) gout->callsite("_Tail__doit");
#endif 
	      _Tail::doit(subArg0, subArg1, subResult,gout); 
	    }
	}
      }
    }	  

    /* 
    // work around 
    {

      typedef typename trait0::matrix_type::
	template ReadView<
        (trait0::READER_SIZE_X % K) == 0
	  ? trait0::READER_SIZE_X / K 
	  : trait0::READER_SIZE_X - (K-1) * (trait0::READER_SIZE_X / K)
	,(trait0::READER_SIZE_Y % K) == 0 
	  ? trait0::READER_SIZE_Y / K
	  : trait0::READER_SIZE_Y - (K-1) * (trait0::READER_SIZE_Y / K)>
	SubRView0;

      typedef typename trait1::matrix_type::
	template ReadView<
        (trait1::READER_SIZE_X % K) == 0
	  ? trait1::READER_SIZE_X / K 
	  : trait1::READER_SIZE_X - (K-1) * (trait1::READER_SIZE_X / K)
	,(trait1::READER_SIZE_Y % K) == 0 
	  ? trait1::READER_SIZE_Y / K
	  : trait1::READER_SIZE_Y - (K-1) * (trait1::READER_SIZE_Y / K)>
	SubRView1;


      typedef typename trait_result::matrix_type::
	template WriteView<
        (trait_result::WRITER_SIZE_X % K) == 0 
	  ? (trait_result::WRITER_SIZE_X / K) 
	  : (trait_result::WRITER_SIZE_X - (K-1)*trait_result::WRITER_SIZE_Y / K)
	,(trait_result::WRITER_SIZE_Y % K) == 0 
	  ? (trait_result::WRITER_SIZE_Y / K) 
	  : (trait_result::WRITER_SIZE_Y - (K-1)*trait_result::WRITER_SIZE_Y / K)>
	SubWView;

      auto subArg0   = arg0.template subRView<SubRView0::VIEW_SIZE_X, SubRView0::VIEW_SIZE_Y>
	(((K-1) * (trait0::READER_SIZE_X/K)), (K-1) * (trait0::READER_SIZE_Y/K));

      auto subArg1   = arg1.template subRView<SubRView1::VIEW_SIZE_X, SubRView1::VIEW_SIZE_Y>
	(((K-1) * (trait1::READER_SIZE_X/K)), (K-1) * (trait1::READER_SIZE_Y/K));
      auto subResult = result.template subWView<SubWView::VIEW_SIZE_X, SubWView::VIEW_SIZE_Y>
	(((K-1) * (trait_result::WRITER_SIZE_X/K)), (K-1) * (trait_result::WRITER_SIZE_Y/K));
      
      algo_matr_impl<SubWView, SubRView0, SubRView1, F, Pred, K, IsMT>
	::doit(subArg0, subArg1, subResult); 
    }
    */
#ifdef VIZ_CALLSITE
    if ( gout ) gout->leave();
#endif
  }
};
/* MT */
template<class RESULT, class Arg0, class Arg1, 
	 template <typename, typename, typename> class F, /*generalized functor*/
	 template<class, int, int, int> class Pred, int K>
struct algo_matr_impl<RESULT, Arg0, Arg1, F, Pred, K, true, true> 
{
  const static int lookahead = 0;
  // no definition on purpose. compiler will complain if wrongly use it.
  static void doit(const Arg0&, const Arg1&, RESULT&, CallsiteOutput*);
  static void doitMT(const Arg0& arg0, const Arg1& arg1, RESULT& result, 
		     mt::barrier_t barrier, CallsiteOutput * gout = NULL)
  {
    typedef F<RESULT, Arg0, Arg1> Algo;

#ifdef VIZ_CALLSITE
    if (gout ) gout->enter();
#endif

#ifndef __NDEBUG
    typedef typename view_trait2<Arg0>::reader_type reader0;
    typedef typename view_trait2<Arg1>::reader_type reader1;
    cout << "A(" << reader0::VIEW_SIZE_X << ", " << reader0::VIEW_SIZE_Y << ") X B("
	 << reader1::VIEW_SIZE_X << ", " << reader1::VIEW_SIZE_Y << ")\n";
    cout << "A\n";
    for (int i=0; i<reader0::VIEW_SIZE_X; ++i, cout << "\n")
      for (int j=0; j<reader0::VIEW_SIZE_Y; ++j)
	{
	  cout << arg0[i][j] << "  ";
	}
    cout << "B\n";
    for (int i=0; i<reader1::VIEW_SIZE_X; ++i, cout << "\n")
      for (int j=0; j<reader1::VIEW_SIZE_Y; ++j)
	{
	  cout << arg1[i][j] << "  ";
	}
#endif
   

    function<void (const Arg0&, const Arg1&, RESULT&, mt::barrier_t)>  compF 
      = &(Algo::doitMT);

    mt::thread_t task(bind(compF, arg0, arg1, result, barrier));

#ifndef __NDEBUG
    cout << "execution instance: "
	 << "\tthread id: " << task.get_id()
	 << std::endl;
#endif
    
    task.detach();

#ifdef VIZ_CALLSITE
    if (gout) gout->leave();
#endif
  }
};

// non-MT leaf
template<class RESULT, class Arg0, class Arg1, 
	 template <typename, typename, typename> class F, /*generalized functor*/
	 template<class, int, int, int> class Pred, int K>
struct algo_matr_impl<RESULT, Arg0, Arg1, F, Pred, K, false, true> 
{
  const static int lookahead = 0;
  static void doitMT(const Arg0& arg0, const Arg1& arg1, RESULT& result, mt::barrier_t, CallsiteOutput*);
  static void doit(const Arg0& arg0, const Arg1& arg1, RESULT& result, CallsiteOutput * gout = NULL)
  {
    typedef F<RESULT, Arg0, Arg1> Algo;

#ifdef VIZ_CALLSITE
    if ( gout ) gout->enter();
#endif

#ifndef __NDEBUG
    typedef typename view_trait2<Arg0>::reader_type reader0;
    typedef typename view_trait2<Arg1>::reader_type reader1;
    cout << "A(" << reader0::VIEW_SIZE_X << ", " << reader0::VIEW_SIZE_Y << ") X B("
	 << reader1::VIEW_SIZE_X << ", " << reader1::VIEW_SIZE_Y << ")\n";
    cout << "A\n";
    for (int i=0; i<reader0::VIEW_SIZE_X; ++i, cout << "\n")
      for (int j=0; j<reader0::VIEW_SIZE_Y; ++j)
	{
	  cout << arg0[i][j] << "  ";
	}
    cout << "B\n";
    for (int i=0; i<reader1::VIEW_SIZE_X; ++i, cout << "\n")
      for (int j=0; j<reader1::VIEW_SIZE_Y; ++j)
	{
	  cout << arg1[i][j] << "  ";
	}
#endif
    Profiler &profiler = Profiler::getInstance();
    profiler.eventStart(prof_time_kernel);
    profiler.eventStart(prof_cnt_cache1);

    Algo::doit(arg0, arg1, result);

    profiler.eventEnd(prof_cnt_cache1);
    profiler.eventEnd(prof_time_kernel);
    

#ifndef __NDEBUG
    cout << "Result\n";
    for (int i=0; i<RESULT::VIEW_SIZE_X; ++i, cout << "\n")
      for (int j=0; j<RESULT::VIEW_SIZE_Y; ++j)
	{
	  cout << result[i][j] << "  ";
	}
#endif

#ifdef VIZ_CALLSITE
    if (gout) gout->leave();
#endif
  }
};



int main()
{
  typedef Matrix<int, 10, 10> M;
  M A;
  
  A.zero();
  for (int i=0; i<10; ++i)
    A[i][i] = i;


  // test iterator
  Matrix<int, 10, 10>::iterator it
    = A.begin();

  for (int i=0, I=M::DIM_M; i != I; ++i)
    *(it + i*(M::DIM_N) + i) = 1;

  //==============================================//
  //==               ITERATOR TEST              ==//
  //==============================================//
  // const_iterator 
  Matrix<int, 10, 10>::const_iterator cit 
    = A.begin();
  cout << "output the whole matrix:" << std::endl;
  for (int i=0, I=M::DIM_M; i != I; ++i, cout << std::endl) 
    for (int j=0, J=M::DIM_N; j != J; ++j)
      cout << *cit++ << " ";
  
  cout << "output the 1st row:\n";
  // row iterator
  for (auto i = A.row_begin(0); i != A.row_end(0); ++i) {
    cout << *i << " ";
  }
  cout << std::endl;

  // column iterator
  typedef M::col_iterator COLUMNS;
  COLUMNS beg = A.col_begin(0);
  COLUMNS end = A.col_end(0);

  if ( beg == end ) {
    cout << "empty" << std::endl;
  }

  cout << "output the 1st column:\n";
  for (auto i = A.col_begin(0); i != A.col_end(0); ++i) {
    cout << *i << std::endl;
  }

  //==============================================//
  //==               VIEW TEST                  ==//
  //==============================================//
  cout << "read from reader, (1,1) element\n";
  auto reader = A.subRView<2, 2>(0, 0);
  cout << reader[1][1] << std::endl;
  cout << "write to writer";
  auto w = A.subWView<5, 5>(5, 5);
  for (int i=0; i<5; ++i) for (int j=0; j<5; ++j) w[i][j] = 7;

  for (int i=0; i<5; ++i) 
    for (int j=0; j<5; ++j) {
      assert( A[5+i][5+j] = 7 
	      && "ill-behavior writer");
    }
  //the follow stmt will fail at *compile-time*,
  //because the requesting rview is out of range;
  
  //auto reader2 = A.rview<3, 10, 1, 2>();
  
  auto writer = A.subWView<2,2>(0, 0);
  writer[0][0] = writer[0][1] = writer[1][0] = writer[1][1] = 9;
  cout << "read from writer, (1,1) element\n";
  cout << writer[1][1] << std::endl;
  //==============================================//
  //==              ALGORITHMS TEST             ==//
  //==============================================//

  int raw_a[] = {1, 2, 3, 4};
  int raw_b[] = {1, 1, 0, 1};
  
  typedef Matrix<int, 2, 2> N;
  Matrix<int, 2, 2> X(raw_a, raw_a + 4);
  Matrix<int, 2, 2> Y(raw_b, raw_b + 4);
  Matrix<int, 2, 2> Z, Z0;
  Z.zero();

  
  // note that raw-major, so this is legit.
  // however, mul_matrix(A, B, B) is illegit!
  multiply(X, Y, Z0);
  dump_m(Z0, "multiply result:");  
  
  //auto x_r = X.subRView<2, 2>(0, 0);
  //auto y_r = Y.subRView<2, 2>(0, 0);
  auto z_w = Z.subWView();

  
  BOOST_STATIC_ASSERT((is_same<view_trait2<N>::writer_type, 
		       decltype(z_w)>::value));
  BOOST_STATIC_ASSERT((is_same<view_trait2<N>::writer_type, 
		N::WriteView<2,2> >::value ));

  matMul<view_trait2<N>::writer_type, N, N>(X, Y, z_w);
  //xliu::matMul(X, Y, z_w);
  dump_m(Z, "view result:");

  //==============================================//
  //==         RECURSIVE CODE GENERATOR         ==//
  //==============================================//
  
  // a real float multiplication

  //typedef Matrix<float, MM_TEST_SIZE_N, MM_TEST_SIZE_N> TestMatrix;
  typedef int TestType;
  typedef Matrix<TestType, MM_TEST_SIZE_N, MM_TEST_SIZE_N> TestMatrix;
  NumRandGen<TestType> gen(2009);
  typedef TestMatrix::writer_type Writer;
  static TestMatrix x, y, z, STD_result;


  for (int i=0; i<TestMatrix::DIM_M; ++i) 
    for (int j=0; j<TestMatrix::DIM_N; ++j) 
#ifndef __NDEBUG      
      x[i][j] = gen()%10, y[i][j] = gen()%10;
#else
      x[i][j] = gen(), y[i][j] = gen();
#endif

#ifndef __NDEBUG
  dump_m(x, "X");
  dump_m(y, "Y");
#endif
  z.zero();
  auto result = z.subWView();

#ifdef VIZ_CALLSITE
  //viz graph
  CallsiteOutput gout = CallsiteOutput::createFileOutput("test_matrix.dot");
  //CallsiteOutput gout = CallsiteOutput::createFromOStream(&cout);
  gout.initializeGraph("G_mat_mul");
#endif

  Profiler &prof = Profiler::getInstance();
  //globals

  auto temp0 = prof.eventRegister("STD multiply");
  auto temp1 = prof.eventRegister("block");
  prof_time_kernel = prof.eventRegister("Algorithm kernel");
  auto temp2 = prof.eventRegister("multi-threaded");
  auto temp3 = prof.eventRegister("mul via views");

  prof_cnt_cache0  = prof.eventRegister("view llcache misses", LL_CACHE_MISS_COUNTER_EVT);
  prof_cnt_cache1  = prof.eventRegister("block llcache misses", LL_CACHE_MISS_COUNTER_EVT);
  prof_cnt_cache2  = prof.eventRegister("STD llcache misses", LL_CACHE_MISS_COUNTER_EVT);

#ifndef __NDEBUG  
  prof.eventStart(temp0);
  prof.eventStart(prof_cnt_cache2);
  multiply<TestType, MM_TEST_SIZE_N, MM_TEST_SIZE_N, MM_TEST_SIZE_N>
    (x, y, STD_result);
  prof.eventEnd(prof_cnt_cache2);
  prof.eventEnd(temp0);


  for (int i=0; i < 80; ++i ) putchar('=');
  putchar('\n');
  
  // peek A00
  for (int ii=0; ii < TestMatrix::DIM_M/2; ++ii, cout<<"\n") 
    for (int jj=0; jj < TestMatrix::DIM_N/2; ++jj)
      cout << STD_result[ii][jj] << "  ";
#endif

#if 0
  typedef algo_matr_impl<Writer, TestMatrix, TestMatrix, 
    matMAddWraper,  /*functor type*/
#if 1
    p_lt_cache_ll   /*predicator*/
#else
    p_very_simple
#endif
    > Map;
  z.zero();
  prof.eventStart(temp1);
  Map::doit(x, y, result);
  prof.eventEnd(temp1);
#endif

#if 1
  typedef algo_matr_impl<Writer, TestMatrix, TestMatrix, 
    matMAddWraper,  /*functor type*/
    //p_very_simple   /*predicator*/
    p_lt_cache_ll
    ,2, true> MT;
  
  z.zero();
  prof.eventStart(temp2);
  gout.root("MM_root");
  MT::doit(x, y, result, &gout);
  prof.eventEnd(temp2);
#endif
  /*
  prof.eventStart(temp3);
  prof.eventStart(prof_cnt_cache0);
  matMul<view_trait2<TestMatrix>::writer_type, view_trait2<TestMatrix>::reader_type,
    view_trait2<TestMatrix>::reader_type>
    (x, y, result);
  prof.eventEnd(prof_cnt_cache0);
  prof.eventEnd(temp3);
  */

#ifndef __NDEBUG 
  for (int i=0; i<TestMatrix::DIM_M; ++i) 
    for (int j=0; j<TestMatrix::DIM_N; ++j)
      assert( result[i][j] == STD_result[i][j]
	      && "wrong answer");
#endif

#ifdef VIZ_CALLSITE
  gout.finalizeGraph();
#endif

  prof.dump();
}
