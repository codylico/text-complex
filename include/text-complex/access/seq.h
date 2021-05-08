/**
 * @file text-complex/access/seq.h
 * @brief Adapter providing sequential access to bytes from a mmaptwo
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_Seq_H_
#define hg_TextComplexAccess_Seq_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct mmaptwo_i;

struct tcmplxA_seq;

/* BEGIN sequential whence */
/**
 * @brief Sequential seek starting position.
 */
enum tcmplxA_seq_whence {
  /**
   * @brief Count from zero.
   */
  tcmplxA_SeqSet = 0,
  /**
   * @brief Count from current position.
   */
  tcmplxA_SeqCur = 1,
  /**
   * @brief Count from the end.
   */
  tcmplxA_SeqEnd = 2
};
/* END   sequential whence */

/* BEGIN sequential */
/**
 * @brief Construct a new sequential.
 * @param x mmaptwo instance
 * @return a pointer to the sequential on success, NULL otherwise
 * @note Does not take control of the mmaptwo instance's lifetime.
 */
TCMPLX_A_API
struct tcmplxA_seq* tcmplxA_seq_new(struct mmaptwo_i* x);

/**
 * @brief Destroy a sequential.
 * @param x (nullable) the sequential to destroy
 * @note Does not destroy the mmaptwo instance.
 */
TCMPLX_A_API
void tcmplxA_seq_destroy(struct tcmplxA_seq* x);

/**
 * @brief Query a sequential's current read position.
 * @param x the sequential to query
 * @return a read position in bytes
 */
TCMPLX_A_API
size_t tcmplxA_seq_get_pos(struct tcmplxA_seq const* x);

/**
 * @brief Configure the read position.
 * @param x the sequential to configure
 * @param i a read position
 * @return the new position, ~0 otherwise
 */
TCMPLX_A_API
size_t tcmplxA_seq_set_pos(struct tcmplxA_seq* x, size_t i);

/**
 * @brief Configure the read position.
 * @param x the sequential to configure
 * @param i a read position
 * @param whence a @link tcmplxA_seq_whence @endlink value
 * @return the new position on success, -1 if the position is too
 *   large for a `long int`, other negative value on error
 */
TCMPLX_A_API
long int tcmplxA_seq_seek(struct tcmplxA_seq* x, long int i, int whence);

/**
 * @brief Read a single byte from the sequential.
 * @return the byte on success, -1 at end of stream, -2 otherwise
 */
TCMPLX_A_API
int tcmplxA_seq_get_byte(struct tcmplxA_seq* x);
/* END   sequential */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_Seq_H_*/
