/*
 * \file tcmplx-access/api.h
 * \brief API-wide declarations for text-complex
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_api_H_
#define hg_TextComplexAccess_api_H_

#include <limits.h>
#include <stddef.h>

#ifdef TCMPLX_A_WIN32_DLL
#  ifdef TCMPLX_A_WIN32_DLL_INTERNAL
#    define TCMPLX_A_API __declspec(dllexport)
#  else
#    define TCMPLX_A_API __declspec(dllimport)
#  endif /*TCMPLX_A_DLL_INTERNAL*/
#else
#  define TCMPLX_A_API
#endif /*TCMPLX_A_WIN32_DLL*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#if UINT_MAX >= 0xFFffFFff
typedef unsigned int tcmplxA_uint32;
#else
typedef unsigned long int tcmplxA_uint32;
#endif /*UINT_MAX*/

/* BEGIN configurations */
/**
 * \brief Check the library's version.
 * \return a version string
 */
TCMPLX_A_API
char const* tcmplxA_api_version(void);
/* END   configurations */

/* BEGIN error codes */
enum tcmplxA_error {
  /** Memory allocation error */
  tcmplxA_ErrMemory = -2,
  /** Initialization error */
  tcmplxA_ErrInit = -1,
  /** Success code */
  tcmplxA_Success = 0
};
/* END   error codes */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_api_H_*/
