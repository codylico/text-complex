/**
 * @brief Test program for insert copy table
 */
#include "testfont.h"
#include "text-complex/access/inscopy.h"
#include "reedkiln/log.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_inscopy_cycle(void* data);
static int test_inscopy_item(void* data);
static int test_inscopy_item_c(void* data);
static int test_inscopy_lengthsort(void* data);
/* TODO: A test for lengthsort to ensure it accounts for copy_first. */
static int test_inscopy_codesort(void* data);
static int test_inscopy_encode(void* data);
static void* test_inscopy_setup(void* user_data);
static void test_inscopy_teardown(void* fixture);
static unsigned long int test_inscopy_rand_length25(void);

static struct reedkiln_box const box_inscopy = {
  test_inscopy_setup,test_inscopy_teardown
};

static struct reedkiln_entry const tests_inscopy[] = {
  {"access/inscopy/cycle", test_inscopy_cycle },
  {"access/inscopy/item", test_inscopy_item, 0, &box_inscopy },
  {"access/inscopy/item_const", test_inscopy_item_c, 0, &box_inscopy },
  {"access/inscopy/lengthsort", test_inscopy_lengthsort, 0, &box_inscopy },
  {"access/inscopy/codesort", test_inscopy_codesort, 0, &box_inscopy },
  {"access/inscopy/encode", test_inscopy_encode, 0, &box_inscopy },
  {NULL}
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



int test_inscopy_cycle(void* data)
{
  struct tcmplxA_inscopy* ptr[1];
  size_t const t = testfont_rand_size_range(0u,256u);
  (void)data;
  ptr[0] = tcmplxA_inscopy_new(t);
  reedkiln_assert(ptr[0]);
  tcmplxA_inscopy_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_inscopy_setup(void* user_data) {
  enum tcmplxA_inscopy_type const t =
      (enum tcmplxA_inscopy_type)testfont_rand_int_range(0,2);
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

int test_inscopy_item(void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* consistency check */{
    size_t const i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p));
    reedkiln_assert
      (tcmplxA_inscopy_at(p, i) == tcmplxA_inscopy_at_c(p, i));
  }
  switch (tcmplxA_inscopy_size(p)) {
  case 286: /* Deflate */
    {
      /* test literals */{
        size_t const i = testfont_rand_size_range(0,255);
        struct tcmplxA_inscopy_row* row = tcmplxA_inscopy_at(p, i);
        reedkiln_assert(row);
        reedkiln_assert(row->type == tcmplxA_InsCopy_Literal);
        reedkiln_assert(row->code == i);
      }
      /* test stop */{
        struct tcmplxA_inscopy_row* row =
          tcmplxA_inscopy_at(p, 256);
        reedkiln_assert(row);
        reedkiln_assert(row->type == tcmplxA_InsCopy_Stop);
        reedkiln_assert(row->code == 256u);
      }
      /* test length */{
        size_t i = testfont_rand_size_range(257,285);
        struct tcmplxA_inscopy_row* row =
          tcmplxA_inscopy_at(p,i);
        reedkiln_assert(row);
        reedkiln_assert(row->code == i);
        reedkiln_assert((row->type&127) == tcmplxA_InsCopy_Copy);
        reedkiln_assert(row->insert_bits <= 5);
        reedkiln_log_printf("[%u]={%u,*%u};",
          (unsigned int)i,
          row->insert_bits, row->insert_first);
      }
    }break;
  case 704: /* Brotli Insert-Copy */
    {
      size_t i = testfont_rand_size_range(0,703);
      struct tcmplxA_inscopy_row* row =
        tcmplxA_inscopy_at(p,i);
      reedkiln_assert(row->code == i);
      reedkiln_assert((row->zero_distance_tf!=0) == i<128);
      reedkiln_assert
        (row->type == tcmplxA_InsCopy_InsertCopy);
      reedkiln_log_printf(
        "[%u]={%u,*%u, copy %u,*%u};",
        (unsigned int)(i),
        row->insert_bits, row->insert_first,
        row->copy_bits, row->copy_first);
    }break;
  case 26: /* Brotli Block Count */
    {
      size_t i = testfont_rand_size_range(0,25);
      struct tcmplxA_inscopy_row* row = tcmplxA_inscopy_at(p,i);
      reedkiln_assert
        (row->type == tcmplxA_InsCopy_BlockCount);
      reedkiln_assert(row->code == i);
      reedkiln_assert(row->insert_bits <= 24);
      reedkiln_log_printf("[%u]={%u,*%u};",
        (unsigned int)(i),
        row->insert_bits, row->insert_first);
    }break;
  default:
    reedkiln_log_printf("Unexpected table length %u.",
      (unsigned)tcmplxA_inscopy_size(p));
    reedkiln_fail();
  }
  return Reedkiln_OK;
}

int test_inscopy_item_c(void* data)
{
  struct tcmplxA_inscopy const* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  (void)data;
  switch (tcmplxA_inscopy_size(p)) {
  case 286: /* Deflate */
    {
      /* test literals */{
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p, testfont_rand_size_range(0,255));
        reedkiln_assert(row);
        reedkiln_assert(row->type == tcmplxA_InsCopy_Literal);
      }
      /* test stop */{
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p, 256);
        reedkiln_assert(row);
        reedkiln_assert(row->type == tcmplxA_InsCopy_Stop);
      }
      /* test length */{
        size_t i = testfont_rand_size_range(257,285);
        struct tcmplxA_inscopy_row const* row =
          tcmplxA_inscopy_at_c(p,i);
        reedkiln_assert(row);
        reedkiln_assert((row->type&127) == tcmplxA_InsCopy_Copy);
        reedkiln_assert(row->insert_bits <= 5);
        reedkiln_log_printf("[%u]={%u,*%u};",
          (unsigned int)i,
          row->insert_bits, row->insert_first);
      }
    }break;
  case 704: /* Brotli Insert-Copy */
    {
      size_t i = testfont_rand_size_range(0,703);
      struct tcmplxA_inscopy_row const* row =
        tcmplxA_inscopy_at_c(p,i);
      reedkiln_assert((row->zero_distance_tf!=0) == i<128);
      reedkiln_assert
        (row->type == tcmplxA_InsCopy_InsertCopy);
      reedkiln_log_printf(
        "[%u]={%u,*%u, copy %u,*%u};",
        (unsigned int)(i),
        row->insert_bits, row->insert_first,
        row->copy_bits, row->copy_first);
    }break;
  case 26: /* Brotli Block Count */
    {
      size_t i = testfont_rand_size_range(0,25);
      struct tcmplxA_inscopy_row const* row = tcmplxA_inscopy_at_c(p,i);
      reedkiln_assert
        (row->type == tcmplxA_InsCopy_BlockCount);
      reedkiln_assert(row->code == i);
      reedkiln_assert(row->insert_bits <= 24);
      reedkiln_log_printf("[%u]={%u,*%u};",
        (unsigned int)(i),
        row->insert_bits, row->insert_first);
    }break;
  default:
    reedkiln_log_printf("Unexpected table length %u.",
      (unsigned)tcmplxA_inscopy_size(p));
    reedkiln_fail();
  }
  return Reedkiln_OK;
}


