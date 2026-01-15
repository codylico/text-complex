/**
 * @file text-complex/access/ringslide.h
 * @brief Sliding window of past bytes
 */
#ifndef hg_TextComplexAccess_RingSlide_H_
#define hg_TextComplexAccess_RingSlide_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Sliding window of past bytes.
 */
struct tcmplxA_ringslide;

/* BEGIN slide ring */
/**
 * @brief Construct a new slide ring.
 * @param n size of sliding window in bytes
 * @return a pointer to the slide ring on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_ringslide* tcmplxA_ringslide_new(tcmplxA_uint32 n);

/**
 * @brief Destroy a slide ring.
 * @param x (nullable) the slide ring to destroy
 */
TCMPLX_A_API
void tcmplxA_ringslide_destroy(struct tcmplxA_ringslide* x);

/**
 * @brief Query the window size of the slide ring.
 * @param x the slide ring to inspect
 * @return a sliding window size
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_ringslide_extent(struct tcmplxA_ringslide const* x);

/**
 * @brief Add the most recent byte.
 * @param x the slide ring to update
 * @param v the byte to add
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_ringslide_add(struct tcmplxA_ringslide* x, unsigned int v);

/**
 * @brief Query the number of bytes held by the sliding window.
 * @param x the slide ring to inspect
 * @return the count of past bytes in the window
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_ringslide_size(struct tcmplxA_ringslide const* x);

/**
 * @brief Query a past byte.
 * @param x the slide ring to inspect
 * @param i number of bytes to go back; zero is most recent
 * @return a reference to the byte at the given index
 */
TCMPLX_A_API
unsigned int tcmplxA_ringslide_peek
  (struct tcmplxA_ringslide const* x, tcmplxA_uint32 i);
/* END   slide ring */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_RingSlide_H_*/
