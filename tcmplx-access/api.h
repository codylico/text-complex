/*
 * \file tcmplx/api.h
 * \brief API-wide declarations for text-complex
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplex_api_H_
#define hg_TextComplex_api_H_

#ifdef TCMPLX_WIN32_DLL
#  ifdef TCMPLX_WIN32_DLL_INTERNAL
#    define TCMPLX_API __declspec(dllexport)
#  else
#    define TCMPLX_API __declspec(dllimport)
#  endif /*TCMPLX_DLL_INTERNAL*/
#else
#  define TCMPLX_API
#endif /*TCMPLX_WIN32_DLL*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* BEGIN configurations */
/**
 * \brief Check the library's version.
 * \return a version string
 */
TCMPLX_API
char const* tcmplx_api_version(void);
/* END   configurations */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplex_api_H_*/
