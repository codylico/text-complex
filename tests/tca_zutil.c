
#include "text-complex/access/zutil.h"
#include "munit/munit.h"
#include <stdlib.h>
#include <string.h>


static MunitResult test_zutil_adler32
    (const MunitParameter params[], void* data);
static MunitResult test_zutil_adler32_long
    (const MunitParameter params[], void* data);

static MunitTest tests_zutil[] = {
  {"adler32", test_zutil_adler32, NULL,NULL,0,NULL},
  {"adler32/long", test_zutil_adler32_long, NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_zutil = {
  "access/zutil/", tests_zutil, NULL, 1, 0
};


static
unsigned long int test_adler32(unsigned char* b, size_t n);



unsigned long int test_adler32(unsigned char* b, size_t n) {
  unsigned long int s1 = 1u;
  unsigned long int s2 = 0u;
  size_t i;
  for (i = 0u; i < n; ++i) {
    s1 += b[i];
    s1 %= 65521;
    s2 += s1;
    s2 %= 65521;
  }
  return s1+(s2*65536);
}

MunitResult test_zutil_adler32
  (const MunitParameter params[], void* data)
{
  unsigned char buf[256];
  size_t const len = munit_rand_int_range(0,256);
  unsigned long int checksums[2];
  (void)params;
  (void)data;
  munit_rand_memory(len, (munit_uint8_t*)buf);
  checksums[0] = test_adler32(buf, len);
  checksums[1] = tcmplxA_zutil_adler32(len, buf, 1);
  munit_assert_ulong(checksums[1], ==, checksums[0]);
  return MUNIT_OK;
}

MunitResult test_zutil_adler32_long
  (const MunitParameter params[], void* data)
{
  unsigned char *buf;
  size_t const len = munit_rand_int_range(0,32767);
  unsigned long int checksums[3];
  (void)params;
  (void)data;
  buf = calloc(1, len);
  if (buf == NULL)
    return MUNIT_SKIP;
  munit_rand_memory(len, (munit_uint8_t*)buf);
  checksums[0] = test_adler32(buf, len);
  checksums[1] = tcmplxA_zutil_adler32(len, buf, 1);
  /* compute by parts */{
    size_t i;
    tcmplxA_uint32 chk = 1;
    for (i = 0; i < len; i += 200) {
      size_t const part_len = (len-i > 200) ? 200 : len-i;
      chk = tcmplxA_zutil_adler32(part_len, buf+i, chk);
    }
    checksums[2] = chk;
  }
  free(buf);
  munit_assert_ulong(checksums[1], ==, checksums[0]);
  munit_assert_ulong(checksums[2], ==, checksums[0]);
  return MUNIT_OK;
}

int main(int argc, char **argv) {
  return munit_suite_main(&suite_zutil, NULL, argc, argv);
}
