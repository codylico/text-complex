
#include "text-complex/access/util.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#  define test_sharp2(s) #s
#  define test_sharp(s) test_sharp2(s)
#  define test_break(s) \
     { munit_error(s); return MUNIT_FAIL; }
#else
#  define test_break(s)
#endif /*NDEBUG*/

typedef int (*test_fn)(void);
static MunitResult test_util_alloc
    (const MunitParameter params[], void* data);

static MunitTest tests_util[] = {
  {"alloc", test_util_alloc, NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_util = {
  "access/util/", tests_util, NULL, 1, 0
};

MunitResult test_util_alloc
  (const MunitParameter params[], void* data)
{
  (void)params;
  (void)data;
  void* ptr[3];
  unsigned long int lg = munit_rand_uint32();
  ptr[0] = tcmplxA_util_malloc(0u);
  if (ptr[0] != NULL)
    test_break("tcmplxA_util_malloc zero");
  ptr[0] = tcmplxA_util_malloc(sizeof(unsigned long int));
  if (ptr[0] == NULL)
    test_break("tcmplxA_util_malloc nonzero");
  tcmplxA_util_free(ptr[0]);
  return MUNIT_OK;
}

int main(int argc, char **argv) {
  return munit_suite_main(&suite_util, NULL, argc, argv);
}
