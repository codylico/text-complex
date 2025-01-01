/**
 * @brief Test program for prefix list
 */
#include "testfont.h"
#include "text-complex/access/fixlist.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


static MunitResult test_fixlist_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_fixlist_item
  (const MunitParameter params[], void* data);
static MunitResult test_fixlist_gen_codes
  (const MunitParameter params[], void* data);
static MunitResult test_fixlist_codesort
  (const MunitParameter params[], void* data);
static MunitResult test_fixlist_gen_lengths
  (const MunitParameter params[], void* data);
static MunitResult test_fixlist_preset
  (const MunitParameter params[], void* data);
static void* test_fixlist_setup
    (const MunitParameter params[], void* user_data);
static void* test_fixlist_gen_setup
    (const MunitParameter params[], void* user_data);
static void* test_fixlist_len_setup
    (const MunitParameter params[], void* user_data);
static void test_fixlist_teardown(void* fixture);


static MunitParameterEnum test_fixlist_gen_params[] = {
  { "prefixes", NULL },
  { NULL, NULL },
};

static MunitParameterEnum test_fixlist_len_params[] = {
  { "frequencies", NULL },
  { NULL, NULL },
};

static MunitTest tests_fixlist[] = {
  {"cycle", test_fixlist_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_fixlist_item,
    test_fixlist_setup,test_fixlist_teardown,0,NULL},
  {"gen_codes", test_fixlist_gen_codes,
    test_fixlist_gen_setup,test_fixlist_teardown,0,test_fixlist_gen_params},
  {"preset", test_fixlist_preset,
    test_fixlist_setup,test_fixlist_teardown,0,NULL},
  {"gen_lengths", test_fixlist_gen_lengths,
    test_fixlist_len_setup,test_fixlist_teardown,0,test_fixlist_len_params},
  {"codesort", test_fixlist_codesort,
    test_fixlist_gen_setup,test_fixlist_teardown,0,test_fixlist_gen_params},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_fixlist = {
  "access/fixlist/", tests_fixlist, NULL, 1, 0
};




MunitResult test_fixlist_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_fixlist_new(288);
  munit_assert_not_null(ptr[0]);
  tcmplxA_fixlist_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_fixlist_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_fixlist_new((size_t)munit_rand_int_range(4,256));
}

void* test_fixlist_gen_setup(const MunitParameter params[], void* user_data) {
  struct tcmplxAtest_fixlist_lex lex;
  struct tcmplxA_fixlist* out;
  if (tcmplxAtest_fixlist_lex_start
    (&lex, munit_parameters_get(params, "prefixes")))
  {
    tcmplxAtest_fixlist_lex_start(&lex, "3*5,2,4*2");
  }
  out = tcmplxA_fixlist_new(lex.total);
  if (out != NULL) {
    size_t i;
    for (i = 0; i < lex.total; ++i) {
      tcmplxA_fixlist_at(out, i)->len = tcmplxAtest_fixlist_lex_next(&lex);
      tcmplxA_fixlist_at(out, i)->value = (unsigned int)i;
    }
  }
  return out;
}

void* test_fixlist_len_setup(const MunitParameter params[], void* user_data) {
  struct tcmplxAtest_fixlist_lex lex;
  struct tcmplxA_fixlist* out;
  int const start_res = tcmplxAtest_fixlist_lex_start
    (&lex, munit_parameters_get(params, "frequencies"));
  out = tcmplxA_fixlist_new
    (start_res==0 ? lex.total : munit_rand_int_range(2,384));
  if (out == NULL)
    return out;
  else if (start_res != 0) {
    size_t const len = tcmplxA_fixlist_size(out);
    size_t i;
    /* generate random */
    for (i = 0; i < len; ++i) {
      tcmplxA_fixlist_at(out, i)->len = munit_rand_int_range(0,32);
      tcmplxA_fixlist_at(out, i)->value = (unsigned int)i;
    }
  } else {
    size_t i;
    for (i = 0; i < lex.total; ++i) {
      tcmplxA_fixlist_at(out, i)->len = tcmplxAtest_fixlist_lex_next(&lex);
      tcmplxA_fixlist_at(out, i)->value = (unsigned int)i;
    }
  }
  return out;
}

void test_fixlist_teardown(void* fixture) {
  tcmplxA_fixlist_destroy((struct tcmplxA_fixlist*)fixture);
  return;
}

MunitResult test_fixlist_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  struct tcmplxA_fixline const* dsp[2];
  size_t sz;
  size_t i;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  sz = tcmplxA_fixlist_size(p);
  i = (size_t)munit_rand_int_range(0,(int)(sz)-1);
  munit_assert_size(sz,>=,4);
  munit_assert_size(sz,<=,256);
  dsp[0] = tcmplxA_fixlist_at(p, i);
  dsp[1] = tcmplxA_fixlist_at_c(p, i);
  munit_assert_not_null(dsp[0]);
  munit_assert_not_null(dsp[1]);
  munit_assert_ptr(dsp[0],==,dsp[1]);
  return MUNIT_OK;
}

