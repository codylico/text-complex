/**
 * @file tests/testfont.h
 * @brief Test fonts.
 */
#ifndef hg_TCMPLXA_TESTFONT_H_
#define hg_TCMPLXA_TESTFONT_H_

#include <stddef.h>

enum tcmplxAtest_testfont {
  tcmplxAtest_OctetMIN = 0,
  /**
   * @brief Bytes with value zero.
   */
  tcmplxAtest_Zero = 0,
  /**
   * @brief Bytes in ascending order.
   */
  tcmplxAtest_Ascending = 1,
  /**
   * @brief Bytes in descending order.
   */
  tcmplxAtest_Descending = 2,
  /**
   * @brief Bytes in pseudo-random order.
   */
  tcmplxAtest_Pseudorandom = 3,
  tcmplxAtest_OctetMAX = 3,
  tcmplxAtest_MAX
};

/**
 * @brief Generate some data.
 * @param n @link tcmplxAtest_testfont @endlink
 * @param i array index
 * @param seed random seed
 * @return the numeric value, or -1 at end of file
 */
int tcmplxAtest_gen_datum(int n, size_t i, unsigned int seed);

/**
 * @brief Query the maximum length of a data source.
 * @param n @link tcmplxAtest_testfont @endlink
 * @return a length, or ~0 if unbounded
 */
size_t tcmplxAtest_gen_size(int n);

/**
 * @brief Generate a memory-backed mapper.
 * @param n @link tcmplxAtest_testfont @endlink
 * @param maxsize maximum length of the mapper
 * @return the mapper on success, `nullptr` otherwise
 */
struct mmaptwo_i* tcmplxAtest_gen_maptwo
  (int n, size_t maxsize, unsigned int seed);

#endif /*hg_TCMPLXA_TESTFONT_H_*/
