// This file is not supposed to be distributed.
//! g++ -o meta meta.cc -I/home/liu/dev/loki-book/

/// A demonstration of code generation using template.
//
#include <iostream>
using namespace std;

// Example 1:
// value generator in compile time. 
// only integer value can be calculated in template
// recursion, including short/int/long, enum and bool
template <int N>
struct Fib{
  const static int value = Fib<N-1>::value + Fib<N-2>::value;
};

template<>
struct Fib<0>
{
  const static int value = 1;
};

template <>
struct Fib<1>
{
  const static int value = 2;
};


// Example 2:
// function generator in compile time

// p_xxx template class is used to simulate predicator
// in lisp, emacs-lisp massively use this name convention
template <int I>
struct p_gt_20 {
  const static bool value = I >= 20;
};

// dump tail, do nothing
struct _Trail{
  static void apply(){}
};

/// 1st template parameter:
// input metadata:
//   if it is integal value, assignment directly.
//   otherwise, give the typename and assign the value late

/// 2nd template parameter:
// predicator: it determine whether recursively generate or stop here.

/// 3rd TP:
// it indicates completion. acts like a sentinel. 
// user of generator should never used it.
template <int L, template<int> class Pred, 
	  bool __SENTINEL__/*never used*/ = Pred<L>::value >
struct Seq
{
  static void apply() {
    Seq<L+1, Pred, Pred<L+1>::value>::apply(); 
    cout << "Fib(" << L << "):" << Fib<L>::value << endl;
  }
};

//tail do 
template<int L, template<int> class Pred>
struct Seq<L, Pred, true>
  : _Trail {};

// Example 3:
// one way to code generate combination of types
// GenScatterHierarchy can unfold a list of types.
// this came from Loki library
#ifdef LOKI_LIBRARY
#include "Typelist.h"
#include "HierarchyGenerators.h"

using namespace Loki;

template <class T>
struct Holder
{
  T value_;
};

class Widget{};

typedef GenScatterHierarchy<
  TYPELIST_3(int, string, Widget),
  Holder>
WidgetInfo;
#endif

int main()
{
  cout << Fib<0>::value << endl
       << Fib<1>::value << endl
       << Fib<2>::value << endl
       << Fib<3>::value << endl;
  
  cout << "Seq<5, p_gt_20:" << endl;
  Seq<5, p_gt_20>::apply();

#ifdef LOKI_LIBRARY
  WidgetInfo obj;
  (static_cast<Holder<string>&>(obj)).value_ = "hello";
  
  cout << Field<1>(obj).value_ << endl;
#endif
}

