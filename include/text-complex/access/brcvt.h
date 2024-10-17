/**
 * @file text-complex/access/brcvt.h
 * @brief Brotli conversion state
 */
#ifndef hg_TextComplexAccess_BrCvt_H_
#define hg_TextComplexAccess_BrCvt_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct tcmplxA_brmeta;

/**
 * @brief Brotli conversion state.
 */
struct tcmplxA_brcvt;

/* BEGIN brcvt state */
/**
 * @brief Construct a new Brotli conversion state.
 * @param block_size amount of input data to process at once
 * @param n maximum sliding window size
 * @param chain_length run-time parameter limiting hash chain length
 * @return a pointer to the Brotli conversion state on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_brcvt* tcmplxA_brcvt_new
  ( tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length);

/**
 * @brief Destroy a Brotli conversion state.
 * @param x (nullable) the Brotli conversion state to destroy
 */
TCMPLX_A_API
void tcmplxA_brcvt_destroy(struct tcmplxA_brcvt* x);

/**
 * @brief Add metadata outside of the input stream.
 * @param x the conversion state to configure
 * @param buf buffer of bytes to add
 * @param sz size of the buffer in bytes
 * @return count of bytes added
 */
TCMPLX_A_API
size_t tcmplxA_brcvt_bypass
  (struct tcmplxA_brcvt* x, unsigned char const* buf, size_t sz);

/**
 * @brief Convert a Brotli stream to a byte stream.
 * @param ps the Brotli conversion state to use
 * @param[out] ret number of (inflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @param[in,out] src pointer to source bytes to inflate
 * @param src_end pointer to end of source buffer
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 */
TCMPLX_A_API
int tcmplxA_brcvt_zsrtostr
  ( struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end);

/**
 * @brief Convert a byte stream to a Brotli stream.
 * @param ps the Brotli conversion state to use
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
int tcmplxA_brcvt_strrtozs
  ( struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end);

/**
 * @brief Add end-of-stream delimiter to a Brotli stream.
 * @param ps the Brotli conversion state to use
 * @param[out] ret number of (deflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @return tcmplxA_Success on success,
 *   tcmplxA_EOF at end of stream, other nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 *
 * @note Any bytes remaining in the conversion state will
 *   be processed before outputting the delimiter (stream terminator bits).
 */
TCMPLX_A_API
int tcmplxA_brcvt_delimrtozs
  (struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz);

/**
 * @brief Flush a Brotli stream to a byte using an empty metadata block.
 * @param ps the Brotli conversion state to use
 * @param[out] ret number of (deflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether successful or failed.
 *
 * @note Any bytes remaining in the conversion state will
 *   be processed before outputting the metadata block.
 */
TCMPLX_A_API
int tcmplxA_brcvt_flush
  (struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz);

/**
 * @brief Access the metadata storage.
 * @param ps the Brotli conversion state to use
 * @return a pointer to the storage instance
 */
TCMPLX_A_API
struct tcmplxA_brmeta* tcmplxA_brcvt_metadata(struct tcmplxA_brcvt* ps);

/**
 * @brief Access the metadata storage.
 * @param ps the Brotli conversion state to use
 * @return a pointer to the storage instance
 */
TCMPLX_A_API
struct tcmplxA_brmeta const* tcmplxA_brcvt_metadata_c
  (struct tcmplxA_brcvt const* ps);
/* END   brcvt state */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BrCvt_H_*/
