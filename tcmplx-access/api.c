/*
 * \file tcmplx-access/api.c
 * \brief API-wide declarations for text-complex
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "api.h"
#include <stddef.h>

/* BEGIN configurations */
char const* tcmplxA_api_version(void) {
  return "0.1";
}
/* END   configurations */

/* BEGIN error codes */
char const* tcmplxA_api_error_toa(int v) {
  switch (v) {
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
  default:
    return NULL;
  }
}
/* END   error codes */
