/**
 * \file text-complex/access/util.c
 * \brief Utility functions for `text-complex`
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/util.h"
#include <stdlib.h>
#include <limits.h>
#if (defined _MSC_VER)
#  include <intrin.h>
#endif /*_MSC_VER*/

enum tcmplxA_util_const {
  tcmplxA_util_UIntPrec = sizeof(unsigned)*CHAR_BIT
};

void* tcmplxA_util_malloc(size_t sz) {
  if (sz == 0)
    return NULL;
  return malloc(sz);
}

void tcmplxA_util_free(void* x) {
  free(x);
  return;
}

unsigned tcmplxA_util_bitwidth(unsigned int x) {
#if (defined __GNUC__)
  return x ? tcmplxA_util_UIntPrec - __builtin_clz(x) : 0;
#elif (defined _MSC_VER)
  unsigned long index = 0;
  if (!x)
    return 0;
  _BitScanReverse(&index, x);
  return (unsigned)index+1u;
#else
  unsigned int y = 0;
  for (; x > 0; x >>= 1)
    y += 1;
  return y;
#endif /*bitwidth*/
}
