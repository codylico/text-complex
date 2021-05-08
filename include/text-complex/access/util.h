/**
 * @file text-complex/access/util.h
 * @brief Utility functions for `text-complex`
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_Util_H_
#define hg_TextComplexAccess_Util_H_

#include "api.h"

/* BEGIN allocation */
/**
 * \brief Allocate some bytes on the heap.
 * \return a pointer to the bytes on success, NULL otherwise
 */
TCMPLX_A_API
void* tcmplxA_util_malloc(size_t sz);

/**
 * \brief Free some bytes from the heap.
 * \param x (nullable) pointer to bytes to free
 */
TCMPLX_A_API
void tcmplxA_util_free(void* x);
/* END   allocation */

#endif /*hg_TextComplexAccess_Util_H_*/
