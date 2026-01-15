
#include "text-complex/access/offtable.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_offtable_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_offtable_item
    (const MunitParameter params[], void* data);
static void* test_offtable_setup
    (const MunitParameter params[], void* user_data);
static void test_offtable_teardown(void* fixture);

static MunitParameterEnum test_offtable_params[] = {
  { NULL, NULL },
};

static MunitTest tests_offtable[] = {
  {"cycle", test_offtable_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,test_offtable_params},
  {"item", test_offtable_item,
      test_offtable_setup,test_offtable_teardown,0,test_offtable_params},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_offtable = {
  "access/offtable/", tests_offtable, NULL, 1, 0
};




MunitResult test_offtable_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_offtable* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_offtable_new(12);
  munit_assert_not_null(ptr[0]);
  tcmplxA_offtable_destroy(ptr[0]);
  tcmplxA_offtable_destroy(NULL);
  return MUNIT_OK;
}

void* test_offtable_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_offtable_new((size_t)munit_rand_int_range(4,256));
}

void test_offtable_teardown(void* fixture) {
  tcmplxA_offtable_destroy((struct tcmplxA_offtable*)fixture);
  return;
}

MunitResult test_offtable_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_offtable* const offtable = (struct tcmplxA_offtable*)data;
  struct tcmplxA_offline const* dsp[2];
  size_t sz;
  size_t i;
  if (offtable == NULL)
    return MUNIT_SKIP;
  (void)params;
  (void)data;
  sz = tcmplxA_offtable_size(offtable);
  i = (size_t)munit_rand_int_range(0,(int)(sz)-1);
  munit_assert_size(sz,>=,4);
  munit_assert_size(sz,<=,256);
  dsp[0] = tcmplxA_offtable_at(offtable, i);
  dsp[1] = tcmplxA_offtable_at_c(offtable, i);
  munit_assert_not_null(dsp[0]);
  munit_assert_not_null(dsp[1]);
  munit_assert_ptr(dsp[0],==,dsp[1]);
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_offtable, NULL, argc, argv);
}
