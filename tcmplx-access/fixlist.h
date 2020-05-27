/**
 * @file tcmplx-access/fixlist.h
 * @brief Prefix code list
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_FixList_H_
#define hg_TextComplexAccess_FixList_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* BEGIN prefix preset identifiers */
/**
 * @brief Identifiers for prefix code list presets.
 */
enum tcmplxA_fix_preset {
  /**
   * @brief Alphabet for Brotli complex prefix codes.
   */
  tcmplxA_FixList_BrotliComplex = 0
};
/* END   prefix preset identifiers */


/* BEGIN prefix code line */
/**
 * @brief A single line from a prefix code list.
 */
struct tcmplxA_fixline {
  unsigned short int code;
  unsigned short int len;
  unsigned long int value;
};
/* END   prefix code line */

/**
 * @brief Prefix code list
 */
struct tcmplxA_fixlist;

/* BEGIN prefix list */
/**
 * @brief Construct a new prefix list.
 * @param n desired number of lines in the table
 * @return a pointer to the prefix list on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixlist* tcmplxA_fixlist_new(size_t n);

/**
 * @brief Destroy a prefix list.
 * @param x (nullable) the prefix list to destroy
 */
TCMPLX_A_API
void tcmplxA_fixlist_destroy(struct tcmplxA_fixlist* x);


/**
 * @brief Query the size of the list.
 * @return the number of lines in this list
 */
TCMPLX_A_API
size_t tcmplxA_fixlist_size(struct tcmplxA_fixlist const* x);

/**
 * @brief Write to a prefix list.
 * @param x the table to write
 * @param i an array index
 * @return a pointer to a code line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixline* tcmplxA_fixlist_at
  (struct tcmplxA_fixlist* x, size_t i);

/**
 * @brief Read from a prefix list.
 * @param x the list to read
 * @param i an array index
 * @return a pointer to a code line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixline const* tcmplxA_fixlist_at_c
  (struct tcmplxA_fixlist const* x, size_t i);

/**
 * @brief Copy a prefix list.
 * @param dst destination list instance
 * @param src source list instance
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_fixlist_copy
  (struct tcmplxA_fixlist* dst, struct tcmplxA_fixlist const* src);

/**
 * @brief Generate prefix codes given a prefix list.
 * @param dst list to populate with codes
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_fixlist_gen_codes(struct tcmplxA_fixlist* dst);

/**
 * @brief Assign a prefix list with a preset code-value list.
 * @param dst list to populate with lengths
 * @param i preset identifier
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_fixlist_preset(struct tcmplxA_fixlist* dst, unsigned int i);
/* END   prefix list */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_FixList_H_*/
