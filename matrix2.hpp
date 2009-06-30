//This file is not supposed to be distributed.
/// HISTORY:
// Jun.23, 09' -- rewrite this file, decouple views from matrix
// Jun.02, 09' -- creation
#ifndef VINA_MATRIX2_HXX
#define VINA_MATRIX2_HXX 1

#include "vector.hpp"
#include "trait.hpp"
#include <tr1/array>
#include "mtsupport.hpp"

namespace vina {
  // fwd decl.
  template <class, int, int> class ReadView2;
  template <class, int, int> class WriteView2;
  template <class, int, int> class ReadView2MT;
  template <class, int, int> class WriteView2MT;

  
  template <class _T, int _M_dim, int _N_dim>
  class Matrix{
    typedef Vector<_T, _N_dim> _Basic;
    typedef Matrix<_T, _M_dim, _N_dim> _Self;
  public:
    typedef _T                value_type;
    typedef _T&               reference;
    typedef const _T&         const_reference;
    typedef _T*               pointer;
    typedef const _T*         const_pointer;
    
    typedef typename _Basic::iterator         iterator;
    typedef typename _Basic::const_iterator   const_iterator;

    typedef typename _Basic::iterator         row_iterator;
    typedef typename _Basic::const_iterator   const_row_iterator;
    struct                                    col_iterator;
    typedef const col_iterator                const_col_iterator;

    typedef _Self                             container_type;
    typedef ReadView2<_T,  _M_dim, _N_dim>    reader_type;
    typedef WriteView2<_T, _M_dim, _N_dim>    writer_type;
    typedef _Basic                            lower;

    enum {
      DIM_M = _M_dim,
      DIM_N = _N_dim,
    };
  
    // ctors
    Matrix() {}

    template<class ForwardIter>
    Matrix(ForwardIter beg, ForwardIter end)
    {
      ForwardIter I = beg;

      for (int i=0; i<_M_dim; ++i)
	for (int j=0; j<_N_dim && I != end; ++j) 
	  _M_[i][j] = *(I++);
    }
    
    typename view_trait<_Basic>::writer_type
    inline operator[] (int dx) { return (_M_[dx]).subWView();}

    typename view_trait<_Basic>::reader_type 
    inline operator[] (int dx) const { return _M_[dx];}


    // iterators
    
    iterator begin() { return _M_[0].begin(); }
    const_iterator begin() const { return _M_[0].begin(); }

    iterator end() { return _M_[_M_dim-1].end(); }

    const_iterator end() const { 
      return const_cast
	<_Self *>(this)->end(); 
    }

    row_iterator row_begin(int dx) {
      return _M_[dx].begin();
    }
    const_row_iterator row_begin(int dx) const {
      return _M_[dx].begin();
    }

    row_iterator row_end(int dx) {
      return _M_[dx].end();
    }
    const_row_iterator row_end(int dx) const {
      return _M_[dx].end();
    }
    
    col_iterator col_begin(int dy)
    {
      return col_iterator(0, dy, this);
    }
    const_col_iterator col_begin(int dy) const
    {
      return col_iterator(0, dy, this);
    }
    col_iterator col_end(int dy)
    {
      return col_iterator(_M_dim, dy, this);
    }
    const_col_iterator col_end(int dy) const {
      return col_iterator(_M_dim, dy, this);
    }

    // manipulators
    void zero() {
      for (int i=0; i != _M_dim; ++i) 
	for (int j=0; j != _N_dim; ++j) 
	  _M_[i][j] = 0;
    }
    pointer data() {
      return _M_[0].data();
    }
    const_pointer data() const {
      return _M_[0].data();
    }

    //views
    template <int SZ_X, int SZ_Y> ReadView2<_T, SZ_X, SZ_Y>
    subRView(int pos_x, int pos_y) const {
      assert( pos_x + SZ_X <= _M_dim && pos_y + SZ_Y <= _N_dim
	      && "out of range");

      return ReadView2<_T, SZ_X, SZ_Y>(data(), _N_dim, pos_x, pos_y);
    }
    
