// This file is not supposed to be distributed.
// Jun. 15, 09' -

/** A demonstration of  pipeline processing
I intend to implement a multi-language scenario to demonstrate
the power of pineline processing. This is critical because
Sequoia lacks of this parallel pattern.

First of all, i defined a natural language type system. a Person
has two language properties : mother language and a foreign language.
Ideally, given a social circle, they will try to figure out unfamiliar
language by transitive help. I use this behavior to simulate information
flow in real system. a person is modeled as an agent who has (and also
limited) methods to deal with a variety information. Strings of languages
represent information flows.

using static type, doit-driven solution is easy to implement. However,
functor objects are much more powerful than classes only consist of static
functions. using variadic template, i intentatively make it. 
**/

#include <unistd.h>  // for sleep
#include <string>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <exception>
#include <tr1/tuple>
#include <tr1/type_traits>
#include <tr1/functional>
#include <boost/ref.hpp>
#include "matrix2.hpp"   // i don't know why it is neccessary,
			 // but gcc complain harshly without it.
			 // i can not find ill dependence in my
			 // library.

#include "vector.hpp"
#include "trait.hpp"

//#include "TypeList.h"//loki
using namespace vina;
using namespace std;


#define NUM_OF_LANG 8
#define MAX_LENGTH_OF_LANG 20
// one word represents a type of language.
std::string hellos[NUM_OF_LANG] = {"hello",            //             
				   "hola",             //spanish      
				   "bonjour",          //french       
				   "ciao",             //italian      
				   "nihao",            //mandarin
				   "konnichiha",       //japanese
				   "hallo",            //duntch
				   "salam",            //arabic
};
std::string names[NUM_OF_LANG] = {"henry",             //an englishman who knows spanish
				  "juan", 
				  "fangfang",
				  "mario"
				  "xliu",
				  "ichiro",
				  "max",
				  "mohanmode"
};
typedef enum {
  English,
  Spanish,
  French,
  Italian
}lang_t;

template <lang_t lang>
struct Lang{
  const static int value = lang;
};

Lang<English> english;
Lang<Spanish> spanish;
Lang<French>  french;
Lang<Italian> italian;

typedef Lang<English> lang_eng;
typedef Lang<Spanish> lang_spn;
typedef Lang<French>  lang_frn;
typedef Lang<Italian> lang_itn;

template <class Native, class Foreign>
struct Person
{

  template< lang_t L>
  static bool understand(Lang<L>) {
    return isUnderstand<L>::value;
  }
  static string myNameIs()
  {
    return names[ Native::value ];
  }
  static string sayHello()
  {
    return hellos[ Native::value ];
  }

  static string sayForeign()
  {
    return hellos[ Foreign::value ];
  }

  template <lang_t L>
  struct isUnderstand {
    const static bool value = tr1::is_same<Lang<L>, Native>::value
      || tr1::is_same<Lang<L>, Foreign>::value;
  };
};

typedef Person<lang_eng, lang_spn> Eng2Spn;       // an englishman who can speak spanish as foreign lang
typedef Person<lang_spn, lang_frn> Spn2Frn;
typedef Person<lang_frn, lang_itn> Frn2Itn;
typedef Person<lang_itn, lang_eng> Itn2Eng;

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
#include "toolkits.hpp"

template <class P, class Reader, class Writer>
void translate_impl(P p, 
		    Reader in, 
		    Writer out, int& length)
{
  typedef typename view_trait<Reader>::reader_type ReaderUnlock;
  typedef typename view_trait<Writer>::writer_type WriterUnlock;

  ReaderUnlock in_(in);
  WriterUnlock out_(out);

  string s = p.sayHello();
  string f = p.sayForeign();
  
  for(int i=0; i<s.length(); ++i)
    {
      if ( s[i] != in_[i] ) 
	{
	  length = -1;
	  return;
	}
    }
  if ( f.length() > Writer::VIEW_SIZE ) 
    {
      throw bad_exception();
    }
  for(int i=0; i<f.length(); ++i)
    out_[i] = f[i];
  
  length = f.length();
}
/*
template <class RESULT, typename... Args>
class FunctionAdapter
{
  typedef tr1::function<RESULT (Args...)>        function_type;
  typedef tr1::function<void (RESULT&, Args...)> mt_function_type;
  			      
  void thread_func(RESULT& result, Args... args){
    result = func_(args...);
  }
  function_type func_;

public:
  FunctionAdapter(function_type func): func_(func) {}
  mt_function_type get()
  {
    mt_function_type f = &FunctionAdapter::thread_func;
    return tr1::bind(f, this);
  }

};
*/
template <class P, bool isMT=false>
struct translate{
  typedef Vector<char, MAX_LENGTH_OF_LANG>  STRING;
  typedef view_trait<STRING>::reader_type   READER;
  typedef view_trait<STRING>::writer_type   WRITER;

