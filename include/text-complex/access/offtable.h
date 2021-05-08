/**
 * \file text-complex/access/offtable.h
 * \brief Access point for TrueType files
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_OffTable_H_
#define hg_TextComplexAccess_OffTable_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct tcmplxA_offtable;

/* BEGIN offset line */
/**
 * \brief Single row of the offset table.
 *
 * Each row describes a data table in a TrueType font file.
 */
struct tcmplxA_offline {
  /**
   * \brief Single row of the offset table.
   *
   * Each row describes a data table in a TrueType font file.
   */
  unsigned char tag[4];
  /**
   * \brief Table name.
   */
  tcmplxA_uint32 checksum;
  /**
   * \brief Table checksum.
   */
  tcmplxA_uint32 offset;
  /**
   * \brief Length of table in bytes.
   */
  tcmplxA_uint32 length;
};
/* END   offset line */

/* BEGIN offset table */
/**
 * \brief Construct a new offset table.
 * \param n desired number of lines in the table
 * \return a pointer to the table on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_offtable* tcmplxA_offtable_new(size_t n);

/**
 * \brief Destroy an offset table.
 * \param x (nullable) the table to destroy
 */
TCMPLX_A_API
void tcmplxA_offtable_destroy(struct tcmplxA_offtable* x);

/**
 * \brief Query the size of the table.
 * \return the number of lines in this table
 */
TCMPLX_A_API
size_t tcmplxA_offtable_size(struct tcmplxA_offtable const* x);

/**
 * \brief Write to an offset table.
 * \param x the table to write
 * \param i an array index
 * \return a pointer to an offset line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_offline* tcmplxA_offtable_at
  (struct tcmplxA_offtable* x, size_t i);

/**
 * \brief Read from an offset table.
 * \param x the table to read
 * \param i an array index
 * \return a pointer to an offset line on success, NULL otherwise
 */
TCMPLX_A_API
struct tcmplxA_offline const* tcmplxA_offtable_at_c
  (struct tcmplxA_offtable const* x, size_t i);

/**
 * \brief Copy an offset table.
 * \param dst destination table instance
 * \param src source table instance
 * \return tcmplxA_Success on success, nonzero otherwise
 */
TCMPLX_A_API
int tcmplxA_offtable_copy
  (struct tcmplxA_offtable* dst, struct tcmplxA_offtable const* src);
/* END   offset table */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_OffTable_H_*/
