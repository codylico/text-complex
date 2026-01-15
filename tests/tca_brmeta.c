
#include "text-complex/access/brmeta.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_brmeta_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_brmeta_item
    (const MunitParameter params[], void* data);
static void* test_brmeta_setup
    (const MunitParameter params[], void* user_data);
static void test_brmeta_teardown(void* fixture);

static MunitParameterEnum test_brmeta_params[] = {
  { NULL, NULL },
};

static MunitTest tests_brmeta[] = {
  {"cycle", test_brmeta_cycle,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,test_brmeta_params},
  {"item", test_brmeta_item,
      test_brmeta_setup,test_brmeta_teardown,0,test_brmeta_params},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_brmeta = {
  "access/brmeta/", tests_brmeta, NULL, 1, 0
};




MunitResult test_brmeta_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brmeta* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_brmeta_new(12);
  munit_assert_not_null(ptr[0]);
  tcmplxA_brmeta_destroy(ptr[0]);
  tcmplxA_brmeta_destroy(NULL);
  return MUNIT_OK;
}

void* test_brmeta_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_brmeta_new((size_t)munit_rand_int_range(0,16));
}

void test_brmeta_teardown(void* fixture) {
  tcmplxA_brmeta_destroy((struct tcmplxA_brmeta*)fixture);
  return;
}

MunitResult test_brmeta_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_brmeta* const table = (struct tcmplxA_brmeta*)data;
  int sz;
  int i;
  unsigned char* ptr;
  if (table == NULL)
    return MUNIT_SKIP;
  (void)params;
  munit_assert(tcmplxA_brmeta_size(table) == 0);
  sz = munit_rand_int_range(1,256);
  {
    int res = tcmplxA_brmeta_emplace(table, sz);
    munit_assert(res == tcmplxA_Success);
  }
  munit_assert(tcmplxA_brmeta_size(table) == 1);
  munit_assert(tcmplxA_brmeta_itemsize(table,0) == sz);
  ptr = tcmplxA_brmeta_itemdata(table,0);
  munit_assert(ptr == tcmplxA_brmeta_itemdata_c(table,0));
  for (i = 0; i < sz; ++i)
    ptr[i] = (unsigned char)i;
  {
    int res = tcmplxA_brmeta_emplace(table, 2);
    munit_assert(res == tcmplxA_Success);
  }
  munit_assert(tcmplxA_brmeta_itemsize(table,0) == sz);
  munit_assert(ptr == tcmplxA_brmeta_itemdata_c(table,0));
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_brmeta, NULL, argc, argv);
}
