// This file is not supposed to be distributed.

/// An example of reduce. There is anothere version
// which is a little slimer than this one. 
// see [reduce2.cc]
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


template <int L, int I = 1>
struct reducer {
  static int reduce()
  {
    return reducer<L-1, 2*I>::reduce() 
      + reducer<L-1, 2*I+1>::reduce();
  }
};

template <int I>
struct reducer<0, I>{
  static int reduce()
  {
    return Tree[I].value;
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

  cout << reducer<3>::reduce() << endl;

}
