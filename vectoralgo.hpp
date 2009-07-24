#ifndef VINA_VECTOR_HXX
#pragma message "This is an internal header file, included by		\
                 other library headers. You should not attempt		\
		 to use it directly."
#else
// for X86_64 ISA
#include <emmintrin.h> // SEE2 instrisincs
#ifdef __SSE_4_1__
#include <smmintrin.h> // SSE4
#endif
  //==============================================//
  //~~             ALGORITHMS                   ~~//
  //==============================================//
template <class T, int DIM_N>
struct vecArithImpl{
  typedef ReadView<T, DIM_N>  RView;
  typedef WriteView<T, DIM_N> Result;
  
  static void add(const RView& X, const RView& Y, 
		  Result& result)
  {
    for(int i=0; i<DIM_N; ++i) 
      result[i] = X[i] + Y[i];
  }
  static void sub(const RView& X, const RView& Y, 
		  Result& result)
  {
    for(int i=0; i<DIM_N; ++i)
      result[i] = X[i] - Y[i];
  }
  static void mul(const T& alpha, const RView& X, 
		  Result& result)
  {
    for(int i=0; i<DIM_N; ++i) 
      result[i] = alpha * X[i];
  }
  static void madd(const T& alpha, const RView& X, 
		   Result& result)
  {
    for(int i=0; i<DIM_N; ++i)
      result[i] += alpha * X[i];
  }
  static void dotprod(const RView& X, const RView& Y, 
		      T& result)
  {
    for (int i=0; i<DIM_N; ++i)
      result += X[i] * Y[i];
  }
};

template <int DIM_N>
struct vecArithImpl<vFloat, DIM_N>
{
  typedef ReadView<vFloat, DIM_N>   RView;
  typedef WriteView<vFloat, DIM_N>  Result;

  static void add(const RView& X, const RView& Y, 
		  Result& result)
  {
    const float * x = X.data();
    const float * y = Y.data();
    float * z = result.data();
    
    __m128 px, py, pz, px1, py1, pz1;
    for(int i=0; i<DIM_N-(DIM_N&0x7); i+=8)
      {
	px  = _mm_load_ps(x);
	py  = _mm_load_ps(y);
	pz  = _mm_add_ps(px, py);
	px1 = _mm_load_ps(x+4);
	py1 = _mm_load_ps(y+4);
	_mm_store_ps(z, pz);
	pz1 = _mm_add_ps(px1, py1);
	_mm_store_ps(z+4, pz1);
	
	x += 8;
	y += 8;
	z += 8;
      }
    for(int i=DIM_N-(DIM_N&0x7); i<DIM_N; ++i)
      {
	result[i] = X[i] + Y[i];
      }
  }

  static void sub(const RView& X, const RView& Y, 
		  Result& result)
  {
    const float * x = X.data();
    const float * y = Y.data();
    float * z = result.data();
    
    __m128 px, py, pz, px1, py1, pz1;
    for(int i=0; i<DIM_N-(DIM_N&0x7); i+=8)
      {
	px  = _mm_load_ps(x);
	py  = _mm_load_ps(y);
	pz  = _mm_sub_ps(px, py);
	px1 = _mm_load_ps(x+4);
	py1 = _mm_load_ps(y+4);
	_mm_store_ps(z, pz);
	pz1 = _mm_sub_ps(px1, py1);
	_mm_store_ps(z+4, pz1);
	
	x += 8;
	y += 8;
	z += 8;
      }
    for(int i=DIM_N-(DIM_N&0x7); i<DIM_N; ++i)
      {
	result[i] = X[i] - Y[i];
      }
  }
  // Y = alpha * X
  static void mul(const float& alpha, const RView& X, 
		  Result& result)
  {
    const float * x = X.data();
    float * y       = result.data();
    float __const_alpha[4] = {alpha, alpha, alpha, alpha};

    __m128 a, px, px1, px2, py, py1, py2;
    a = _mm_load_ps(__const_alpha);

    for(int i=0; i<DIM_N - (DIM_N%12); i+=12)
      {
	px  = _mm_load_ps(x);
	px1 = _mm_load_ps(x+4);
	px2 = _mm_load_ps(x+8);
	py  = _mm_mul_ps(a, px);
	py1 = _mm_mul_ps(a, px1);
	py2 = _mm_mul_ps(a, px2);
	_mm_store_ps(y, py);
	_mm_store_ps(y+4, py1);
	_mm_store_ps(y+8, py2);
	x += 12;
	y += 12;
      }
    for(int i=DIM_N - DIM_N%12; i<DIM_N; ++i)
      {
	result[i] = alpha * X[i];	
      }
  }
  // Y = Y + alpha * X
  // saddly, there is no madd instr. for Fp in SSE
  static void madd(const float& alpha, const RView& X, 
		   Result& result)
  {
    const float * x = X.data();
    float * y       = result.data();
    float __const_alpha[4] = {alpha, alpha, alpha, alpha};

    __m128 a, px, px1, py, py1;
    a = _mm_load_ps(&__const_alpha[0]);

    for(int i=0; i<DIM_N - (DIM_N&0x7); i+=8)
      {
	px  = _mm_load_ps(x);
	px1 = _mm_load_ps(x+4);
	py  = _mm_load_ps(y);
	py1 = _mm_load_ps(y+4);
	py  = _mm_add_ps(py,  _mm_mul_ps(a, px));
	py1 = _mm_add_ps(py1, _mm_mul_ps(a, px1));

	_mm_store_ps(y, py);
	_mm_store_ps(y+4, py1);
	x += 8;
	y += 8;
      }
    for(int i=DIM_N - (DIM_N&0x7); i<DIM_N; ++i)
      {
	result[i] += alpha * X[i];	
      }
  }

