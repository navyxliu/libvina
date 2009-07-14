// This file is not supposed to be distributed.
/// HISTORY
// Jun.22, 09' -- rewrite this file, using primitive array
// and decouple view from vector after discussed with ydf
// Jun.02, 09' -- file creation

#ifndef VINA_VECTOR_HXX
#define VINA_VECTOR_HXX 1

#include <cstddef>
#include <cassert>
#include <algorithm>
#include <functional>
#include <iterator>

#include "mtsupport.hpp"
#include "profiler.hpp"
#include "trait.hpp"

namespace vina {

  // forward decl
  template <class, int, int> class Matrix;
  template <class, int>  class ReadView;
  template <class, int>  class WriteView;
  template <class, int>  class ReadViewMT;
  template <class, int>  class WriteViewMT;

  template <class T>
  struct vector_type{
    typedef T type;
  };
  //specialization for vector instr.
  typedef vector_type<float> vFloat;
  typedef vector_type<int>   vInt;

  //==============================================//
  //~~               VECTOR                     ~~//
  //==============================================//
  template <class _T, int _N_>
  class Vector{
    typedef Vector<_T, _N_> _Self;
  public:
    typedef _T                    value_type;
    typedef _Self                 _Vector_type;
    typedef _T&                   reference;
    typedef const _T&             const_reference;
    typedef size_t                size_type;
    typedef _T*                   pointer;
    typedef const _T*             const_pointer;
    typedef _T                    _M_ty;
    //views
    typedef ReadView<_T, _N_>     reader_type;
    typedef WriteView<_T, _N_>    writer_type;
    typedef ReadViewMT<_T, _N_>   reader_mt_type;
    typedef WriteViewMT<_T, _N_>  writer_mt_type;
    typedef _Self                 container_type;
    //iterators
    typedef _T*        iterator;
    typedef const _T*  const_iterator;
    
    enum { DIM_N = _N_, 
	   VIEW_SIZE = _N_,
    };
    
    //ctors
    Vector() {}
    explicit Vector(const _T& val)
    { 
       std::fill(_M_.begin(), _M_.end(), val);      
    }
    Vector(_T const array[_N_]) 
    {
      std::copy(&array[0], array + _N_,
		std::back_inserter(_M_));
    }
    template<class InputIterator>
    Vector(InputIterator beg, 
	   InputIterator end)
    {
      int i = 0;
      while ( beg != end && i < _N_ ) {
	_M_[i++] = *beg++;
      }
    }
    Vector<_T, _N_>&
    operator=(const _Self& rhs)
    {
      iterator iter = this->begin();
      std::copy(rhs.begin(), rhs.end(), iter);
    }

    //accessors
    reference operator[](int dx)
    {
      return _M_[dx];
    }
    const_reference operator[](int dx) const 
    {
      return _M_[dx];
    }

    template <int SZ>
    ReadView<_T, SZ> subRView(unsigned offset = 0) const
    { return ReadView<_T, SZ>(data(), offset);}
    
    reader_type subRView() const 
    { return ReadView<_T, _N_>(data()); }
 
    template <int SZ>
    operator ReadView<_T, SZ> () const
    {
      static_assert( SZ <= _N_, "cast: illegal sub");
      return ReadView<_T, SZ>(data());
    }
    
    template <int SZ>
    WriteView<_T, SZ> subWView(unsigned offset = 0)
    {
      return WriteView<_T, SZ>(data(), offset);
    }
    writer_type subWView()
    {return WriteView<_T, _N_>(data());}
    
    iterator begin() { return _M_; }
    const_iterator begin() const { return _M_; }
    iterator end() { return _M_+_N_; }
    const_iterator end() const { return _M_+_N_; }

    // raw pointer
    pointer data() { return _M_;}
    const_pointer data() const {return _M_; }
    //manipulators
    void zero() { 
      std::fill(begin(), end(), 0);
    }
    //arithmetic operations
    template <class U, int N>
    friend Vector<U, N> operator+(const Vector<U, N>&,
				  const Vector<U, N>&);
    template <class U, int N>
    friend bool operator== (const Vector<U, N>&,
			    const Vector<U, N>&);

