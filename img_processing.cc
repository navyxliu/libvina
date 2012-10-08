#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "matrix2.hpp"
#include "imgsupport.hpp"
#include "toolkits.hpp"
#include "frame.hpp"

const char *FILE_NAME = "lena.png";
const int  MAX_NAME = 1024;

typedef vina::Matrix<unsigned char, 
	     IMG_TEST_SIZE_M + 5,
	     IMG_TEST_SIZE_N + 5>
Conv2dBuffer;

typedef vina::Matrix<float, 5, 5>
KernelBuffer;

typedef vina::Matrix<unsigned char, 
		     IMG_TEST_SIZE_M, 
		     IMG_TEST_SIZE_N>
ImageBuffer;

char            fileName[MAX_NAME];
int             imageX;
int             imageY;
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
  const static bool value = x<= IMG_TEST_GRANULARITY;
};

int 
main(int argc, char **argv)
{

    // use default image file if not specified
    if(argc == 4)
    {
        strcpy(fileName, argv[1]);
        imageX = atoi(argv[2]);
	assert( imageX == IMG_TEST_SIZE_M
		&& "wrong dimention: imageX" );
        imageY = atoi(argv[3]);
	assert( imageY == IMG_TEST_SIZE_N
		&& "wrong dimention: imageY" );
    }
    else{
        printf("Usage: %s <image-file> <width> <height>\n", argv[0]);
        strcpy(fileName, FILE_NAME);
        imageX = IMG_TEST_SIZE_M;
        imageY = IMG_TEST_SIZE_N;
        printf("\nUse default image \"%s\", (%d,%d)\n", fileName, imageX, imageY);
    }

    vina::PngImage image;
/*
    // open raw image file
    if(image.loadFromFile(fileName) )
    {
      printf("succeeded in loading file: %s\n", 
	     fileName);
      printf("image width=%4u,height=%4u\n", 
	     image.getWidth(), image.getHeight());
      printf("bit_depth=%4d, channel=%4d\n", 
	     image.getBitDepth(), image.getChannel());
    }
*/
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

    //unsigned char * raw = image.getData();
    unsigned char raw_data[256*256];
    FILE *f_raw = fopen("lena.raw", "rb");
    if ( f_raw == NULL ) {
      printf("fopen failed\n");
    }
    if( fread(raw_data, 256, 256, f_raw)
	!= 256 ) {
      printf("fread failed\n");
      exit(1);
    }
    /*
    for (int i=0; i<256 * 256; ++i) 
      {
	if ( *(raw+i) != 
	     *(raw_data+i) ) {
	  printf("raw[%2d]=%02x != raw_data[%2d]=%02x\n",
		 i, (unsigned int)raw[i],
		 i, (unsigned int)raw_data[i]);
	  exit(1);
	}
      }
   */ 
    /*   Conv2dBuffer in(raw, raw + IMG_TEST_SIZE_M * IMG_TEST_SIZE_N,
		    IMG_TEST_SIZE_M, IMG_TEST_SIZE_N, 0);
    */
#ifdef USE_PNG
    ImageBuffer in(raw, raw+IMG_TEST_SIZE_M * IMG_TEST_SIZE_N);
#else
    ImageBuffer in(raw_data, raw_data+IMG_TEST_SIZE_M * IMG_TEST_SIZE_N);
#endif

    KernelBuffer kerl(kernel, kernel+ 25);
    ImageBuffer out, STD_out;
    //dump_m(kerl, "kernel before");

    ///    auto writer = out.subWView();
    vina::conv2d(in, kerl, STD_out);
    //dump_m(kerl, "kernel after");
    
    vina::PngImage furry_lena;
    const char * fname = "furry_lena.png";
    if ( furry_lena.storeToFile(fname, IMG_TEST_SIZE_M, IMG_TEST_SIZE_N, 8, 
				PNG_COLOR_TYPE_GRAY) ) {
      printf("going to write file %s\n", fname);
    }

    spmd_initialize();
    typedef conv2d_map<ImageBuffer, ImageBuffer, KernelBuffer, 
    matConv2dWrapper, p_simple, 2> TF;
    TF::doit(in, kerl, out);

    //sleep(1);
    while (! spmd_all_complete() );

    spmd_cleanup();
    for(int i=0; i<256; ++i) for (int j=0; j<256; ++j) {
	if ( out[i][j] !=  STD_out[i][j] ) {
	  printf("out[%2d][%2d]=%2d is wrong, correct value is %d\n",
		 i, j, out[i][j], STD_out[i][j]);
	  exit(1);
	}      
    }
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
    furry_lena.setData(out.data());
    return 0;
}















