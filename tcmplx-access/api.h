/*
 * \file tcmplx-access/api.h
 * \brief API-wide declarations for text-complex
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_api_H_
#define hg_TextComplexAccess_api_H_

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

/* BEGIN configurations */
/**
 * \brief Check the library's version.
 * \return a version string
 */
TCMPLX_A_API
char const* tcmplxA_api_version(void);
/* END   configurations */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_api_H_*/
