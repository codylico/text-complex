/**
 * @brief Test program for zcvt state
 */
#include "testfont.h"
#include "text-complex/access/zcvt.h"
#include "text-complex/access/zutil.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_zcvt_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_zcvt_item
  (const MunitParameter params[], void* data);
static MunitResult test_zcvt_zsrtostr_none
  (const MunitParameter params[], void* data);
static void* test_zcvt_setup
    (const MunitParameter params[], void* user_data);
static void test_zcvt_teardown(void* fixture);

static MunitParameterEnum test_zcvt_params[] = {
  { NULL, NULL },
};

static MunitTest tests_zcvt[] = {
  {"cycle", test_zcvt_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_zcvt_item,
    test_zcvt_setup,test_zcvt_teardown,0,test_zcvt_params},
  {"in/none", test_zcvt_zsrtostr_none,
    test_zcvt_setup,test_zcvt_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_zcvt = {
  "access/zcvt/", tests_zcvt, NULL, 1, 0
};




MunitResult test_zcvt_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_zcvt* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_zcvt_new(64,128,16);
  munit_assert_not_null(ptr[0]);
  tcmplxA_zcvt_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_zcvt_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_zcvt_new(64,128,16);
}

void test_zcvt_teardown(void* fixture) {
  tcmplxA_zcvt_destroy((struct tcmplxA_zcvt*)fixture);
  return;
}

MunitResult test_zcvt_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_zcvt* const p = (struct tcmplxA_zcvt*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  (void)data;
  return MUNIT_OK;
}

MunitResult test_zcvt_zsrtostr_none
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_zcvt* const p = (struct tcmplxA_zcvt*)data;
  unsigned char buf[128+11] = {8u,29u,1u};
  size_t const len = (size_t)munit_rand_int_range(0,128);
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* "compress" some data */{
    uint32_t checksum;
    unsigned char *const check_buffer = buf+7+len;
    munit_rand_memory(len, (munit_uint8_t*)(buf+7));
    checksum = tcmplxA_zutil_adler32(len, buf+7, 1);
    buf[3] = (len)&0xff;
    buf[4] = (len>>8)&0xff;
    buf[5] = (~len)&0xff;
    buf[6] = (~(len>>8))&0xff;
    check_buffer[0] = (checksum>>24)&0xff;
    check_buffer[1] = (checksum>>16)&0xff;
    check_buffer[2] = (checksum>> 8)&0xff;
    check_buffer[3] = (checksum    )&0xff;
  }
  /* extract some data */{
    size_t const total = len+11;
    unsigned char to_buf[128];
    size_t ret;
    unsigned char const* src = buf;
    int const res = tcmplxA_zcvt_zsrtostr
      (p, &ret, to_buf, 128, &src, src+total);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_size(ret, ==, len);
    munit_assert_size(src-buf, ==, total);
    munit_assert_memory_equal(len, to_buf, buf+7);
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_zcvt, NULL, argc, argv);
}
