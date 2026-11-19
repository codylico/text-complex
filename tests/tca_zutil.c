
#include "testfont.h"
#include "text-complex/access/zutil.h"
#include "reedkiln/reedkiln.h"
#include <stdlib.h>
#include <string.h>


static int test_zutil_adler32(void* data);
static int test_zutil_adler32_long(void* data);

static struct reedkiln_entry const tests_zutil[] = {
  {"access/zutil/adler32", test_zutil_adler32 },
  {"access/zutil/adler32/long", test_zutil_adler32_long },
  {NULL}
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

int test_zutil_adler32(void* data)
{
  unsigned char buf[256];
  size_t const len = testfont_rand_size_range(0,256);
  unsigned long int checksums[2];
  (void)data;
  reedkiln_memrand(buf, len);
  checksums[0] = test_adler32(buf, len);
  checksums[1] = tcmplxA_zutil_adler32(len, buf, 1);
  reedkiln_assert(checksums[1] == checksums[0]);
  return Reedkiln_OK;
}

int test_zutil_adler32_long(void* data)
{
  unsigned char *buf;
  size_t const len = testfont_rand_size_range(0,32767);
  unsigned long int checksums[3];
  (void)data;
  buf = calloc(1, len);
  if (buf == NULL)
    return Reedkiln_IGNORE;
  reedkiln_memrand(buf, len);
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
  reedkiln_assert(checksums[1] == checksums[0]);
  reedkiln_assert(checksums[2] == checksums[0]);
  return Reedkiln_OK;
}

int main(int argc, char **argv) {
  return reedkiln_main(tests_zutil, argc, argv, NULL);
}
