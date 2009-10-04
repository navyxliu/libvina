// This file is not supposed to be distributed.
// Sep.29, 2009: Add MKL impl.
//
#ifndef VINA_MATRIX2_HXX
#pragma message "This is an internal header file, included by		\
                 other library headers. You should not attempt		\
		 to use it directly."
#else
// for X86_64 ISA
#include <emmintrin.h> // SEE2 instrisincs
#define vSplat(v, i) ({_m128 a=v; a=_mm_shuffle_ps(a, a, \
	_MM_SHUFFLE(i, i, i, i)); a;})

#ifdef MKL
#include <mkl_cblas.h>
#endif

  //==============================================//
  //~~             ALGORITHMS                   ~~//
  //==============================================//

template<class T, int SIZE_A, int SIZE_B, int SIZE_C>
struct matArithImpl{
  typedef ReadView2<T, SIZE_A, SIZE_B>   RView0;
  typedef ReadView2<T, SIZE_B, SIZE_C>   RView1;
  typedef WriteView2<T, SIZE_A, SIZE_C>  Result;
    
  static void mul(const RView0& X, const RView1& Y, 
		  Result& result)
  {
    for(int i=0; i<SIZE_A; ++i)
      for(int j=0; j<SIZE_C; ++j)
	{
	  T tmp = 0;
	  for(int k=0; k<SIZE_B; ++k)
	    {
	      tmp += X[i][k] * Y[k][j];
	    }
	  result[i][j] = tmp;
	}
  }
  static void madd(const RView0& X, const RView1& Y, 
		   Result& result)
  {
    for(int i=0; i<SIZE_A; ++i)
      for(int j=0; j<SIZE_C; ++j)
	{
	  T tmp = 0;
	  for(int k=0; k<SIZE_B; ++k)
	    {
	      tmp += X[i][k] * Y[k][j];
	    }

	  result[i][j] += tmp;
	}    
  }
};

template<class T, int SIZE_A, int SIZE_B>
struct matArithImpl2{
  typedef ReadView2<T, SIZE_A, SIZE_B>  RView;
  typedef WriteView2<T, SIZE_A, SIZE_B> Result;

  static void add(const RView& X, const RView& Y, 
		  Result& result)
  {
    for(int i=0; i<SIZE_A; ++i)
      for(int j=0; j<SIZE_B; ++j)
	result[i][j] = X[i][j] + Y[i][j];
  }
};

template<int SIZE_A, int SIZE_B, int SIZE_C>
struct matArithImpl<vFloat, SIZE_A, SIZE_B,SIZE_C>{

  typedef ReadView2<vFloat, SIZE_A, SIZE_B>   RView0;
  typedef ReadView2<vFloat, SIZE_B, SIZE_C>   RView1;
  typedef WriteView2<vFloat, SIZE_A, SIZE_C>  Result;
    
  static void mul(const RView0& X, const RView1& Y, 
		  Result& result)
  {
    const float * a = X.data();
    const size_t A_DIM_N = X.dimN();

    const float * b = Y.data();
    const size_t B_DIM_N = Y.dimN();
    
    float * c = result.data();
    const size_t C_DIM_N = result.dimN();
#ifndef MKL
    for(int i=0; i<SIZE_A; i+=4)
      {
	for(int j=0; j<SIZE_B; j+=4)
	  {
	    __m128 c00 = _mm_setzero_ps();
	    __m128 c10 = _mm_setzero_ps();
	    __m128 c20 = _mm_setzero_ps();
	    __m128 c30 = _mm_setzero_ps();

	    for(int k=0; k<SIZE_C; k+=4)
	      {
		__m128 ac0 = {a[k], a[k], a[k], a[k]};
		__m128 ac1 = {a[A_DIM_N+k], a[A_DIM_N+k], a[A_DIM_N+k], a[A_DIM_N+k]};
		__m128 ac2 = {a[2*A_DIM_N+k], a[2*A_DIM_N+k], a[2*A_DIM_N+k], a[2*A_DIM_N+k]};
		__m128 ac3 = {a[3*A_DIM_N+k], a[3*A_DIM_N+k], a[3*A_DIM_N+k], a[3*A_DIM_N+k]};
		
		__m128 b00 = _mm_load_ps( b+(k*B_DIM_N) + j);
		__m128 b10 = _mm_load_ps( b+((k+1)*B_DIM_N) + j);
		__m128 b20 = _mm_load_ps( b+((k+2)*B_DIM_N) + j);
		__m128 b30 = _mm_load_ps( b+((k+3)*B_DIM_N) + j);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b00));
		ac0 = _mm_set_ps1(a[k+1]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b00));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+1]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b00));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+1]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b00));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+1]);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b10));
		ac0 = _mm_set_ps1(a[k+2]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b10));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+2]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b10));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+2]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b10));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+2]);

		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b20));
		ac0 = _mm_set_ps1(a[k+3]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b20));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+3]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b20));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+3]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b20));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+3]);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b30));
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b30));
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b30));
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b30));
	      }
	    _mm_store_ps(c+j, c00);
	    _mm_store_ps(c+C_DIM_N+j,   c10);
	    _mm_store_ps(c+C_DIM_N*2+j, c20);
	    _mm_store_ps(c+C_DIM_N*3+j, c30);
	  }// for j
	c += 4*C_DIM_N;
	a += 4*A_DIM_N;
      }// for i
