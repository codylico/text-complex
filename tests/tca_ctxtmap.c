/**
 * @brief Test program for context map
 */
#include "../tcmplx-access/ctxtmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "munit/munit.h"
#include "testfont.h"


static MunitResult test_ctxtmap_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_ctxtmap_item
  (const MunitParameter params[], void* data);
static MunitResult test_ctxtmap_distcontext
  (const MunitParameter params[], void* data);
static MunitResult test_ctxtmap_litcontext
  (const MunitParameter params[], void* data);
static void* test_ctxtmap_setup
    (const MunitParameter params[], void* user_data);
static void test_ctxtmap_teardown(void* fixture);


static MunitTest tests_ctxtmap[] = {
  {"cycle", test_ctxtmap_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_ctxtmap_item,
    test_ctxtmap_setup,test_ctxtmap_teardown,0,NULL},
  {"distance_context", test_ctxtmap_distcontext,
    NULL,NULL,0,NULL},
  {"literal_context", test_ctxtmap_litcontext,
    NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_ctxtmap = {
  "access/ctxtmap/", tests_ctxtmap, NULL, 1, 0
};




MunitResult test_ctxtmap_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtmap* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_ctxtmap_new(0,0);
  munit_assert_not_null(ptr[0]);
  tcmplxA_ctxtmap_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_ctxtmap_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_ctxtmap_new
    (testfont_rand_size_range(1,256), munit_rand_int_range(0,1)?4:64);
}

void test_ctxtmap_teardown(void* fixture) {
  tcmplxA_ctxtmap_destroy((struct tcmplxA_ctxtmap*)fixture);
  return;
}

MunitResult test_ctxtmap_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ctxtmap* const p = (struct tcmplxA_ctxtmap*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* */{
    unsigned char* data_ptr = tcmplxA_ctxtmap_data(p);
    size_t j;
    munit_assert_ptr_equal(data_ptr, tcmplxA_ctxtmap_data_c(p));
    for (j = 0; j < tcmplxA_ctxtmap_block_types(p); ++j) {
      size_t i;
      for (i = 0; i < tcmplxA_ctxtmap_contexts(p); ++i, ++data_ptr) {
        unsigned char const x =
          (unsigned char)(testfont_rand_int_range(0,255));
        tcmplxA_ctxtmap_set(p,j,i,x);
        munit_assert_uchar(x, ==, tcmplxA_ctxtmap_get(p,j,i));
        munit_assert_uchar(x, ==, *data_ptr);
      }
    }
  }
  return MUNIT_OK;
}

MunitResult test_ctxtmap_distcontext
  (const MunitParameter params[], void* data)
{
  unsigned long int len = munit_rand_int_range(0,10);
  int ctxt;
  /* query the context */{
    ctxt = tcmplxA_ctxtmap_distance_context(len);
    if (len < 2) {
      munit_assert_int(ctxt,<,0);
    }
  }
  /* inspect the result */if (len >= 2) {
    switch (len) {
    case 2: munit_assert_int(ctxt,==,0); break;
    case 3: munit_assert_int(ctxt,==,1); break;
    case 4: munit_assert_int(ctxt,==,2); break;
    default: munit_assert_int(ctxt,==,3); break;
    }
  }
  return MUNIT_OK;
}

MunitResult test_ctxtmap_litcontext
  (const MunitParameter params[], void* data)
{
  int m = munit_rand_int_range(0,4);
  int ctxt;
  munit_uint8_t hist[2];
  munit_rand_memory(sizeof(hist), hist);
  /* query the context */{
    ctxt = tcmplxA_ctxtmap_literal_context(m,hist[0],hist[1]);
    if (m >= 4) {
      munit_assert_int(ctxt,<,0);
    }
  }
  /* inspect the result */if (m < 4) {
    switch (m) {
    case tcmplxA_CtxtMap_LSB6:
      munit_assert_int(ctxt,==,hist[0]&0x3f);
      break;
    case tcmplxA_CtxtMap_MSB6:
      munit_assert_int(ctxt,==,(hist[0]>>2));
      break;
    case tcmplxA_CtxtMap_UTF8:
    case tcmplxA_CtxtMap_Signed:
      /* ? */
      break;
    }
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_ctxtmap, NULL, argc, argv);
}
