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
static void* test_ctxtmap_setup
    (const MunitParameter params[], void* user_data);
static void test_ctxtmap_teardown(void* fixture);


static MunitTest tests_ctxtmap[] = {
  {"cycle", test_ctxtmap_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_ctxtmap_item,
    test_ctxtmap_setup,test_ctxtmap_teardown,0,NULL},
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


int main(int argc, char **argv) {
  return munit_suite_main(&suite_ctxtmap, NULL, argc, argv);
}
