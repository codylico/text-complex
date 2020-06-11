/**
 * @file tcmplx-access/inscopy.h
 * @brief table for insert and copy lengths
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_InsCopy_H_
#define hg_TextComplexAccess_InsCopy_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Type of insert copy table to generate.
 */
enum tcmplxA_inscopy_preset {
  /**
   * @brief Select the DEFLATE 285-code literal-length alphabet.
   */
  tcmplxA_InsCopy_Deflate = 0,
  /**
   * @brief Select the Brotli 704-code insert-and-copy-length alphabet.
   */
  tcmplxA_InsCopy_Brotli = 1
};

/**
 * @brief Type of insert copy table row.
 */
enum tcmplxA_inscopy_type {
  /**
   * @brief DEFLATE literal value.
   */
  tcmplxA_InsCopy_Literal = 0,
  /**
   * @brief DEFLATE block stop code.
   */
  tcmplxA_InsCopy_Stop = 1,
  /**
   * @brief DEFLATE insert length.
   */
  tcmplxA_InsCopy_Insert = 2,
  /**
   * @brief Brotli insert-copy length code.
   */
  tcmplxA_InsCopy_InsertCopy = 3
};

/**
 * @brief Single row of an insert copy table.
 */
struct tcmplxA_inscopy_row {
  /**
   * @brief What type of value this row indicates.
   */
  unsigned char type;
  /**
   * @brief Whether this row indicates a reused (thus omitted)
   *   back distance.
   */
  unsigned char zero_distance_tf;
  /**
   * @brief Number of extra bits for the (Brotli) insert code or
   *   the (DEFLATE) length code.
   */
  unsigned char insert_bits;
  /**
   * @brief Number of extra bits for the copy code. Zero for DEFLATE.
   */
  unsigned char copy_bits;
  /**
   * @brief First value for insert length.
   */
  unsigned short insert_first;
  /**
   * @brief First value for copy length.
   */
  unsigned short copy_first;
};

/**
 * @brief table for insert and copy lengths.
 */
struct tcmplxA_inscopy;

/* BEGIN insert copy table */
/**
 * @brief Construct a new insert copy table.
 * @param t a @link tcmplxA_inscopy_type @endlink value
 * @return a pointer to the insert copy table on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_inscopy* tcmplxA_inscopy_new(int t);

/**
 * @brief Destroy a insert copy table.
 * @param x (nullable) the insert copy table to destroy
 */
TCMPLX_A_API
void tcmplxA_inscopy_destroy(struct tcmplxA_inscopy* x);

/**
 * \brief Query the size of the table.
 * \return the number of rows in this table
 */
TCMPLX_A_API
size_t tcmplxA_inscopy_size(struct tcmplxA_inscopy const* x);

/**
 * \brief Read from an insert copy table.
 * \param x the table to read
 * \param i an array index
 * \return a pointer to a table row on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_inscopy_row const* tcmplxA_inscopy_at_c
  (struct tcmplxA_inscopy const* x, size_t i);

/**
 * \brief Copy an insert copy table.
 * \param dst destination table instance
 * \param src source table instance
 * \return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_inscopy_copy
  (struct tcmplxA_inscopy* dst, struct tcmplxA_inscopy const* src);
/* END   insert copy table */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_InsCopy_H_*/
