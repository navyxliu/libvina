// This file is not supposed to be distributed.

/// Another example of reduction. Almost the same as reduce.cc
// except using some trickies to reduce code. 
// reduce is node-oriented, it is very intuitive.
// reduce2 is level-oriented.
// programmer should be aware of template code growth.
// i can *NOT* see any general ways to control this expansion.
/*
CODE SIZE comparison
[liu@biology3 Examples]$ ls -l reduce reduce2
-rwxrwxr-x 1 liu liu 12279 2009-06-05 16:14 reduce
-rwxrwxr-x 1 liu liu 11169 2009-06-05 16:13 reduce2

GENERATED FUNCTIONS comparison

[liu@biology3 Examples]$ nm -C reduce | grep reduce
0000000000400cc4 W reducer<0, 10>::reduce()
0000000000400cb8 W reducer<0, 11>::reduce()
0000000000400cac W reducer<0, 12>::reduce()
0000000000400ca0 W reducer<0, 13>::reduce()
0000000000400c94 W reducer<0, 14>::reduce()
0000000000400c88 W reducer<0, 15>::reduce()
0000000000400cdc W reducer<0, 8>::reduce()
0000000000400cd0 W reducer<0, 9>::reduce()
0000000000400b89 W reducer<1, 4>::reduce()
0000000000400b6a W reducer<1, 5>::reduce()
0000000000400b4b W reducer<1, 6>::reduce()
0000000000400b2c W reducer<1, 7>::reduce()
0000000000400a7d W reducer<2, 2>::reduce()
0000000000400a5e W reducer<2, 3>::reduce()
00000000004009f7 W reducer<3, 1>::reduce()

[liu@biology3 Examples]$ nm -C reduce2 | grep reduce
00000000004009d8 W reducer2<0>::reduce(int)
0000000000400b8b W reducer2<1>::reduce(int)
0000000000400ab2 W reducer2<2>::reduce(int)
0000000000400a21 W reducer2<3>::reduce(int)

*/
#include <iostream>
using namespace std;

// bi-tree node
struct Node{
  int left;
  int right;
  
  int value;
};

// a full bi-tree
/*

L              TREE

3                1
               /   \
2             2     3
             / \   / \
1           4   5    ...
           / \         \
0         8   9 ...     15

*/


#define TREE_SIZE 15
static Node Tree[1+TREE_SIZE];



template <int L>
struct reducer2
{
  enum { HIGHT = 4};
  static int reduce(int guard) {
    int sum = 0;
    if ( guard != 0 ) return 0;

    for (int i=0; i < (1<<(HIGHT - L)); ++i) {
      sum += reducer2<L-1>::reduce(i);
    }
    return sum;
  }
};

template <>
struct reducer2<0>
{
  enum {HIGHT = 4};
  static int reduce(int i) {
    return Tree[(1<<(HIGHT-1)) + i].value;
  }
};

template<int L, int I=1>
struct builder {
  static void doit()
  {
    builder<L-1, (I*2)>::doit();
    builder<L-1, (I*2)+1>::doit();

    Tree[I].left  = I * 2;
    Tree[I].right = I * 2 + 1;    
  }
};

template<int I>
struct builder<0, I>
{
  static void doit()
  {
    Tree[I].left = Tree[I].right = 0;
  }
};


int main()
{
  builder<3>::doit();

  for ( int i=8, j = 1; i <= TREE_SIZE; ++i ) {
    Tree[i].value = j++;
  }
  
  for ( int i=1; i <= TREE_SIZE; ++i ) 
    cout << Tree[i].left << "\t" << Tree[i].right << "\t" << Tree[i].value << endl;

  cout << reducer2<3>::reduce(0) << endl;
}
