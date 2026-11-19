
#include "testfont.h"
#include "text-complex/access/offtable.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_offtable_cycle(void* data);
static int test_offtable_item(void* data);
static void* test_offtable_setup(void* user_data);
static void test_offtable_teardown(void* fixture);


static struct reedkiln_box const box_offtable = {
  test_offtable_setup,test_offtable_teardown
};

static struct reedkiln_entry const tests_offtable[] = {
  {"access/offtable/cycle", test_offtable_cycle },
  {"access/offtable/item", test_offtable_item, 0, &box_offtable },
  {NULL}
};





int test_offtable_cycle(void* data)
{
  struct tcmplxA_offtable* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_offtable_new(12);
  reedkiln_assert(ptr[0]);
  tcmplxA_offtable_destroy(ptr[0]);
  tcmplxA_offtable_destroy(NULL);
  return Reedkiln_OK;
}

void* test_offtable_setup(void* user_data) {
  return tcmplxA_offtable_new(testfont_rand_size_range(4,256));
}

void test_offtable_teardown(void* fixture) {
  tcmplxA_offtable_destroy((struct tcmplxA_offtable*)fixture);
  return;
}

int test_offtable_item(void* data)
{
  struct tcmplxA_offtable* const offtable = (struct tcmplxA_offtable*)data;
  struct tcmplxA_offline const* dsp[2];
  size_t sz;
  size_t i;
  if (offtable == NULL)
    return Reedkiln_IGNORE;
  (void)data;
  sz = tcmplxA_offtable_size(offtable);
  i = testfont_rand_size_range(0,sz-1);
  reedkiln_assert(sz >= 4);
  reedkiln_assert(sz <= 256);
  dsp[0] = tcmplxA_offtable_at(offtable, i);
  dsp[1] = tcmplxA_offtable_at_c(offtable, i);
  reedkiln_assert(dsp[0]);
  reedkiln_assert(dsp[1]);
  reedkiln_assert(dsp[0] == dsp[1]);
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests_offtable, argc, argv, NULL);
}
