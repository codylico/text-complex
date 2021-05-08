/**
 * @brief Test program for slide ring
 */
#include "testfont.h"
#include "text-complex/access/ringslide.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_ringslide_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_ringslide_item
  (const MunitParameter params[], void* data);
static void* test_ringslide_setup
    (const MunitParameter params[], void* user_data);
static void test_ringslide_teardown(void* fixture);

static MunitParameterEnum test_ringslide_params[] = {
  { NULL, NULL },
};

static MunitTest tests_ringslide[] = {
  {"cycle", test_ringslide_cycle,
    NULL,NULL,0,NULL},
  {"item", test_ringslide_item,
    test_ringslide_setup,test_ringslide_teardown,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_ringslide = {
  "access/ringslide/", tests_ringslide, NULL, 1, 0
};




MunitResult test_ringslide_cycle
  (const MunitParameter params[], void* data)
{
  uint32_t const num = (uint32_t)munit_rand_int_range(128,16777216);
  struct tcmplxA_ringslide* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_ringslide_new(num);
  munit_assert_not_null(ptr[0]);
  munit_assert_uint32(tcmplxA_ringslide_extent(ptr[0]), ==, num);
  tcmplxA_ringslide_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_ringslide_setup(const MunitParameter params[], void* user_data) {
  uint32_t const num = (uint32_t)munit_rand_int_range(128,16777216);
  return tcmplxA_ringslide_new(num);
}

void test_ringslide_teardown(void* fixture) {
  tcmplxA_ringslide_destroy((struct tcmplxA_ringslide*)fixture);
  return;
}

MunitResult test_ringslide_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_ringslide* const p = (struct tcmplxA_ringslide*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  (void)data;
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  return munit_suite_main(&suite_ringslide, NULL, argc, argv);
}
