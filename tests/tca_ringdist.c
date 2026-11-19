/**
 * @brief Test program for distance ring
 */
#include "testfont.h"
#include "text-complex/access/ringdist.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


struct test_ringdist_fixture {
  int special_tf;
  unsigned int direct_count;
  unsigned int postfix_size;
  struct tcmplxA_ringdist* rd;
};

static struct { unsigned int code; unsigned int bits; }
const test_ringdist_bit_comp_1951[30] = {
    { 0, 0}, {1, 0}, {2, 0}, {3, 0},
    { 4, 1}, {5, 1}, {6, 2}, {7, 2},
    { 8, 3}, {9, 3},{10, 4},{11, 4},
    {12, 5},{13, 5},{14, 6},{15, 6},
    {16, 7},{17, 7},{18, 8},{19, 8},
    {20, 9},{21, 9},{22,10},{23,10},
    {24,11},{25,11},{26,12},{27,12},
    {28,13},{29,13}
  };

static int test_ringdist_cycle(void* data);
static int test_ringdist_1951_bit_count(void* data);
static int test_ringdist_7932_bit_count(void* data);
static int test_ringdist_1951_decode(void* data);
static int test_ringdist_7932_decode(void* data);
static int test_ringdist_encode(void* data);
static int test_ringdist_reconfigure(void* data);
static void* test_ringdist_1951_setup(void* user_data);
static void* test_ringdist_7932_setup(void* user_data);
static void test_ringdist_teardown(void* fixture);


static struct reedkiln_box const box_ringdist_1951 = {
  test_ringdist_1951_setup,test_ringdist_teardown
};
static struct reedkiln_box const box_ringdist_7932 = {
  test_ringdist_7932_setup,test_ringdist_teardown
};

static struct reedkiln_entry const tests_ringdist[] = {
  {"access/ringdist/cycle", test_ringdist_cycle },
  {"access/ringdist/1951/bit_count", test_ringdist_1951_bit_count, 0, &box_ringdist_1951 },
  {"access/ringdist/7932/bit_count", test_ringdist_7932_bit_count, 0, &box_ringdist_7932 },
  {"access/ringdist/1951/decode", test_ringdist_1951_decode, 0, &box_ringdist_1951 },
  {"access/ringdist/7932/decode", test_ringdist_7932_decode, Reedkiln_TODO, &box_ringdist_7932 },
  {"access/ringdist/1951/encode", test_ringdist_encode, 0, &box_ringdist_1951 },
  {"access/ringdist/7932/encode", test_ringdist_encode, 0, &box_ringdist_7932 },
  {"access/ringdist/7932/reconfigure", test_ringdist_reconfigure, 0, &box_ringdist_7932 },
  {NULL}
};





