template <class P, typename... Tail>
  struct pipeline<P, Tail...> {
    typedef typename P::input_type in_t;
    typedef typename P::output_type out_t;
   
    static out_t doit(in_t in) {
       pipeline<Tail...>::doit( P::doit(in) );
    }
};  