   private:
#ifdef __GNUC__
    _T  _M_[_N_] __attribute__((aligned(16)));  // 16-byte boundary
#else                                       
    _T  _M_[_N_];                               //FIXME: use extensions for other compilers
#endif
  };
  
  template <class _T, int _N_>
  class Vector<vector_type<_T>, _N_>
    : public Vector<_T, _N_> {

    typedef vector_type<_T>        _ty;
    typedef Vector<_T, _N_>        _Base;
    typedef Vector<_ty, _N_>       _Self;
  public:
    typedef _ty                    value_type;
    typedef _Self                  container_type;
    typedef ReadView<_ty, _N_>     reader_type;
    typedef WriteView<_ty, _N_>    writer_type;
    typedef ReadViewMT<_ty, _N_>   reader_mt_type;
    typedef WriteViewMT<_ty, _N_>  writer_mt_type;
    typedef _T                     _M_ty;

    template <int SZ>
    ReadView<_ty, SZ> subRView(unsigned offset = 0) const
    { return ReadView<_ty, SZ>(_Base::template data(), offset);}
    
    reader_type subRView() const 
    { return ReadView<_ty, _N_>(_Base::template data()); }
 
    template <int SZ>
    operator ReadView<_ty, SZ> () const
    {
      static_assert( SZ <= _N_, "cast: illegal sub");
      return ReadView<_ty, SZ>(_Base::data());
    }
    
    template <int SZ>
    WriteView<_ty, SZ> subWView(unsigned offset = 0)
    {
      return WriteView<_ty, SZ>(_Base::template data(), offset);
    }
    writer_type subWView()
    {return WriteView<_ty, _N_>(_Base::data());}

  };
  //==============================================//
  //~~               VIEW                       ~~//
  //==============================================//
  template <class T, int _sz_>
  class ReadView {
  public:
    enum {
	   VIEW_SIZE = _sz_,
         };

    /// T might be vector_type, but _M_ty is the type of container.
    // vector_type only affects specialization to support vector instr.
    // such as SSE, altiVec, but it should not change the behaviors of
    // Vector
    typedef T                    value_type;
    typedef Vector<T, _sz_>      _Vector_type;
    typedef ReadView<T, _sz_>    reader_type;
    typedef WriteView<T, _sz_>   writer_type;
    typedef ReadViewMT<T, _sz_>  reader_mt_type;
    typedef WriteViewMT<T, _sz_> writer_mt_type;
    typedef _Vector_type         container_type;
    typedef typename _Vector_type::iterator        iterator;
    typedef typename _Vector_type::const_iterator  const_iterator;
    typedef typename _Vector_type::const_reference const_reference;
    typedef typename _Vector_type::_M_ty           _M_ty;

    ReadView(const _M_ty* vec, unsigned offset = 0) 
      : impl_(vec), offset_(offset)
    {}
    // accessors
    const_reference operator[] (int dx) const {
      assert( dx >= 0 && dx < _sz_ 
	      && "out of range");
      return impl_[offset_ + dx];
    }
    const_iterator begin() const {
      return impl_[offset_];
    }
    const_iterator end() const {
      return impl_[offset_ + _sz_];
    }
    const _M_ty* data() const {
      return &(impl_[offset_]);
    }
    template <int SZ>
    ReadView<T, SZ> subRView(unsigned pos) const
    {
      return ReadView<T, SZ>(impl_, offset_ + pos);
    }

  private:
    size_t         offset_;
    const _M_ty*   impl_;
  };

  template <class T, int _sz_>
  class WriteView {
  public:
    enum {
      VIEW_SIZE = _sz_
    };
    typedef T                    value_type;
    typedef Vector<T, _sz_>      _Vector_type;
    typedef ReadView<T, _sz_>    reader_type;
    typedef WriteView<T, _sz_>   writer_type;
    typedef ReadViewMT<T, _sz_>  reader_mt_type;
    typedef WriteViewMT<T, _sz_> writer_mt_type;
    typedef _Vector_type         container_type;
    typedef typename _Vector_type::iterator       iterator;
    typedef typename _Vector_type::const_iterator const_iterator;
    typedef typename _Vector_type::reference      reference;
    typedef typename _Vector_type::_M_ty          _M_ty;

