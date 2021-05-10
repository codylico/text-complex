/**
 * @brief Test program for hash chain
 */
#include "testfont.h"
#include "text-complex/access/hashchain.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_hashchain_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_hashchain_item
  (const MunitParameter params[], void* data);
static MunitResult test_hashchain_add
  (const MunitParameter params[], void* data);
static MunitResult test_hashchain_addring
  (const MunitParameter params[], void* data);
static void* test_hashchain_setup
    (const MunitParameter params[], void* user_data);
static void* test_hashchain_setupsmall
    (const MunitParameter params[], void* user_data);
static void test_hashchain_teardown(void* fixture);

static MunitParameterEnum test_hashchain_params[] = {
  { NULL, NULL },
};

static MunitTest tests_hashchain[] = {
  {"cycle", test_hashchain_cycle,
    NULL,NULL,0,NULL},
  {"item", test_hashchain_item,
    test_hashchain_setup,test_hashchain_teardown,0,NULL},
  {"add", test_hashchain_add,
    test_hashchain_setup,test_hashchain_teardown,0,NULL},
  {"add/ring", test_hashchain_addring,
    test_hashchain_setupsmall,test_hashchain_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_hashchain = {
  "access/hashchain/", tests_hashchain, NULL, 1, 0
};




MunitResult test_hashchain_cycle
  (const MunitParameter params[], void* data)
{
  uint32_t const num = (uint32_t)munit_rand_int_range(128,16777216);
  size_t const len = (size_t)munit_rand_int_range(1,128);
  struct tcmplxA_hashchain* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_hashchain_new(num, len);
  munit_assert_not_null(ptr[0]);
  munit_assert_uint32(tcmplxA_hashchain_extent(ptr[0]), ==, num);
  tcmplxA_hashchain_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_hashchain_setup(const MunitParameter params[], void* user_data) {
  uint32_t const num = (uint32_t)munit_rand_int_range(128,16777216);
  size_t const len = (size_t)munit_rand_int_range(1,128);
  return tcmplxA_hashchain_new(num, len);
}

void* test_hashchain_setupsmall
    (const MunitParameter params[], void* user_data)
{
  uint32_t const num = (uint32_t)munit_rand_int_range(128,512);
  size_t const len = (size_t)munit_rand_int_range(1,16);
  return tcmplxA_hashchain_new(num, len);
}

void test_hashchain_teardown(void* fixture) {
  tcmplxA_hashchain_destroy((struct tcmplxA_hashchain*)fixture);
  return;
}

MunitResult test_hashchain_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  (void)data;
  return MUNIT_OK;
}

MunitResult test_hashchain_add
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  int const add_count = munit_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* fill the buffer */{
    munit_rand_memory(add_count, (munit_uint8_t*)buf);
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_hashchain_add(p, buf[i]);
      munit_assert_int(res,==,tcmplxA_Success);
    }
  }
  /* check the size */{
    munit_assert_uint32(tcmplxA_hashchain_size(p), ==, add_count);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      munit_assert_uchar(tcmplxA_hashchain_peek(p, i),==,buf[j]);
    }
  }
  return MUNIT_OK;
}

MunitResult test_hashchain_addring
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  int const add_count = munit_rand_int_range(1,64);
  uint32_t const extent = tcmplxA_hashchain_extent(p);
  uint32_t const skip_count = extent - munit_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* fill the buffer */{
    munit_rand_memory(add_count, (munit_uint8_t*)buf);
  }
  /* skip some items */{
    uint32_t i;
    for (i = 0; i < skip_count; ++i) {
      tcmplxA_hashchain_add(p, ((unsigned int)i)&255u);
    }
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_hashchain_add(p, buf[i]);
      munit_assert_int(res,==,tcmplxA_Success);
    }
  }
  /* check the size */{
    uint32_t const expect_size =
        ((unsigned int)(add_count+skip_count) > extent)
      ? extent : add_count+skip_count;
    munit_assert_uint32(tcmplxA_hashchain_size(p), ==, expect_size);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      munit_assert_uchar(tcmplxA_hashchain_peek(p, i),==,buf[j]);
    }
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_hashchain, NULL, argc, argv);
}