    template <int SZ_X, int SZ_Y> WriteView2<_T, SZ_X, SZ_Y>
    subWView(int pos_x, int pos_y) {
      assert( pos_x + SZ_X <= _M_dim && pos_y + SZ_Y <= _N_dim
	      && "out of range");

      return WriteView2<_T, SZ_X, SZ_Y>(data(), _N_dim, pos_x, pos_y);
    }

    reader_type subRView() const 
    {
      return ReadView2<_T, _M_dim, _N_dim>(data(), _N_dim, 0, 0);
    }
    writer_type subWView() 
    {
      return WriteView2<_T, _M_dim, _N_dim>(data(), _N_dim, 0, 0);
    }

    //auto cast
    template <int SZ_X, int SZ_Y>
    operator ReadView2<_T, SZ_X, SZ_Y>() const 
    {
      return ReadView2<_T, SZ_X, SZ_Y>(data(), _N_dim, 0, 0);
    }
  private:
#if __GNUC__
    _Basic _M_ [_M_dim] __attribute__((aligned(16)));
#else
    _Basic _M_ [_M_dim];
#endif
  };

  template <class T, int _M_dim, int _N_dim>
  class Matrix<vector_type<T>, _M_dim, _N_dim> 
    : public Matrix<T, _M_dim, _N_dim> {
    typedef vector_type<T>                  _ty;
    typedef Vector<T, _N_dim>               _Basic;
    typedef Matrix<T, _M_dim, _N_dim>       _Base;
  public:
    typedef _ty                                    value_type;    
    typedef ReadView2<_ty,  _M_dim, _N_dim>        reader_type;
    typedef WriteView2<_ty, _M_dim, _N_dim>        writer_type;
    typedef Vector<_ty, _N_dim>                    lower;
    typedef _Basic                                 container_type;

    template <int SZ_X, int SZ_Y> ReadView2<_ty, SZ_X, SZ_Y>
    subRView(int pos_x, int pos_y) const {
      assert( pos_x + SZ_X <= _M_dim && pos_y + SZ_Y <= _N_dim
	      && "out of range");

      return ReadView2<_ty, SZ_X, SZ_Y>(_Base::data(), _N_dim, pos_x, pos_y);
    }
    
    template <int SZ_X, int SZ_Y> WriteView2<_ty, SZ_X, SZ_Y>
    subWView(int pos_x, int pos_y) {
      assert( pos_x + SZ_X <= _M_dim && pos_y + SZ_Y <= _N_dim
	      && "out of range");

      return WriteView2<_ty, SZ_X, SZ_Y>(_Base::data(), _N_dim, pos_x, pos_y);
    }

    reader_type subRView() const 
    {
      return ReadView2<_ty, _M_dim, _N_dim>(_Base::data(), _N_dim, 0, 0);
    }
    writer_type subWView() 
    {
      return WriteView2<_ty, _M_dim, _N_dim>(_Base::data(), _N_dim, 0, 0);
    }

    //auto cast
    template <int SZ_X, int SZ_Y>
    operator ReadView2<_ty, SZ_X, SZ_Y>() const
    {
      return ReadView2<_ty, SZ_X, SZ_Y>(_Base::data(), _N_dim, 0, 0);
    }
  };

  //==============================================//
  //~~               ITERATORS                  ~~//
  //==============================================//
  template<class _T, int _M_dim, int _N_dim>
  struct Matrix<_T, _M_dim, _N_dim>::col_iterator {
    typedef Matrix<_T, _M_dim, _N_dim> _Base_ty;
    typedef col_iterator               _Self;

    col_iterator(int dx, int dy, _Base_ty * const m) 
      : offsetX_(dx), offsetY_(dy), impl_(m){}