#else
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, SIZE_A, SIZE_B, SIZE_C,
         1.0f/*alpha*/, a, A_DIM_N, b, B_DIM_N, 0.0f/*beta*/, c, C_DIM_N); 
#endif
  }
  static void madd(const RView0& X, const RView1& Y, 
		   Result& result)
  {
    const float * a = X.data();
    const size_t A_DIM_N = X.dimN();

    const float * b = Y.data();
    const size_t B_DIM_N = Y.dimN();
    
    float * c = result.data();
    const size_t C_DIM_N = result.dimN();
#ifndef MKL
    for(int i=0; i<SIZE_A; i+=4)
      {
	for(int j=0; j<SIZE_B; j+=4)
	  {
	    __m128 c00 = _mm_setzero_ps();
	    __m128 c10 = _mm_setzero_ps();
	    __m128 c20 = _mm_setzero_ps();
	    __m128 c30 = _mm_setzero_ps();

	    for(int k=0; k<SIZE_C; k+=4)
	      {
		__m128 ac0 = {a[k], a[k], a[k], a[k]};
		__m128 ac1 = {a[A_DIM_N+k], a[A_DIM_N+k], a[A_DIM_N+k], a[A_DIM_N+k]};
		__m128 ac2 = {a[2*A_DIM_N+k], a[2*A_DIM_N+k], a[2*A_DIM_N+k], a[2*A_DIM_N+k]};
		__m128 ac3 = {a[3*A_DIM_N+k], a[3*A_DIM_N+k], a[3*A_DIM_N+k], a[3*A_DIM_N+k]};
		
		__m128 b00 = _mm_load_ps( b+(k*B_DIM_N) + j);
		__m128 b10 = _mm_load_ps( b+((k+1)*B_DIM_N) + j);
		__m128 b20 = _mm_load_ps( b+((k+2)*B_DIM_N) + j);
		__m128 b30 = _mm_load_ps( b+((k+3)*B_DIM_N) + j);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b00));
		ac0 = _mm_set_ps1(a[k+1]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b00));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+1]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b00));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+1]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b00));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+1]);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b10));
		ac0 = _mm_set_ps1(a[k+2]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b10));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+2]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b10));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+2]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b10));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+2]);

		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b20));
		ac0 = _mm_set_ps1(a[k+3]);
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b20));
		ac1 = _mm_set_ps1(a[A_DIM_N+k+3]);
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b20));
		ac2 = _mm_set_ps1(a[2*A_DIM_N+k+3]);
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b20));
		ac3 = _mm_set_ps1(a[3*A_DIM_N+k+3]);
		
		c00 = _mm_add_ps(c00, _mm_mul_ps(ac0, b30));
		c10 = _mm_add_ps(c10, _mm_mul_ps(ac1, b30));
		c20 = _mm_add_ps(c20, _mm_mul_ps(ac2, b30));
		c30 = _mm_add_ps(c30, _mm_mul_ps(ac3, b30));
	      }
	    _mm_store_ps(c+j,_mm_add_ps(_mm_load_ps(c+j), c00));
	    _mm_store_ps(c+C_DIM_N+j, _mm_add_ps(_mm_load_ps(c+C_DIM_N+j), c10));
	    _mm_store_ps(c+C_DIM_N*2+j, _mm_add_ps(_mm_load_ps(c+C_DIM_N*2+j), c20));
	    _mm_store_ps(c+C_DIM_N*3+j, _mm_add_ps(_mm_load_ps(c+C_DIM_N*3+j), c30));
	  }// for j
	c += 4*C_DIM_N;
	a += 4*A_DIM_N;
      }// for i    
#else
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, SIZE_A, SIZE_B, SIZE_C,
         1.0f/*alpha*/, a, A_DIM_N, b, B_DIM_N, 1.0f/*beta*/, c, C_DIM_N); 
#endif
  }
};

template<int SIZE_A, int SIZE_B>
struct matArithImpl2<vFloat, SIZE_A, SIZE_B>
{
  typedef ReadView2<vFloat, SIZE_A, SIZE_B>  RView;
  typedef WriteView2<vFloat, SIZE_A, SIZE_B> Result;

