// XFAIL: Linux
// RUN: %amp_device -D__GPU__ -Xclang -fhsa-ext %s -m64 -emit-llvm -c -S -O2 -o %t.ll && mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t && %embed_kernel kernel.cl %t/kernel.o && popd
// RUN: %cxxamp -Xclang -fhsa-ext %link %t/kernel.o %s -o %t.out && %t.out
#include <iostream>
#include <iomanip>
#include <amp.h>
#include <ctime>
#include "hsa_new.h"

#define DEBUG 1

// An HSA version of C++AMP program
int main ()
{
  // Removed until linking/alloc qualifier issue is solved
  auto ptr_a = newInit.ptr_a;
  auto ptr_b = newInit.ptr_b;
  auto ptr_c = newInit.ptr_c;

  // define inputs and output
  const int vecSize = 16;
  const int tileSize = 4;
  const int tileCount = vecSize / tileSize;

  // launch kernel
  unsigned long int sumCPP[vecSize];
  Concurrency::array_view<unsigned long int, 1> sum(vecSize, sumCPP);

  clock_t m_start = clock();
  parallel_for_each(
    Concurrency::extent<1>(vecSize).tile<tileSize>(),
    [=](Concurrency::tiled_index<tileSize> tidx) restrict(amp) {

    put_ptr_a(ptr_a);
    put_ptr_b(ptr_b);
    put_ptr_c(ptr_c);

    sum[tidx.global[0]] = (unsigned long int)new unsigned int(tidx.local[0]);
  });
  clock_t m_stop = clock();

#if DEBUG
  for (int i = 0; i < vecSize; i++)
  {
    unsigned int *p = (unsigned int*)sum[i];
    printf("Value of addr %p is %u\n", (void*)p, *p);
  }
#endif

  // Verify
  int error = 0;
  for(int i = 0; i < vecSize; i++) {
    unsigned int *p = (unsigned int*)sum[i];
    error += abs(*p - (i % tileSize));
  }
  if (error == 0) {
    std::cout << "Verify success!\n";
  } else {
    std::cout << "Verify failed!\n";
  }
  clock_t t1 = clock();
  clock_t t2 = clock();
  clock_t m_overhead = t2 - t1;
  double elapsed = ((double)(m_stop - m_start - m_overhead)) / CLOCKS_PER_SEC;
  std::cout << "Execution time of amp restrict lambda is " << std::dec << elapsed << " s.\n";
  std::cout << "System call is executed " << std::dec << newInit.get_syscall_count() << " times\n";
  return (error != 0);
}

