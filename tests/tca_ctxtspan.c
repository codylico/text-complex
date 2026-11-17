/**
 * @brief Test program for context map
 */
#include "testfont.h"
#include "text-complex/access/ctxtspan.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_ctxtspan_guess(void* data);
static int test_ctxtspan_guess_lsb(void* data);
static int test_ctxtspan_guess_msb(void* data);
static int test_ctxtspan_guess_utf8(void* data);
static int test_ctxtspan_guess_signed(void* data);
static int test_ctxtspan_subdivide(void* data);

static struct reedkiln_entry const tests_ctxtspan[] = {
  {"access/ctxtspan/guess", test_ctxtspan_guess },
  {"access/ctxtspan/guess/lsb", test_ctxtspan_guess_lsb },
  {"access/ctxtspan/guess/msb", test_ctxtspan_guess_msb },
  {"access/ctxtspan/guess/utf8", test_ctxtspan_guess_utf8 },
  {"access/ctxtspan/guess/signed", test_ctxtspan_guess_signed },
  {"access/ctxtspan/subdivide", test_ctxtspan_subdivide },
  {NULL}
};



int test_ctxtspan_guess(void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  reedkiln_assert(score.vec[0] || score.vec[1] || score.vec[2] || score.vec[3]);
  return Reedkiln_OK;
}

int test_ctxtspan_guess_lsb(void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  for (i = 0; i < sizeof(buf); ++i)
    buf[i] &= 63;
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  reedkiln_assert(score.vec[tcmplxA_CtxtMap_LSB6] > score.vec[tcmplxA_CtxtMap_MSB6]);
  return Reedkiln_OK;
}

int test_ctxtspan_guess_msb(void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  for (i = 0; i < sizeof(buf); ++i)
    buf[i] &= 252;
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  reedkiln_assert(score.vec[tcmplxA_CtxtMap_MSB6] > score.vec[tcmplxA_CtxtMap_LSB6]);
  return Reedkiln_OK;
}

int test_ctxtspan_guess_utf8(void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  for (i = 0; i < sizeof(buf); ++i) {
    if ((buf[i] & 0x80) && (i < sizeof(buf)-1)) {
      buf[i] = (buf[i] | 0xC0) & 0xDF;
      i += 1;
      buf[i] = (buf[i] | 0x80) & 0xBF;
    } else {
      buf[i] &= 0x7F;
    }
  }
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  reedkiln_assert(score.vec[tcmplxA_CtxtMap_UTF8] > score.vec[tcmplxA_CtxtMap_LSB6]);
  return Reedkiln_OK;
}


int test_ctxtspan_guess_signed(void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  int sign;
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  sign = (buf[0] & 128);
  if (sign) {
    for (i = 1; i < sizeof(buf); ++i)
      buf[i] |= 224;
  } else {
    for (i = 1; i < sizeof(buf); ++i)
      buf[i] &= 31;
  }
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  if (sign) {
    reedkiln_assert(score.vec[tcmplxA_CtxtMap_Signed] > score.vec[tcmplxA_CtxtMap_UTF8]);
  }
  reedkiln_assert(score.vec[tcmplxA_CtxtMap_Signed] > score.vec[tcmplxA_CtxtMap_MSB6]);
  return Reedkiln_OK;
}

int test_ctxtspan_subdivide(void* data)
{
  unsigned char buf[32] = {0};
  unsigned len = munit_rand_uint32()%33;
  struct tcmplxA_ctxtspan spans = {0};
  size_t i;
  (void)data;
  reedkiln_memrand(buf, sizeof(buf));
  tcmplxA_ctxtspan_subdivide(&spans, buf, len, 0);
  reedkiln_assert(spans.count <= tcmplxA_CtxtSpan_Size);
  reedkiln_assert(spans.total_bytes == len);
  for (i = 0; i < spans.count; ++i) {
    size_t next;
    if (i == spans.count-1)
      next = spans.total_bytes;
    else
      next = spans.offsets[i+1];
    reedkiln_assert(spans.offsets[i] < next);
    reedkiln_assert(spans.modes[i] < tcmplxA_CtxtMap_ModeMax);
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(&tests_ctxtspan, argc, argv, NULL);
}
