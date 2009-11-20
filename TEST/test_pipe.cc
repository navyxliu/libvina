#include "../lang.hpp"

template <typename... People>
struct SayHelloAll;

template <>
struct SayHelloAll<>
{
  static void doit()
  {}
};

template <class P, typename... Last>
struct SayHelloAll<P, Last...>{
  static void doit()
  {
    cout << P::sayHello() << "\n";
    SayHelloAll<Last...>::doit();
  }
};

void helloTo(string lang)
{
  cout << lang << "\nThe End$\n";
}

//someone says `salute' to p, p translates it to his 
//and then passes it to first person in last.
template <class CAR, typename... CDR> 
void helloTo(string salute,  CAR p, CDR... args)
{ 
  helloTo(p.sayHello(), args...);
}


void test_first()
{
  cout << "end of test_first$\n";
}
template <typename... List>
void test_first(List... args)
{
  auto list = tr1::make_tuple(args...);
  cout << get<0>(list).sayHello() << "\n";
}

void test_traversal()
{
  cout << "end of test_traversal$\n";
}

template <typename T, typename... List>
void test_traversal(T head, List... args)
{
  cout << head.sayHello() << "\n";
  test_traversal(args...);
}

//==============================================//
//~~   PIPELINE Transform template            ~~//
//==============================================//
// another implementation. 
struct unused{};
template <class P0, class P1=unused, class P2=unused,
	  class P3=unused, class P4=unused, class P5=unused>
struct lang_pipeline2{
  static void doit(string str)
  {
    
    if ( tr1::is_same<P1, unused>::value ) {
      cout << P0::sayForeign() << "\n";
    }
    else {
      string translated = translate<P0>::doit(str);
      lang_pipeline2<P1, P2, P3, P4, P5>::doit(translated);
    }
    
  }
};
template<>
struct lang_pipeline2<unused, unused, unused, unused, unused, unused>
{
  static void doit(string);
};
