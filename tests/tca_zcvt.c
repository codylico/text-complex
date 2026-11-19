/**
 * @brief Test program for zcvt state
 */
#include "testfont.h"
#include "text-complex/access/zcvt.h"
#include "text-complex/access/zutil.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_zcvt_cycle(void* data);
static int test_zcvt_item(void* data);
static int test_zcvt_zsrtostr_none(void* data);
static void* test_zcvt_setup(void* user_data);
static void test_zcvt_teardown(void* fixture);


static struct reedkiln_box const box_zcvt = {
  test_zcvt_setup,test_zcvt_teardown
};

static struct reedkiln_entry const tests_zcvt[] = {
  {"access/zcvt/cycle", test_zcvt_cycle },
  {"access/zcvt/in/none", test_zcvt_zsrtostr_none, 0,&box_zcvt },
  {NULL}
};





int test_zcvt_cycle(void* data)
{
  struct tcmplxA_zcvt* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_zcvt_new(64,128,16);
  reedkiln_assert(ptr[0]);
  tcmplxA_zcvt_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_zcvt_setup(void* user_data) {
  return tcmplxA_zcvt_new(64,128,16);
}

void test_zcvt_teardown(void* fixture) {
  tcmplxA_zcvt_destroy((struct tcmplxA_zcvt*)fixture);
  return;
}


int test_zcvt_zsrtostr_none(void* data)
{
  struct tcmplxA_zcvt* const p = (struct tcmplxA_zcvt*)data;
  unsigned char buf[128+11] = {8u,29u,1u};
  size_t const len = testfont_rand_size_range(0,128);
  if (p == NULL)
    return Reedkiln_SKIP;
  /* "compress" some data */{
    unsigned checksum;
    unsigned char *const check_buffer = buf+7+len;
    reedkiln_memrand(buf+7, len);
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
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(ret == len);
    reedkiln_assert(src-buf == total);
    reedkiln_assert(memcmp(len, to_buf, buf+7) == 0);
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(&tests_zcvt, argc, argv, NULL);
}
