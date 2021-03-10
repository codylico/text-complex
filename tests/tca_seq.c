/**
 * @brief Test program for sequential
 */
#include "../tcmplx-access/seq.h"
#include "mmaptwo/mmaptwo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "munit/munit.h"
#include "testfont.h"

#ifndef NDEBUG
#  define test_sharp2(s) #s
#  define test_sharp(s) test_sharp2(s)
#  define test_break(s) \
     { munit_error(s); return MUNIT_FAIL; }
#else
#  define test_break(s)
#endif /*NDEBUG*/

struct test_seq_fixture {
  struct tcmplxA_seq* seq;
  struct mmaptwo_i* mapper;
  int gen;
  unsigned int seed;
  size_t len;
};

static MunitResult test_seq_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_seq_null_eof
  (const MunitParameter params[], void* data);
static MunitResult test_seq_eof
  (const MunitParameter params[], void* data);
static MunitResult test_seq_seek
  (const MunitParameter params[], void* data);
static MunitResult test_seq_whence
  (const MunitParameter params[], void* data);

static void* test_seq_null_setup
    (const MunitParameter params[], void* user_data);
static void* test_seq_setup
    (const MunitParameter params[], void* user_data);
static void test_seq_teardown(void* fixture);


static MunitTest tests_seq[] = {
  {"cycle", test_seq_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"null_eof", test_seq_null_eof,
    test_seq_null_setup,test_seq_teardown,0,NULL},
  {"eof", test_seq_eof,
    test_seq_setup,test_seq_teardown,0,NULL},
  {"seek", test_seq_seek,
    test_seq_setup,test_seq_teardown,0,NULL},
  {"null_seek", test_seq_seek,
    test_seq_null_setup,test_seq_teardown,0,NULL},
  {"whence", test_seq_whence,
    test_seq_setup,test_seq_teardown,0,NULL},
  {"null_whence", test_seq_whence,
    test_seq_null_setup,test_seq_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_seq = {
  "access/seq/", tests_seq, NULL, 1, 0
};




MunitResult test_seq_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_seq* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_seq_new(NULL);
  munit_assert_not_null(ptr[0]);
  tcmplxA_seq_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_seq_null_setup(const MunitParameter params[], void* user_data) {
  struct test_seq_fixture* fixt = munit_new(struct test_seq_fixture);
  fixt->seq = tcmplxA_seq_new(NULL);
  fixt->mapper = NULL;
  fixt->len = 0u;
  fixt->gen = -1;
  return fixt;
}

void* test_seq_setup(const MunitParameter params[], void* user_data) {
  struct test_seq_fixture* fixt = munit_new(struct test_seq_fixture);
  fixt->gen = munit_rand_int_range(0,tcmplxAtest_MAX);
  fixt->len = (size_t)munit_rand_int_range(0,32767);
  if (fixt->len > tcmplxAtest_gen_size(fixt->gen))
    fixt->len = tcmplxAtest_gen_size(fixt->gen);
  fixt->seed = (unsigned int)munit_rand_uint32();
  fixt->mapper = tcmplxAtest_gen_maptwo(fixt->gen, fixt->len, fixt->seed);
  if (fixt->mapper != NULL) {
    fixt->seq = tcmplxA_seq_new(fixt->mapper);
  } else fixt->seq = NULL;
  return fixt;
}

void test_seq_teardown(void* fixture) {
  struct test_seq_fixture* fixt = (struct test_seq_fixture*)fixture;
  tcmplxA_seq_destroy(fixt->seq);
  if (fixt->mapper)
    mmaptwo_close(fixt->mapper);
  return;
}

MunitResult test_seq_null_eof
  (const MunitParameter params[], void* data)
{
  struct test_seq_fixture* fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_assert_size(tcmplxA_seq_get_pos(p),==,0u);
  munit_assert_int(tcmplxA_seq_get_byte(p),==,-1);
  munit_assert_size(tcmplxA_seq_get_pos(p),==,0u);
  return MUNIT_OK;
}

MunitResult test_seq_eof
  (const MunitParameter params[], void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_logf(MUNIT_LOG_INFO,
    "inspecting until %" MUNIT_SIZE_MODIFIER "u", fixt->len);
  if (fixt->len > 0u) {
    size_t i;
    for (i = 0u; i < fixt->len; ++i) {
      int b;
      int const d = tcmplxAtest_gen_datum(fixt->gen, i, fixt->seed);
      munit_assert_size(tcmplxA_seq_get_pos(p),==,i);
      b = tcmplxA_seq_get_byte(p);
      if (b!=d)
        munit_logf(MUNIT_LOG_WARNING, "breaks at %" MUNIT_SIZE_MODIFIER "u", i);
      munit_assert_int(b,==,d);
    }
    munit_assert_size(tcmplxA_seq_get_pos(p),==,i);
    munit_assert_int(tcmplxA_seq_get_byte(p),==,-1);
  } else {
    munit_assert_size(tcmplxA_seq_get_pos(p),==,0u);
    munit_assert_int(tcmplxA_seq_get_byte(p),==,-1);
    munit_assert_size(tcmplxA_seq_get_pos(p),==,0u);
  }
  return MUNIT_OK;
}

MunitResult test_seq_seek
  (const MunitParameter params[], void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_logf(MUNIT_LOG_INFO,
    "inspecting until %" MUNIT_SIZE_MODIFIER "u", fixt->len);
  /* */{
    int j;
    for (j = 0; j < 20; ++j) {
      size_t i;
      if (j == 0u)
        i = fixt->len;
      else if (j == 1u)
        i = fixt->len-1u;
      else if (j == 19)
        i = 0u;
      else
        i = (size_t)munit_rand_int_range(0, 32767);
      if (i > fixt->len) {
        /* expect failure */
        size_t res = tcmplxA_seq_set_pos(p,i);
        if (res != ~(size_t)0u)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks past %" MUNIT_SIZE_MODIFIER "u", i);
        munit_assert_size(~(size_t)0u,==,res);
      } else if (i == fixt->len) {
        int b;
        int const d = -1;
        munit_assert_size(i,==,tcmplxA_seq_set_pos(p,i));
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks at the end (%" MUNIT_SIZE_MODIFIER "u)", i);
        munit_assert_int(b,==,d);
      } else {
        /* expect success */
        int b;
        int const d = tcmplxAtest_gen_datum(fixt->gen, i, fixt->seed);
        munit_assert_size(i,==,tcmplxA_seq_set_pos(p,i));
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks at %" MUNIT_SIZE_MODIFIER "u", i);
        munit_assert_int(b,==,d);
      }
    }
  }
  return MUNIT_OK;
}

MunitResult test_seq_whence
  (const MunitParameter params[], void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_logf(MUNIT_LOG_INFO,
    "inspecting until %" MUNIT_SIZE_MODIFIER "u", fixt->len);
  /* */{
    int j;
    size_t i = 0u;
    size_t const fail = ~(size_t)0u;
    for (j = 0; j < 20; ++j) {
      long int const dist = munit_rand_int_range(-32767, 32767);
      int whence = munit_rand_int_range(0,2);
      size_t landing_i;
      /* predict where the read position should land */
      switch (whence) {
      case tcmplxA_SeqSet:
        landing_i = (dist < 0) ? fail : (size_t)(dist);
        break;
      case tcmplxA_SeqCur:
        if (dist < 0) {
          size_t pdist = (size_t)(-dist);
          landing_i = (pdist > i) ? fail : i-pdist;
        } else {
          size_t pdist = (size_t)(dist);
          landing_i = (pdist > fixt->len-i) ? fail : i+pdist;
        }
        break;
      case tcmplxA_SeqEnd:
        if (dist > 0)
          landing_i = fail;
        else {
          size_t pdist = (size_t)(-dist);
          landing_i = (pdist > fixt->len) ? fail : fixt->len-pdist;
        }
        break;
      }
      if (landing_i > fixt->len) {
        /* expect failure */
        long int res = tcmplxA_seq_seek(p, dist, whence);
        if (res >= -1L)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks past %" MUNIT_SIZE_MODIFIER "u to "
            "%" MUNIT_SIZE_MODIFIER "u (%li,%i)",
            i,landing_i,dist,(int)(whence));
        munit_assert_long(res,<,-1L);
      } else if (landing_i == fixt->len) {
        int b;
        int const d = -1;
        long int res = tcmplxA_seq_seek(p,dist,whence);
        if (res != (long int)(landing_i))
          munit_logf(MUNIT_LOG_WARNING,
            "breaks from %" MUNIT_SIZE_MODIFIER "u to "
            "%" MUNIT_SIZE_MODIFIER "u (%li,%i)",
            i,landing_i,dist,(int)(whence));
        munit_assert_long((long int)(landing_i),==,res);
        i = landing_i;
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks at the end (%" MUNIT_SIZE_MODIFIER "u)", i);
        munit_assert_int(b,==,d);
      } else {
        /* expect success */
        int b;
        int const d = tcmplxAtest_gen_datum(fixt->gen, landing_i, fixt->seed);
        long int res = tcmplxA_seq_seek(p,dist,whence);
        if (res != (long int)landing_i)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks from %" MUNIT_SIZE_MODIFIER "u to "
            "%" MUNIT_SIZE_MODIFIER "u (%li,%i)",
            i,landing_i,dist,(int)whence);
        munit_assert_long((long int)landing_i,==,res);
        i = landing_i;
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          munit_logf(MUNIT_LOG_WARNING,
            "breaks at %" MUNIT_SIZE_MODIFIER "u", i);
        munit_assert_int(b,==,d);
        i += 1u;
      }
    }
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_seq, NULL, argc, argv);
}
