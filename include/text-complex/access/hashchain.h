/**
 * @file text-complex/access/hashchain.h
 * @brief Duplicate lookup hash chain
 */
#ifndef hg_TextComplexAccess_HashChain_H_
#define hg_TextComplexAccess_HashChain_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Duplicate lookup hash chain.
 */
struct tcmplxA_hashchain;

/* BEGIN hash chain */
/**
 * @brief Construct a new hash chain.
 * @param n maximum sliding window size
 * @param chain_length run-time parameter limiting hash chain length
 * @return a pointer to the hash chain on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_hashchain* tcmplxA_hashchain_new
  (tcmplxA_uint32 n, size_t chain_length);

/**
 * @brief Destroy a hash chain.
 * @param x (nullable) the hash chain to destroy
 */
TCMPLX_A_API
void tcmplxA_hashchain_destroy(struct tcmplxA_hashchain* x);

/**
 * @brief Query the window size of the slide ring.
 * @param x the slide ring to inspect
 * @return a sliding window size
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_hashchain_extent(struct tcmplxA_hashchain const* x);

/**
 * @brief Add the most recent byte.
 * @param x the slide ring to update
 * @param v the byte to add
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_hashchain_add(struct tcmplxA_hashchain* x, unsigned int v);

/**
 * @brief Query the number of bytes held by the sliding window.
 * @param x the slide ring to inspect
 * @return the count of past bytes in the window
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_hashchain_size(struct tcmplxA_hashchain const* x);

/**
 * @brief Query a past byte.
 * @param x the slide ring to inspect
 * @param i number of bytes to go back; zero is most recent
 * @return a reference to the byte at the given index
 */
TCMPLX_A_API
unsigned int tcmplxA_hashchain_peek
  (struct tcmplxA_hashchain const* x, tcmplxA_uint32 i);

/**
 * @brief Search for a byte sequence.
 * @param x the slide ring to inspect
 * @param b three byte sequence for which to look
 * @param pos number of bytes to go back; zero is most recent
 * @return a backward distance where to find the three-byte
 *   sequence, or `(tcmplxA_uint32)-1` if not found
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_hashchain_find
  ( struct tcmplxA_hashchain const* x, unsigned char const* b,
    tcmplxA_uint32 pos);
/* END   hash chain */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_HashChain_H_*/