MunitResult test_fixlist_gen_codes
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_assert_int(tcmplxA_fixlist_gen_codes(p),==, tcmplxA_Success);
  /* inspect the new codes */{
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p);
    munit_logf(MUNIT_LOG_DEBUG,
      "total %" MUNIT_SIZE_MODIFIER "u", len);
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      munit_logf(MUNIT_LOG_DEBUG,
        "  [%" MUNIT_SIZE_MODIFIER "u] = {%#x, l %u, v %lu}",
        i, line->code, line->len, line->value);
      munit_assert_uint((line->code>>(line->len)), ==, 0u);
    }
  }
  return MUNIT_OK;
}

MunitResult test_fixlist_codesort
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_assert_int(tcmplxA_fixlist_gen_codes(p),==, tcmplxA_Success);
  munit_assert_int(tcmplxA_fixlist_codesort(p),==, tcmplxA_Success);
  /* inspect the new codes */if (tcmplxA_fixlist_size(p) > 1) {
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p)-1;
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      struct tcmplxA_fixline const* const b_line = tcmplxA_fixlist_at_c(p,i+1);
      munit_assert_uint(line->len, <=, b_line->len);
      munit_assert_uint(
          (line->code<<(15-line->len)), <=, (b_line->code<<(15-b_line->len))
        );
    }
  }
  /* binary search */{
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p);
    for (i = 0; i < len; ++i) {
      unsigned int n;
      unsigned int bits;
      size_t j;
      /* */{
        struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
        n = line->len;
        bits = line->code;
      }
      if (n == 0)
        continue;
      j = tcmplxA_fixlist_codebsearch(p, n, bits);
      munit_assert_size(j, ==, i);
    }
  }
  return MUNIT_OK;
}

MunitResult test_fixlist_gen_lengths
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  tcmplxA_uint32* ph;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* extract the histogram */{
    size_t const sz = tcmplxA_fixlist_size(p);
    size_t i;
    ph = calloc(sz, sizeof(tcmplxA_uint32));
    for (i = 0; i < sz; ++i) {
      ph[i] = tcmplxA_fixlist_at_c(p,i)->len;
    }
  }
  munit_assert_int(tcmplxA_fixlist_gen_lengths(p,ph,15),==, tcmplxA_Success);
  /* inspect the new code lengths */{
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p);
    size_t nonzero_count = 0u;
    unsigned long int sum = 0u;
    munit_logf(MUNIT_LOG_DEBUG,
      "total %" MUNIT_SIZE_MODIFIER "u", len);
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      munit_logf(MUNIT_LOG_DEBUG,
        "  [%" MUNIT_SIZE_MODIFIER "u] = {-, l %u, v %lu}",
        i, line->len, line->value);
      if (ph[i] > 0) {
        munit_assert_uint(line->len, >, 0u);
        munit_assert_uint(line->len, <=, 15u);
        sum += (32768u>>(line->len));
        nonzero_count += 1u;
      } else {
        munit_assert_uint(line->len, ==, 0u);
      }
    }
    if (nonzero_count == 0u) {
      munit_assert_ulong(sum, ==, 0u);
    } else if (nonzero_count == 1u) {
      munit_assert_ulong(sum, ==, 16384u);
    } else munit_assert_ulong(sum, ==, 32768u);
  }
  free(ph);
  return MUNIT_OK;
}

MunitResult test_fixlist_preset
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  struct tcmplxA_fixline ds[2];
  unsigned int x;
  size_t i;
  size_t sz;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  x = (unsigned int)testfont_rand_int_range(0,7);
  /* generate once */{
    munit_assert_int(tcmplxA_fixlist_preset(p, x),==,tcmplxA_Success);
  }
  /* */{
    sz = tcmplxA_fixlist_size(p);
    munit_assert_size(sz,>,0u);
    i = testfont_rand_size_range(0,sz-1);
    ds[0] = *tcmplxA_fixlist_at_c(p, i);
  }
  /* generate again */{
    munit_assert_int(tcmplxA_fixlist_preset(p, x),==,tcmplxA_Success);
    munit_assert_size(tcmplxA_fixlist_size(p),==,sz);
  }
  /* */{
    ds[1] = *tcmplxA_fixlist_at_c(p, i);
  }
  munit_assert(ds[0].code == ds[1].code);
  munit_assert(ds[0].len == ds[1].len);
  munit_assert(ds[0].value == ds[1].value);
  return MUNIT_OK;
}



int main(int argc, char **argv) {
  return munit_suite_main(&suite_fixlist, NULL, argc, argv);
}