  static string doit(string str)
  {
    STRING in, out;
    string res;
    int length;

    copy(str.begin(), str.end(), in.begin());
    auto writer = out.subWView();
    translate_impl<P, READER, WRITER>(P(), in, writer, length);

    if (length < 0 )
      throw bad_exception();
    
    return string(out.begin(), length);
  }
};    
// MT specialization
template <class P>
struct translate<P, true>
{

  typedef Vector<char, MAX_LENGTH_OF_LANG>  STRING;
  typedef view_trait<STRING>::reader_type   READER;
  typedef view_trait<STRING>::writer_type   WRITER;
  
  static string doit(string str)
  {
    
    typedef view_trait<STRING>::reader_mt_type   READERMT;
    typedef view_trait<STRING>::writer_mt_type   WRITERMT;

    STRING in, out;
    string res;
    int length;
    
    copy(str.begin(), str.end(), in.begin());

    auto reader = in.subWView().subRViewMT();
    auto writer = out.subWView().subWViewMT();

    static_assert( tr1::is_same<decltype(reader), READERMT>::value, 
		   "reader is not thread-safe");
    static_assert( tr1::is_same<decltype(writer), WRITERMT>::value, 
		   "writer is not thread-safe");
    // open gate

    function<void (P, READERMT, WRITERMT, int&)> func 
      = &translate_impl<P, READERMT, WRITERMT>;
    
    //translate_impl<P, READER, WRITER>(P(), reader, writer, length);
    auto Comp = tr1::bind(func, P(), reader, writer, boost::ref(length));
    
    mt::thread_t task(Comp);
    cout.width(8);
    cout << P::myNameIs() << " is pondering...\n";
    sleep(1);
    reader.set();
    writer.set();
    
    task.join();

    return string(out.begin(), length);
  }

};
//==============================================//
//~~   PIPELINE Transform template            ~~//
//==============================================//
// Primary-template
template <typename... People>
struct lang_pipeline;

template <>
struct lang_pipeline<>
{
  static void doit(string str)
  {
    
    cout << "gotcha: " << str << "\n";
  }
};

template<class P, typename... Last>
struct lang_pipeline<P, Last...>{
  static void doit(string str)
  {
    auto translated = translate<P, true>::doit(str);
    lang_pipeline<Last...>::doit(translated); 

  }
};
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

int 
main()
{
  static_assert(Eng2Spn::isUnderstand<English>::value
		,"eng2spn know english");
  static_assert(Eng2Spn::isUnderstand<Spanish>::value
		,"eng2spn know spanish");
  static_assert(false == Eng2Spn::isUnderstand<French>::value
		,"eng2spn does not know french");

  cout << Eng2Spn::sayHello() << "\n";
  cout << Frn2Itn::sayHello() << "\n";
  cout << "#end.\n";

  Eng2Spn henry;
  Spn2Frn juan;
  Frn2Itn fangfang;
  Itn2Eng mario;

  auto salon = tr1::make_tuple(henry, juan, fangfang, mario);
    
  SayHelloAll<Eng2Spn, Spn2Frn, Frn2Itn, Itn2Eng>::doit();
  
  cout << "^begin helloTo\n";
  helloTo("hello");
  helloTo("hello", henry, juan);
  helloTo("hello", henry, juan, fangfang);
  helloTo("hello", henry, juan, fangfang, mario);
  cout << "^~~~~begin test_first\n";
  test_first(henry, juan, fangfang);
  test_first(juan, fangfang);
  test_first(fangfang);
  test_first();

  cout << "^~~~~begin test traversal\n";
  test_traversal(juan, fangfang);

  cout << "^~~~~lang_pipeline2\n";
  lang_pipeline2<Eng2Spn>::doit("hello");
  lang_pipeline2<Eng2Spn, Spn2Frn>::doit("hello");
  lang_pipeline2<Eng2Spn, Spn2Frn, Frn2Itn>::doit("hello");

  //this is multi-threaded version.
  cout << "^~~~~lang_pipeline\n";
  lang_pipeline<Eng2Spn>::doit("hello");
  lang_pipeline<Eng2Spn, Spn2Frn>::doit("hello");
  lang_pipeline<Eng2Spn, Spn2Frn, Frn2Itn>::doit("hello");

}



