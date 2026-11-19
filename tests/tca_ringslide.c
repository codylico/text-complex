/**
 * @brief Test program for slide ring
 */
#include "testfont.h"
#include "text-complex/access/ringslide.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_ringslide_cycle(void* data);
static int test_ringslide_item(void* data);
static int test_ringslide_add(void* data);
static int test_ringslide_addring(void* data);
static void* test_ringslide_setup(void* user_data);
static void* test_ringslide_setupsmall(void* user_data);
static void test_ringslide_teardown(void* fixture);


static struct reedkiln_box const box_ringslide = {
  test_ringslide_setup,test_ringslide_teardown
};
static struct reedkiln_box const box_ringslide_small = {
  test_ringslide_setupsmall,test_ringslide_teardown
};

static struct reedkiln_entry const tests_ringslide[] = {
  {"access/ringslide/cycle", test_ringslide_cycle },
  {"access/ringslide/add", test_ringslide_add, 0,&box_ringslide },
  {"access/ringslide/add/ring", test_ringslide_addring, 0,&box_ringslide_small },
  {NULL}
};





int test_ringslide_cycle(void* data)
{
  unsigned const num = testfont_rand_uint_range(128,16777216);
  struct tcmplxA_ringslide* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_ringslide_new(num);
  reedkiln_assert(ptr[0]);
  reedkiln_assert(tcmplxA_ringslide_extent(ptr[0]) == num);
  tcmplxA_ringslide_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_ringslide_setup(void* user_data) {
  unsigned const num = testfont_rand_uint_range(128,16777216);
  return tcmplxA_ringslide_new(num);
}

void* test_ringslide_setupsmall(void* user_data)
{
  unsigned const num = testfont_rand_uint_range(128,512);
  return tcmplxA_ringslide_new(num);
}

void test_ringslide_teardown(void* fixture) {
  tcmplxA_ringslide_destroy((struct tcmplxA_ringslide*)fixture);
  return;
}

int test_ringslide_add(void* data)
{
  struct tcmplxA_ringslide* const p = (struct tcmplxA_ringslide*)data;
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
      int const res = tcmplxA_ringslide_add(p, buf[i]);
      reedkiln_assert(res == tcmplxA_Success);
    }
  }
  /* check the size */{
    reedkiln_assert(tcmplxA_ringslide_size(p) == add_count);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      reedkiln_assert(tcmplxA_ringslide_peek(p, i) == buf[j]);
    }
  }
  return Reedkiln_OK;
}

int test_ringslide_addring(void* data)
{
  struct tcmplxA_ringslide* const p = (struct tcmplxA_ringslide*)data;
  int const add_count = testfont_rand_int_range(1,64);
  unsigned const extent = tcmplxA_ringslide_extent(p);
  unsigned const skip_count = extent - testfont_rand_uint_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* fill the buffer */{
    reedkiln_memrand(buf, add_count);
  }
  /* skip some items */{
    unsigned i;
    for (i = 0; i < skip_count; ++i) {
      tcmplxA_ringslide_add(p, i&255u);
    }
  }
  /* add the items */{
    int i;
    for (i = 0; i < add_count; ++i) {
      int const res = tcmplxA_ringslide_add(p, buf[i]);
      reedkiln_assert(res == tcmplxA_Success);
    }
  }
  /* check the size */{
    unsigned const expect_size =
        ((unsigned int)(add_count+skip_count) > extent)
      ? extent : add_count+skip_count;
    reedkiln_assert(tcmplxA_ringslide_size(p) == expect_size);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      reedkiln_assert(tcmplxA_ringslide_peek(p, i) == buf[j]);
    }
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(&tests_ringslide, argc, argv, NULL);
}