  static void add(const RView& X, const RView& Y, 
		  Result& result)
  {
    const float * xx = X.data();
    const float * yy = Y.data();
    float * zz       = result.data();

    const size_t A_DIM_N = X.dimN();
    const size_t B_DIM_N = Y.dimN();      
    const size_t C_DIM_N = result.dimN();

    const float * x, *y;
    float * z;
    __m128 px, py, pz, px1, py1, pz1;

    for(int i=0; i<SIZE_A; i++) {
      x = xx + i * A_DIM_N;
      y = yy + i * B_DIM_N;
      z = zz + i * C_DIM_N;
      
      for(int j=0; j<SIZE_B-(SIZE_B&0x7); j+=8)
	{
	  px = _mm_load_ps(x);
	  py = _mm_load_ps(y);
	  px1 = _mm_load_ps(x+4);
	  py1 = _mm_load_ps(y+4);

	  pz = _mm_add_ps(px, py);
	  pz1 = _mm_add_ps(px1, py1);
	  _mm_store_ps(z, pz);
	  _mm_store_ps(z+4, pz1);
	  x += 8;
	  y += 8;
	  z += 8;
	}
      for(int j=SIZE_B - (SIZE_B&0x7); j<SIZE_B; ++j)
	{
	  *(z++) = *(x++) + *(y++);
	}
    }
  }
};

template <class T, int DIM_A, int DIM_B,
	  class U, int KERL_A, int KERL_B>
struct matConvlImpl{
  typedef FramedReadView2<T, DIM_A, DIM_B>   Arg0;
  typedef ReadView2<U, KERL_A, KERL_B>       Arg1;
  typedef WriteView2<T, DIM_A, DIM_B>        Result;
  
  static void 
  conv2d(const Arg0& in, const Arg1& kernel,
	 Result& result)
  {
    /*
    printf("in=%p kernel=%p\n", &in, &kernel);
    printf("in.data=%p, kernel.data=%p, result=%p\n",
	   in.data(), kernel.data(), result.data());
    printf("DIM_A=%2d, DIM_B=%2d, KEREL_A=%2d KERNEL_B=%2d\n",
	   DIM_A, DIM_B, KERL_A, KERL_B);
    */
    for (int i=0; i<DIM_A; ++i) 
      for (int j=0; j<DIM_B; ++j){
	result[i][j] = 0;
	int dx, dy;
	for (int ki=0; ki<KERL_A; ++ki, dx++) 
	  {
	    dx=i+ki-(KERL_A>>1);
	    for (int kj=0; kj<KERL_B; ++kj, dy++)  {
		dy=j+kj-(KERL_B>>1);
		//if (dx >= 0 && dx < DIM_A && dy >= 0 && dy < DIM_B){

		/*	       
		  printf("result=%2d += kernel[%2d][%2d]=%f * in[%2d][%2d]=%2d\n",
			 (unsigned int)result[i][j], ki, kj, kernel[ki][kj], dx, dy, 
			 (unsigned int)in[dx][dy]);
		*/
		result[i][j] += kernel[ki][kj] * in[dx][dy];
	    }
	  }
      }
  }//func
};

// a standard MM. for debug
template<class T, int dim_A, int dim_B, int dim_C> 
void multiply(const Matrix<T, dim_A, dim_B>& X, 
	      const Matrix<T, dim_B, dim_C>& Y, 
		Matrix<T, dim_A, dim_C>& R)
{
  for ( int i=0; i != dim_A; ++i)
    for (int j=0; j != dim_C; ++j)
      {
	T tmp = 0;
	for (int k=0; k != dim_B; ++k)
	  tmp += X[i][k] * Y[k][j];
	R[i][j] = tmp;
      }
}
// a standard Conv2D for debug

template<class T, int DIM_A, int DIM_B,
	 class U, int KERNL_A, int KERNL_B>
void conv2d(const Matrix<T, DIM_A, DIM_B>& in,
	    const Matrix<U, KERNL_A, KERNL_B>& kernel,
	    Matrix<T, DIM_A, DIM_B>& result)
{
  printf("in=%p kernel=%p\n", &in, &kernel);
  printf("in.data=%p, kernel.data=%p, result=%p\n",
	 in.data(), kernel.data(), result.data());
  printf("DIM_A=%2d, DIM_B=%2d, KEREL_A=%2d KERNEL_B=%2d\n",
	 DIM_A, DIM_B, KERNL_A, KERNL_B);

  for (int i=0; i<DIM_A; ++i) 
    for (int j=0; j<DIM_B; ++j){
      result[i][j] = 0;
      int dx, dy;
      for (int ki=0; ki<KERNL_A; ++ki, dx++) {
	  dx=i+ki-(KERNL_A>>1);
	  for (int kj=0; kj<KERNL_B; ++kj, dy++) {
	      dy=j+kj-(KERNL_B>>1);
	      if (dx >= 0 && dx < DIM_A && dy >= 0 && dy < DIM_B) 
	      {
		/*
		  printf("result=%2d += kernel[%2d][%2d]=%f * in[%2d][%2d]=%2d\n",
			 (unsigned int)result[i][j], ki, kj, kernel[ki][kj], dx, dy, 
			 (unsigned int)in[dx][dy]);
		*/
		  result[i][j] += kernel[ki][kj] * in[dx][dy];
	      }
	  }
      }
  }
}
#endif 
