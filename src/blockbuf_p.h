/**
 * @file text-complex/access/blockbuf_p.h
 * @brief DEFLATE block buffer
 */
#ifndef hg_TextComplexAccess_BlockBuf_pH_
#define hg_TextComplexAccess_BlockBuf_pH_

#include "text-complex/access/api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct tcmplxA_blockstr {
  unsigned char* p;
  tcmplxA_uint32 sz;
  tcmplxA_uint32 cap;
};

/**
 * @brief Initialize a block string.
 * @param x the block string to initialize
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_blockstr_init(struct tcmplxA_blockstr* x, tcmplxA_uint32 cap);
/**
 * @brief Close a block string.
 * @param x the block string to close
 */
void tcmplxA_blockstr_close(struct tcmplxA_blockstr* x);
/**
 * @brief Close a block string.
 * @param x the block string to extend
 * @param sz new capacity
 * @return tcmplxA_Success on success
 */
int tcmplxA_blockstr_reserve(struct tcmplxA_blockstr* x, tcmplxA_uint32 sz);
/**
 * @brief Close a block string.
 * @param x the block string to close
 * @param buf bytes to add
 * @param n number of bytes to add
 * @return tcmplxA_Success on success
 */
int tcmplxA_blockstr_append
  (struct tcmplxA_blockstr* x, unsigned char const* buf, size_t n);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BlockBuf_pH_*/
