/**
 * @brief Test program for brcvt state
 */
#include "testfont.h"
#include "text-complex/access/brcvt.h"
#include "text-complex/access/zutil.h"
#include "text-complex/access/brmeta.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_brcvt_cycle(void* data);
static int test_brcvt_item(void* data);
static int test_brcvt_metadata_ptr(void* data);
static int test_brcvt_metadata_cycle(void* data);
static int test_brcvt_zsrtostr_none(void* data);
static int test_brcvt_flush(void* data);
static void* test_brcvt_setup(void* user_data);
static void test_brcvt_teardown(void* fixture);

static struct reedkiln_box const box_brcvt = {
  &test_brcvt_setup, &test_brcvt_teardown
};

static struct reedkiln_entry const tests_brcvt[] = {
  {"access/brcvt/cycle", test_brcvt_cycle },
  {"access/brcvt/metadata_ptr", test_brcvt_metadata_ptr, 0, &box_brcvt },
  {"access/brcvt/metadata_cycle", test_brcvt_metadata_cycle, 0, &box_brcvt },
  {"access/brcvt/in/none", test_brcvt_zsrtostr_none, 0,&box_brcvt },
  {"access/brcvt/flush", test_brcvt_flush, 0,&box_brcvt },
  {NULL}
};




int test_brcvt_cycle(void* data)
{
  struct tcmplxA_brcvt* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_brcvt_new(64,128,16);
  reedkiln_assert(ptr[0]);
  tcmplxA_brcvt_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_brcvt_setup(void* user_data) {
  return tcmplxA_brcvt_new(64,128,16);
}

void test_brcvt_teardown(void* fixture) {
  tcmplxA_brcvt_destroy((struct tcmplxA_brcvt*)fixture);
  return;
}


int test_brcvt_metadata_ptr(void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_assert(tcmplxA_brcvt_metadata(p) != NULL);
  reedkiln_assert(tcmplxA_brcvt_metadata(p) ==
    tcmplxA_brcvt_metadata_c(p));
  return Reedkiln_OK;
}

int test_brcvt_metadata_cycle(void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  struct tcmplxA_brcvt* const q = tcmplxA_brcvt_new(4096,4096,4096);
  unsigned char text[16] = {0};
  int const text_len = testfont_rand_int_range(1,16);
  unsigned char buf[256] = {0};
  size_t buf_len = 0;
  reedkiln_memrand(&text[0], sizeof(text));
  if (p == NULL || q == NULL) {
    tcmplxA_brcvt_destroy(q);
    return Reedkiln_SKIP;
  }
  /* store */
  {
    struct tcmplxA_brmeta* const meta = tcmplxA_brcvt_metadata(p);
    int const res = tcmplxA_brmeta_emplace(meta, text_len);
    reedkiln_assert(res == tcmplxA_Success);
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
    reedkiln_assert(res == tcmplxA_ErrPartial);
    reedkiln_assert(buf_len <= sizeof(buf));
    res = tcmplxA_brcvt_delimrtozs(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    reedkiln_assert(res >= tcmplxA_Success);
    reedkiln_assert(exbuf_len <= sizeof(buf)-buf_len);
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
    reedkiln_assert(res >= tcmplxA_Success);
  }
  /* inspect */
  {
    struct tcmplxA_brmeta const* q_meta = tcmplxA_brcvt_metadata_c(q);
    reedkiln_assert(q_meta != NULL);
    reedkiln_assert(tcmplxA_brmeta_size(q_meta) == 1);
    reedkiln_assert(tcmplxA_brmeta_itemsize(q_meta,0) == text_len);
    reedkiln_assert(
      memcmp(tcmplxA_brmeta_itemdata(q_meta,0), text, text_len) == 0);
  }
  tcmplxA_brcvt_destroy(q);
  return Reedkiln_OK;
}

int test_brcvt_zsrtostr_none(void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  unsigned char buf[128+5] = {12u};
  size_t const len = (size_t)munit_rand_int_range(0,128);
  if (p == NULL)
    return Reedkiln_SKIP;
  /* See Section 11.1 of RFC 7932: `BrotliCompressTrivial` */
  /* "compress" some data */if (len == 0) {
    buf[0] = 6;
  } else {
    size_t const r = len - 1;
    unsigned char *const check_buffer = buf+4+len;
    reedkiln_memrand(buf+4, len);
    buf[1] = (unsigned char)((r & 31u) << 3);
    buf[2] = (unsigned char)(r >> 5);
    buf[3] = (unsigned char)(8 + (r >> 13));
    check_buffer[0] = 3;
  }
  /* extract some data */{
    size_t const total = (len > 0) ? 5+len : 1;
    unsigned char to_buf[128];
    size_t ret = 0;
    unsigned char const* src = buf;
    int const res = tcmplxA_brcvt_zsrtostr
      (p, &ret, to_buf, 128, &src, src+total);
    reedkiln_assert(res == tcmplxA_EOF);
    reedkiln_assert(ret == len);
    reedkiln_assert(src-buf == total);
    reedkiln_assert(memcmp(to_buf, buf+4, len) == 0);
  }
  return Reedkiln_OK;
}

int test_brcvt_flush(void* data)
{
  struct tcmplxA_brcvt* const p = (struct tcmplxA_brcvt*)data;
  struct tcmplxA_brcvt* const q = tcmplxA_brcvt_new(4096,4096,4096);
  unsigned char text[16] = {0};
  int const text_len = munit_rand_int_range(3,16);
  int const flush_len = munit_rand_int_range(1,text_len-1);
  unsigned char buf[256] = {0};
  size_t buf_len = 0;
  reedkiln_memrand(&text[0], sizeof(text));
  if (p == NULL || q == NULL) {
    tcmplxA_brcvt_destroy(q);
    return Reedkiln_SKIP;
  }
  /* encode */
  {
    unsigned char const *text_p = text;
    size_t exbuf_len = 0;
    int res;
    res = tcmplxA_brcvt_strrtozs(p, &buf_len, buf, sizeof(buf),
      &text_p, text+flush_len);
    reedkiln_assert(res == tcmplxA_ErrPartial);
    reedkiln_assert(buf_len <= sizeof(buf));
    res = tcmplxA_brcvt_flush(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    reedkiln_assert(res == tcmplxA_ErrPartial);
    reedkiln_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
    exbuf_len = 0;
    res = tcmplxA_brcvt_strrtozs(p,
      &exbuf_len, buf+buf_len, sizeof(buf)-buf_len,
      &text_p, text+text_len);
    reedkiln_assert(res == tcmplxA_ErrPartial);
    reedkiln_assert(exbuf_len <= sizeof(buf)-buf_len);
    buf_len += exbuf_len;
    exbuf_len = 0;
    res = tcmplxA_brcvt_delimrtozs(p, &exbuf_len,
      buf+buf_len, sizeof(buf)-buf_len);
    reedkiln_assert(res >= tcmplxA_EOF);
    reedkiln_assert(exbuf_len <= sizeof(buf)-buf_len);
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
    reedkiln_assert(res >= tcmplxA_Success);
    reedkiln_assert(dummy_len == text_len);
    reedkiln_assert_memory_equal(text_len, dummy, text);
  }
  tcmplxA_brcvt_destroy(q);
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(&tests_brcvt, argc, argv, NULL);
}
