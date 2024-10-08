/**
 * @brief Test program for brcvt state
 */
#include "testfont.h"
#include "text-complex/access/brcvt.h"
#include "text-complex/access/zutil.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_brcvt_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_brcvt_item
  (const MunitParameter params[], void* data);
static MunitResult test_brcvt_metadata_ptr
  (const MunitParameter params[], void* data);
static MunitResult test_brcvt_zsrtostr_none
  (const MunitParameter params[], void* data);
static void* test_brcvt_setup
    (const MunitParameter params[], void* user_data);
static void test_brcvt_teardown(void* fixture);

static MunitParameterEnum test_brcvt_params[] = {
  { NULL, NULL },
};

static MunitTest tests_brcvt[] = {
  {"cycle", test_brcvt_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_brcvt_item,
    test_brcvt_setup,test_brcvt_teardown,0,test_brcvt_params},
  {"metadata_ptr", test_brcvt_metadata_ptr,
    test_brcvt_setup,test_brcvt_teardown,0,test_brcvt_params},
  {"in/none", test_brcvt_zsrtostr_none,
    test_brcvt_setup,test_brcvt_teardown,MUNIT_TEST_OPTION_TODO,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_brcvt = {
  "access/brcvt/", tests_brcvt, NULL, 1, 0
};




MunitResult test_brcvt_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_brcvt_new(64,128,16);
  munit_assert_not_null(ptr[0]);
  tcmplxA_brcvt_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_brcvt_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_brcvt_new(64,128,16);
}

void test_brcvt_teardown(void* fixture) {
  tcmplxA_brcvt_destroy((struct tcmplxA_brcvt*)fixture);
  return;
}

MunitResult test_brcvt_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  return MUNIT_OK;
}

MunitResult test_brcvt_metadata_ptr
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_assert(tcmplxA_brcvt_metadata(p) != NULL);
  munit_assert(tcmplxA_brcvt_metadata(p) ==
    tcmplxA_brcvt_metadata_c(p));
  return MUNIT_OK;
}

MunitResult test_brcvt_zsrtostr_none
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
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
    buf[3] = (len>>8)&0xff;
    buf[4] = (len)&0xff;
    buf[5] = (~(len>>8))&0xff;
    buf[6] = (~len)&0xff;
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
    int const res = tcmplxA_brcvt_zsrtostr
      (p, &ret, to_buf, 128, &src, src+total);
    munit_assert_int(res, ==, tcmplxA_Success);
    munit_assert_size(ret, ==, len);
    munit_assert_size(src-buf, ==, total);
    munit_assert_memory_equal(len, to_buf, buf+7);
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_brcvt, NULL, argc, argv);
}
