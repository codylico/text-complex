/**
 * @brief Test program for block buffer
 */
#include "testfont.h"
#include "text-complex/access/blockbuf.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_blockbuf_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_blockbuf_item
  (const MunitParameter params[], void* data);
static MunitResult test_blockbuf_gen
  (const MunitParameter params[], void* data);
static MunitResult test_blockbuf_bypass
  (const MunitParameter params[], void* data);
static void* test_blockbuf_setup
    (const MunitParameter params[], void* user_data);
static void test_blockbuf_teardown(void* fixture);

static MunitParameterEnum test_blockbuf_params[] = {
  { NULL, NULL },
};

static MunitTest tests_blockbuf[] = {
  {"cycle", test_blockbuf_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_blockbuf_item,
    test_blockbuf_setup,test_blockbuf_teardown,0,NULL},
  {"gen", test_blockbuf_gen,
    test_blockbuf_setup,test_blockbuf_teardown,0,NULL},
  {"bypass", test_blockbuf_bypass,
    test_blockbuf_setup,test_blockbuf_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_blockbuf = {
  "access/blockbuf/", tests_blockbuf, NULL, 1, 0
};




MunitResult test_blockbuf_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_blockbuf* ptr[1];
  uint32_t const block_size = munit_rand_int_range(1u,128u);
  uint32_t const window_size = munit_rand_int_range(128u,512u);
  size_t const chain_length = (size_t)munit_rand_int_range(1u,32u);
  int const bdict_tf = munit_rand_int_range(0,1);
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_blockbuf_new
    (block_size, window_size, chain_length, bdict_tf);
  munit_assert_not_null(ptr[0]);
  munit_assert_uint32(tcmplxA_blockbuf_capacity(ptr[0]),==,block_size);
  tcmplxA_blockbuf_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_blockbuf_setup(const MunitParameter params[], void* user_data) {
  uint32_t const block_size = munit_rand_int_range(1u,128u);
  uint32_t const window_size = munit_rand_int_range(128u,512u);
  size_t const chain_length = (size_t)munit_rand_int_range(1u,32u);
  int const bdict_tf = munit_rand_int_range(0,1);
  return tcmplxA_blockbuf_new
    (block_size, window_size, chain_length, bdict_tf);
}

void test_blockbuf_teardown(void* fixture) {
  tcmplxA_blockbuf_destroy((struct tcmplxA_blockbuf*)fixture);
  return;
}

MunitResult test_blockbuf_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = testfont_rand_size_range
    (1, tcmplxA_blockbuf_capacity(p));
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* add some stuff */{
    unsigned char buf[128];
    int res;
    munit_rand_memory(count, (munit_uint8_t*)buf);
    res = tcmplxA_blockbuf_write(p, buf, count);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, count);
  }
  return MUNIT_OK;
}

MunitResult test_blockbuf_gen
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = tcmplxA_blockbuf_capacity(p);
  unsigned char buf[128];
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* build the text */{
    size_t i;
    munit_rand_memory(count, (munit_uint8_t*)buf);
    for (i = 0; i < count; ++i) {
      buf[i] = (buf[i]&3u)|80u;
    }
  }
  /* add to input */{
    int res;
    res = tcmplxA_blockbuf_write(p, buf, count);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, count);
  }
  /* */{
    int const res = tcmplxA_blockbuf_gen_block(p);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, 0);
  }
  /* inspect */{
    unsigned char const* const output = tcmplxA_blockbuf_output_data(p);
    tcmplxA_uint32 const len = tcmplxA_blockbuf_output_size(p);
    munit_assert_uint32(len, >, 0u);
    munit_assert_ptr_not_null(output);
    /* parse the command buffer */{
      unsigned char inflated_buf[128];
      size_t j = 0;
      uint32_t i;
      for (i = 0; i < len; ++i) {
        uint32_t n;
        unsigned char s80 = output[i]&0x80;
        if (output[i]&0x40) {
          munit_assert_uint32(i+2u, <=, len);
          n = ((output[i]&0x3f)*256)+output[i+1];
          i += 2;
        } else {
          n = output[i]&0x3f;
          i += 1;
        }
        if (s80) {
          uint32_t backward;
          uint32_t x;
          munit_assert_uint32(i+2u, <=, len);
          munit_assert_uchar(output[i]&0xC0, ==, 0x80)/* short distances */;
          backward = ((output[i]&0x3f)*256) + (output[i+1]) + 1;
          munit_assert_size(backward, <=, j);
          for (x = 0; x < n && j < 128; ++x, ++j) {
            inflated_buf[j] = inflated_buf[j-backward];
          }
          i += 1;
        } else {
          uint32_t x;
          for (x = 0u; x < n && j < 128; ++x, ++i, ++j) {
            munit_assert_uint32(i, <, len);
            inflated_buf[j] = output[i];
          }
          i -= 1;
        }
      }
      munit_assert_size(j, ==, count);
      munit_assert_memory_equal(j, inflated_buf, buf);
    }
  }
  /* clear */{
    tcmplxA_blockbuf_clear_output(p);
    munit_assert_uint32(tcmplxA_blockbuf_output_size(p), ==, 0);
  }
  return MUNIT_OK;
}

MunitResult test_blockbuf_bypass
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = tcmplxA_blockbuf_capacity(p);
  unsigned char buf[3];
  if (p == NULL || count < 3)
    return MUNIT_SKIP;
  (void)params;
  /* build the text */{
    size_t i;
    munit_rand_memory(3, (munit_uint8_t*)buf);
    for (i = 0; i < 3; ++i) {
      buf[i] = (buf[i]&3u)|80u;
    }
  }
  /* add to slide ring */{
    size_t const bypass_count = tcmplxA_blockbuf_bypass(p, buf, 3);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, 0u);
    munit_assert_size(bypass_count, ==, 3);
  }
  /* add to input */{
    int res;
    res = tcmplxA_blockbuf_write(p, buf, 3);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, 3u);
  }
  /* */{
    int const res = tcmplxA_blockbuf_gen_block(p);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_uint32(tcmplxA_blockbuf_input_size(p), ==, 0);
  }
  /* inspect */{
    unsigned char const copy_cmd[] = {131u, 128u, 2u};
    unsigned char const* const output = tcmplxA_blockbuf_output_data(p);
    tcmplxA_uint32 const len = tcmplxA_blockbuf_output_size(p);
    munit_assert_uint32(len, ==, 3u);
    munit_assert_ptr_not_null(output);
    munit_assert_memory_equal(3, output, copy_cmd);
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_blockbuf, NULL, argc, argv);
}
