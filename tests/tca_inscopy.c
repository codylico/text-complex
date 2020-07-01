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
static MunitResult test_inscopy_item_c
  (const MunitParameter params[], void* data);
static MunitResult test_inscopy_lengthsort
  (const MunitParameter params[], void* data);
static MunitResult test_inscopy_codesort
  (const MunitParameter params[], void* data);
static MunitResult test_inscopy_encode
  (const MunitParameter params[], void* data);
static void* test_inscopy_setup
    (const MunitParameter params[], void* user_data);
static void test_inscopy_teardown(void* fixture);
static unsigned long int test_inscopy_rand_length25(void);

static MunitTest tests_inscopy[] = {
  {"cycle", test_inscopy_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_inscopy_item,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {"item_const", test_inscopy_item_c,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {"lengthsort", test_inscopy_lengthsort,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {"codesort", test_inscopy_codesort,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {"encode", test_inscopy_encode,
    test_inscopy_setup,test_inscopy_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_inscopy = {
  "access/inscopy/", tests_inscopy, NULL, 1, 0
};




unsigned long int test_inscopy_rand_length25(void) {
  /*
   * Non-uniform sampling of ranges [0,8192) | [8192,16801792)
   */
  unsigned long int const out = testfont_rand_uint_range(0u,16391u);
  static unsigned long int const remap_diff = 16777216lu - 8192lu;
  return (out >= 8192u)
    ? ((out<<11) + testfont_rand_uint_range(0u,2047u) - remap_diff)
    : out;
}



MunitResult test_inscopy_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy* ptr[1];
  size_t const t = testfont_rand_size_range(0u,256u);
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_inscopy_new(t);
  munit_assert_not_null(ptr[0]);
  tcmplxA_inscopy_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_inscopy_setup(const MunitParameter params[], void* user_data) {
  enum tcmplxA_inscopy_type const t =
      (enum tcmplxA_inscopy_type)munit_rand_int_range(0,2);
  struct tcmplxA_inscopy *p = tcmplxA_inscopy_new(0u);
  if (p != NULL) {
    int const res = tcmplxA_inscopy_preset(p, t);
    if (res != tcmplxA_Success) {
      tcmplxA_inscopy_destroy(p);
      p = NULL;
    }
  }
  return p;
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
  /* consistency check */{
    size_t const i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p));
    munit_assert_ptr_equal
      (tcmplxA_inscopy_at(p, i), tcmplxA_inscopy_at_c(p, i));
  }
  switch (tcmplxA_inscopy_size(p)) {
  case 286: /* Deflate */
    {
      /* test literals */{
        size_t const i = testfont_rand_size_range(0,255);
        struct tcmplxA_inscopy_row* row = tcmplxA_inscopy_at(p, i);
        munit_assert_ptr_not_null(row);
        munit_assert_uint(row->type, ==, tcmplxA_InsCopy_Literal);
        munit_assert_size(row->code, ==, i);
      }
      /* test stop */{
        struct tcmplxA_inscopy_row* row =
          tcmplxA_inscopy_at(p, 256);
        munit_assert_ptr_not_null(row);
        munit_assert_uint(row->type, ==, tcmplxA_InsCopy_Stop);
        munit_assert_short(row->code, ==, 256u);
      }
      /* test length */{
        size_t i = testfont_rand_size_range(257,285);
        struct tcmplxA_inscopy_row* row =
          tcmplxA_inscopy_at(p,i);
        munit_assert_ptr_not_null(row);
        munit_assert_size(row->code, ==, i);
        munit_assert_uint(row->type&127, ==, tcmplxA_InsCopy_Copy);
        munit_assert_uint(row->insert_bits, <=, 5);
        munit_logf(MUNIT_LOG_DEBUG, "[%u] = {bits: %u, first: %u}",
          (unsigned int)i,
          row->insert_bits, row->insert_first);
      }
    }break;
  case 704: /* Brotli Insert-Copy */
    {
      size_t i = testfont_rand_size_range(0,703);
      struct tcmplxA_inscopy_row* row =
        tcmplxA_inscopy_at(p,i);
      munit_assert_size(row->code, ==, i);
      munit_assert_int((row->zero_distance_tf!=0), ==, i<128);
      munit_assert_int
        (row->type, ==, tcmplxA_InsCopy_InsertCopy);
      munit_logf(MUNIT_LOG_DEBUG,
        "[%u] = {bits: %u, first: %u, copy_bits: %u, copy_first: %u}",
        (unsigned int)(i),
        row->insert_bits, row->insert_first,
        row->copy_bits, row->copy_first);
    }break;
  case 26: /* Brotli Block Count */
    {
      size_t i = testfont_rand_size_range(0,25);
      struct tcmplxA_inscopy_row* row = tcmplxA_inscopy_at(p,i);
      munit_assert_int
        (row->type, ==, tcmplxA_InsCopy_BlockCount);
      munit_assert_size(row->code, ==, i);
      munit_assert_uint(row->insert_bits, <=, 24);
      munit_logf(MUNIT_LOG_DEBUG, "[%u] = {bits: %u, first: %u}",
        (unsigned int)(i),
        row->insert_bits, row->insert_first);
    }break;
  default:
    munit_errorf("Unexpected table length %" MUNIT_SIZE_MODIFIER "u.",
      tcmplxA_inscopy_size(p));
  }
  return MUNIT_OK;
}

MunitResult test_inscopy_item_c
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy const* const p = (struct tcmplxA_inscopy*)data;
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
        munit_assert_uint(row->type&127, ==, tcmplxA_InsCopy_Copy);
        munit_assert_uint(row->insert_bits, <=, 5);
        munit_logf(MUNIT_LOG_DEBUG, "[%u] = {bits: %u, first: %u}",
          (unsigned int)i,
          row->insert_bits, row->insert_first);
      }
    }break;
  case 704: /* Brotli Insert-Copy */
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
  case 26: /* Brotli Block Count */
    {
      size_t i = testfont_rand_size_range(0,25);
      struct tcmplxA_inscopy_row const* row = tcmplxA_inscopy_at_c(p,i);
      munit_assert_int
        (row->type, ==, tcmplxA_InsCopy_BlockCount);
      munit_assert_size(row->code, ==, i);
      munit_assert_uint(row->insert_bits, <=, 24);
      munit_logf(MUNIT_LOG_DEBUG, "[%u] = {bits: %u, first: %u}",
        (unsigned int)(i),
        row->insert_bits, row->insert_first);
    }break;
  default:
    munit_errorf("Unexpected table length %" MUNIT_SIZE_MODIFIER "u.",
      tcmplxA_inscopy_size(p));
  }
  return MUNIT_OK;
}


