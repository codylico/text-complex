/**
 * @brief Test program for distance ring
 */
#include "../tcmplx-access/ringdist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "munit/munit.h"
#include "testfont.h"


struct test_ringdist_params {
  int special_tf;
  unsigned int direct_count;
  unsigned int postfix_size;
  struct tcmplxA_ringdist* rd;
};

static struct { unsigned int code; unsigned int bits; }
const test_ringdist_bit_comp_1951[30] = {
    { 0, 0}, {1, 0}, {2, 0}, {3, 0},
    { 4, 1}, {5, 1}, {6, 2}, {7, 2},
    { 8, 3}, {9, 3},{10, 4},{11, 4},
    {12, 5},{13, 5},{14, 6},{15, 6},
    {16, 7},{17, 7},{18, 8},{19, 8},
    {20, 9},{21, 9},{22,10},{23,10},
    {24,11},{25,11},{26,12},{27,12},
    {28,13},{29,13}
  };

static MunitResult test_ringdist_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_ringdist_1951_bit_count
  (const MunitParameter params[], void* data);
static MunitResult test_ringdist_7932_bit_count
  (const MunitParameter params[], void* data);
static MunitResult test_ringdist_1951_decode
  (const MunitParameter params[], void* data);
static MunitResult test_ringdist_7932_decode
  (const MunitParameter params[], void* data);
static void* test_ringdist_1951_setup
    (const MunitParameter params[], void* user_data);
static void* test_ringdist_7932_setup
    (const MunitParameter params[], void* user_data);
static void test_ringdist_teardown(void* fixture);

static MunitParameterEnum test_ringdist_params[] = {
  { NULL, NULL },
};

static MunitTest tests_ringdist[] = {
  {"cycle", test_ringdist_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,test_ringdist_params},
  {"1951/bit_count", test_ringdist_1951_bit_count,
    test_ringdist_1951_setup,test_ringdist_teardown,0,NULL},
  {"7932/bit_count", test_ringdist_7932_bit_count,
    test_ringdist_7932_setup,test_ringdist_teardown,0,NULL},
  {"1951/decode", test_ringdist_1951_decode,
    test_ringdist_1951_setup,test_ringdist_teardown,0,NULL},
  {"7932/decode", test_ringdist_7932_decode,
    test_ringdist_7932_setup,test_ringdist_teardown,
    MUNIT_TEST_OPTION_TODO,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_ringdist = {
  "access/ringdist/", tests_ringdist, NULL, 1, 0
};




MunitResult test_ringdist_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ringdist* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_ringdist_new(0,4,0);
  munit_assert_not_null(ptr[0]);
  tcmplxA_ringdist_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_ringdist_1951_setup
    (const MunitParameter params[], void* user_data)
{
  struct test_ringdist_params *out = (struct test_ringdist_params*)calloc
    (1,sizeof(struct test_ringdist_params));
  if (out != NULL) {
    out->special_tf = 0;
    out->direct_count = 4;
    out->postfix_size = 0;
    out->rd = tcmplxA_ringdist_new
      (out->special_tf, out->direct_count, out->postfix_size);
    if (out->rd == NULL) {
      free(out);
      out = NULL;
    }
  }
  return out;
}

void* test_ringdist_7932_setup
    (const MunitParameter params[], void* user_data)
{
  struct test_ringdist_params *out = (struct test_ringdist_params*)calloc
    (1,sizeof(struct test_ringdist_params));
  if (out != NULL) {
    out->special_tf = 1;
    out->direct_count = testfont_rand_uint_range(0u,120u);
    out->postfix_size = testfont_rand_uint_range(0u,3u);
    out->rd = tcmplxA_ringdist_new
      (out->special_tf, out->direct_count, out->postfix_size);
    if (out->rd == NULL) {
      free(out);
      out = NULL;
    }
  }
  return out;
}

void test_ringdist_teardown(void* fixture) {
  struct test_ringdist_params *fixt = (struct test_ringdist_params*)fixture;
  if (fixt != NULL) {
    tcmplxA_ringdist_destroy(fixt->rd);
    free(fixt);
  }
  return;
}

MunitResult test_ringdist_1951_bit_count
  (const MunitParameter params[], void* data)
{
  struct test_ringdist_params *const fixt =
    (struct test_ringdist_params*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* inspect */{
    size_t i;
    for (i = 0u; i < 30u; ++i) {
      munit_assert_uint(
          tcmplxA_ringdist_bit_count(p,test_ringdist_bit_comp_1951[i].code),
            ==,test_ringdist_bit_comp_1951[i].bits
        );
    }
  }
  return MUNIT_OK;
}

MunitResult test_ringdist_7932_bit_count
  (const MunitParameter params[], void* data)
{
  struct test_ringdist_params *const fixt =
    (struct test_ringdist_params*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  unsigned int alpha_size;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* recompute alphabet size */{
    alpha_size = 16 + fixt->direct_count + (48 << fixt->postfix_size);
  }
  /* iterate over the alphabet */{
    unsigned int dcode;
    unsigned int const ndirect = fixt->direct_count;
    unsigned int const npostfix = fixt->postfix_size;
    for (dcode = 0; dcode < alpha_size; ++dcode) {
      if (dcode < 16) {
        /* special code */
        munit_assert_uint(tcmplxA_ringdist_bit_count(p, dcode), ==, 0);
      } else if (dcode < 16 + ndirect) {
        /* direct code */
        munit_assert_uint(tcmplxA_ringdist_bit_count(p, dcode), ==, 0);
      } else {
        /* indirect code */
        unsigned int const ndistbits =
          1 + ((dcode - ndirect - 16) >> (npostfix + 1));
        munit_assert_uint(tcmplxA_ringdist_bit_count(p, dcode), ==, ndistbits);
      }
    }
  }
  return MUNIT_OK;
}

MunitResult test_ringdist_1951_decode
  (const MunitParameter params[], void* data)
{
  struct test_ringdist_params *const fixt =
    (struct test_ringdist_params*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  unsigned int back_dist;
  unsigned int decomposed_code;
  unsigned int decomposed_extra;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* compose 1951 encoding of distance */{
    unsigned int back_bit_count = 0u;
    unsigned int num_bits;
    back_dist = testfont_rand_uint_range(1u,32768u);
    /* count bits in distance */{
      unsigned int bd;
      for (bd = back_dist-1u; bd > 0u; bd >>= 1u) {
        back_bit_count += 1u;
      }
    }
    /* select bit length */{
      num_bits = back_bit_count > 2u ? (back_bit_count-2u) : 0u;
    }
    /* decompose */{
      if (num_bits == 0u) {
        decomposed_code = back_dist-1u;
        decomposed_extra = 0u;
      } else {
        decomposed_code = (num_bits*2u)+2u + (
              (back_dist-1u)&(1<<(back_bit_count-2u))
            ?1u:0u);
        decomposed_extra = (back_dist-1u)&((1<<num_bits)-1u);
      }
    }
  }
  /* inspect */{
    munit_assert_uint(
        tcmplxA_ringdist_decode(p,decomposed_code,decomposed_extra),
          ==,back_dist
      );
  }
  return MUNIT_OK;
}

MunitResult test_ringdist_7932_decode
  (const MunitParameter params[], void* data)
{
  struct test_ringdist_params *const fixt =
    (struct test_ringdist_params*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  return MUNIT_SKIP;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_ringdist, NULL, argc, argv);
}
