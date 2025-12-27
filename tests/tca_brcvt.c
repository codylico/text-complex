/**
 * @brief Test program for brcvt state
 */
#include "testfont.h"
#include "text-complex/access/brcvt.h"
#include "text-complex/access/zutil.h"
#include "text-complex/access/brmeta.h"
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
static MunitResult test_brcvt_metadata_cycle
  (const MunitParameter params[], void* data);
static MunitResult test_brcvt_zsrtostr_none
  (const MunitParameter params[], void* data);
static MunitResult test_brcvt_flush
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
  {"metadata_cycle", test_brcvt_metadata_cycle,
    test_brcvt_setup,test_brcvt_teardown,0,test_brcvt_params},
  {"in/none", test_brcvt_zsrtostr_none,
    test_brcvt_setup,test_brcvt_teardown,MUNIT_TEST_OPTION_TODO,NULL},
  {"flush", test_brcvt_flush,
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

MunitResult test_brcvt_metadata_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  struct tcmplxA_brcvt* const q = tcmplxA_brcvt_new(4096,4096,4096);
  unsigned char text[16] = {0};
  int const text_len = munit_rand_int_range(1,16);
  unsigned char buf[256] = {0};
  size_t buf_len = 0;
  munit_rand_memory(sizeof(text), &text[0]);
  if (p == NULL || q == NULL) {
    tcmplxA_brcvt_destroy(q);
    return MUNIT_SKIP;
  }
  (void)params;
  /* store */
  {
    struct tcmplxA_brmeta* const meta = tcmplxA_brcvt_metadata(p);
    int const res = tcmplxA_brmeta_emplace(meta, text_len);
    munit_assert(res == tcmplxA_Success);
    memcpy(tcmplxA_brmeta_itemdata(meta,0), text, text_len);
  }
  /* encode */
  {
    unsigned char dummy[1] = {0};
    unsigned char const *dummy_p = dummy;
    size_t exbuf_len = 0;
    int res;
    res = tcmplxA_brcvt_strrtozs(p, &buf_len, buf, sizeof(buf),
      &dummy_p, dummy+sizeof(dummy));
    munit_assert(res == tcmplxA_ErrPartial);
    munit_assert(buf_len <= sizeof(buf));
    res = tcmplxA_brcvt_delimrtozs(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    munit_assert(res >= tcmplxA_Success);
    munit_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
  }
  /* decode */
  {
    unsigned char dummy[16] = {0};
    size_t dummy_len = 0;
    int res;
    unsigned char const* buf_ptr = buf;
    res = tcmplxA_brcvt_zsrtostr(q, &dummy_len, dummy, sizeof(dummy),
      &buf_ptr, buf+buf_len);
    munit_assert(res >= tcmplxA_Success);
  }
  /* inspect */
  {
    struct tcmplxA_brmeta const* q_meta = tcmplxA_brcvt_metadata_c(q);
    munit_assert(q_meta != NULL);
    munit_assert(tcmplxA_brmeta_size(q_meta) == 1);
    munit_assert(tcmplxA_brmeta_itemsize(q_meta,0) == text_len);
    munit_assert_memory_equal(text_len,
      tcmplxA_brmeta_itemdata(q_meta,0), text);
  }
  tcmplxA_brcvt_destroy(q);
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

MunitResult test_brcvt_flush
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  struct tcmplxA_brcvt* const q = tcmplxA_brcvt_new(4096,4096,4096);
  unsigned char text[16] = {0};
  int const text_len = munit_rand_int_range(3,16);
  int const flush_len = munit_rand_int_range(1,text_len-1);
  unsigned char buf[256] = {0};
  size_t buf_len = 0;
  munit_rand_memory(sizeof(text), &text[0]);
  if (p == NULL || q == NULL) {
    tcmplxA_brcvt_destroy(q);
    return MUNIT_SKIP;
  }
  (void)params;
  /* encode */
  {
    unsigned char const *text_p = text;
    size_t exbuf_len = 0;
    int res;
    res = tcmplxA_brcvt_strrtozs(p, &buf_len, buf, sizeof(buf),
      &text_p, text+flush_len);
    munit_assert(res == tcmplxA_Success);
    munit_assert(buf_len <= sizeof(buf));
    res = tcmplxA_brcvt_flush(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    munit_assert(res == tcmplxA_Success);
    munit_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
    exbuf_len = 0;
    res = tcmplxA_brcvt_strrtozs(p,
      &exbuf_len, buf+buf_len, sizeof(buf)-buf_len,
      &text_p, text+text_len);
    munit_assert(res == tcmplxA_Success);
    munit_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
    exbuf_len = 0;
    res = tcmplxA_brcvt_delimrtozs(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    munit_assert(res >= tcmplxA_Success);
    munit_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
  }
  /* decode */
  {
    unsigned char dummy[16] = {0};
    size_t dummy_len = 0;
    int res;
    unsigned char const* buf_ptr = buf;
    res = tcmplxA_brcvt_zsrtostr(q, &dummy_len, dummy, sizeof(dummy),
      &buf_ptr, buf+buf_len);
    munit_assert(res >= tcmplxA_Success);
    munit_assert(dummy_len == text_len);
    munit_assert_memory_equal(text_len, dummy, text);
  }
  tcmplxA_brcvt_destroy(q);
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_brcvt, NULL, argc, argv);
}
