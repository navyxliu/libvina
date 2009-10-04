#include "vector.hpp"
#include "trait.hpp"
#include "toolkits.hpp"
#include "profiler.hpp"

#include <stdio.h>
using namespace vina;

#ifdef __NDEBUG
#define CHECK_ANSWER
#endif

#ifndef CHECK_ANSWER   
#define CHECK_ANSWER for(int i=0; i<VEC_TEST_SIZE_N; ++i)		   \
  if (fabs(c[i]-c_v[i]) > 1e-6)				   \
    {							   \
      printf("WA: c_v[%2d]=%f, correct value=%f\n",	   \
	       i, c_v[i], c[i]);			   \
      exit(1);						   \
    }							   
#endif

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
  NumRandGen<VEC_TEST_TYPE> gen;
  typedef Vector<VEC_TEST_TYPE, VEC_TEST_SIZE_N> vector_t;
  typedef view_trait<vector_t>::writer_type writer_t;

  // using vector instr.
  typedef Vector<vector_type<VEC_TEST_TYPE>, VEC_TEST_SIZE_N> vector_v_t;
  typedef view_trait<vector_v_t>::writer_type writer_v_t;

  static vector_t a, b, c;
  static vector_v_t a_v, b_v, c_v;

  for (int i=0; i < VEC_TEST_SIZE_N; ++i)
    a[i] = a_v[i] = gen();
  for (int i=0; i < VEC_TEST_SIZE_N; ++i)
    b[i] = b_v[i] = gen();

  auto reader = a.subRView();
  
  VEC_TEST_TYPE dummy0 = reader[0];
  auto writer = c.subWView();
  auto writer_v = c_v.subWView();
  writer[0] = dummy0;
  dummy0 = writer[1];

  printf("a address %p\nb address %p\n", a.data(), b.data());
  
  Profiler& prof = Profiler::getInstance();
  auto scltimer = prof.eventRegister("scalar");
  auto vectimer = prof.eventRegister("SSE");

  typedef vecAddWrapper<writer_t, vector_t, vector_t> add_wrapper;
  typedef vecAddWrapper<writer_v_t, vector_v_t, vector_v_t> add_wrapper_v;
  
  
#ifndef __NDEBUG
  //dump_v(a, "a");
  //dump_v(b, "b");
#endif
  prof.eventStart(scltimer);
  add_wrapper::doit(a, b, writer);
  prof.eventEnd(scltimer);

  prof.eventStart(vectimer);
  add_wrapper_v::doit(a_v, b_v, writer_v);
  prof.eventEnd(vectimer);

  CHECK_ANSWER

  prof.dump();
  
  prof.getEvent(scltimer)->reset();
  prof.getEvent(vectimer)->reset();
  
  typedef vecMulWrapper<writer_t, vector_t> mul_wrapper;
  typedef vecMulWrapper<writer_v_t, vector_v_t> mul_wrapper_v;
  
  prof.eventStart(scltimer);
  mul_wrapper::doit(0.73f, a, writer);
  prof.eventEnd(scltimer);

  prof.eventStart(vectimer);
  mul_wrapper_v::doit(0.73f, a_v, writer_v);
  prof.eventEnd(vectimer);

  CHECK_ANSWER
    

  prof.dump();
  

  prof.getEvent(scltimer)->reset();
  prof.getEvent(vectimer)->reset();
  
  typedef vecMAddWrapper<writer_t, vector_t> madd_wrapper;
  typedef vecMAddWrapper<writer_v_t, vector_v_t> madd_wrapper_v;
  
  prof.eventStart(scltimer);
  mul_wrapper::doit(0.73f, a, writer);
  prof.eventEnd(scltimer);

  prof.eventStart(vectimer);
  mul_wrapper_v::doit(0.73f, a_v, writer_v);
  prof.eventEnd(vectimer);

  CHECK_ANSWER
  
  prof.dump();
}
