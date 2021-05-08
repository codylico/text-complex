/**
 * \file text-complex/access/util.c
 * \brief Utility functions for `text-complex`
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/util.h"
#include <stdlib.h>

void* tcmplxA_util_malloc(size_t sz) {
  if (sz == 0)
    return NULL;
  return malloc(sz);
}

void tcmplxA_util_free(void* x) {
  free(x);
  return;
}
