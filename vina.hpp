#ifndef VINA_HPP_
#define VINA_HPP_

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <tr1/type_traits>

//FIXME: cleanup this
typedef unsigned int uint32_t;
namespace vina {

  // dump tail, do nothing
  struct _Trail{
    static void apply(){}
  };

  enum {VINA_TYPE_SEQ = 0
        , VINA_TYPE_PAR
        , VINA_TYPE_NUMBER 
  };

  //Template class decl
  // no implementation in purpose
  struct seq_func_undefined;
  struct par_func_undefined;

  // definitions are in seq.hpp and par.hpp, respectively
  template<class S, uint32_t I, class F> struct seq;
  template<class S, uint32_t I, class F> struct par; 
 
  typedef seq<void, 1, seq_func_undefined> seq_tail;
  typedef par<void, 1, par_func_undefined> par_tail;

  template<int tag, class S, uint32_t I, class F> 
  struct select_surrounding;
}//end of NS
#endif /*VINA_HPP_*/