MunitResult test_inscopy_codesort
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* run code-sort */{
    int const res = tcmplxA_inscopy_codesort(p);
    munit_assert_int(res,==,tcmplxA_Success);
  }
  /* inspect */if (tcmplxA_inscopy_size(p) >= 2) {
    size_t i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p)-2);
    struct tcmplxA_inscopy_row const* first = tcmplxA_inscopy_at_c(p, i);
    struct tcmplxA_inscopy_row const* second = tcmplxA_inscopy_at_c(p, i+1);
    munit_assert_uint(first->code, <, second->code);
  }
  return MUNIT_OK;
}

MunitResult test_inscopy_lengthsort
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* run length-sort */{
    int const res = tcmplxA_inscopy_lengthsort(p);
    munit_assert_int(res,==,tcmplxA_Success);
  }
  /* inspect */if (tcmplxA_inscopy_size(p) >= 2) {
    size_t i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p)-2);
    struct tcmplxA_inscopy_row const* first = tcmplxA_inscopy_at_c(p, i);
    struct tcmplxA_inscopy_row const* second = tcmplxA_inscopy_at_c(p, i+1);
    if (first->zero_distance_tf != second->zero_distance_tf) {
      munit_assert_false(first->zero_distance_tf);
      munit_assert_true(second->zero_distance_tf);
    } else if (first->insert_first != second->insert_first) {
      munit_assert_uint(first->insert_first, <, second->insert_first);
    } else {
      munit_assert_uint(first->copy_first, <=, second->copy_first);
    }
  }
  return MUNIT_OK;
}

