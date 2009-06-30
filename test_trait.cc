// This file is not supposed to be distributed.
#include "vector.hpp"
#include "matrix2.hpp"
#include "trait.hpp"
#include "toolkits.hpp"

#include <iostream>
using namespace vina;

int main()
{
  typedef Vector<int, 10> V;
  V v;

  // Vector
  verify(view_trait<V>::value_type,     int);
  //  verify(view_trait<V>::vector_type,    V);
  typedef ReadView<int,10> rview_10;
  typedef WriteView<int, 10> wview_10;
  typedef ReadView<int, 5> rview_5;
  typedef WriteView<int, 5> wview_5;
  verify(view_trait<V>::reader_type,    rview_10);
  verify(view_trait<V>::writer_type,    wview_10);
  verify(view_trait<V>::iterator,       V::iterator);
  verify(view_trait<V>::const_iterator, V::const_iterator);
  verify(decltype(v.subRView()),        rview_10);
  verify(decltype(v.subWView()),        wview_10);
  verify(decltype(v.subRView<5>(5)),    rview_5);
  verify(decltype(v.subWView<5>(5)),    wview_5);

  
  assert_s(view_trait<V>::READER_SIZE ==   10);
  assert_s(view_trait<V>::WRITER_SIZE ==   10);

  // RView and WView
  typedef decltype(v.subRView<5>(0)) Reader;
  typedef decltype(v.subWView<5>(0)) Writer;
  

  assert_s(view_trait<Reader>::READER_SIZE  ==   5);
  assert_s(view_trait<Reader>::WRITER_SIZE  ==   5);

  assert_s(view_trait<Writer>::READER_SIZE  ==   5);
  assert_s(view_trait<Writer>::WRITER_SIZE  ==   5);

  // MT Views
  auto wview  = v.subWView();
  auto wview2 = v.subWView<5>(5);
  typedef WriteViewMT<int, 10> wview_mt_10;
  typedef WriteViewMT<int, 5>  wview_mt_5;
  typedef ReadViewMT<int, 10>  rview_mt_10;
  typedef ReadViewMT<int, 5>   rview_mt_5;
  verify(decltype(wview.subWViewMT()),  wview_mt_10);
  verify(decltype(wview2.subWViewMT()), wview_mt_5);
  verify(decltype(wview.subRViewMT()),  rview_mt_10);
  verify(decltype(wview2.subRViewMT()), rview_mt_5);
  
  typedef decltype(wview.subWViewMT())  WriterMT;
  typedef decltype(wview.subRViewMT())  ReaderMT;

  verify(view_trait<WriterMT>::value_type,  int);
  //verify(view_trait<WriterMT>::vector_type, V);
  verify(view_trait<WriterMT>::reader_type,    rview_10);
  verify(view_trait<WriterMT>::writer_type,    wview_10);
  verify(view_trait<WriterMT>::reader_mt_type, rview_mt_10);
  verify(view_trait<WriterMT>::writer_mt_type, wview_mt_10);


  assert_s(view_trait<WriterMT>::READER_SIZE  ==   10);
  assert_s(view_trait<WriterMT>::WRITER_SIZE  ==   10);

  verify(view_trait<ReaderMT>::value_type,  int);
  //verify(view_trait<ReaderMT>::vector_type, V);
  verify(view_trait<ReaderMT>::reader_type, rview_10);
  verify(view_trait<ReaderMT>::writer_type, wview_10);
  verify(view_trait<ReaderMT>::reader_mt_type, rview_mt_10);
  verify(view_trait<ReaderMT>::writer_mt_type, wview_mt_10);

  assert_s(view_trait<ReaderMT>::READER_SIZE  ==   10);
  assert_s(view_trait<ReaderMT>::WRITER_SIZE  ==   10);



  //==============================================//
  //~~               MATRIX                     ~~//
  //==============================================//

  typedef Matrix<int, 10, 10> M;
  typedef Matrix<vector_type<int>, 10, 10> M_v;

  M m;
  M_v m_v;

  typedef ReadView2<int, 10, 10>  RView;
  typedef WriteView2<int, 10, 10> WView;
  typedef ReadView2<int, 5, 5>    RView1;
  typedef WriteView2<int, 5, 5>   WView1;

  verify(view_trait2<M>::value_type,    int);
  //  verify(view_trait2<M>::vector_type,   V);
  //  verify(view_trait2<M>::matrix_type,   M);
  verify(view_trait2<M>::reader_type,   RView);
  verify(view_trait2<M>::writer_type,   WView);
  verify(view_trait2<M>::iterator,      V::iterator);
  verify(view_trait2<M>::const_iterator,V::const_iterator);
  verify(decltype(m.subRView()),        RView);
  verify(decltype(m.subWView()),        WView);
  verify(decltype(m.subRView<5,5>(5, 5)),  RView1);
  verify(decltype(m.subWView<5,5>(5, 5)),  WView1);
 
  assert_s(view_trait2<M>::READER_SIZE_X    ==   10);
  assert_s(view_trait2<M>::READER_SIZE_Y    ==   10);
  assert_s(view_trait2<M>::WRITER_SIZE_X    ==   10);
  assert_s(view_trait2<M>::WRITER_SIZE_Y    ==   10);
  
  assert_s(view_trait2<RView1>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<RView1>::READER_SIZE_Y  ==   5);
  assert_s(view_trait2<RView1>::WRITER_SIZE_X  ==   5);
  assert_s(view_trait2<RView1>::WRITER_SIZE_Y  ==   5);

  assert_s(view_trait2<WView1>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1>::WRITER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1>::WRITER_SIZE_Y  ==   5);

  typedef ReadView2<vector_type<int>, 10, 10>  RView_v;
  typedef WriteView2<vector_type<int>, 10, 10> WView_v;
  typedef ReadView2<vector_type<int>, 5, 5>    RView1_v;
  typedef WriteView2<vector_type<int>, 5, 5>   WView1_v;

  verify(view_trait2<M_v>::value_type,    vector_type<int>);
  verify(view_trait2<M_v>::_M_ty,         int);

  verify(view_trait2<M_v>::reader_type,   RView_v);
  verify(view_trait2<M_v>::writer_type,   WView_v);
  verify(view_trait2<M_v>::iterator,      V::iterator);
  verify(view_trait2<M_v>::const_iterator,V::const_iterator);
  verify(decltype(m_v.subRView()),        RView_v);
  verify(decltype(m_v.subWView()),        WView_v);
  verify(decltype(m_v.subRView<5,5>(5, 5)),  RView1_v);
  verify(decltype(m_v.subWView<5,5>(5, 5)),  WView1_v);
 
  assert_s(view_trait2<M_v>::READER_SIZE_X    ==   10);
  assert_s(view_trait2<M_v>::READER_SIZE_Y    ==   10);
  assert_s(view_trait2<M_v>::WRITER_SIZE_X    ==   10);
  assert_s(view_trait2<M_v>::WRITER_SIZE_Y    ==   10);
  
  assert_s(view_trait2<RView1_v>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<RView1_v>::READER_SIZE_Y  ==   5);
  assert_s(view_trait2<RView1_v>::WRITER_SIZE_X  ==   5);
  assert_s(view_trait2<RView1_v>::WRITER_SIZE_Y  ==   5);

  assert_s(view_trait2<WView1_v>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1_v>::READER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1_v>::WRITER_SIZE_X  ==   5);
  assert_s(view_trait2<WView1_v>::WRITER_SIZE_Y  ==   5);
  
  std::cout << "test traits passed." << std::endl;
}
