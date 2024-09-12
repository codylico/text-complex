/**
 * \file text-complex/access/api.c
 * \brief API-wide declarations for text-complex
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/api.h"
#include <stddef.h>

/* BEGIN configurations */
char const* tcmplxA_api_version(void) {
  return "0.3.7-alpha";
}
/* END   configurations */

/* BEGIN error codes */
char const* tcmplxA_api_error_toa(int v) {
  switch (v) {
  case tcmplxA_ErrInsCopyMissing:
    return "An expected insert-copy code is missing";
  case tcmplxA_ErrPartial:
    return "Partial output";
  case tcmplxA_ErrZDictionary:
    return "Expecting a ZLIB dictionary";
  case tcmplxA_ErrBlockOverflow:
    return "Block buffer may overflow";
  case tcmplxA_ErrOutOfRange:
    return "Array index out of range";
  case tcmplxA_ErrRingDistOverflow:
    return "Numeric overflow produced by a distance code conversion";
  case tcmplxA_ErrRingDistUnderflow:
    return "Numeric underflow produced by a distance code conversion";
  case tcmplxA_ErrFixCodeAlloc:
    return "Prefix code lengths of same bit count were too numerous";
  case tcmplxA_ErrFixLenRange:
    return "Prefix code lengths were too large";
  case tcmplxA_ErrParam:
    return "Invalid parameter given";
  case tcmplxA_ErrSanitize:
    return "File sanity check failed";
  case tcmplxA_ErrMemory:
    return "Memory allocation error";
  case tcmplxA_ErrInit:
    return "Initialization error";
  case tcmplxA_Success:
    return "Success";
  case tcmplxA_EOF:
    return "End of stream";
  default:
    return NULL;
  }
}
/* END   error codes */
