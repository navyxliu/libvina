#include <iostream>
#include <stdio.h>
#include "seq-new.hpp"
using namespace vina;

#if 0
// use for testing SEQ
//TODO: move to an isolated file
template <int I>
struct p_gt_20 {
  const static bool value = I >= 20;
};
// ditto
template <int I>
struct ECHO
{
  void operator() () const {
    std::cout << "I = " << I << std::endl;
  }
  static void g() {

    std::cout << "I = " << I << std::endl;
  }
};
#endif

struct echo {
  void
  operator() (int i) 
  { 
    std::cout << "i = " << i << std::endl;
  }
  void
  operator() (int i, int j)
  {
    std::cout << "i = " << i << ", j = " << j << std::endl;
  }
  void
  operator() (int i, int j, int k)
  {
    std::cout << "i = " << i << ", j = " << j << ", k = " << k << std::endl;
  }
};

void 
test_iterator()
{
  std::cout << "iterator (5)\n";
  for (int i=0; i<5; ++i) seq_handler_f<echo>()();

  std::cout << "iterate (5, 2)\n";
  for (int i=0; i<5; ++i) for (int j=0; j<2; ++j) seq_handler_g<echo, 2>()();

  std::cout << "iterate (2, 5)\n";
  for (int i=0; i<2; ++i) for (int j=0; j<5; ++j) seq_handler_g<echo, 5>()();

  std::cout << "iterator(1, 3, 2)\n";
  for (int i=0; i<1; ++i) 
    for (int j=0; j<3; ++j) 
      for (int k=0; k<2; ++k) 
        seq_handler_h<echo, 2, 3>()();

  std::cout << "iterator(4, 2, 3)\n";
  for (int i=0; i<4; ++i) 
    for (int j=0; j<2; ++j)
      for (int k=0; k<3; ++k) 
        seq_handler_h<echo, 3, 2>()();
}
#if 0
void
test_Seq() 
{
  std::cout << "Seq<5, p_gt_20:" << std::endl;
  Seq<5, p_gt_20, ECHO>::apply();

  /*
  std::cout << "Par<5, p_gt_20:" << std::endl;
  Par<5, p_gt_20, ECHO>::apply();
  sleep(1);
  */
}
#endif
void
test_seq()
{
  typedef seq<seq_tail, 5, seq_func_undefined> S5;
  //S5::apply(); //compiler error: no definition of type seq_func_undefined
  
  seq_handler_f<echo>::reset();
  std::cout << "S5\n";
  typedef seq<seq_tail, 5, seq_handler_f<echo>> S5_func;
  S5_func::apply();

  seq_handler_g<echo, 5>::reset(); 
  std::cout << "S5_2\n";
  typedef seq<S5, 2/*the most-outer*/, seq_handler_g<echo, 5/*the most-nested*/>> S5_2;
  S5_2::apply();

  seq_handler_h<echo, 5, 2>::reset();
  std::cout << "S5_2_3\n";
  typedef seq<S5_2, 3, seq_handler_h<echo, 5, 2>> S5_2_3;
  S5_2_3::apply();

  std::cout << "BLOCK: (3, 2, 5)\n";
  typedef seq<seq<seq<seq_tail, 3>, 2>, 5, seq_handler_h<echo, 5, 2>> blk;
  blk::apply(); 
}

/*a case study of seq using bubble sort algorithm*/
int A[] = {2, 3, 1, 19, 0};

struct BubbleFunc
{
  void operator() (int i)
  {
    for ( int j=0; j<sizeof(A)/sizeof(A[0]) - i - 1; ++j)
      if ( A[j] > A[j+1] ) {
	int h = A[j];
	A[j] = A[j+1];
	A[j+1] = h;
      }
  }
};
int 
main()
{
  std::cout << "TEST iterators\n";
  test_iterator();

  std::cout << "TEST seq\n";
  test_seq();

  std::cout << "\n\nBubble sort for 5 elements\n";
  seq<seq_tail, 5, seq_handler_f<BubbleFunc>>::apply();
  
  for(int i=0; i<5; ++i) printf("%2d ", A[i]);
  printf("\n\n");
}
