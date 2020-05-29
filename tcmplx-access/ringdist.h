/**
 * @file tcmplx-access/ringdist.h
 * @brief Distance ring buffer
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_RingDist_H_
#define hg_TextComplexAccess_RingDist_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Distance ring buffer.
 */
struct tcmplxA_ringdist;

/* BEGIN distance ring */
/**
 * @brief Construct a new distance ring.
 * @param special_tf nonzero to support Brotli distance codes
 * @param direct number of direct codes (0..120)
 * @param postfix postfix bit count (0..3)
 * @return a pointer to the distance ring on success, NULL otherwise
 * @note For RFC 1951 semantics, use `special_tf=0`, `direct=4`, `postfix=0`.
 */
TCMPLX_A_API
struct tcmplxA_ringdist* tcmplxA_ringdist_new
  (int special_tf, unsigned int direct, unsigned int postfix);

/**
 * @brief Destroy a distance ring.
 * @param x (nullable) the distance ring to destroy
 */
TCMPLX_A_API
void tcmplxA_ringdist_destroy(struct tcmplxA_ringdist* x);

/**
 * @brief Compute the number of extra bits for a distance code.
 * @param dcode distance code to check
 * @return a bit count
 * @note This function does not advance the ring buffer. It also does
 *   not check for distance code overflow.
 */
TCMPLX_A_API
unsigned int tcmplxA_ringdist_bit_count
  (struct tcmplxA_ringdist const* x, unsigned int dcode);

/**
 * @brief Convert a distance code to a flat backward distance.
 * @param dcode distance code to convert
 * @param extra any extra bits required by the code
 * @return a flat distance, or zero on conversion error
 * @note On successful conversion, the distance ring buffer is advanced
 *   as necessary.
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_ringdist_decode
  (struct tcmplxA_ringdist* x, unsigned int dcode, tcmplxA_uint32 extra);

/**
 * @brief Convert a flat backward distance to a distance code.
 * @param back_dist backward distance to convert
 * @param[out] extra any extra bits required by the code
 * @param[out] ae @em error-code api_error::Success on success,
 *   nonzero otherwise
 * @return a distance code, or UINT_MAX on error
 * @note On successful conversion, the distance ring buffer is advanced
 *   as necessary.
 */
TCMPLX_A_API
unsigned int tcmplxA_ringdist_encode
  (struct tcmplxA_ringdist* x, unsigned int back_dist, tcmplxA_uint32 *extra);

/**
 * @brief Copy a distance ring configuration and state
 *   to another distance ring.
 * @param dst ring to receive the copy
 * @param src ring to copy
 */
TCMPLX_A_API
void tcmplxA_ringdist_copy
  (struct tcmplxA_ringdist* dst, struct tcmplxA_ringdist const* src);
/* END   distance ring */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_RingDist_H_*/
