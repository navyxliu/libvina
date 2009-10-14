#include "../matrix2.hpp"
#include <assert.h>
#include <stdio.h>



using namespace vina;

int main()
{
  typedef Matrix<int, 10, 10> M;
  M A;
  
  A.zero();
  for(int i=0; i<10; ++i) for(int j=0; j<10; ++j) 
			    assert( A[i][j] == 0 );
  for (int i=0; i<10; ++i)
    A[i][i] = i;

typedef vina::Matrix<float , 
		    1024, 
		    1024>
ImageBuffer;

  ImageBuffer in, STD_out;
  for (int i=0; i<1024; ++i) for (int j=0; j<1024; ++j) 
    STD_out[i][j] = in[i][j];


  // test iterator
  /*
  Matrix<int, 10, 10>::iterator it
    = A.begin();

  for (int i=0, I=M::DIM_M; i != I; ++i)
    *(it + i*(M::DIM_N) + i) = 1;
    */
  //==============================================//
  //==               ITERATOR TEST              ==//
  //==============================================//
  //FIXME: broken
  // const_iterator 
  /*
  Matrix<int, 10, 10>::const_iterator cit 
    = A.begin();
  int c;
  for (int i=0, I=M::DIM_M; i != I; ++i) 
    for (int j=0, J=M::DIM_N; j != J; ++j)
      if ( (c = *(cit++)) != A[i][j] ) {
	printf("wrong iterator: *cit=%2d, A[%2d][%2d] = %2d\n",
	       c, i, j, A[i][j]);
	exit(1);
      }
  */
  // row iterator
  int COL = 0;
  for (auto i = A.row_begin(0); i != A.row_end(0); ++i) {
    assert(*i == A[0][COL++]
	   && "wrong row iterator");
  }

  // column iterator
  typedef M::col_iterator COLUMNS;
  COLUMNS beg = A.col_begin(0);
  COLUMNS end = A.col_end(0);
  int ROW= 0;
  for (auto i = A.col_begin(0); 
       i != A.col_end(0); ++i) {
    assert(*i == A[ROW][0] && "wrong column iterator");
  }

  //==============================================//
  //==               VIEW TEST                  ==//
  //==============================================//
  //write to writer
  auto w = A.subWView<5, 5>(5, 5);
  for (int i=0; i<5; ++i) for (int j=0; j<5; ++j) w[i][j] = 7;

  for (int i=0; i<5; ++i) 
    for (int j=0; j<5; ++j) {
      assert( A[5+i][5+j] = 7 
	      && "ill-behavior writer");
    }
  
  auto writer = A.subWView<2,2>(0, 0);
  writer[0][0] = writer[0][1] = writer[1][0] = writer[1][1] = 9;
  assert( writer[0][0] == 9 && writer[0][1] == 9
	  && writer[1][0] && writer[1][1] == 9);

  //read from reader, (1,1) element
  auto reader = A.subRView<2, 2>(0, 0);
  assert( reader[0][0] == 9 && reader[0][1] == 9
	  && reader[1][0] && reader[1][1] == 9);

  int kernelInt[25] = { 1,  4,  6,  4, 1,
			4, 16, 24, 16, 4,
			6, 24, 36, 24, 6,
			4, 16, 24, 16, 4,
			1,  4,  6,  4, 1 };

  Matrix<int, 5, 5> kernel(kernelInt, kernelInt+25);
  Matrix<int, 5, 5> image;

  for(int i=0; i<5; ++i) for(int j=0; j<5; ++j)
			   assert( kernel[i][j] == kernelInt[i*5 +j]  
				   && "wrong iterator constuctor");
  auto kerl_w = kernel.subWView();
  for(int i=0; i<5; ++i) for(int j=0; j<5; ++j)
			   if ( kerl_w[i][j] != kernelInt[i*5 +j]){
			     printf("kernel_w[%2d][%2d] = %d is wrong, correct value = %d\n",
				    i, j, kerl_w[i][j], kernelInt[i*5+j]);
			     exit(1);
			   }

  auto kerl_r = kernel.subRView();
  for(int i=0; i<5; ++i) for(int j=0; j<5; ++j)
			   assert( kerl_r[i][j] == kernelInt[i*5 +j]  
				   && "reader view failed");
/*
  auto frame = kernel.frame<5, 5>(2, 0, 0, 0);
  for (int i=-2; i<7; ++i) 
    for (int j=-2; j<7; ++j) 
      {
	if ( i >= 0 && i < 5
	     && j>=0 && j<5) {
	  assert( frame[i][j] == kernel[i][j]);
	}
	else {
	  assert( frame[i][j] == 0 );
	}
      }
      */
  //==============================================//
  //==              ALGORITHMS TEST             ==//
  //==============================================//


  printf("test passed.\n");
}
