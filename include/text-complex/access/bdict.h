/**
 * @file text-complex/access/bdict.h
 * @brief Built-in dictionary
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_BDict_H_
#define hg_TextComplexAccess_BDict_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* BEGIN built-in dictionary */
/**
 * @brief Acquire the number of words for a given word length.
 * @param j a word length in bytes
 * @return a word count for the given length, or zero if unsupported
 */
TCMPLX_A_API
unsigned int tcmplxA_bdict_word_count(unsigned int j);

/**
 * @brief Acquire a word.
 * @param j a word length in bytes
 * @param i word array index
 * @return a pointer to a word on success, NULL otherwise
 */
TCMPLX_A_API
unsigned char const* tcmplxA_bdict_get_word(unsigned int j, unsigned int i);

/**
 * @brief Transform a word.
 * @param[in,out] buf a buffer holding the word (at least 38 chars)
 * @param[in,out] len length of word in bytes
 * @param k transform selector (in range `[0,121)`)
 * @return tcmplxA_Success on success, tcmplxA_ErrParam on unrecognized
 *   transform, tcmplxA_ErrMemory if the resulting word is too long
 */
TCMPLX_A_API
int tcmplxA_bdict_transform
  (unsigned char* buf, unsigned int* len, unsigned int k);
/* END   built-in dictionary */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BDict_H_*/
