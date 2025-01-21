/**
 * @brief Test program for prefix gasp vector
 */
#include "testfont.h"
#include "text-complex/access/gaspvec.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


static MunitResult test_gaspvec_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_gaspvec_item
  (const MunitParameter params[], void* data);
static void* test_gaspvec_setup
    (const MunitParameter params[], void* user_data);
static void test_gaspvec_teardown(void* fixture);


static MunitTest tests_gaspvec[] = {
  {"cycle", test_gaspvec_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"item", test_gaspvec_item,
    test_gaspvec_setup,test_gaspvec_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_gaspvec = {
  "access/gaspvec/", tests_gaspvec, NULL, 1, 0
};




MunitResult test_gaspvec_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_gaspvec* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_gaspvec_new(128);
  munit_assert(ptr[0] != NULL);
  tcmplxA_gaspvec_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_gaspvec_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_gaspvec_new((size_t)munit_rand_int_range(4,256));
}

void test_gaspvec_teardown(void* fixture) {
  tcmplxA_gaspvec_destroy((struct tcmplxA_gaspvec*)fixture);
  return;
}

MunitResult test_gaspvec_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_gaspvec* const p = (struct tcmplxA_gaspvec*)data;
  struct tcmplxA_fixlist const* dsp[2];
  size_t sz;
  size_t i;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  sz = tcmplxA_gaspvec_size(p);
  i = (size_t)munit_rand_int_range(0,(int)(sz)-1);
  munit_assert(sz >= 4);
  munit_assert(sz <= 256);
  dsp[0] = tcmplxA_gaspvec_at(p, i);
  dsp[1] = tcmplxA_gaspvec_at_c(p, i);
  munit_assert(dsp[0] != NULL);
  munit_assert(dsp[1] != NULL);
  munit_assert(dsp[0] == dsp[1]);
  return MUNIT_OK;
}




int main(int argc, char **argv) {
  return munit_suite_main(&suite_gaspvec, NULL, argc, argv);
}
