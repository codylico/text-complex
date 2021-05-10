/**
 * @file text-complex/access/ringslide_p.h
 * @brief Sliding window of past bytes
 */
#ifndef hg_TextComplexAccess_RingSlide_pH_
#define hg_TextComplexAccess_RingSlide_pH_

#include "text-complex/access/api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @internal
 * @brief Sliding window of bytes.
 */
struct tcmplxA_ringslide {
  tcmplxA_uint32 n;
  tcmplxA_uint32 pos;
  tcmplxA_uint32 cap;
  tcmplxA_uint32 sz;
  unsigned char* p;
};

/**
 * @brief Initialize a slide ring.
 * @param x the slide ring to initialize
 * @param n maximum sliding window size
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_ringslide_init
  (struct tcmplxA_ringslide* x, tcmplxA_uint32 n);
/**
 * @brief Close a slide ring.
 * @param x the slide ring to close
 */
void tcmplxA_ringslide_close(struct tcmplxA_ringslide* x);


#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_RingSlide_pH_*/
