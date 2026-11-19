
#include "text-complex/access/brmeta.h"
#include "testfont.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_brmeta_cycle(void* data);
static int test_brmeta_item(void* data);
static void* test_brmeta_setup(void* user_data);
static void test_brmeta_teardown(void* fixture);

static struct reedkiln_box const box_brmeta = {
  &test_brmeta_setup, &test_brmeta_teardown
};

static struct reedkiln_entry const tests_brmeta[] = {
  {"access/brmeta/cycle", test_brmeta_cycle },
  {"access/brmeta/item", test_brmeta_item, 0, &box_brmeta },
  {NULL}
};





int test_brmeta_cycle(void* data)
{
  struct tcmplxA_brmeta* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_brmeta_new(12);
  reedkiln_assert(ptr[0]);
  tcmplxA_brmeta_destroy(ptr[0]);
  tcmplxA_brmeta_destroy(NULL);
  return Reedkiln_OK;
}

void* test_brmeta_setup(void* user_data) {
  return tcmplxA_brmeta_new(testfont_rand_size_range(0,16));
}

void test_brmeta_teardown(void* fixture) {
  tcmplxA_brmeta_destroy((struct tcmplxA_brmeta*)fixture);
  return;
}

int test_brmeta_item(void* data)
{
  struct tcmplxA_brmeta* const table = (struct tcmplxA_brmeta*)data;
  int sz;
  int i;
  unsigned char* ptr;
  if (table == NULL)
    return Reedkiln_SKIP;
  reedkiln_assert(tcmplxA_brmeta_size(table) == 0);
  sz = testfont_rand_int_range(1,256);
  {
    int res = tcmplxA_brmeta_emplace(table, sz);
    reedkiln_assert(res == tcmplxA_Success);
  }
  reedkiln_assert(tcmplxA_brmeta_size(table) == 1);
  reedkiln_assert(tcmplxA_brmeta_itemsize(table,0) == sz);
  ptr = tcmplxA_brmeta_itemdata(table,0);
  reedkiln_assert(ptr == tcmplxA_brmeta_itemdata_c(table,0));
  for (i = 0; i < sz; ++i)
    ptr[i] = (unsigned char)i;
  {
    int res = tcmplxA_brmeta_emplace(table, 2);
    reedkiln_assert(res == tcmplxA_Success);
  }
  reedkiln_assert(tcmplxA_brmeta_itemsize(table,0) == sz);
  reedkiln_assert(ptr == tcmplxA_brmeta_itemdata_c(table,0));
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests_brmeta, argc, argv, NULL);
}
