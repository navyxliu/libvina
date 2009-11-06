// This file is not supposed to be distributed.
// Jun. 15, 09' - create file
// Oct. 05, 09' - add new pattern pipe, see pipe.hpp 
// Oct. 15, 09' - modify examples, refactor the code.

#include <unistd.h>  // for sleep
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <exception>
#include <tr1/tuple>


#include <boost/ref.hpp>

#include "lang.hpp"
#include "toolkits.hpp"
#include "vector.hpp"
#include "trait.hpp"
#include "pipe.hpp"

#ifndef ITER
#define ITER 5
#endif
#define LANG_BURN_TIME 500000
using namespace std;
using namespace vina;

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
  //runtime interface
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

  typedef std::string input_type;
  typedef std::string output_type;
  typedef Person<Native, Foreign> computation_type;

  static output_type doit(const input_type & in)
  {
    assert ( sayHello() == in && "wrong input");
    return sayForeign();
  }

  //static interface
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

typedef vina::Vector<char, MAX_LENGTH_OF_LANG>  STRING;

//This is implementation for translation procedure. 
//Reader/Writer could be blocking or non-blocing.
//
template <class P, class Reader, class Writer>
void translate_impl(P p, 
		    Reader * in, 
		    Writer * out, 
		    int& length)
{

  string s = p.sayHello();
  string f = p.sayForeign();

  //This is blocking operation.
  //maybe from ReadViewMT -> ReadView
  typename view_trait<Reader>::reader_type 
    in_ = *in;
  typedef typename view_trait<Writer>::writer_type
    writer_t;

  //error detection
  for(int i=0; i<s.length(); ++i)
  {
      if ( s[i] != in_[i] ) 
	{
	  length = -1;
	  throw bad_exception();
	}
  }

  if ( f.length() > Writer::VIEW_SIZE ) 
    {
      length = -1;
      throw bad_exception();
    }

  for(int i=0; i<f.length(); ++i)
    ((writer_t)(*out))[i] = f[i];

  // pretend it is a "tough" task needs to  comsume 1 second.
  //sleep(1);
  burn_usecs(LANG_BURN_TIME);
  out->set();
  
  length = f.length();
}


template <class P, bool isMT=false>
struct translate{
  typedef view_trait<STRING>::reader_type   READER;
  typedef view_trait<STRING>::writer_type   WRITER;

  //interface for vina::pipe
  typedef READER* input_type;
  typedef READER* output_type;

  static READER* doit(READER* in)
  {
    int length;
    STRING * out_buf = new STRING();
    WRITER * out = new WRITER(out_buf->subWView());
    
    translate_impl<P, READER, WRITER>(P(), in, out, length);
    return new READER(*out);
  }
};

// MT specialization
template <class P>
struct translate<P, true>
{
  typedef view_trait<STRING>::reader_mt_type   READERMT;
  typedef view_trait<STRING>::writer_mt_type   WRITERMT;
  typedef view_trait<STRING>::writer_type      WRITER;

  //interface for vina::pipe
  typedef READERMT* input_type;
  typedef READERMT* output_type;
  
  //don't read data from in, we can NOT assume that data is avaiable then we call this
  //function, it's just a creation of pipeline stage.
  static READERMT* 
  doit(READERMT * in)
  {
    int length;

    STRING * out_buf = new STRING();
    WRITER writer = out_buf->subWView();
    WRITERMT * out = new WRITERMT(new WRITER(writer));
    
    static_assert( tr1::is_same<decltype(out), WRITERMT *>::value, 
		   "writer is not thread-safe");
    // open gate
    function<void (P, READERMT*, WRITERMT*, int&)> func 
      = &translate_impl<P, READERMT, WRITERMT>;
    
    auto Comp = tr1::bind(func, P(), in, out, length);
    
    mt::thread_t task(Comp);

    return new READERMT(out);
  }
};

struct timeval tv, tv2;

namespace vina{
template<>
struct pipeline<>
{
  static const bool _IsTail = true;
  typedef view_trait<STRING>::reader_type READER;
  
  template <class U>
  static void output(U* in)
  {
    static int cnt;
    int i;
    READER reader = *(in);
    char hello[READER::VIEW_SIZE+1];

    for(i=0; i<READER::VIEW_SIZE; ++i)
      hello[i] = reader[i];
    hello[i] = '\0';

    cout << "final result: " << hello << endl;
    gettimeofday(&tv2, NULL);
    printf("iter #%2d Elapsed %d usec\n", cnt++,
	   (tv2.tv_sec - tv.tv_sec) * 1000000 + (tv2.tv_usec - tv.tv_usec));

  }
  
  template <class T>
  static void doit(T * in)
  {
    tr1::function<void (T*)> func(&(output<T>));
    mt::thread_t thr(func, in);
  }
};
}

//==============================================//
//~~   PIPELINE Transform template            ~~//
//==============================================//

int 
main()
{
  static_assert(Eng2Spn::isUnderstand<English>::value
		,"eng2spn know english");
  static_assert(Eng2Spn::isUnderstand<Spanish>::value
		,"eng2spn know spanish");
  static_assert(false == Eng2Spn::isUnderstand<French>::value
		,"eng2spn does not know french");

  assert( initialize_ck_burning() && "ck-burning failed");
  cout << "^~~~~lang_pipeline\n";

  typedef pipeline<translate<Eng2Spn>, 
    translate<Spn2Frn>,
    translate<Frn2Itn>
    > MYPIPE;
  char RawStr[] = "hello";
  STRING hello (RawStr, RawStr + 5);
  auto input = hello.subRView();

  view_trait<STRING>::reader_mt_type 
  input2(&input);

  typedef pipeline<translate<Eng2Spn, true>,
    translate<Spn2Frn, true>, 
    translate<Frn2Itn, true>,
    translate<Itn2Eng, true>
    > MYPIPE2;
  
  gettimeofday(&tv, NULL);
  for (int i=0; i<ITER; ++i) {
    MYPIPE2::doit(&input2);
    //sleep(1);
  }

  
  sleep(3);
  /*
  gettimeofday(&tv, NULL);
  for (int i=0; i<ITER; ++i) {
    MYPIPE::doit(&input);
    sleep(1);
  }
  sleep(3);
  */
}