int test_inscopy_codesort(void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* run code-sort */{
    int const res = tcmplxA_inscopy_codesort(p);
    reedkiln_assert(res == tcmplxA_Success);
  }
  /* inspect */if (tcmplxA_inscopy_size(p) >= 2) {
    size_t i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p)-2);
    struct tcmplxA_inscopy_row const* first = tcmplxA_inscopy_at_c(p, i);
    struct tcmplxA_inscopy_row const* second = tcmplxA_inscopy_at_c(p, i+1);
    reedkiln_assert(first->code < second->code);
  }
  return Reedkiln_OK;
}

int test_inscopy_lengthsort(void* data)
{
  struct tcmplxA_inscopy* const p = (struct tcmplxA_inscopy*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* run length-sort */{
    int const res = tcmplxA_inscopy_lengthsort(p);
    reedkiln_assert(res == tcmplxA_Success);
  }
  /* inspect */if (tcmplxA_inscopy_size(p) >= 2) {
    size_t i = testfont_rand_size_range(0,tcmplxA_inscopy_size(p)-2);
    struct tcmplxA_inscopy_row const* first = tcmplxA_inscopy_at_c(p, i);
    struct tcmplxA_inscopy_row const* second = tcmplxA_inscopy_at_c(p, i+1);
    if (first->zero_distance_tf != second->zero_distance_tf) {
      reedkiln_assert(!first->zero_distance_tf);
      reedkiln_assert(second->zero_distance_tf);
    } else if (first->insert_first != second->insert_first) {
      reedkiln_assert(first->insert_first < second->insert_first);
    } else {
      reedkiln_assert(first->copy_first <= second->copy_first);
    }
  }
  return Reedkiln_OK;
}

int test_inscopy_encode(void* data)
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
    return Reedkiln_SKIP;
  /* run length-sort */{
    int const res = tcmplxA_inscopy_lengthsort(p);
    if (res != tcmplxA_Success)
      return Reedkiln_SKIP;
  }
  switch (tcmplxA_inscopy_size(p)) {
  case 704: /* Brotli insert-copy pairs */{
      unsigned long int const ins_len = test_inscopy_rand_length25();
      unsigned long int const cpy_len = test_inscopy_rand_length25();
      int const zero_dist = (testfont_rand_int_range(0,1) != 0);
      int const expect_success =
        (ins_len <= 16799809ul && cpy_len >= 2 && cpy_len <= 16779333ul);
      size_t encode_index =
        tcmplxA_inscopy_encode(p, ins_len, cpy_len, zero_dist);
      if ((encode_index == not_found) && zero_dist) {
        reedkiln_log_printf("Retrying without zero distance.\n");
        encode_index = tcmplxA_inscopy_encode(p, ins_len, cpy_len, 0);
      }
      if ((encode_index != not_found) == expect_success) {
        if (expect_success) {
          struct tcmplxA_inscopy_row const* const row =
            tcmplxA_inscopy_at_c(p, encode_index);
          unsigned long int const ins_extra = ins_len - row->insert_first;
          unsigned long int const cpy_extra = cpy_len - row->copy_first;
          reedkiln_log_printf(
            "Encode (insert: %lu, copy: %lu, zero: %s) as "
            "<%u, %lu:%u, %lu:%u (zero: %s)>",
            ins_len, cpy_len, zero_dist?"true":"false",
            row->code, ins_extra, row->insert_bits, cpy_extra, row->copy_bits,
            row->zero_distance_tf?"true":"false");
        } else {
          reedkiln_log_printf(
            "Encode (insert: %lu, copy: %lu, zero: %s) properly rejected.",
            ins_len, cpy_len, zero_dist?"true":"false");
        }
      } else {
        reedkiln_log_printf("Table row %u selected for "
          "%s configuration (insert: %lu, copy: %lu, zero: %s).",
          (unsigned)encode_index, (expect_success?"valid":"impossible"),
          ins_len, cpy_len, zero_dist?"true":"false");
        reedkiln_fail();
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
          reedkiln_log_printf(
            "Encode (copy: %lu) as <%u, %lu:%u>",
            cpy_len, row->code, cpy_extra, row->copy_bits);
        } else {
          reedkiln_log_printf(
            "Encode (copy: %lu) properly rejected.", cpy_len);
        }
      } else {
        reedkiln_log_printf("Table row %i selected for "
          "%s configuration (copy: %li).",
          (int)encode_index, (expect_success?"valid":"impossible"), (long)cpy_len);
        reedkiln_fail();
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
          reedkiln_log_printf(
            "Encode (block-count: %lu) as <%u, %lu:%u>",
            block_len, row->code, block_extra, row->insert_bits);
        } else {
          reedkiln_log_printf(
            "Encode (block-count: %lu) properly rejected.", block_len);
        }
      } else {
        reedkiln_log_printf("Table row %i selected for "
          "%s configuration (block-count: %li).",
          (int)encode_index, (expect_success?"valid":"impossible"), (long)block_len);
        reedkiln_fail();
      }
    }break;
  }
  return Reedkiln_OK;
}

int main(int argc, char **argv) {
  return reedkiln_main(tests_inscopy, argc, argv, NULL);
}
