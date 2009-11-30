#ifndef TF_HPP_
#define TF_HPP_
template <class TASK,
          template<class _arg0, class _arg1> class PRED,
          bool __SENTINEL__ = PRED<typename TASK::ARG0, typename TASK::ARG1>::value>
struct TF_hierarchy
{
  typedef TASK::ARG0 _arg0;
  typedef TASK::ARG1 _arg1;
  typedef TASK::RESULT _result;
  
  inline static void
  doit(_arg0 arg0, _arg1 arg1, 
       _result res)
  {
    TASK::inner(arg0, arg1, res);
  }
};

template <class TASK,
          template<class _arg0, class _arg1> class PRED>
struct TF_hierarchy<FUNC,PRED, true>
{
  typedef TASK::ARG0 _arg0;
  typedef TASK::ARG1 _arg1;
  typedef TASK::RESULT _result;

  inline static void
  doit(_arg0 arg0, _arg1 arg1, 
        _result res) 
  {
    TASK::leaf(arg0, arg1, res);
  }
};

#endif/*TF_HPP_*/
