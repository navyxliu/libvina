//==============================================//
//~~               TRAITS                     ~~//
//==============================================//

#ifndef TRAIT_HXX
#define TRAIT_HXX 1

namespace vina {
  template <class T>
  struct view_trait{
    typedef typename T::value_type                   value_type;
    typedef typename T::container_type               container_type;
    typedef typename T::reader_type                  reader_type;
    typedef typename T::writer_type                  writer_type;
    typedef typename T::iterator                     iterator;
    typedef typename T::const_iterator               const_iterator;
    typedef typename T::reader_mt_type               reader_mt_type;
    typedef typename T::writer_mt_type               writer_mt_type;
    typedef typename T::_M_ty                        _M_ty;

    enum {
      READER_SIZE = reader_type::VIEW_SIZE,
      WRITER_SIZE = writer_type::VIEW_SIZE,
    };
  };
  // 2-dimension view
  template<class T>
  struct view_trait2 : view_trait<typename T::lower> {
    typedef typename T::container_type               container_type;
    typedef typename T::reader_type                  reader_type;
    typedef typename T::writer_type                  writer_type;
    
    enum {
      READER_SIZE_X = reader_type::VIEW_SIZE_X,
      READER_SIZE_Y = reader_type::VIEW_SIZE_Y,
      WRITER_SIZE_X = writer_type::VIEW_SIZE_X,
      WRITER_SIZE_Y = writer_type::VIEW_SIZE_Y,
    };
  };
  
  template <bool b=false>
  struct bool_{};

  template <>
  struct bool_<true>{};
}

#endif /*endof TRAIT_HXX*/
