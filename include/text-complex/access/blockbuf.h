/**
 * @file text-complex/access/blockbuf.h
 * @brief DEFLATE block buffer
 */
#ifndef hg_TextComplexAccess_BlockBuf_H_
#define hg_TextComplexAccess_BlockBuf_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief DEFLATE block buffer.
 */
struct tcmplxA_blockbuf;

/* BEGIN block buffer */
/**
 * @brief Construct a new block buffer.
 * @param block_size amount of input data to process at once
 * @param n maximum sliding window size
 * @param chain_length run-time parameter limiting hash chain length
 * @param bdict_tf whether to use the built-in dictionary
 * @return a pointer to the block buffer on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_blockbuf* tcmplxA_blockbuf_new
  ( tcmplxA_uint32 block_size, tcmplxA_uint32 n, size_t chain_length,
    int bdict_tf);

/**
 * @brief Destroy a block buffer.
 * @param x (nullable) the block buffer to destroy
 */
TCMPLX_A_API
void tcmplxA_blockbuf_destroy(struct tcmplxA_blockbuf* x);

/**
 * @brief Process the current block of input bytes.
 * @param x the block buffer to do the processing
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The input buffer is cleared on success.
 */
TCMPLX_A_API
int tcmplxA_blockbuf_gen_block(struct tcmplxA_blockbuf* x);

/**
 * @brief Look at the output.
 * @param x the block buffer to do the processing
 * @return the number of bytes in the output buffer
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_blockbuf_output_size(struct tcmplxA_blockbuf const* x);

/**
 * @brief Look at the output.
 * @param x the block buffer to >>>
 * @return the bytes in the output buffer
 * @note Format: @verbatim
  Each command represented as a sequence of bytes
    [X][...]
  where
    X<128 represents an insert command, and
    X>=128 represents a copy command.
  Either way, the length is encoded as such:
    (X&64) == 0 -> length is (X&63)
    (X&64) == 64 -> length is ((X&63)<<8) + (Y&255) + 64
  where Y is the byte immediately following X.

  Insert commands are made up of an [X] or [X][Y] sequence
  followed by the literal bytes. A copy command is made up of
  an [X] or [X][Y] sequence followed by an encoded distance
  value.

  Distance values are represented as a sequence of bytes
    [R][...]
  where
    R<128 represents a bdict reference,
    (R&192) == 128 represents a 14-bit distance
    R>=192 represents a 30-bit distance.
  A bdict reference uses two extra bytes:
    [R][B1][B2]
  The filter is encoded in R as (R&127), the word index as (B1<<8)+B2,
  and the word length is the copy length.

  The 14-bit distance can be extracted from a two-byte sequence:
    [R][Q]
  as ((R&63)<<8) + (Q&255).

  The 30-bit distance can be extracted from a four-byte sequence:
    [R][S1][S2][S3]
  as ((R&63)<<24) + ((S1&255)<<16) + ((S2&255)<<8) + (S3&255) + 16384.
 @endverbatim
 *
 * @note Examples (values in hexadecimal): @verbatim
 (03)(41)(62)(63)         -> "Abc"
 (83)(80)(01)             -> copy 3 bytes, distance 1
 (01)(54)(83)(80)(01)     -> "TTTT"
 (C0)(05)(90)(02)         -> copy 69 bytes, distance 4098
 (C0)(06)(C0)(00)(00)(03) -> copy 70 bytes, distance 16387
 (84)(05)(00)(02)         -> "life the " (bdict length 4 filter 5 word 2)
 @endverbatim
 */
TCMPLX_A_API
unsigned char const* tcmplxA_blockbuf_output_data
  (struct tcmplxA_blockbuf const* x);

/**
 * @brief Clear the output buffer.
 * @param x the block buffer to edit
 */
TCMPLX_A_API
void tcmplxA_blockbuf_clear_output(struct tcmplxA_blockbuf* x);

/**
 * @brief Add some input bytes.
 * @param x the block buffer to do the processing
 * @param buf data to add to the input
 * @param sz size of the added input
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_blockbuf_write
  (struct tcmplxA_blockbuf* x, unsigned char const* buf, size_t sz);

/**
 * @brief Check how many input bytes await processing.
 * @param x the block buffer to do the processing
 * @return a byte count
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_blockbuf_input_size(struct tcmplxA_blockbuf const* x);

/**
 * @brief Check how many input bytes can be processed at once.
 * @param x the block buffer to do the processing
 * @return a byte count
 */
TCMPLX_A_API
tcmplxA_uint32 tcmplxA_blockbuf_capacity(struct tcmplxA_blockbuf const* x);
/* END   block buffer */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BlockBuf_H_*/
