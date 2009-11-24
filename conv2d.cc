// This file is not supposed to be distribued. 
// History
// Jul. 14, 09': create file, finally debug it right. 
// Oct. 13, 09': modified for libSPMD. split for two files: this file(conv2d) 
// aims to test conv2d algorithm; img_processing.cc is designed to meaningful 
// image processing application.
//
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "matrix2.hpp"
#include "imgsupport.hpp"
#include "toolkits.hpp"
#include "frame.hpp"
#define ITER 1
#ifdef __NDEBUG
#define CHECK_RESULT(X) 
#endif

#ifndef CHECK_RESULT 
#define CHECK_RESULT(_R_) for (int i=0; i<IMG_TEST_SIZE_M; ++i)		\
    for (int j=0; j<IMG_TEST_SIZE_N; ++j)				\
      {									\
	if( fabs(_R_[i][j] - STD_out[i][j]) > 1e-3) {			\
	  printf("z[%2d][%2d]= %.4f WA, correct is %.4f\n",		\
		 i,j, _R_[i][j], STD_out[i][j]);			\
	  exit(1);							\
	}								\
      }									
#endif

typedef vina::Matrix<float, 
	     IMG_TEST_SIZE_M + 5,
	     IMG_TEST_SIZE_N + 5>
Conv2dBuffer;

typedef vina::Matrix<float, 5, 5>
KernelBuffer;

typedef vina::Matrix<float , 
		     IMG_TEST_SIZE_M, 
		     IMG_TEST_SIZE_N>
ImageBuffer;

using namespace vina;

template <class RESULT, class IN, class KERL,
	  template <typename, typename, typename> class Func,
	  template <typename, int, int >          class Pred,
	  int K=2, bool IsMT = true>
struct conv2d_map 
{
  
  typedef mappar2<conv2d_map, K, IsMT, 
		  Pred<typename vina::view_trait2<RESULT>::value_type,
		       vina::view_trait2<RESULT>::WRITER_SIZE_X,
		       vina::view_trait2<RESULT>::WRITER_SIZE_Y>::value
		  >Map;
  
  typedef view_trait2<IN>   trait0;
  typedef view_trait2<KERL>   trait1;
  typedef view_trait2<RESULT> trait_result;
  typedef typename trait_result::value_type T;
  
  typedef FramedReadView2<typename trait0::value_type, 
			  trait0::READER_SIZE_X,
			  trait0::READER_SIZE_Y>      Arg0;
  typedef typename trait1::reader_type                Arg1;
  typedef typename trait_result::writer_type          Result;

  // divide methold
  typedef FramedReadView2<T, trait0::READER_SIZE_X/K,
			  trait0::READER_SIZE_X/K> 
  SubRView0;

  typedef typename trait1::reader_type
  SubRView1;

  typedef WriteView2<T, trait_result::WRITER_SIZE_X/K,
		     trait_result::WRITER_SIZE_Y/K>
  SubWView;
  
  const static bool _pred = Pred<T, trait_result::WRITER_SIZE_X,
				 trait_result::WRITER_SIZE_Y>::value;

  typedef conv2d_map<SubWView, SubRView0, SubRView1,Func, Pred, K, IsMT>
  SubTask;

  typedef std::tr1::function<void (const Arg0&, const Arg1&, Result&)>
  _Comp;
  static _Comp
  computation() {
    return &(Func<Result, Arg0, Arg1>::doit);
  }

  typedef std::tr1::function<void (void *, void *, void *)>
  _Comp_void;

  static _Comp_void * 
  computation_ptr() {
    return new _Comp_void(&(Func<Result, Arg0, Arg1>::doit_ptr));
  }

  static void
  doit(const IN& arg0, const KERL& arg1,
       RESULT& result)
  {
    printf("_pred = %d, SubTask::_pred=%d\n", 
	   _pred, SubTask::_pred);
    auto writer = result.subWView();
    Map::doit(arg0.frame(0), arg1, writer);
  }
};

template <class T, int x, int y>
struct p_simple {
  const static bool value = y<= IMG_TEST_GRANULARITY;
};

static ImageBuffer out;
static ImageBuffer STD_out;
 
static ImageBuffer in;