    col_iterator(const col_iterator& other)
      : offsetX_(other.offsetX_), offsetY_(other.offsetY_),
	impl_(other.impl_){}
    // default assign is okay
    
    _T* operator->() {
      return &((impl_->operator[](offsetX_))[offsetY_]);
    }
    const _T* operator->() const {
      return (const_cast<_Self *>(this))-> operator->();
    }
    _T& operator*() {
      return ((impl_->operator[](offsetX_))[offsetY_]);
    }
    const _T& operator*() const {
      return (const_cast<_Self *>(this))-> operator* ();
    }
    _Self&
    operator++() {++offsetX_; return *this;}
    
    _Self operator++(int) {
      _Self I = *this;
      ++offsetX_;
      return I;
    }
    // comparson returns true only if they derived from same matrix
    // and two offsets are the same too
    bool operator==(const _Self& RHS){
      return impl_ == (RHS.impl_)
	&& offsetX_ == RHS.offsetX_ 
	&& offsetY_ == RHS.offsetY_;
    }
    bool operator!=(const _Self& RHS) {
      return !operator==(RHS);
    }
  private:
    size_t offsetX_;
    const size_t offsetY_;
    _Base_ty * const impl_;
  };

  //==============================================//
  //~~               VIEWS                      ~~//
  //==============================================//

  template <class _T, int _sz_x, int _sz_y>
  class ReadView2 {
    typedef Vector<_T, _sz_y>                     _Vector_type;
    typedef Matrix<_T, _sz_x, _sz_y>              _Matrix_type;
    struct  _index_helper;
  public:
    typedef _T                                     value_type;
    typedef ReadView2<_T, _sz_x, _sz_y>            reader_type;
    typedef WriteView2<_T, _sz_x, _sz_y>           writer_type;
    typedef _Matrix_type                           container_type;

    typedef typename _Vector_type::iterator              iterator;
    typedef typename _Vector_type::const_iterator        const_iterator;
    typedef typename _Matrix_type::row_iterator          row_iterator;
    typedef typename _Matrix_type::const_iterator        const_row_iterator;
    typedef typename _Matrix_type::col_iterator          col_iterator;
    typedef typename _Matrix_type::const_col_iterator    const_col_iterator;
    typedef typename _Vector_type::_M_ty                 _M_ty;
    typedef _Vector_type                                 lower;
    enum { 
      VIEW_SIZE_X  = _sz_x,
      VIEW_SIZE_Y  = _sz_y,
    };
    
    ReadView2(const _M_ty* base, unsigned dimN, unsigned offsetX, unsigned offsetY)
      : impl_(base), dim_N_(dimN), offsetX_(offsetX), offsetY_(offsetY)
    {}

    _index_helper
    inline operator[] (int dx) const {
      const _M_ty * reader = impl_ + (offsetX_ + dx) * dim_N_;
      return _index_helper(reader, offsetY_);
    }

    // auto cast
    template <int SZ_X, int SZ_Y>
    operator ReadView2<_T, SZ_X, SZ_Y>() const
    {
      static_assert(SZ_X <= _sz_x, "illegal sub");
      static_assert(SZ_Y <= _sz_y, "illegal sub");

      return ReadView2<_T, SZ_X, SZ_Y>(impl_, dim_N_, offsetX_, offsetY_);
    }
    // subRView
    template <int SZ_X, int SZ_Y>
    ReadView2<_T, SZ_X, SZ_Y> subRView(int pos_x, int pos_y) const 
    {
      assert(offsetY_ + pos_y + SZ_Y <= dim_N_
	     && "illegal sub");      
      return ReadView2<_T, SZ_X, SZ_Y>(impl_, dim_N_, offsetX_ + pos_x,
				      offsetY_ + pos_y);
    }
    const _M_ty* data() const {
      return impl_ + dim_N_*(offsetX_) + offsetY_;
    }
    const size_t dimN() const {
      return dim_N_;
    }

