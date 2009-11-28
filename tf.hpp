#ifndef TF_HPP_
#define TF_HPP_
template <template<class,class, class, 
            template<class, class> class, int> class FUNC,
	  typename ARG0, typename ARG1, typename RESULT,
          template<class _arg0, class _arg1> class PRED,
          int K, 
          bool __SENTINEL__ = PRED<ARG0, ARG1>::value>
struct TF_hierarchy
{
  inline static void
  doit(const ARG0& arg0, const ARG1& arg1, 
       RESULT& res)
  {
    FUNC<ARG0, ARG1, RESULT, PRED, K>::doit(arg0, arg1, res);
  }
};
template <template<class,class, class, 
            template<class, class> class, int> class FUNC,
	  typename ARG0, typename ARG1, typename RESULT,
          template<class _arg0, class _arg1> class PRED,
          int K>
struct TF_hierarchy<FUNC, ARG0, ARG1, RESULT, PRED, K, true>
{
   inline static void
   doit(const ARG0& arg0, const ARG1& arg1, 
        RESULT& res) 
  {
    FUNC<ARG0, ARG1, RESULT, PRED, K>::comp(arg0, arg1, res);
  }
};

#endif/*TF_HPP_*/
