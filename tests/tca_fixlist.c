/**
 * @brief Test program for prefix list
 */
#include "testfont.h"
#include "text-complex/access/fixlist.h"
#include "reedkiln/reedkiln.h"
#include "reedkiln/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


static int test_fixlist_cycle(void* data);
static int test_fixlist_item(void* data);
static int test_fixlist_gen_codes(void* data);
static int test_fixlist_codesort(void* data);
static int test_fixlist_gen_lengths(void* data);
static int test_fixlist_preset(void* data);
static void* test_fixlist_setup(void* user_data);
static void* test_fixlist_gen_setup(void* user_data);
static void* test_fixlist_len_setup(void* user_data);
static void test_fixlist_teardown(void* fixture);


static struct reedkiln_box const box_fixlist = {
  test_fixlist_setup,test_fixlist_teardown
};
static struct reedkiln_box const box_fixlist_gen = {
  test_fixlist_gen_setup,test_fixlist_teardown
};
static struct reedkiln_box const box_fixlist_len = {
  test_fixlist_len_setup,test_fixlist_teardown
};

static struct reedkiln_entry const tests_fixlist[] = {
  {"access/fixlist/cycle", test_fixlist_cycle },
  {"access/fixlist/item", test_fixlist_item, 0, &box_fixlist },
  {"access/fixlist/gen_codes", test_fixlist_gen_codes, 0, &box_fixlist_gen },
  {"access/fixlist/preset", test_fixlist_preset, 0, &box_fixlist },
  {"access/fixlist/gen_lengths", test_fixlist_gen_lengths, 0, &box_fixlist_len },
  {"access/fixlist/codesort", test_fixlist_codesort, 0, &box_fixlist_gen },
  {NULL}
};





int test_fixlist_cycle(void* data)
{
  struct tcmplxA_fixlist* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_fixlist_new(288);
  reedkiln_assert(ptr[0]);
  tcmplxA_fixlist_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_fixlist_setup(void* user_data) {
  return tcmplxA_fixlist_new((size_t)munit_rand_int_range(4,256));
}

void* test_fixlist_gen_setup(void* user_data) {
  struct tcmplxAtest_fixlist_lex lex;
  struct tcmplxA_fixlist* out;
  struct tcmplxAtest_arg const* const user_arg = (struct tcmplxAtest_arg*)user_data;
  if (tcmplxAtest_fixlist_lex_start(&lex, user_arg->prefixes))
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

void* test_fixlist_len_setup(void* user_data) {
  struct tcmplxAtest_fixlist_lex lex;
  struct tcmplxA_fixlist* out;
  struct tcmplxAtest_arg const* const user_arg = (struct tcmplxAtest_arg*)user_data;
  int const start_res = tcmplxAtest_fixlist_lex_start(&lex, user_arg->frequencies);
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

int test_fixlist_item(void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  struct tcmplxA_fixline const* dsp[2];
  size_t sz;
  size_t i;
  if (p == NULL)
    return Reedkiln_SKIP;
  sz = tcmplxA_fixlist_size(p);
  i = (size_t)munit_rand_int_range(0,(int)(sz)-1);
  reedkiln_assert(sz >= 4);
  reedkiln_assert(sz <= 256);
  dsp[0] = tcmplxA_fixlist_at(p, i);
  dsp[1] = tcmplxA_fixlist_at_c(p, i);
  reedkiln_assert(dsp[0]);
  reedkiln_assert(dsp[1]);
  reedkiln_assert(dsp[0] == dsp[1]);
  return Reedkiln_OK;
}

int test_fixlist_gen_codes(void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_assert(tcmplxA_fixlist_gen_codes(p) == tcmplxA_Success);
  /* inspect the new codes */{
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p);
    reedkiln_log_printf("total %u", (unsigned)len);
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      reedkiln_log_printf("; [%u]={%#x l %u v %lu}",
        (unsigned)i, line->code, line->len, line->value);
      reedkiln_assert((line->code>>(line->len)) == 0u);
    }
  }
  return Reedkiln_OK;
}

int test_fixlist_codesort(void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_assert(tcmplxA_fixlist_gen_codes(p) == tcmplxA_Success);
  reedkiln_assert(tcmplxA_fixlist_codesort(p) == tcmplxA_Success);
  /* inspect the new codes */if (tcmplxA_fixlist_size(p) > 1) {
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p)-1;
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      struct tcmplxA_fixline const* const b_line = tcmplxA_fixlist_at_c(p,i+1);
      reedkiln_assert(line->len <= b_line->len);
      reedkiln_assert(
          (line->code<<(15-line->len)) <= (b_line->code<<(15-b_line->len))
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
      reedkiln_assert(j == i);
    }
  }
  return Reedkiln_OK;
}