  private: 
    struct _index_helper {
      //ctor
      _index_helper(const _M_ty* vec, unsigned offset)
	: row_(vec), offset_(offset){}
      inline const _M_ty& operator[] (unsigned dy)
      {
	return row_[offset_ + dy];
      }
      
    private:
      const _M_ty*  row_;
      const size_t  offset_;
    };

    const _M_ty* impl_;
    size_t       offsetX_, offsetY_;
    size_t       dim_N_;
  }; 
 
  template <class _T, int _sz_x, int _sz_y>
  class WriteView2 {
    typedef Vector<_T, _sz_y>                     _Vector_type;
    typedef Matrix<_T, _sz_x, _sz_y>              _Matrix_type;
    struct _index_helper;
  public:
    enum { 
      VIEW_SIZE_X  = _sz_x,
      VIEW_SIZE_Y  = _sz_y,
    };
    typedef _T                                     value_type;
    typedef ReadView2<_T, _sz_x, _sz_y>            reader_type;
    typedef WriteView2<_T, _sz_x, _sz_y>           writer_type;
    typedef _Matrix_type                           container_type;
    typedef typename _Vector_type::iterator              iterator;
    typedef typename _Vector_type::const_iterator        const_iterator;
    typedef typename _Matrix_type::row_iterator          row_iterator;
    typedef typename _Matrix_type::const_iterator        const_row_iterator;
    typedef typename _Matrix_type::col_iterator          col_iterator;
    typedef typename _Matrix_type::const_col_iterator    const_col_iterator;
    typedef typename _Vector_type::_M_ty          _M_ty;
    typedef _Vector_type lower;

    WriteView2(_M_ty* base, unsigned dimN, unsigned offsetX, unsigned offsetY)
      : impl_(base), dim_N_(dimN), offsetX_(offsetX), offsetY_(offsetY)
    {}
    WriteView2() {}
    _index_helper
    inline operator[] (int dx) {
      _M_ty * writer = impl_ + dim_N_*(dx + offsetX_);
      return _index_helper(writer, offsetY_);
    }
   
    template <int SZ_X, int SZ_Y>
    operator ReadView2<_T, SZ_X, SZ_Y>() const
    {
      static_assert(SZ_X <= _sz_x, "illegal sub");
      static_assert(SZ_Y <= _sz_y, "illegal sub");

      return ReadView2<_T, SZ_X, SZ_Y>(impl_, dim_N_, offsetX_, offsetY_);
    }

    template <int SZ_X, int SZ_Y>
    WriteView2<_T, SZ_X, SZ_Y> subWView(int pos_x, int pos_y) 
    {
      assert(offsetY_ + pos_y + SZ_Y <= dim_N_
	     && "illegal sub");      

      return WriteView2<_T, SZ_X, SZ_Y>(impl_, dim_N_, offsetX_ + pos_x,
					offsetY_ + pos_y);
    }

    template <int SZ_X, int SZ_Y>
    ReadView2<_T, SZ_X, SZ_Y> subRView(int pos_x, int pos_y) const
    {
      assert(offsetY_ + pos_y + SZ_Y <= dim_N_
	     && "illegal sub");      

      return ReadView2<_T, SZ_X, SZ_Y>(impl_, dim_N_, offsetX_ + pos_x,
				       offsetY_ + pos_y);
    }
    ReadView2<_T, _sz_x, _sz_y> subRView() const 
    {
      return ReadView2<_T, _sz_x, _sz_y>(impl_, dim_N_, offsetX_, offsetY_);
    }

    _M_ty* data() {
      return impl_ + dim_N_ * offsetX_ + offsetY_;
    }
    const _M_ty* data() const {
      return impl_ + dim_N_ * offsetX_ + offsetY_;
    }
    const size_t dimN() const {
      return dim_N_;
    }
  private:
    struct _index_helper {
       _index_helper(_M_ty * vec, unsigned offset)
	: row_(vec), offset_(offset){}

