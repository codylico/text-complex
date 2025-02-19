/**
 * @file text-complex/access/ctxtmap.h
 * @brief Context map for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_CtxtMap_H_
#define hg_TextComplexAccess_CtxtMap_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Context map for compressed streams.
 * @note The layout for a context map is block-type major.
 */
struct tcmplxA_ctxtmap;

/**
 * @brief Literal context modes.
 */
enum tcmplxA_ctxtmap_mode {
  tcmplxA_CtxtMap_LSB6 = 0,
  tcmplxA_CtxtMap_MSB6 = 1,
  tcmplxA_CtxtMap_UTF8 = 2,
  tcmplxA_CtxtMap_Signed = 3,
  tcmplxA_CtxtMap_ModeMax
};

/* BEGIN context map */
/**
 * @brief Construct a new context map.
 * @param btypes number of block types
 * @param ctxts number of contexts
 * @return a pointer to the context map on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_ctxtmap* tcmplxA_ctxtmap_new(size_t btypes, size_t ctxts);

/**
 * @brief Destroy a context map.
 * @param x (nullable) the context map to destroy
 */
TCMPLX_A_API
void tcmplxA_ctxtmap_destroy(struct tcmplxA_ctxtmap* x);

/**
 * @brief Inspect the number of block types for a context map.
 * @param x the context map to inspect
 * @return a block type count
 */
TCMPLX_A_API
size_t tcmplxA_ctxtmap_block_types(struct tcmplxA_ctxtmap const* x);
/**
 * @brief Inspect the number of contexts for a context map.
 * @param x the context map to inspect
 * @return a context count
 */
TCMPLX_A_API
size_t tcmplxA_ctxtmap_contexts(struct tcmplxA_ctxtmap const* x);
/**
 * @brief Access the contiguous storage for this map.
 * @param x the context map to inspect
 * @return a pointer to entry zero-zero in the map
 */
TCMPLX_A_API
unsigned char* tcmplxA_ctxtmap_data(struct tcmplxA_ctxtmap* x);
/**
 * @brief Access the contiguous storage for this map.
 * @param x the context map to inspect
 * @return a pointer to entry zero-zero in the map
 */
TCMPLX_A_API
unsigned char const* tcmplxA_ctxtmap_data_c(struct tcmplxA_ctxtmap const* x);

/**
 * @brief Inspect the prefix code identifier for a context map.
 * @param x the context map to inspect
 * @param i row (block type) selector
 * @param j column (context) selector
 * @return the prefix code identifier at the given coordinates
 */
TCMPLX_A_API
int tcmplxA_ctxtmap_get(struct tcmplxA_ctxtmap const* x, size_t i, size_t j);
/**
 * @brief Modify the prefix code identifier for a context map.
 * @param x the context map to inspect
 * @param i row (block type) selector
 * @param j column (context) selector
 * @param v new prefix code identifier
 */
TCMPLX_A_API
void tcmplxA_ctxtmap_set(struct tcmplxA_ctxtmap* x, size_t i, size_t j, int v);

/**
 * @brief Inspect the context mode for a block type in a context map.
 * @param x the context map to inspect
 * @param i row (block type) selector
 * @return the context mode at the given coordinates
 */
TCMPLX_A_API
int tcmplxA_ctxtmap_get_mode(struct tcmplxA_ctxtmap const* x, size_t i);
/**
 * @brief Modify the context mode for a block type in a context map.
 * @param x the context map to inspect
 * @param i row (block type) selector
 * @param v new context mode
 */
TCMPLX_A_API
void tcmplxA_ctxtmap_set_mode(struct tcmplxA_ctxtmap* x, size_t i, int v);

/**
 * @brief Calculate a distance context from a copy length.
 * @param copylen a copy length
 * @return a distance context on success, negative otherwise
 */
TCMPLX_A_API
int tcmplxA_ctxtmap_distance_context(unsigned long int copylen);

/**
 * @brief Calculate a literal context from recent history.
 * @param mode a context map mode
 * @param p1 most recent byte
 * @param p2 the byte before the most recent
 * @return a literal context on success, negative otherwise
 */
TCMPLX_A_API
int tcmplxA_ctxtmap_literal_context
  (int mode, unsigned int p1, unsigned int p2);

/**
 * @brief Apply a move-to-front transform to the map.
 * @param x the map to modify
 * @note This function intends to reverse the inverse
 *   move-to-front transform described in RFC 7932 section 7.3.
 */
TCMPLX_A_API
void tcmplxA_ctxtmap_apply_movetofront(struct tcmplxA_ctxtmap* x);
/**
 * @brief Apply the Brotli inverse move-to-front transform to the map.
 * @param x the map to modify
 * @see RFC 7932 section 7.3.
 */
TCMPLX_A_API
void tcmplxA_ctxtmap_revert_movetofront(struct tcmplxA_ctxtmap* x);
/* END   context map */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_CtxtMap_H_*/