    WriteView( _M_ty* vec, unsigned offset = 0) 
      : impl_(vec), offset_(offset)
    {}
    // accessors
    reference operator[] (int dx){
      assert( dx >= 0 && dx < _sz_
	      && "out of range");

      return impl_[offset_ + dx];
    }
    iterator begin() {
      return impl_[offset_];
    }
    iterator end() {
      return impl_[offset_ + _sz_];
    }

    template <int SZ>
    WriteView<T, SZ> subWView(unsigned pos = 0)
    {
      assert( pos + SZ <= _sz_ && "illegal sub"); 
      return WriteView<T, SZ>(impl_, offset_ + pos);
    }

    template <int SZ>
    ReadView<T, SZ> subRView(unsigned pos = 0) const
    {
      assert( pos  + SZ <= _sz_ && "illegal sub");
      return ReadView<T, SZ>(impl_, offset_ + pos);
    }
    
    ReadView<T, _sz_> subRView() const 
    {
      return ReadView<T, _sz_>(impl_);
    }
    
    writer_mt_type subWViewMT()
    {
      return WriteViewMT<T, _sz_>(this);
    }
    
    reader_mt_type subRViewMT() const 
    {
      return ReadViewMT<T, _sz_>(this);
    }
    // implicit cast
    template <int SZ>
    operator ReadView<T, SZ>() const
    {
      static_assert(SZ <= _sz_, "illegal sub");
      return ReadView<T, SZ>(impl_, offset_);
    }
    _M_ty* data() {
      return &(impl_[offset_]);
    }
  private:
    size_t offset_;
    _M_ty* impl_;
  };

  //==============================================//
  //~~               MT VIEW                    ~~//
  //==============================================//

  // MT Views act like ports of harbor. It guarantee the safe
  // data-path from one thread to the other thread.
  // This is shared memory system implementation. because there is
  // is no corresponding signal in x86_64 and POSIX env for threads,
  // we simulate it with conditional variable.
  template <class T, int _sz_>
  class WriteViewMT : public mt::signal_t {
  public:
    enum { 
      VIEW_SIZE = _sz_,
    };

    typedef T                 value_type;
    typedef Vector<T, _sz_>   _Vector_type;
    typedef ReadView<T, _sz_>    reader_type;
    typedef WriteView<T, _sz_>   writer_type;
    typedef ReadViewMT<T, _sz_>  reader_mt_type;
    typedef WriteViewMT<T, _sz_> writer_mt_type;
    typedef _Vector_type         container_type;
    typedef typename _Vector_type::_M_ty          _M_ty;
    typedef typename _Vector_type::iterator       iterator;
    typedef typename _Vector_type::const_iterator const_iterator;
    
    WriteViewMT(writer_type* rhs)
    {
      src_ = rhs;
    }
    operator WriteView<T, _sz_>()
    {
      wait();
      return *src_;
    }
    
  private:
    writer_type * src_;
  };

  template <class T, int _sz_>
  class ReadViewMT : public mt::signal_t {
  public:
    enum { 
	   VIEW_SIZE = _sz_,
         };

    typedef T                    value_type;
    typedef Vector<T, _sz_>      _Vector_type;
    typedef ReadView<T, _sz_>    reader_type;
    typedef WriteView<T, _sz_>   writer_type;
    typedef ReadViewMT<T, _sz_>  reader_mt_type;
    typedef WriteViewMT<T, _sz_> writer_mt_type;
    typedef _Vector_type         container_type;
    typedef typename _Vector_type::_M_ty          _M_ty;
    typedef typename _Vector_type::iterator       iterator;
    typedef typename _Vector_type::const_iterator const_iterator;

    ReadViewMT(const writer_type*  rhs)
    {
      src_ = rhs;
    }
    operator ReadView<T, _sz_>()
    {
      wait();
      return src_ -> subRView();
    }
  private:
    const writer_type * src_;
  };

  //==============================================//
  //~~               OPERATIONS                 ~~//
  //==============================================//
  