  static void dotprod(const RView& X, const RView& Y, 
		      float& result)
    ;
};

template <int DIM_N>
struct vecArithImpl<vInt, DIM_N>
{
  typedef ReadView<vInt, DIM_N>   RView;
  typedef WriteView<vInt, DIM_N>  Result;
  static void add(const RView& X, const RView& Y, 
		  Result& result)
  {
    const int * x = X.data();
    const int * y = Y.data();
    int * z = result.data();
    
    __m128i px, py, pz, px1, py1, pz1;
    for(int i=0; i<DIM_N-(DIM_N&0x7); i+=8)
      {
	px  = _mm_load_si128((const __m128i *)x);
	py  = _mm_load_si128((const __m128i *)y);
	pz  = _mm_add_epi32(px, py);
	px1 = _mm_load_si128((const __m128i *)(x+4));
	py1 = _mm_load_si128((const __m128i *)(y+4));
	_mm_store_si128((__m128i *)z, pz);
	pz1 = _mm_add_epi32(px1, py1);
	_mm_store_si128((__m128i *)(z+4), pz1);
	
	x += 8;
	y += 8;
	z += 8;
      }
    for(int i=DIM_N-(DIM_N&0x7); i<DIM_N; ++i)
      {
	result[i] = X[i] + Y[i];
      }
  }

  static void sub(const RView& X, const RView& Y, 
		  Result& result)
  {
    const int * x = X.data();
    const int * y = Y.data();
    int * z = result.data();
    
    __m128i px, py, pz, px1, py1, pz1;
    for(int i=0; i<DIM_N-(DIM_N&0x7); i+=8)
      {
	px  = _mm_load_si128((const __m128i *)x);
	py  = _mm_load_si128((const __m128i *)y);
	pz  = _mm_sub_epi32(px, py);
	px1 = _mm_load_si128((const __m128i *)(x+4));
	py1 = _mm_load_si128((const __m128i *)(y+4));
	_mm_store_si128((__m128i *)z, pz);
	pz1 = _mm_sub_epi32(px1, py1);
	_mm_store_si128((__m128i *)(z+4), pz1);
	
	x += 8;
	y += 8;
	z += 8;
      }
    for(int i=DIM_N-(DIM_N&0x7); i<DIM_N; ++i)
      {
	result[i] = X[i] - Y[i];
      }
  }
  // Y = alpha * X
  static void mul(const int& alpha, const RView& X, 
		  Result& result)
#ifdef __SSE_4_1__
  {
    const int * x = X.data();
    int * y       = result.data();
    __m128i alpha_p = _mm_set_epi32(alpha, alpha, alpha, alpha);
    __m128i px, px1, px2, py, py1, py2;

    for(int i=0; i<DIM_N - (DIM_N%12); i+=12)
      {
	px  = _mm_load_si128((const __m128i *)x);
	px1 = _mm_load_si128((const __m128i *)(x+4));
	px2 = _mm_load_si128((const __m128i *)(x+8));
	py  = _mm_mullo_epi32(alpha_p, px);
	py1 = _mm_mullo_epi32(alpha_p, px1);
	py2 = _mm_mullo_epi32(alpha_p, px2);
	_mm_store_si128((__m128i *)y, py);
	_mm_store_si128((__m128i *)(y+4), py1);
	_mm_store_si128((__m128i *)(y+8), py2);
	x += 12;
	y += 12;
      }
    for(int i=DIM_N - DIM_N%12; i<DIM_N; ++i)
      {
	result[i] = alpha * X[i];	
      }
  }
#else 
  ;
#endif
  // Y = Y + alpha * X
  // saddly, there is no madd instr. for Fp in SSE
  static void madd(const float& alpha, const RView& X, 
		   Result& result)
#ifdef __SSE_4_1__    
  {
    const int * x = X.data();
    int * y       = result.data();
    __m128i px, px1, px2, py, py1, py2;
    __m128i alpha_p = _mm_set_epi32(alpha, alpha, alpha, alpha);

    for(int i=0; i<DIM_N - (DIM_N%12); i+=12)
      {
	px  = _mm_load_si128((const __m128i *)x);
	px1 = _mm_load_si128((const __m128i *)(x+4));
	px2 = _mm_load_si128(x+8)
	py  = _mm_load_si128(y);
	py1 = _mm_load_si128(y+4);
	py2 = _mm_load_si128(y+8);

	py  = _mm_add_epi32(py,  _mm_mullo_epi32(alpha_a, px));
	py1 = _mm_add_epi32(py1, _mm_mullo_epi32(alpha_a, px1));
	py2 = _mm_add_epi32(py2, _mm_mullo_epi32(alpha_a, px2));

	_mm_store_si128(y, py);
	_mm_store_si128(y+4, py1);
	_mm_store_si128(y+8, py2);

	x += 12;
	y += 12;
      }
    for(int i=DIM_N - (DIM_N%12); i<DIM_N; ++i)
      {
	result[i] += alpha * X[i];	
      }
  }
#else 
  ;
#endif
  static void dotprod(const RView& X, const RView& Y, 
		      float& result)
    ;
  
};

// standard dot-production, for debug
template <class T, int SIZE>
T dotproduct(const Vector<T, SIZE>& x, 
	     const Vector<T, SIZE>& y)
{
  T res = 0;
  for (int i=0; i<SIZE; ++i)
    res += x[i] * y[i];
  return res;
}
// saxpy for debug
template <class T, int SIZE>
void saxpy(const T& alpha, const Vector<T, SIZE>& x,
	Vector<T, SIZE>& y)
{
  for (int i=0; i<SIZE; ++i)
    y[i] += alpha * x[i];
}
#endif
