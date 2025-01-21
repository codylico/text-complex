/**
 * @file text-complex/access/gaspvec.h
 * @brief Forest of prefix trees
 */
#ifndef hg_TextComplexAccess_GaspVec_H_
#define hg_TextComplexAccess_GaspVec_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Vector of prefix trees.
 */
struct tcmplxA_gaspvec;

/**
 * @brief Construct a new prefix gasp vector.
 * @param n desired number of lines in the table
 * @return a pointer to the prefix gasp vector on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_gaspvec* tcmplxA_gaspvec_new(size_t n);

/**
 * @brief Destroy a prefix gasp vector.
 * @param x (nullable) the prefix  gasp vector to destroy
 */
TCMPLX_A_API
void tcmplxA_gaspvec_destroy(struct tcmplxA_gaspvec* x);


/**
 * @brief Query the size of the gasp vector.
 * @param x vector to query
 * @return the number of prefix lists in this vector
 */
TCMPLX_A_API
size_t tcmplxA_gaspvec_size(struct tcmplxA_gaspvec const* x);

/**
 * @brief Write to a prefix list in a gasp vector.
 * @param x the vector to write
 * @param i an array index
 * @return a pointer to a prefix list on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixlist* tcmplxA_gaspvec_at
  (struct tcmplxA_gaspvec* x, size_t i);

/**
 * @brief Read from a prefix list in a gasp vector.
 * @param x the vector to read
 * @param i an array index
 * @return a pointer to a prefix list on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_fixlist const* tcmplxA_gaspvec_at_c
  (struct tcmplxA_gaspvec const* x, size_t i);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_GaspVec_H_*/
