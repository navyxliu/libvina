#include <stdio.h>
#include <matrix2.hpp>
#include <toolkits.hpp>
#include <tf.hpp>
using namespace vina;

int
main(int argc, char *argv[])
{
   typedef Matrix<float, MM_TEST_SIZE_N, MM_TEST_SIZE_N>
   matrix_t;
   static matrix_t A, B, C;
   NumRandGen<float> gen;
   for(int i=0; i<MM_TEST_SIZE_N; ++i) for(int j=0; j<MM_TEST_SIZE_N; ++j) 
    A[i][j] = gen(), B[i][j] = gen();

   C.zero();
}