int 
main(int argc, char **argv)
{
  printf("2-dimentional Convolution test program\n\
  IMG_TEST_SIZE_M=%4d\n\
  IMG_TEST_SIZE_N=%4d\n\
  IMG_TEST_GRANULARITY=%4d\n\
  IMG_TEST_K=%4d\n"
#ifdef __USE_LIBSPMD
"thread: LIBSPMD\n"
#else
"thread: pthread\n"
#endif
  "see Makefile TEST_INFO to set parameters\n", 
	 IMG_TEST_SIZE_M, IMG_TEST_SIZE_N, IMG_TEST_GRANULARITY, IMG_TEST_K);

  Profiler &prof = Profiler::getInstance();
  auto temp0 = prof.eventRegister("STD conv2d");
  auto temp1 = prof.eventRegister("MT");
 
    // define 5x5 Gaussian kernel
  float kernel[25] = { 1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f,
                       4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
                       6/256.0f, 24/256.0f, 36/256.0f, 24/256.0f, 6/256.0f,
                       4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
                       1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f };

    // Separable kernel
  float kernelX[5] = { 1/16.0f,  4/16.0f,  6/16.0f,  4/16.0f, 1/16.0f };
  float kernelY[5] = { 1/16.0f,  4/16.0f,  6/16.0f,  4/16.0f, 1/16.0f };

    // integer kernel
  float kernelFactor = 1 / 256.0f;
  int kernelInt[25] = { 1,  4,  6,  4, 1,
                        4, 16, 24, 16, 4,
                        6, 24, 36, 24, 6,
                        4, 16, 24, 16, 4,
                        1,  4,  6,  4, 1 };

  NumRandGen<float> gen(2009);

  float * raw_data = (float*) malloc ( sizeof(float) * IMG_TEST_SIZE_M * IMG_TEST_SIZE_N ); 
  if ( raw_data == NULL ) {
    printf("malloc failed"); 
    exit(1);
  } 
  for (int i=0; i<IMG_TEST_SIZE_M * IMG_TEST_SIZE_N; ++i) 
    *(raw_data + i) = gen();

  float Comp = 2. * IMG_TEST_SIZE_M * IMG_TEST_SIZE_N * 5 * 5;
  KernelBuffer kerl(kernel, kernel+ 25);
  
  for (int i=0; i<IMG_TEST_SIZE_M; ++i) 
    for (int j=0; j<IMG_TEST_SIZE_N; ++j) {
      in[i][j] = *(raw_data + i * IMG_TEST_SIZE_N + j);
    }
  
  //dump_m(kerl, "kernel before");

/*
  for (int i=0; i<IMG_TEST_SIZE_M; ++i) for (int j=0; j<IMG_TEST_SIZE_N; ++j) 
    STD_out[i][j] = in[i][j];
*/
#ifndef __NDEBUG
  printf("start\n");
  ///    auto writer = out.subWView();
  prof.eventStart(temp0);
  for (int i=0; i<ITER; ++i) 
  vina::conv2d(in, kerl, STD_out);
  prof.eventEnd(temp0);

  printf("STD gflop=%f\n", Gflops(Comp, prof.getEvent(temp0)->elapsed()/ITER));
#endif
  //dump_m(kerl, "kernel after");
  //
  _spmd_initialize(IMG_TEST_K);

  typedef conv2d_map<ImageBuffer, ImageBuffer, KernelBuffer, 
    matConv2dWrapper, p_simple, 2> TF;
  prof.eventStart(temp1);

  for ( int i=0; i<ITER; ++i) 
    TF::doit(in, kerl, out);
  while (! spmd_all_complete() );
  prof.eventEnd(temp1); 

  printf("MT gflop=%f\n", Gflops(Comp, 
    prof.getEvent(temp1)->elapsed()/ITER));
  spmd_cleanup();

  CHECK_RESULT(out);
 /* 
  auto frame = in.frame(0);
 
  auto sub = frame.subRView<128, 128>(0, 128);
    
  for(int j=0; j<128; ++j) 
    if ( sub[-2][j] != in[126][j]) {
	printf("sub[-2][%2d]=%d, correct value is %d",
	       j, (unsigned int)sub[-2][j], (unsigned int)in[126][j]);
	exit(1);
      }
  */ 
  prof.dump();
  return 0;
}


