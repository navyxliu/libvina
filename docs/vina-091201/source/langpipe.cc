  //template full specialization
  template<>
  struct TF_pipeline<>
  {
    //last stage defintions
    //T* is the type of input
    template<class T>
    static void impl(T* in)
    {
      //omit...
    }
    template<class T>
    static void 
    doit(T * in)
    {
      std::tr1::function<void (T*)> 
        func(&(impl<T>));

      mt::thread_t thr(func, in);
   } 
  };

  //customize pipeline TF class
  typedef TF_pipeline<
    translate<Eng2Frn>,
    translate<Frn2Spn>, 
    translate<Spn2Itn>,
    translate<Itn2Chn>
  > MYPIPE;

  MYPIPE::doit(&input);
