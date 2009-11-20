#include <iostream>
#include <stdio.h>

#include "seq.hpp"
int g_counter = 0;
using namespace vina;
template<class F>
struct f{
  void
  operator() () {
    //std::cout << "i = " << cnt_ << std::endl;
    F()(cnt_);
    ++ cnt_;
  }
  static  int cnt_;
};

/*this function objection is used in two-level iteration, which is 
 *for (i=0; i<I; ++i) for(j=0; j<J; ++j) ..
 *invariance: i * J  + j == cnt;
 **/
template <int J>
struct h{
  static int cnt_;

  void 
  operator() () {
    int j = cnt_ % J;
    int i = (cnt_ - j) / J ;
    std::cout << "i = " << i << ", j = " << j << std::endl;
    ++ cnt_; // increament iteration counter; 
             // the most-nested iterator is the fastest one
  }
};

/*3-level iteration
  invariance: i * (K*J) + j * K + k == cnt
 */
template<int K, int J>
struct g{
  static int cnt_;

  void 
  operator() () {
    int k = cnt_ % K;
    int j = (cnt_ % (K * J) - k) / K;
    int i = (cnt_ - j * K - k) / (K*J);

    std::cout << "i = " << i << ", j = " << j << ",k = " << k << std::endl;
    ++ cnt_; // increament iteration counter; 
             // the most-nested iterator is the fastest one
  }

};
template <class F>
int f<F>::cnt_ = 0;

template <int J>
int h<J>::cnt_ = 0;
template <int K, int J>
int g<K, J>::cnt_ = 0;

template <int I>
struct p_gt_20 {
  const static bool value = I >= 20;
};

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

void 
test_iterator()
{
  h<2> func; 
  std::cout << "iterate (5, 2)\n";
  for (int i=0; i<5; ++i) for (int j=0; j<2; ++j) func();

  h<5> func2;
  std::cout << "iterate (2, 5)\n";
  for (int i=0; i<2; ++i) for (int j=0; j<5; ++j) func2();

  g<2, 3> func3;
  std::cout << "iterator(1, 3, 2)\n";
  for (int i=0; i<1; ++i) for (int j=0; j<3; ++j) for (int k=0; k<2; ++k) func3();
  
  g<3, 2> func4;
  std::cout << "iterator(4, 2, 3)\n";
  for (int i=0; i<4; ++i) for (int j=0; j<2; ++j) for (int k=0; k<3; ++k) func3();
  
}
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
void
test_seq()
{
  typedef seq<seq_init, seq_func_undefined, 5> S5;
  //std::cout << "S5\n";
  //S5::apply(); //compiler error: no definition of type seq_func_undefined
  /*
  typedef seq<seq_init, f, 5> S5_func;
  S5_func::apply();
  
  std::cout << "S5_2\n";
  typedef seq<S5, f, 2> S5_2;
  
  S5_2::apply();
  
  typedef seq<S5_2, f, 3> S5_2_3;
  S5_2_3::apply();
  */
  
  std::cout << "iterates using seq S5_2_func\n";
  typedef seq<S5, h<2>, 2> S5_2_func;
  S5_2_func::apply();
  
  //more nest
  std::cout << "iterates using seq S5_2_3_func\n";
  typedef seq<S5_2_func, g<3, 2>, 3> S5_2_3_func;
  S5_2_3_func::apply();
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
  //test_seq();
  
  seq<seq_init, seq_handler_f<BubbleFunc>, 5>::apply();
  
  for(int i=0; i<5; ++i) printf("%2d ", A[i]);
  printf("\n\n");
  
}
