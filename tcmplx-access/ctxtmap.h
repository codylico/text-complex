/**
 * @file tcmplx-access/ctxtmap.h
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
 * @brief Inspect the number of contexts for a context map.
 * @param x the context map to inspect
 * @return a context count
 */
TCMPLX_A_API
unsigned char* tcmplxA_ctxtmap_data(struct tcmplxA_ctxtmap* x);
/**
 * @brief Inspect the number of contexts for a context map.
 * @param x the context map to inspect
 * @return a context count
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
/* END   context map */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_CtxtMap_H_*/
