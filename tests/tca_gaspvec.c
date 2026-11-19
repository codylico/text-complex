/**
 * @brief Test program for prefix gasp vector
 */
#include "testfont.h"
#include "text-complex/access/gaspvec.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int test_gaspvec_cycle(void* data);
static int test_gaspvec_item(void* data);
static int test_gaspvec_skip(void* data);
static void* test_gaspvec_setup(void* user_data);
static void test_gaspvec_teardown(void* fixture);


static struct reedkiln_box const box_gaspvec = {
  test_gaspvec_setup,test_gaspvec_teardown
};

static struct reedkiln_entry const tests_gaspvec[] = {
  {"access/gaspvec/cycle", test_gaspvec_cycle },
  {"access/gaspvec/item", test_gaspvec_item, 0, &box_gaspvec },
  {"access/gaspvec/skip", test_gaspvec_skip, 0, &box_gaspvec },
  {NULL}
};





int test_gaspvec_cycle(void* data)
{
  struct tcmplxA_gaspvec* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_gaspvec_new(128);
  reedkiln_assert(ptr[0] != NULL);
  tcmplxA_gaspvec_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_gaspvec_setup(void* user_data) {
  return tcmplxA_gaspvec_new((size_t)testfont_rand_int_range(4,256));
}

void test_gaspvec_teardown(void* fixture) {
  tcmplxA_gaspvec_destroy((struct tcmplxA_gaspvec*)fixture);
  return;
}

int test_gaspvec_item(void* data)
{
  struct tcmplxA_gaspvec* const p = (struct tcmplxA_gaspvec*)data;
  struct tcmplxA_fixlist const* dsp[2];
  size_t sz;
  size_t i;
  if (p == NULL)
    return Reedkiln_SKIP;
  sz = tcmplxA_gaspvec_size(p);
  i = testfont_rand_size_range(0,sz-1);
  reedkiln_assert(sz >= 4);
  reedkiln_assert(sz <= 256);
  dsp[0] = tcmplxA_gaspvec_at(p, i);
  dsp[1] = tcmplxA_gaspvec_at_c(p, i);
  reedkiln_assert(dsp[0] != NULL);
  reedkiln_assert(dsp[1] != NULL);
  reedkiln_assert(dsp[0] == dsp[1]);
  return Reedkiln_OK;
}

int test_gaspvec_skip(void* data)
{
  struct tcmplxA_gaspvec* const p = (struct tcmplxA_gaspvec*)data;
  struct tcmplxA_fixlist const* dsp[2];
  size_t sz;
  size_t i;
  unsigned short x;
  if (p == NULL)
    return Reedkiln_SKIP;
  sz = tcmplxA_gaspvec_size(p);
  i = testfont_rand_size_range(0,sz-1);
  x = (unsigned short)testfont_rand_int_range(0,65534);
  reedkiln_assert(sz >= 4);
  reedkiln_assert(sz <= 256);
  reedkiln_assert(tcmplxA_gaspvec_get_skip(p,i) == USHRT_MAX);
  tcmplxA_gaspvec_set_skip(p, i, x);
  reedkiln_assert(tcmplxA_gaspvec_get_skip(p, i) == x);
  return Reedkiln_OK;
}



int main(int argc, char **argv) {
  return reedkiln_main(tests_gaspvec, argc, argv, NULL);
}
