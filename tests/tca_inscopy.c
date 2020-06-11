/**
 * @brief Test program for insert copy table
 */
#include "../tcmplx-access/inscopy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "munit/munit.h"
#include "testfont.h"


static MunitResult test_inscopy_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_inscopy_item
  (const MunitParameter params[], void* data);
static void* test_inscopy_setup
    (const MunitParameter params[], void* user_data);
static void test_inscopy_teardown(void* fixture);

static MunitTest tests_inscopy[] = {
  {"cycle", test_inscopy_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_inscopy_item,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_inscopy = {
  "access/inscopy/", tests_inscopy, NULL, 1, 0
};




MunitResult test_inscopy_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy* ptr[1];
  enum tcmplxA_inscopy_type const t =
      munit_rand_int_range(0,1)
    ? tcmplxA_InsCopy_Deflate
    : tcmplxA_InsCopy_Brotli;
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_inscopy_new(t);
  munit_assert_not_null(ptr[0]);
  tcmplxA_inscopy_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_inscopy_setup(const MunitParameter params[], void* user_data) {
  enum tcmplxA_inscopy_type const t =
      munit_rand_int_range(0,1)
    ? tcmplxA_InsCopy_Deflate
    : tcmplxA_InsCopy_Brotli;
  return tcmplxA_inscopy_new(t);
}

void test_inscopy_teardown(void* fixture) {
  tcmplxA_inscopy_destroy((struct tcmplxA_inscopy*)fixture);
  return;
}

MunitResult test_inscopy_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  (void)data;
  switch (tcmplxA_inscopy_size(p)) {
  case 286: /* Deflate */
    {
      /* test literals */{
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p, testfont_rand_size_range(0,255));
        munit_assert_ptr_not_null(row);
        munit_assert_uint(row->type, ==, tcmplxA_InsCopy_Literal);
      }
      /* test stop */{
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p, 256);
        munit_assert_ptr_not_null(row);
        munit_assert_uint(row->type, ==, tcmplxA_InsCopy_Stop);
      }
      /* test length */{
        size_t i = testfont_rand_size_range(257,285);
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p,i);
        munit_assert_ptr_not_null(row);
        munit_assert_uint(row->type, ==, tcmplxA_InsCopy_Insert);
        munit_assert_uint(row->insert_bits, <=, 5);
        munit_logf(MUNIT_LOG_DEBUG, "[%u] = {bits: %u, first: %u}",
          (unsigned int)i,
          row->insert_bits, row->insert_first);
      }
    }break;
  case 704: /* Brotli */
    {
      size_t i = testfont_rand_size_range(0,703);
      struct tcmplxA_inscopy_row const* row =
        tcmplxA_inscopy_at_c(p,i);
      munit_assert_int((row->zero_distance_tf!=0), ==, i<128);
      munit_assert_int
        (row->type, ==, tcmplxA_InsCopy_InsertCopy);
      munit_logf(MUNIT_LOG_DEBUG,
        "[%u] = {bits: %u, first: %u, copy_bits: %u, copy_first: %u}",
        (unsigned int)(i),
        row->insert_bits, row->insert_first,
        row->copy_bits, row->copy_first);
    }break;
  default:
    munit_errorf("Unexpected table length %" MUNIT_SIZE_MODIFIER "u.",
      tcmplxA_inscopy_size(p));
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_inscopy, NULL, argc, argv);
}