      inline _M_ty & operator[] (unsigned dy)
      {
	return row_[offset_ + dy];
      }
    private:
      _M_ty * row_;
      const size_t  offset_;
    };

    _M_ty * impl_;
    size_t offsetX_, offsetY_;
    size_t dim_N_;
  };

  //==============================================//
  //~~          ALGORITHM INTERFACES            ~~//
  //==============================================//

#include "matrixalgo.hpp"  
  ///Template function signature:
  // normal function has its signature and function type
  // e.g, a function like this: void foo(int I, char Ch){},
  // "void foo(int, char)" is its signature.
  // "void (int, char)" is its type.
  // for template function, we use template parameters to
  // represent common characteristics of a bunch of TF.
  

  template<class Result, class Arg0, class Arg1>
  struct matMulWrapper {
    typedef typename view_trait2<Result>::value_type T;
    const static int SIZE_A = view_trait2<Result>::WRITER_SIZE_X;
    const static int SIZE_B = view_trait2<Arg0>::READER_SIZE_Y;
    const static int SIZE_C = view_trait2<Result>::WRITER_SIZE_Y;

    static void doit(const Arg0& arg0, const Arg1& arg1, 
		     Result& result)
    {
      matArithImpl<T, SIZE_A, SIZE_B, SIZE_C>::mul(arg0, arg1, result);
    }
    static void doitMT(const Arg0& arg0, const Arg1& arg1, 
		       Result& result, mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };

  template<class Result, class Arg0, class Arg1>
  struct matMAddWrapper {
    typedef typename view_trait2<Result>::value_type T;
    typedef typename view_trait2<Result>::_M_ty     _M_ty;
    const static int SIZE_A = view_trait2<Result>::WRITER_SIZE_X;
    const static int SIZE_B = view_trait2<Arg0>::READER_SIZE_Y;
    const static int SIZE_C = view_trait2<Result>::WRITER_SIZE_Y;

    static void doit(const Arg0& arg0, const Arg1& arg1,
		     Result& result)
    {
      matArithImpl<T, SIZE_A, SIZE_B, SIZE_C>::madd(arg0, arg1, result);
    }
    static void doitMT(const Arg0& arg0, const Arg1& arg1, 
		       Result& result, mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };
  
  template<class Result, class Arg0, class Arg1>
  struct matAddWrapper {
    typedef typename view_trait2<Result>::value_type T;
    const static int SIZE_A = view_trait2<Result>::WRITER_SIZE_X;
    const static int SIZE_B = view_trait2<Result>::WRITER_SIZE_Y;

    static void doit(const Arg0& arg0, const Arg1& arg1, 
		     Result& result)
    {
      matArithImpl2<T, SIZE_A, SIZE_B>::add(arg0, arg1, result);
    }

    static void doitMT(const Arg0& arg0, const Arg1& arg1, 
		       Result& result, mt::barrier_t barrier)
    {
      doit(arg0, arg1, result);
      barrier->wait();
    }
  };
  
  template<class Arg0, class Arg1>
  struct matAddWrapper2{
    typedef typename view_trait2<Arg0>::value_type T;
    typedef typename view_trait2<Arg0>::reader_type Reader;
    typedef typename view_trait2<Arg0>::writer_type Writer;
    const static int SIZE_A = view_trait2<Arg0>::WRITER_SIZE_X;
    const static int SIZE_B = view_trait2<Arg0>::WRITER_SIZE_Y;
    
    static void doit(Arg0& arg0, const Arg1& arg1)
    {
      Reader r = arg0.subRView();
      Writer w = arg0;
      matArithImpl2<T, SIZE_A, SIZE_B>::add(r, arg1, w);
    }

    static void doitMT(Arg0& arg0, const Arg1& arg1, mt::barrier_t barrier)
    {
      doit(arg0, arg1);
      barrier->wait();
    }
    
  };
} // end of NS

#endif /*VINA_MATRIX2_HXX*/
