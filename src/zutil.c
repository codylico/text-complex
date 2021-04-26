/*
 * @file tcmplx-access/api.c
 * @brief Tools for DEFLATE
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/zutil.h"

/* BEGIN configurations */
tcmplxA_uint32 tcmplxA_zutil_adler32
  (size_t len, unsigned char const* buf, tcmplxA_uint32 chk)
{
  tcmplxA_uint32 s1 = chk&0xFFff;
  tcmplxA_uint32 s2 = (chk>>16);
  size_t i;
  for (i = 0u; i < len; ) {
    size_t j;
    for (j = 0u; j < 5550u && i < len; ++j, ++i) {
      s1 += buf[i];
      s2 += s1;
    }
    s1 %= 65521u;
    s2 %= 65521u;
  }
  return s1|(s2<<16);
}
/* END   configurations */
