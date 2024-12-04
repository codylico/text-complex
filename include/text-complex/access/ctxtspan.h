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

/** @brief Context score vector. */
struct tcmplxA_ctxtscore {
    /** Literal context fitness scores, one per mode. */
    unsigned vec[tcmplxA_CtxtMap_ModeMax];
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

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_CtxtSpan_H_*/