MunitResult test_inscopy_encode
  (const MunitParameter params[], void* data)
{
  /*
   * DEFLATE insert max: 258
   * Brotli insert max: 16799809
   * Brotli copy max: 16779333
   * Brotli block-count max: 16793840
   */
  size_t const not_found = ((size_t)-1);
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* run length-sort */{
    int const res = tcmplxA_inscopy_lengthsort(p);
    if (res != tcmplxA_Success)
      return MUNIT_SKIP;
  }
  switch (tcmplxA_inscopy_size(p)) {
  case 704: /* Brotli insert-copy pairs */{
      unsigned long int const ins_len = test_inscopy_rand_length25();
      unsigned long int const cpy_len = test_inscopy_rand_length25();
      int const zero_dist = (munit_rand_int_range(0,1) != 0);
      int const expect_success =
        (ins_len <= 16799809ul && cpy_len >= 2 && cpy_len <= 16779333ul);
      size_t encode_index =
        tcmplxA_inscopy_encode(p, ins_len, cpy_len, zero_dist);
      if ((encode_index == not_found) && zero_dist) {
        munit_log(MUNIT_LOG_DEBUG, "Retrying without zero distance.");
        encode_index = tcmplxA_inscopy_encode(p, ins_len, cpy_len, 0);
      }
      if ((encode_index != not_found) == expect_success) {
        if (expect_success) {
          struct tcmplxA_inscopy_row const* const row =
            tcmplxA_inscopy_at_c(p, encode_index);
          unsigned long int const ins_extra = ins_len - row->insert_first;
          unsigned long int const cpy_extra = cpy_len - row->copy_first;
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (insert: %lu, copy: %lu, zero: %s) as "
            "<%u, %lu:%u, %lu:%u (zero: %s)>",
            ins_len, cpy_len, zero_dist?"true":"false",
            row->code, ins_extra, row->insert_bits, cpy_extra, row->copy_bits,
            row->zero_distance_tf?"true":"false");
        } else {
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (insert: %lu, copy: %lu, zero: %s) properly rejected.",
            ins_len, cpy_len, zero_dist?"true":"false");
        }
      } else {
        munit_errorf("Table row %" MUNIT_SIZE_MODIFIER "u selected for "
          "%s configuration (insert: %lu, copy: %lu, zero: %s).",
          encode_index, (expect_success?"valid":"impossible"),
          ins_len, cpy_len, zero_dist?"true":"false");
      }
    }break;
  case 286: /* DEFLATE */{
      unsigned long int const cpy_len = testfont_rand_uint_range(0u,260u);
      int const expect_success = (cpy_len >= 3 && cpy_len <= 258);
      size_t const encode_index =
        tcmplxA_inscopy_encode(p, 0u, cpy_len, 0);
      if ((encode_index != not_found) == expect_success) {
        if (expect_success) {
          struct tcmplxA_inscopy_row const* const row =
            tcmplxA_inscopy_at_c(p, encode_index);
          unsigned long int const cpy_extra = cpy_len - row->copy_first;
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (copy: %lu) as <%u, %lu:%u>",
            cpy_len, row->code, cpy_extra, row->copy_bits);
        } else {
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (copy: %lu) properly rejected.", cpy_len);
        }
      } else {
        munit_errorf("Table row %" MUNIT_SIZE_MODIFIER "u selected for "
          "%s configuration (copy: %lu).",
          encode_index, (expect_success?"valid":"impossible"), cpy_len);
      }
    }break;
  case 26: /* Brotli block counts */{
      unsigned long int const block_len = test_inscopy_rand_length25();
      int const expect_success = (block_len >= 1 && block_len <= 16793840ul);
      size_t const encode_index =
        tcmplxA_inscopy_encode(p, block_len, 0u, 0);
      if ((encode_index != not_found) == expect_success) {
        if (expect_success) {
          struct tcmplxA_inscopy_row const* const row =
            tcmplxA_inscopy_at_c(p, encode_index);
          unsigned long int const block_extra = block_len - row->insert_first;
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (block-count: %lu) as <%u, %lu:%u>",
            block_len, row->code, block_extra, row->insert_bits);
        } else {
          munit_logf(MUNIT_LOG_DEBUG,
            "Encode (block-count: %lu) properly rejected.", block_len);
        }
      } else {
        munit_errorf("Table row %" MUNIT_SIZE_MODIFIER "u selected for "
          "%s configuration (block-count: %lu).",
          encode_index, (expect_success?"valid":"impossible"), block_len);
      }
    }break;
  }
}

int main(int argc, char **argv) {
  return munit_suite_main(&suite_inscopy, NULL, argc, argv);
}
