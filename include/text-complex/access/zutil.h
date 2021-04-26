/**
 * @file tcmplx-access/zutil.h
 * @brief Tools for DEFLATE
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_ZUtil_H_
#define hg_TextComplexAccess_ZUtil_H_

#include "api.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/**
 * @brief Compute a checksum.
 * @param len length of buffer to process
 * @param buf buffer to process
 * @param chk last check value (use `1` for initial value)
 * @return new checksum
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_zutil_adler32
  (size_t len, unsigned char const* buf, tcmplxA_uint32 chk);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_InsCopy_H_*/
