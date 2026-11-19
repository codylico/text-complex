/**
 * @brief Test program for hash chain
 */
#include "testfont.h"
#include "text-complex/access/hashchain.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_hashchain_cycle(void* data);
static int test_hashchain_add(void* data);
static int test_hashchain_addring(void* data);
static int test_hashchain_find(void* data);
static void* test_hashchain_setup(void* user_data);
static void* test_hashchain_setupsmall(void* user_data);
static void test_hashchain_teardown(void* fixture);


static struct reedkiln_box box_hashchain = {
  test_hashchain_setup,test_hashchain_teardown
};
static struct reedkiln_box box_hashchain_small = {
  test_hashchain_setupsmall,test_hashchain_teardown
};

static struct reedkiln_entry tests_hashchain[] = {
  {"access/hashchain/cycle", test_hashchain_cycle },
  {"access/hashchain/add", test_hashchain_add, 0, &box_hashchain },
  {"access/hashchain/add/ring", test_hashchain_addring, 0, &box_hashchain_small },
  {"access/hashchain/find", test_hashchain_find, 0, &box_hashchain_small },
  {NULL}
};





int test_hashchain_cycle(void* data)
{
  unsigned const num = testfont_rand_uint_range(128,16777216);
  size_t const len = testfont_rand_size_range(1,128);
  struct tcmplxA_hashchain* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_hashchain_new(num, len);
  reedkiln_assert(ptr[0]);
  reedkiln_assert(tcmplxA_hashchain_extent(ptr[0]) == num);
  tcmplxA_hashchain_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_hashchain_setup(void* user_data) {
  unsigned const num = testfont_rand_uint_range(128,16777216);
  size_t const len = testfont_rand_size_range(1,128);
  return tcmplxA_hashchain_new(num, len);
}

void* test_hashchain_setupsmall(void* user_data)
{
  unsigned const num = testfont_rand_uint_range(128,512);
  size_t const len = testfont_rand_size_range(1,16);
  return tcmplxA_hashchain_new(num, len);
}

void test_hashchain_teardown(void* fixture) {
  tcmplxA_hashchain_destroy((struct tcmplxA_hashchain*)fixture);
  return;
}

int test_hashchain_add(void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  int const add_count = testfont_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* fill the buffer */{
    reedkiln_memrand(buf, add_count);
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_hashchain_add(p, buf[i]);
      reedkiln_assert(res == tcmplxA_Success);
    }
  }
  /* check the size */{
    reedkiln_assert(tcmplxA_hashchain_size(p) == add_count);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      reedkiln_assert(tcmplxA_hashchain_peek(p, i) == buf[j]);
    }
  }
  return Reedkiln_OK;
}

int test_hashchain_addring(void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  int const add_count = testfont_rand_int_range(1,64);
  unsigned const extent = tcmplxA_hashchain_extent(p);
  unsigned const skip_count = extent - testfont_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* fill the buffer */{
    reedkiln_memrand(buf, add_count);
  }
  /* skip some items */{
    unsigned i;
    for (i = 0; i < skip_count; ++i) {
      tcmplxA_hashchain_add(p, i&255u);
    }
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_hashchain_add(p, buf[i]);
      reedkiln_assert(res == tcmplxA_Success);
    }
  }
  /* check the size */{
    unsigned const expect_size =
        ((unsigned int)(add_count+skip_count) > extent)
      ? extent : add_count+skip_count;
    reedkiln_assert(tcmplxA_hashchain_size(p) == expect_size);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      reedkiln_assert(tcmplxA_hashchain_peek(p, i) == buf[j]);
    }
  }
  return Reedkiln_OK;
}

int test_hashchain_find(void* data)
{
  struct tcmplxA_hashchain* const p = (struct tcmplxA_hashchain*)data;
  int const add_count = testfont_rand_int_range(3,64);
  unsigned int seed = testfont_rand_uint_range(1,255);
  unsigned const extent = tcmplxA_hashchain_extent(p);
  unsigned const skip_count = extent - testfont_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* fill the buffer */{
    int i;
    for (i = 0; i < add_count; ++i) {
      buf[i] = (unsigned char)(seed++);
    }
  }
  /* skip some items */{
    unsigned i;
    for (i = 0; i < skip_count; ++i) {
      tcmplxA_hashchain_add(p, 0u);
    }
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_hashchain_add(p, buf[i]);
      reedkiln_assert(res == tcmplxA_Success);
    }
  }
  /* search for the stored bytes */{
    int j;
    for (j = 0; j < add_count-2; ++j) {
      unsigned const i = (unsigned)(add_count-j-1);
      unsigned const k = tcmplxA_hashchain_find(p, buf+j, 0u);
      if (k == ((unsigned)-1) && j < add_count-3)
        continue;
      reedkiln_assert(k == i);
    }
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests_hashchain, argc, argv, NULL);
}
