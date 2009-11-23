// This file is not supposed to be distribued.
//History:
//Oct. 05, 2009: create file.
#include <tr1/type_traits>
#include <tr1/functional>

namespace vina {
  //primary
  template <typename... Stages>
  struct pipeline;
  
  /*
  template <>
  struct pipeline<> 
  {
    const static bool _IsTail = true;
    static void doit(std::string arg)
    {
      std::cout << arg << std::endl;
    }
  };
  */

  template <class P, typename... Tail>
  struct pipeline<P, Tail...> {
    typedef typename P::input_type in_t;
    typedef typename P::output_type out_t;
    
    static const bool _IsTail = false;

    static out_t doit(in_t in)
    {
      /* 
      static_assert(std::tr1::is_same<typename pipeline<Tail...>::in_t, out_t>::value, 
		    "output type is not same as input type in next stage");
      */
      pipeline<Tail...>::doit(  P::doit(in)  );
    }
  };  
}
