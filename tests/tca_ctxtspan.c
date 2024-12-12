/**
 * @brief Test program for context map
 */
#include "testfont.h"
#include "text-complex/access/ctxtspan.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_ctxtspan_guess
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtspan_guess_lsb
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtspan_guess_msb
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtspan_guess_utf8
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtspan_guess_signed
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtspan_subdivide
    (const MunitParameter params[], void* data);

static MunitTest tests_ctxtspan[] = {
  {"guess", test_ctxtspan_guess, NULL,NULL,0,NULL},
  {"guess/lsb", test_ctxtspan_guess_lsb, NULL,NULL,0,NULL},
  {"guess/msb", test_ctxtspan_guess_msb, NULL,NULL,0,NULL},
  {"guess/utf8", test_ctxtspan_guess_utf8, NULL,NULL,0,NULL},
  {"guess/signed", test_ctxtspan_guess_signed, NULL,NULL,0,NULL},
  {"subdivide", test_ctxtspan_subdivide, NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_ctxtspan = {
  "access/ctxtspan/", tests_ctxtspan, NULL, 1, 0
};



MunitResult test_ctxtspan_guess
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  munit_assert(score.vec[0] || score.vec[1] || score.vec[2] || score.vec[3]);
  return MUNIT_OK;
}

MunitResult test_ctxtspan_guess_lsb
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
  for (i = 0; i < sizeof(buf); ++i)
    buf[i] &= 63;
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  munit_assert(score.vec[tcmplxA_CtxtMap_LSB6] > score.vec[tcmplxA_CtxtMap_MSB6]);
  return MUNIT_OK;
}

MunitResult test_ctxtspan_guess_msb
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
  for (i = 0; i < sizeof(buf); ++i)
    buf[i] &= 252;
  tcmplxA_ctxtspan_guess(&score, buf, sizeof(buf));
  munit_assert(score.vec[tcmplxA_CtxtMap_MSB6] > score.vec[tcmplxA_CtxtMap_LSB6]);
  return MUNIT_OK;
}

MunitResult test_ctxtspan_guess_utf8
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
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
  munit_assert(score.vec[tcmplxA_CtxtMap_UTF8] > score.vec[tcmplxA_CtxtMap_LSB6]);
  return MUNIT_OK;
}


MunitResult test_ctxtspan_guess_signed
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtscore score = {{0}};
  unsigned char buf[32] = {0};
  int i;
  int sign;
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
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
    munit_assert(score.vec[tcmplxA_CtxtMap_Signed] > score.vec[tcmplxA_CtxtMap_UTF8]);
  }
  munit_assert(score.vec[tcmplxA_CtxtMap_Signed] > score.vec[tcmplxA_CtxtMap_MSB6]);
  return MUNIT_OK;
}

MunitResult test_ctxtspan_subdivide
    (const MunitParameter params[], void* data)
{
  unsigned char buf[32] = {0};
  unsigned len = munit_rand_uint32()%33;
  struct tcmplxA_ctxtspan spans = {0};
  size_t i;
  (void)params;
  (void)data;
  munit_rand_memory(sizeof(buf), buf);
  tcmplxA_ctxtspan_subdivide(&spans, buf, len, 0);
  munit_assert(spans.count <= tcmplxA_CtxtSpan_Size);
  munit_assert(spans.total_bytes == len);
  for (i = 0; i < spans.count; ++i) {
    size_t next;
    if (i == spans.count-1)
      next = spans.total_bytes;
    else
      next = spans.offsets[i+1];
    munit_assert(spans.offsets[i] < next);
    munit_assert(spans.modes[i] < tcmplxA_CtxtMap_ModeMax);
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_ctxtspan, NULL, argc, argv);
}
