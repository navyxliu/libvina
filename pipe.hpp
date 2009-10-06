// This file is not supposed to be distribued.
//History:
//Oct. 05, 2009: create file.

namespace vina {
  //primary
  template <typename... Stages>
  struct pipeline;

  template <>
  struct pipeline<> 
  {
    const static bool _IsTail = true;
    static void doit(string arg)
    {
      std::cout << arg << std::endl;
    }
  };

  template <class P, typename... Tail>
  struct pipeline<P, Tail...> {
    typedef typename P::input_type in_t;
    typedef typename P::output_type out_t;
    typedef typename P::computation_type comp_t;
    const static bool _IsTail = false;
    static void doit(const in_t & in)
    {
      pipeline<Tail...>::doit(P::doit(in));
    }
  };
}
