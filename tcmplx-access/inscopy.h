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
enum tcmplxA_inscp_preset {
  /**
   * @brief Select the DEFLATE 286-code literal-length alphabet.
   */
  tcmplxA_InsCopy_Deflate = 0,
  /**
   * @brief Select the Brotli 704-code insert-and-copy-length alphabet.
   */
  tcmplxA_InsCopy_BrotliIC = 1,
  /**
   * @brief Select the Brotli 26-code block count alphabet.
   */
  tcmplxA_InsCopy_BrotliBlock = 2
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
   * @brief DEFLATE copy length.
   */
  tcmplxA_InsCopy_Copy = 2,
  /**
   * @brief Brotli insert-copy length code.
   */
  tcmplxA_InsCopy_InsertCopy = 3,
  /**
   * @brief Brotli block count code.
   */
  tcmplxA_InsCopy_BlockCount = 4,
  /**
   * @brief DEFLATE copy length, minus one valid value in the extra bits.
   * @note DEFLATE code 284 is in this category, accepting one less than
   *   the proper five-bit extra range (i.e. 227-258) according to RFC1951.
   * @note This type compares as equal to @link tcmplxA_InsCopy_Copy @endlink
   *   for length-based sorting.
   */
  tcmplxA_InsCopy_CopyMinus1 = 130
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
  /**
   * @brief Alphabet value.
   */
  unsigned short int code;
};

/**
 * @brief table for insert and copy lengths.
 */
struct tcmplxA_inscopy;

/* BEGIN insert copy table */
/**
 * @brief Construct a new insert copy table.
 * @param n table row count
 * @return a pointer to the insert copy table on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_inscopy* tcmplxA_inscopy_new(size_t n);

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
 * \brief Access a row of an insert copy table.
 * \param x the table to access
 * \param i an array index
 * \return a pointer to a table row on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_inscopy_row* tcmplxA_inscopy_at
  (struct tcmplxA_inscopy* x, size_t i);

/**
 * \brief Copy an insert copy table.
 * \param dst destination table instance
 * \param src source table instance
 * \return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_inscopy_copy
  (struct tcmplxA_inscopy* dst, struct tcmplxA_inscopy const* src);

/**
 * @brief Assign an insert copy table with a preset code-value list.
 * @param dst list to populate with codes
 * @param i a @link tcmplxA_inscopy_type @endlink value
 * @return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_inscopy_preset(struct tcmplxA_inscopy* dst, int i);

/**
 * @brief Sort an insert-copy table by alphabet code.
 * @param ict list to sort
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note Useful for decoding from a compressed stream.
 */
TCMPLX_A_API
int tcmplxA_inscopy_codesort(struct tcmplxA_inscopy* ict);

/**
 * @brief Sort an insert-copy table by insert length (primary)
 *   and copy length (secondary).
 * @param ict list to sort
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note Useful for encoding to a compressed stream.
 */
TCMPLX_A_API
int tcmplxA_inscopy_lengthsort(struct tcmplxA_inscopy* ict);

/**
 * @brief Encode a pair of insert and copy lengths using a table.
 * @param ict the table to read (sorted for length)
 * @param i insert length
 * @param c copy length, or zero if unused
 * @param z_tf nonzero to select the zero distance variation,
 *     zero for normal variation
 * @return an index of a matching table row if found, SIZE_MAX otherwise
 * @note The zero distance variation is only available for a small
 *   subset of preset insert-copy tables. If the zero distance variation
 *   is unavailable, try again with the default (nonzero distance)
 *   variation instead.
 */
TCMPLX_A_API
size_t tcmplxA_inscopy_encode
  ( struct tcmplxA_inscopy const* ict, unsigned long int i,
    unsigned long int c, int z_tf);
/* END   insert copy table */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_InsCopy_H_*/