int test_fixlist_gen_lengths(void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  tcmplxA_uint32* ph;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* extract the histogram */{
    size_t const sz = tcmplxA_fixlist_size(p);
    size_t i;
    ph = calloc(sz, sizeof(tcmplxA_uint32));
    for (i = 0; i < sz; ++i) {
      ph[i] = tcmplxA_fixlist_at_c(p,i)->len;
    }
  }
  reedkiln_assert(tcmplxA_fixlist_gen_lengths(p,ph,15) == tcmplxA_Success);
  /* inspect the new code lengths */{
    size_t i;
    size_t const len = tcmplxA_fixlist_size(p);
    size_t nonzero_count = 0u;
    unsigned long int sum = 0u;
    reedkiln_log_printf("total %u", (unsigned)len);
    for (i = 0; i < len; ++i) {
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(p,i);
      reedkiln_log_printf("; [%u]={l %u v %lu}",
        (unsigned)i, line->len, line->value);
      if (ph[i] > 0) {
        reedkiln_assert(line->len > 0u);
        reedkiln_assert(line->len <= 15u);
        sum += (32768u>>(line->len));
        nonzero_count += 1u;
      } else {
        reedkiln_assert(line->len == 0u);
      }
    }
    if (nonzero_count == 0u) {
      reedkiln_assert(sum == 0u);
    } else if (nonzero_count == 1u) {
      reedkiln_assert(sum == 16384u);
    } else reedkiln_assert(sum == 32768u);
  }
  free(ph);
  return Reedkiln_OK;
}

int test_fixlist_preset(void* data)
{
  struct tcmplxA_fixlist* const p = (struct tcmplxA_fixlist*)data;
  struct tcmplxA_fixline ds[2];
  unsigned int x;
  size_t i;
  size_t sz;
  if (p == NULL)
    return Reedkiln_SKIP;
  x = (unsigned int)testfont_rand_int_range(0,7);
  /* generate once */{
    reedkiln_assert(tcmplxA_fixlist_preset(p, x) == tcmplxA_Success);
  }
  /* */{
    sz = tcmplxA_fixlist_size(p);
    reedkiln_assert(sz > 0u);
    i = testfont_rand_size_range(0,sz-1);
    ds[0] = *tcmplxA_fixlist_at_c(p, i);
  }
  /* generate again */{
    reedkiln_assert(tcmplxA_fixlist_preset(p, x) == tcmplxA_Success);
    reedkiln_assert(tcmplxA_fixlist_size(p) == sz);
  }
  /* */{
    ds[1] = *tcmplxA_fixlist_at_c(p, i);
  }
  reedkiln_assert(ds[0].code == ds[1].code);
  reedkiln_assert(ds[0].len == ds[1].len);
  reedkiln_assert(ds[0].value == ds[1].value);
  return Reedkiln_OK;
}



int main(int argc, char **argv) {
  struct tcmplxAtest_arg tfa = {NULL};
  int res = 0;
  tcmplxAtest_arg_init(&tfa);
  res = testfont_do_main(&tests_fixlist, &tfa, argc, argv);
  tcmplxAtest_arg_close(&tfa);
  return res;
}
