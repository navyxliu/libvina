#include "toolkits.hpp"
using namespace vina;

#include <stdio.h>

int main()
{
  float ghz = caliberate();
  printf("the freqency of processor is %.2fGhz\n", ghz);

}
