/**
 * @file text-complex/access/fixlist.h
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
  tcmplxA_FixList_BrotliComplex = 0,
  /**
   * @brief Code lengths for Brotli simple prefix list `(NSYM=1)`.
   * @note Sort the values before applying the prefix codes.
   *   Use @link tcmplxA_fixlist_gen_codes @endlink if necessary.
   */
  tcmplxA_FixList_BrotliSimple1 = 1,
  /**
   * @brief Code lengths for Brotli simple prefix list `(NSYM=2)`.
   * @note Sort the values before applying the prefix codes.
   *   Use @link tcmplxA_fixlist_gen_codes @endlink if necessary.
   */
  tcmplxA_FixList_BrotliSimple2 = 2,
  /**
   * @brief Code lengths for Brotli simple prefix list `(NSYM=3)`.
   * @note Sort the values before applying the prefix codes.
   *   Use @link tcmplxA_fixlist_gen_codes @endlink if necessary.
   */
  tcmplxA_FixList_BrotliSimple3 = 3,
  /**
   * @brief Code lengths for Brotli simple prefix list
   *   `(NSYM=4, tree-select bit 0)`.
   * @note Sort the values before applying the prefix codes.
   *   Use @link tcmplxA_fixlist_gen_codes @endlink if necessary.
   */
  tcmplxA_FixList_BrotliSimple4A = 4,
  /**
   * @brief Code lengths for Brotli simple prefix list
   *   `(NSYM=4, tree-select bit 1)`.
   * @note Sort the values before applying the prefix codes.
   *   Use @link tcmplxA_fixlist_gen_codes @endlink if necessary.
   */
  tcmplxA_FixList_BrotliSimple4B = 5
};
/* END   prefix preset identifiers */


/* BEGIN prefix code line */
/**
 * @brief A single line from a prefix code list.
 */
struct tcmplxA_fixline {
  /** @brief Alphabet code for a prefix. */
  unsigned short int code;
  /** @brief Length of prefix in bits. */
  unsigned short int len;
  /** @brief Prefix bits. */
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
 *
 * This function fills in the bit strings of each prefix code,
 *   using the corresponding @link tcmplxA_fixline#len @endlink values.
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

/**
 * @brief Generate prefix code lengths given a prefix list and
 *   histogram of code frequencies.
 * @param dst list to populate with code lengths
 * @param table frequency histogram, flat array as long as number
 *   of prefixes in the list
 * @param max_bits maximum output length
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_fixlist_gen_lengths
  ( struct tcmplxA_fixlist* dst, tcmplxA_uint32 const* table,
    unsigned int max_bits);
/* END   prefix list */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_FixList_H_*/
