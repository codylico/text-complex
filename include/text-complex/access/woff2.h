/**
 * @file tcmplx-access/woff2.h
 * @brief WOFF2 file utility API
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_Woff2_H_
#define hg_TextComplexAccess_Woff2_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct mmaptwo_i;

struct tcmplxA_woff2;

/* BEGIN woff2 utilities */
/**
 * @brief Convert a tag to a WOFF2 table type.
 * @param s the tag to convert
 * @return the table type number, or 63 if unavailable
 */
TCMPLX_A_API
unsigned int tcmplxA_woff2_tag_toi(unsigned char const* s);

/**
 * @brief Convert a WOFF2 table type to a tag.
 * @param s the tag to convert
 * @return a tag name, or NULL if no tag name matches the given value
 * @note Only checks the 6 least significant bits.
 */
TCMPLX_A_API
unsigned char const* tcmplxA_woff2_tag_fromi(unsigned int x);
/* END   woff2 utilities */

/* BEGIN woff2 */
/**
 * @brief Construct a new woff2 accessor.
 * @param xfh File access instance to use for the Woff2.
 * @param sane_tf Sanitize the file before processing.
 *   (Use nonzero for default behavior.)
 * @return a pointer to the woff2 on success, NULL otherwise
 * @note The woff2 takes ownership of the mmaptwo instance.
 */
TCMPLX_A_API
struct tcmplxA_woff2* tcmplxA_woff2_new(struct mmaptwo_i* xfh, int sane_tf);

/**
 * @brief Destroy a woff2 accessor.
 * @param x (nullable) the woff2 to destroy
 */
TCMPLX_A_API
void tcmplxA_woff2_destroy(struct tcmplxA_woff2* x);

/**
 * @brief Acquire the list of transformed offsets.
 * @param x a woff2 accessor
 * @return an offset table
 */
struct tcmplxA_offtable const* tcmplxA_woff2_get_offsets
  (struct tcmplxA_woff2 const* x);
/* END   woff2 */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_Woff2_H_*/
