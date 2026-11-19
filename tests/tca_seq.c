/**
 * @brief Test program for sequential
 */
#include "testfont.h"
#include "text-complex/access/seq.h"
#include "mmaptwo/mmaptwo.h"
#include "reedkiln/log.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct test_seq_fixture {
  struct tcmplxA_seq* seq;
  struct mmaptwo_i* mapper;
  int gen;
  unsigned int seed;
  size_t len;
};

static int test_seq_cycle(void* data);
static int test_seq_null_eof(void* data);
static int test_seq_eof(void* data);
static int test_seq_seek(void* data);
static int test_seq_whence(void* data);

static void* test_seq_null_setup(void* user_data);
static void* test_seq_setup(void* user_data);
static void test_seq_teardown(void* fixture);


static struct reedkiln_box const box_seq_null = {
  test_seq_null_setup,test_seq_teardown
};
static struct reedkiln_box const box_seq = {
  test_seq_setup,test_seq_teardown
};

static struct reedkiln_entry const tests_seq[] = {
  {"access/seq/cycle", test_seq_cycle },
  {"access/seq/null_eof", test_seq_null_eof, 0,&box_seq_null },
  {"access/seq/eof", test_seq_eof, 0,&box_seq },
  {"access/seq/seek", test_seq_seek, 0,&box_seq },
  {"access/seq/null_seek", test_seq_seek, 0,&box_seq_null },
  {"access/seq/whence", test_seq_whence, 0,&box_seq },
  {"access/seq/null_whence", test_seq_whence, 0,&box_seq_null },
  {NULL}
};





int test_seq_cycle(void* data)
{
  struct tcmplxA_seq* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_seq_new(NULL);
  reedkiln_assert(ptr[0]);
  tcmplxA_seq_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_seq_null_setup(void* user_data) {
  struct test_seq_fixture* fixt =
    (struct test_seq_fixture*)calloc(1, sizeof(struct test_seq_fixture));
  fixt->seq = tcmplxA_seq_new(NULL);
  fixt->mapper = NULL;
  fixt->len = 0u;
  fixt->gen = -1;
  return fixt;
}

void* test_seq_setup(void* user_data) {
  struct test_seq_fixture* fixt =
    (struct test_seq_fixture*)calloc(1, sizeof(struct test_seq_fixture));
  fixt->gen = testfont_rand_int_range(0,tcmplxAtest_MAX);
  fixt->len = testfont_rand_size_range(0,32767);
  if (fixt->len > tcmplxAtest_gen_size(fixt->gen))
    fixt->len = tcmplxAtest_gen_size(fixt->gen);
  fixt->seed = reedkiln_rand();
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
  free(fixt);
  return;
}

int test_seq_null_eof(void* data)
{
  struct test_seq_fixture* fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_assert(tcmplxA_seq_get_pos(p) == 0u);
  reedkiln_assert(tcmplxA_seq_get_byte(p) == -1);
  reedkiln_assert(tcmplxA_seq_get_pos(p) == 0u);
  return Reedkiln_OK;
}

int test_seq_eof(void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_log_printf("inspecting until %i;", (unsigned)fixt->len);
  if (fixt->len > 0u) {
    size_t i;
    for (i = 0u; i < fixt->len; ++i) {
      int b;
      int const d = tcmplxAtest_gen_datum(fixt->gen, i, fixt->seed);
      reedkiln_assert(tcmplxA_seq_get_pos(p) == i);
      b = tcmplxA_seq_get_byte(p);
      if (b!=d)
        reedkiln_log_printf("breaks at %i;", (int)i);
      reedkiln_assert(b == d);
    }
    reedkiln_assert(tcmplxA_seq_get_pos(p) == i);
    reedkiln_assert(tcmplxA_seq_get_byte(p) == -1);
  } else {
    reedkiln_assert(tcmplxA_seq_get_pos(p) == 0u);
    reedkiln_assert(tcmplxA_seq_get_byte(p) == -1);
    reedkiln_assert(tcmplxA_seq_get_pos(p) == 0u);
  }
  return Reedkiln_OK;
}

int test_seq_seek(void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_log_printf("inspecting until %i; ", (int)fixt->len);
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
        i = testfont_rand_size_range(0, 32767);
      if (i > fixt->len) {
        /* expect failure */
        size_t res = tcmplxA_seq_set_pos(p,i);
        if (res != ~(size_t)0u)
          reedkiln_log_printf("breaks past %i; ", (int)i);
        reedkiln_assert(~(size_t)0u == res);
      } else if (i == fixt->len) {
        int b;
        int const d = -1;
        reedkiln_assert(i == tcmplxA_seq_set_pos(p,i));
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          reedkiln_log_printf("breaks at the end (%i);", (int)i);
        reedkiln_assert(b == d);
      } else {
        /* expect success */
        int b;
        int const d = tcmplxAtest_gen_datum(fixt->gen, i, fixt->seed);
        reedkiln_assert(i == tcmplxA_seq_set_pos(p,i));
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          reedkiln_log_printf("breaks at %i", (int)i);
        reedkiln_assert(b == d);
      }
    }
  }
  return Reedkiln_OK;
}

int test_seq_whence(void* data)
{
  struct test_seq_fixture const* const fixt = (struct test_seq_fixture*)data;
  struct tcmplxA_seq* const p = fixt->seq;
  if (p == NULL)
    return Reedkiln_SKIP;
  reedkiln_log_printf("inspecting until %i; ", (int)fixt->len);
  /* */{
    int j;
    size_t i = 0u;
    size_t const fail = ~(size_t)0u;
    for (j = 0; j < 20; ++j) {
      long int const dist = testfont_rand_int_range(-32767, 32767);
      int whence = testfont_rand_int_range(0,2);
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
          reedkiln_log_printf("breaks past %i to %i (%li,%i);",
            (int)i,(int)landing_i,dist,(int)(whence));
        reedkiln_assert(res < -1L);
      } else if (landing_i == fixt->len) {
        int b;
        int const d = -1;
        long int res = tcmplxA_seq_seek(p,dist,whence);
        if (res != (long int)(landing_i))
          reedkiln_log_printf("breaks from %i to %i (%li,%i);",
            (int)i,(int)landing_i,dist,(int)(whence));
        reedkiln_assert((long int)(landing_i) == res);
        i = landing_i;
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          reedkiln_log_printf("breaks at the end (%i);", (int)i);
        reedkiln_assert(b == d);
      } else {
        /* expect success */
        int b;
        int const d = tcmplxAtest_gen_datum(fixt->gen, landing_i, fixt->seed);
        long int res = tcmplxA_seq_seek(p,dist,whence);
        if (res != (long int)landing_i)
          reedkiln_log_printf("breaks from %i to %i (%li,%i);",
            (int)i,(int)landing_i,dist,(int)whence);
        reedkiln_assert((long int)landing_i == res);
        i = landing_i;
        b = tcmplxA_seq_get_byte(p);
        if (b!=d)
          reedkiln_log_printf("breaks at %i;", (int)i);
        reedkiln_assert(b == d);
        i += 1u;
      }
    }
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests_seq, argc, argv, NULL);
}
