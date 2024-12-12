/**
 * @file text-complex/access/ctxtspan.h
 * @brief Context span heuristic for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_CtxtSpan_H_
#define hg_TextComplexAccess_CtxtSpan_H_

#include "ctxtmap.h"
#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

enum tcmplxA_ctxtspan_expr {
    tcmplxA_CtxtSpan_Size = 16
};

/** @brief Context score vector. */
struct tcmplxA_ctxtscore {
    /** Literal context fitness scores, one per mode. */
    unsigned vec[tcmplxA_CtxtMap_ModeMax];
};

/** @brief Context mode spans. */
struct tcmplxA_ctxtspan {
    /** @brief Total bytes covered. */
    size_t total_bytes;
    /** @brief Starting offset for each span. */
    size_t offsets[tcmplxA_CtxtSpan_Size];
    /** @brief Context mode for each span. */
    unsigned char modes[tcmplxA_CtxtSpan_Size];
    /** @brief Number of offsets in the span. */
    size_t count;
};

/**
 * @brief Calculate a literal context fitness score.
 * @param[in,out] results score accumulation vector;
 *   zero-fill before for initial call
 * @param buf array of bytes to parse
 * @param buf_len length of array in bytes
 */
TCMPLX_A_API
void tcmplxA_ctxtspan_guess(struct tcmplxA_ctxtscore* results,
    void const* buf, size_t buf_len);

/**
 * @brief Calculate context modes for parts of a byte array.
 * @param[out] results new modes for the byte array
 * @param buf array of bytes to parse
 * @param buf_len length of array in bytes
 * @param margin maximum score difference to allow for bad coalescing
 */
TCMPLX_A_API
void tcmplxA_ctxtspan_subdivide(struct tcmplxA_ctxtspan* results,
    void const* buf, size_t buf_len, unsigned margin);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_CtxtSpan_H_*/
