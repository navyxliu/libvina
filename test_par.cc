//This file is not supposed to be distributed.
// test file for par.hpp.
// command: g++ -o test_par test_par.cc -std=c++0x -fopenmp -I.

#include <stdio.h>
#include "par.hpp"  // included omp.h
#include "seq.hpp"
using namespace vina;

struct echo
{
  void operator() ()
  {
     int tid = omp_get_thread_num();
     printf("thread %2d is running\n", tid);
  } 
};

int
main(int argc, char * argv[])
{
   printf("P3::apply()\n");
   typedef par<par_tail, 3, echo>  P3;
   P3::apply();

   printf("S2P3::apply()\n");
   typedef seq<par<par_tail, 3, echo>, 2> S2P3;
   S2P3::apply();
   
   printf("P3S2::apply()\n");
   typedef par<seq<seq_tail, 2, echo>, 3> P3S2;
   P3S2::apply();   
}
