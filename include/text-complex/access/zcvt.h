/**
 * @file text-complex/access/zcvt.h
 * @brief zlib conversion state
 */
#ifndef hg_TextComplexAccess_ZCvt_H_
#define hg_TextComplexAccess_ZCvt_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief zlib conversion state.
 */
struct tcmplxA_zcvt;

/* BEGIN zcvt state */
/**
 * @brief Construct a new zlib conversion state.
 * @param block_size amount of input data to process at once
 * @param n maximum sliding window size
 * @param chain_length run-time parameter limiting hash chain length
 * @return a pointer to the zlib conversion state on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_zcvt* tcmplxA_zcvt_new
  ( tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length);

/**
 * @brief Destroy a zlib conversion state.
 * @param x (nullable) the zlib conversion state to destroy
 */
TCMPLX_A_API
void tcmplxA_zcvt_destroy(struct tcmplxA_zcvt* x);

/**
 * @brief Convert a zlib stream to a byte stream.
 * @param ps the zlib conversion state to use
 * @param[out] ret number of (inflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @param[in,out] src pointer to source bytes to inflate
 * @param src_end pointer to end of source buffer
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 *
 * @note This function returns tcmplxA_ErrZDictionary if the input
 *   stream expects a ZLIB dictionary. If the caller expects to
 *   supply a dictionary, use @link tcmplxA_zcvt_bypass @endlink
 *   to add the dictionary corresponding to the state's `checksum` field.
 * @see @link tcmplxA_zcvt_checksum @endlink
 */
TCMPLX_A_API
int tcmplxA_zcvt_zsrtostr
  ( struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end);

/**
 * @brief Query the checksum field of a conversion state.
 * @param x the conversion state to query
 * @return the value of the checksum field.
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_zcvt_checksum(struct tcmplxA_zcvt const* x);

/**
 * @brief Add dictionary data outside of the input stream.
 * @param x the conversion state to configure
 * @param buf buffer of bytes to add
 * @param sz size of the buffer in bytes
 * @return count of bytes added
 */
TCMPLX_A_API
size_t tcmplxA_zcvt_bypass
  (struct tcmplxA_zcvt* x, unsigned char const* buf, size_t sz);

/**
 * @brief Convert a byte stream to a zlib stream.
 * @param ps the zlib conversion state to use
 * @param[out] ret number of (deflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @param[in,out] src pointer to source bytes to deflate
 * @param src_end pointer to end of source buffer
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 */
TCMPLX_A_API
int tcmplxA_zcvt_strrtozs
  ( struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end);

/**
 * @brief Add end-of-stream delimiter to a zlib stream.
 * @param ps the zlib conversion state to use
 * @param[out] ret number of (deflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @return tcmplxA_Success on success,
 *   tcmplxA_EOF at end of stream, other nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 *
 * @note Any bytes remaining in the conversion state will
 *   be processed before outputting the delimiter (Adler32 checksum).
 */
TCMPLX_A_API
int tcmplxA_zcvt_delimrtozs
  (struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz);
/* END   zcvt state */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_ZCvt_H_*/
