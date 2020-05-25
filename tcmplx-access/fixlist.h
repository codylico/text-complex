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
 * @brief Query the size of the table.
 * @return the number of lines in this table
 */
TCMPLX_A_API
size_t tcmplxA_fixlist_size(struct tcmplxA_fixlist const* x);

/**
 * @brief Write to an offset table.
 * @param x the table to write
 * @param i an array index
 * @return a pointer to an offset line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixline* tcmplxA_fixlist_at
  (struct tcmplxA_fixlist* x, size_t i);

/**
 * @brief Read from an offset table.
 * @param x the table to read
 * @param i an array index
 * @return a pointer to an offset line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixline const* tcmplxA_fixlist_at_c
  (struct tcmplxA_fixlist const* x, size_t i);

/**
 * @brief Copy an offset table.
 * @param dst destination table instance
 * @param src source table instance
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_fixlist_copy
  (struct tcmplxA_fixlist* dst, struct tcmplxA_fixlist const* src);
/* END   prefix list */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_FixList_H_*/