int test_ringdist_cycle(void* data)
{
  struct tcmplxA_ringdist* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_ringdist_new(0,4,0);
  reedkiln_assert(ptr[0]);
  tcmplxA_ringdist_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_ringdist_1951_setup(void* user_data)
{
  struct test_ringdist_fixture *out = (struct test_ringdist_fixture*)calloc
    (1,sizeof(struct test_ringdist_fixture));
  if (out != NULL) {
    out->special_tf = 0;
    out->direct_count = 4;
    out->postfix_size = 0;
    out->rd = tcmplxA_ringdist_new
      (out->special_tf, out->direct_count, out->postfix_size);
    if (out->rd == NULL) {
      free(out);
      out = NULL;
    }
  }
  return out;
}

void* test_ringdist_7932_setup(void* user_data)
{
  struct tcmplxAtest_arg const* const user_arg =
    (struct tcmplxAtest_arg const*)user_data;
  struct test_ringdist_fixture *out = (struct test_ringdist_fixture*)calloc
    (1,sizeof(struct test_ringdist_fixture));
  if (out != NULL) {
    out->special_tf = 1;
    /* inspect params */{
      /* NDIRECT */{
        char const* p = user_arg->ndirect;
        char* ep = NULL;
        unsigned long int n = ~0lu;
        if (p != NULL) {
          n = strtoul(p,&ep,0);
        }
        if (ep != NULL && *ep == '\0' && n < UINT_MAX) {
          out->direct_count = (unsigned int)n;
        } else out->direct_count = testfont_rand_uint_range(0u,120u);
      }
      /* NPOSTFIX */{
        char const* p = user_arg->npostfix;
        char* ep = NULL;
        unsigned long int n = ~0lu;
        if (p != NULL) {
          n = strtoul(p,&ep,0);
        }
        if (ep != NULL && *ep == '\0' && n < UINT_MAX) {
          out->postfix_size = (unsigned int)n;
        } else out->postfix_size = testfont_rand_uint_range(0u,3u);
      }
    }
    out->rd = tcmplxA_ringdist_new
      (out->special_tf, out->direct_count, out->postfix_size);
    if (out->rd == NULL) {
      free(out);
      out = NULL;
    }
  }
  return out;
}

void test_ringdist_teardown(void* fixture) {
  struct test_ringdist_fixture *fixt = (struct test_ringdist_fixture*)fixture;
  if (fixt != NULL) {
    tcmplxA_ringdist_destroy(fixt->rd);
    free(fixt);
  }
  return;
}

int test_ringdist_1951_bit_count(void* data)
{
  struct test_ringdist_fixture *const fixt =
    (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* inspect */{
    size_t i;
    for (i = 0u; i < 30u; ++i) {
      reedkiln_assert(
          tcmplxA_ringdist_bit_count(p,test_ringdist_bit_comp_1951[i].code)
            == test_ringdist_bit_comp_1951[i].bits
        );
    }
  }
  return Reedkiln_OK;
}

int test_ringdist_7932_bit_count(void* data)
{
  struct test_ringdist_fixture *const fixt =
    (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  unsigned int alpha_size;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* recompute alphabet size */{
    alpha_size = 16 + fixt->direct_count + (48 << fixt->postfix_size);
  }
  /* iterate over the alphabet */{
    unsigned int dcode;
    unsigned int const ndirect = fixt->direct_count;
    unsigned int const npostfix = fixt->postfix_size;
    for (dcode = 0; dcode < alpha_size; ++dcode) {
      if (dcode < 16) {
        /* special code */
        reedkiln_assert(tcmplxA_ringdist_bit_count(p, dcode) == 0);
      } else if (dcode < 16 + ndirect) {
        /* direct code */
        reedkiln_assert(tcmplxA_ringdist_bit_count(p, dcode) == 0);
      } else {
        /* indirect code */
        unsigned int const ndistbits =
          1 + ((dcode - ndirect - 16) >> (npostfix + 1));
        reedkiln_assert(tcmplxA_ringdist_bit_count(p, dcode) == ndistbits);
      }
    }
  }
  return Reedkiln_OK;
}

int test_ringdist_1951_decode(void* data)
{
  struct test_ringdist_fixture *const fixt =
    (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  unsigned int back_dist;
  unsigned int decomposed_code;
  unsigned int decomposed_extra;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* compose 1951 encoding of distance */{
    unsigned int back_bit_count = 0u;
    unsigned int num_bits;
    back_dist = testfont_rand_uint_range(1u,32768u);
    /* count bits in distance */{
      unsigned int bd;
      for (bd = back_dist-1u; bd > 0u; bd >>= 1u) {
        back_bit_count += 1u;
      }
    }
    /* select bit length */{
      num_bits = back_bit_count > 2u ? (back_bit_count-2u) : 0u;
    }
    /* decompose */{
      if (num_bits == 0u) {
        decomposed_code = back_dist-1u;
        decomposed_extra = 0u;
      } else {
        decomposed_code = (num_bits*2u)+2u + (
              (back_dist-1u)&(1<<(back_bit_count-2u))
            ?1u:0u);
        decomposed_extra = (back_dist-1u)&((1<<num_bits)-1u);
      }
    }
  }
  /* inspect */{
    reedkiln_assert(
        tcmplxA_ringdist_decode(p,decomposed_code,decomposed_extra,0)
          == back_dist
      );
  }
  return Reedkiln_OK;
}

int test_ringdist_7932_decode(void* data)
{
  struct test_ringdist_fixture *const fixt =
    (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  if (p == NULL)
    return Reedkiln_SKIP;
  // TODO Implement this test.
  return Reedkiln_SKIP;
}

int test_ringdist_encode(void* data)
{
  struct test_ringdist_fixture *const fixt =
    (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  if (p == NULL)
    return Reedkiln_SKIP;
  /* round-trip test */{
    int j;
    struct tcmplxA_ringdist* const q = tcmplxA_ringdist_new
      (fixt->special_tf, fixt->direct_count, fixt->postfix_size);
    if (q == NULL)
      return Reedkiln_SKIP;
    else tcmplxA_ringdist_copy(q, p);
    for (j = 0; j < 100; ++j) {
      unsigned int back_dist = testfont_rand_uint_range(1u,32768u);
      tcmplxA_uint32 decomposed_extra;
      unsigned int decomposed_code;
      decomposed_code =
        tcmplxA_ringdist_encode(p, back_dist, &decomposed_extra,0);
      reedkiln_assert(decomposed_code != UINT_MAX);
      reedkiln_assert(
          tcmplxA_ringdist_decode(q,decomposed_code,decomposed_extra,0)
            == back_dist
        );
    }
    tcmplxA_ringdist_destroy(q);
  }
  return Reedkiln_OK;
}

int test_ringdist_reconfigure(void* data)
{
  struct test_ringdist_fixture *const fixt = (struct test_ringdist_fixture*)data;
  struct tcmplxA_ringdist* const p = (fixt!=NULL) ? fixt->rd : NULL;
  tcmplxA_uint32 decomposed_extra = 0;
  unsigned int decomposed_code = 0;
  unsigned int back_dist = testfont_rand_uint_range(1u,32768u);
  int res = 0;
  if (p == NULL)
    return Reedkiln_SKIP;
  decomposed_code = tcmplxA_ringdist_encode(p, back_dist, &decomposed_extra, 32768u);
  reedkiln_assert(decomposed_code != UINT_MAX);
  res = tcmplxA_ringdist_reconfigure(p, 1, testfont_rand_uint_range(0,120), fixt->postfix_size);
  reedkiln_assert(res == tcmplxA_Success);
  decomposed_code = tcmplxA_ringdist_encode(p, back_dist, &decomposed_extra, 32768u);
  reedkiln_assert(decomposed_code == 0);
  return Reedkiln_OK;
}




int main(int argc, char **argv) {
  struct tcmplxAtest_arg tfa = {NULL};
  int res = 0;
  tcmplxAtest_arg_init(&tfa);
  res = testfont_do_main(tests_ringdist, &tfa, argc, argv);
  tcmplxAtest_arg_close(&tfa);
  return res;
}
