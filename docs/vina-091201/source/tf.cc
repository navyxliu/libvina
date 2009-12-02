//prime template, used when "s"==false
template <class TASK,
  template<class, class> class PRED,
  bool s = PRED<typename TASK::ARG0,
                typename TASK::ARG1>::value>
struct TF_hierarchy{
   inline static void doit(a, b, result) {
      TASK::inner(a, b, result); 
   }
};

//partial specialization template, used when "s"==true
template <class TASK,
   template<class, class> class PRED>
struct TF_hierarchy<TASK, PRED, true> {
   inline static void doit(a, b, result) {
      TASK::leaf(a, b, result); 
   }
};
