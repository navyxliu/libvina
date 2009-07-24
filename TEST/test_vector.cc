#include "../vector.hpp"
#include "../toolkits.hpp"

#include <stdio.h>
using namespace vina;


#define CHECK_ANSWER for(int i=0; i<VEC_TEST_SIZE_N; ++i)		   \
  if (fabs(c[i]-c_v[i]) > 1e-6)				   \
    {							   \
      printf("WA: c_v[%2d]=%f, correct value=%f\n",	   \
	       i, c_v[i], c[i]);			   \
      exit(1);						   \
    }							   


template<class T, int N>
void dump_v(const Vector<T, N>& vect, const std::string& verbose="")
{
  if ( verbose.length() != 0 ) {
    std::cout << verbose << std::endl;
  }
  auto iter = vect.begin();
  for(int i=0; i<N; ++i)
    std::cout << *(iter++) << "  ";
  std::cout << std::endl;
}

int main()
{
  NumRandGen<float> gen;
  typedef Vector<float, VEC_TEST_SIZE_N> vector_t;
  typedef view_trait<vector_t>::writer_type writer_t;

  // using vector instr.
  typedef Vector<vector_type<float>, VEC_TEST_SIZE_N> vector_v_t;
  typedef view_trait<vector_v_t>::writer_type writer_v_t;

  static vector_t a, b, c;
  static vector_v_t a_v, b_v, c_v;

  for (int i=0; i < VEC_TEST_SIZE_N; ++i)
    a[i] = a_v[i] = gen();
  for (int i=0; i < VEC_TEST_SIZE_N; ++i)
    b[i] = b_v[i] = gen();

  auto reader = a.subRView();
  
  float dummy0 = reader[0];
  auto writer = c.subWView();
  auto writer_v = c_v.subWView();
  writer[0] = dummy0;
  dummy0 = writer[1];
  
  typedef vecAddWrapper<writer_t, vector_t, vector_t> add_wrapper;
  typedef vecAddWrapper<writer_v_t, vector_v_t, vector_v_t> add_wrapper_v;
  
  add_wrapper::doit(a, b, writer);
  add_wrapper_v::doit(a_v, b_v, writer_v);
  CHECK_ANSWER

  typedef vecMulWrapper<writer_t, vector_t> mul_wrapper;
  typedef vecMulWrapper<writer_v_t, vector_v_t> mul_wrapper_v;
  mul_wrapper::doit(0.73f, a, writer);
  mul_wrapper_v::doit(0.73f, a_v, writer_v);
  CHECK_ANSWER

  typedef vecMAddWrapper<writer_t, vector_t> madd_wrapper;
  typedef vecMAddWrapper<writer_v_t, vector_v_t> madd_wrapper_v;
  mul_wrapper::doit(0.73f, a, writer);
  mul_wrapper_v::doit(0.73f, a_v, writer_v);
  CHECK_ANSWER
  

}