  template <class U, int N>
  Vector<U, N> operator+(const Vector<U, N>& lhs,
			 const Vector<U, N>& rhs)
  {
    Vector<U, N> result;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), 
		   result.begin(), std::plus<U>());
    return result;
  }
  
  template <class U, int N>
  bool operator==(const Vector<U, N>& lhs, 
		  const Vector<U, N>& rhs)
  {
    auto res = std::mismatch(lhs.begin(), lhs.end(), 
			     rhs.begin());
    return res.first == lhs.end();
  }
  
  template <class U, int N>
  bool operator!=(const Vector<U, N>& lhs, 
		  const Vector<U, N>& rhs)
  {
    return !operator==(lhs, rhs);
  } 
  //==============================================//
  //~~          ALGORITHM INTERFACES            ~~//
  //==============================================//
#include "vectoralgo.hpp"

  template<class Result, class Arg0, class Arg1>
  struct vecAddWrapper {
    typedef typename view_trait<Result>::value_type  T;
    const static int DIM_N = view_trait<Result>::WRITER_SIZE;

    static void doit(const Arg0& arg0, const Arg1& arg1, 
		     Result& result)
    {
      vecArithImpl<T, DIM_N>::add(arg0, arg1, result);
    }
    static void doitMT(const Arg0& arg0, const Arg1& arg1, 
		       Result& result, mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };
  template<class Result, class Arg0, class Arg1>
  struct vecSubWrapper {
    typedef typename view_trait<Result>::value_type  T;
    const static int DIM_N = view_trait<Result>::WRITER_SIZE;

    static void doit(const Arg0& arg0, const Arg1& arg1, 
		     Result& result)
    {
      vecArithImpl<T, DIM_N>::sub(arg0, arg1, result);
    }
    static void doitMT(const Arg0& arg0, const Arg1& arg1, 
		       Result& result, mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };

  template<class Result, class Arg0>
  struct vecMulWrapper {
    typedef typename view_trait<Result>::value_type  T;
    typedef typename view_trait<Result>::_M_ty       _M_ty;
    const static int DIM_N = view_trait<Result>::WRITER_SIZE;

    static void doit(const _M_ty& alpha, const Arg0& arg0, 
		     Result& result)
    {
      vecArithImpl<T, DIM_N>::mul(alpha, arg0, result);
    }
    static void doitMT(const _M_ty& alpha, const Arg0& arg0,
		       Result& result, mt::barrier_t barrier)
    {
      doit(alpha, arg0, result);
      barrier->wait();
    }
  };
  template<class Result, class Arg0>
  struct vecMAddWrapper {
    typedef typename view_trait<Result>::value_type  T;
    typedef typename view_trait<Result>::_M_ty       _M_ty;
    const static int DIM_N = view_trait<Result>::WRITER_SIZE;

    static void 
    doit(const _M_ty& alpha, const Arg0& arg0, 
		     Result& result)
    {
      vecArithImpl<T, DIM_N>::madd(alpha, arg0, result);
    }
    static void 
    doitMT(const _M_ty& alpha, const Arg0& arg0,
	   Result& result, mt::barrier_t barrier)
    {
      doit(alpha, arg0, result);
      barrier->wait();
    }
#ifndef __NDEBUG
    static void 
    doitMT_t(const _M_ty& alpha, const Arg0& arg0, 
	     Result& result, mt::barrier_t barrier, 
	     event_id t)
    {
      Profiler::getInstance().eventStart(t);
      doit(alpha, arg0, result);
      Profiler::getInstance().eventEnd(t);
      barrier->wait();
    }
#endif

  };
  template <class T, class Arg0, class Arg1>
  struct vecDotProdWrapper {
    const static int DIM_N = view_trait<Arg0>::READER_SIZE;

    static void 
    doit(const Arg0& arg0, const Arg1& arg1, 
	 T& result)
    {
      vecArithImpl<T, DIM_N>::dotprod(arg0, arg1, result);
    }

    static void
    doitMT(const Arg0& arg0, const Arg1& arg1, T& result, 
	   mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };
} // end of NS

#endif /* VINA_VECTOR_HXX */
