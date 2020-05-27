/**
 * @file tests/testfont.h
 * @brief Test fonts.
 */
#ifndef hg_TCMPLXA_TESTFONT_H_
#define hg_TCMPLXA_TESTFONT_H_

#include <stddef.h>

struct MunitArgument_;

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

struct tcmplxAtest_arg {
  char* font_path;
};

/* Parses stuff like (len) '*' (repeat_count) [',' ...]
 * ex:
 *   "7*12,9*100,8,1,0*10,12*2"
 */
struct tcmplxAtest_fixlist_lex {
  char const* p;
  size_t total;
  size_t left;
  int prefix_len;
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

/**
 * @brief Duplicate a string.
 * @param[out] out output pointer
 * @param arg string to copy, or NULL to copy nothing
 * @return zero on success, nonzero otherwise
 */
int tcmplxAtest_strdup(char** out, char const* arg);

/**
 * @brief Initialize a testfont argument set.
 * @param tfa the set to initialize
 * @return zero
 */
int tcmplxAtest_arg_init(struct tcmplxAtest_arg* tfa);

/**
 * @brief Close out a testfont argument set.
 * @param tfa the set to close
 */
void tcmplxAtest_arg_close(struct tcmplxAtest_arg* tfa);

/**
 * @brief Get an argument list for munit-plus.
 * @return the argument list
 */
struct MunitArgument_ const* tcmplxAtest_get_args(void);

/**
 * @brief Start a code string parsing.
 * @param x parser struct
 * @param s the string to parse
 * @return zero on success, other nonzero on parse error
 */
int tcmplxAtest_fixlist_lex_start
  (struct tcmplxAtest_fixlist_lex *x, char const* s);

/**
 * @brief Acquire the next code length from a string.
 * @param x parser struct
 * @return a length, or -1 at end of parser
 */
int tcmplxAtest_fixlist_lex_next(struct tcmplxAtest_fixlist_lex *x);

/**
 * @brief Like `munit_rand_int_range`, but protects against
 *   empty intervals.
 * @param a min
 * @param b max
 * @return a number
 */
int testfont_rand_int_range(int a, int b);
/**
 * @brief Like `munit_rand_int_range`, but protects against
 *   empty intervals.
 * @param a min
 * @param b max
 * @return a number
 */
size_t testfont_rand_size_range(size_t a, size_t b);


#endif /*hg_TCMPLXA_TESTFONT_H_*/
