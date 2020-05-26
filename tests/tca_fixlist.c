/**
 * @brief Test program for prefix list
 */
#include "../tcmplx-access/fixlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "munit/munit.h"


static MunitResult test_fixlist_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_fixlist_item
  (const MunitParameter params[], void* data);
static void* test_fixlist_setup
    (const MunitParameter params[], void* user_data);
static void test_fixlist_teardown(void* fixture);


static MunitTest tests_fixlist[] = {
  {"cycle", test_fixlist_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_fixlist_item,
    test_fixlist_setup,test_fixlist_teardown,0,NULL},
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


int main(int argc, char **argv) {
  return munit_suite_main(&suite_fixlist, NULL, argc, argv);
}
