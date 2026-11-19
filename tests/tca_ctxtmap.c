/**
 * @brief Test program for context map
 */
#include "testfont.h"
#include "text-complex/access/ctxtmap.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_ctxtmap_cycle(void* data);
static int test_ctxtmap_item(void* data);
static int test_ctxtmap_distcontext(void* data);
static int test_ctxtmap_litcontext(void* data);
static int test_ctxtmap_imtf(void* data);
static int test_ctxtmap_mtf(void* data);
static int test_ctxtmap_modes(void* data);
static void* test_ctxtmap_setup(void* user_data);
static void test_ctxtmap_teardown(void* fixture);

/* NOTE from RFC7932 */
static void InverseMoveToFrontTransform(unsigned char* v, int len);

static struct reedkiln_box const box_ctxtmap = {
  test_ctxtmap_setup,test_ctxtmap_teardown
};

static struct reedkiln_entry const tests_ctxtmap[] = {
  {"access/ctxtmap/cycle", test_ctxtmap_cycle },
  {"access/ctxtmap/item", test_ctxtmap_item, 0, &box_ctxtmap },
  {"access/ctxtmap/distance_context", test_ctxtmap_distcontext },
  {"access/ctxtmap/literal_context", test_ctxtmap_litcontext },
  {"access/ctxtmap/imtf", test_ctxtmap_imtf, 0, &box_ctxtmap },
  {"access/ctxtmap/mtf", test_ctxtmap_mtf, 0, &box_ctxtmap },
  {"access/ctxtmap/modes", test_ctxtmap_modes, 0, &box_ctxtmap },
  {NULL}
};


/* NOTE from RFC7932 */
void InverseMoveToFrontTransform(unsigned char* v, int v_len) {
  unsigned char mtf[256];
  int i;
  for (i = 0; i < 256; ++i) {
    mtf[i] = (unsigned char)i;
  }
  for (i = 0; i < v_len; ++i) {
    unsigned char index = v[i];
    unsigned char value = mtf[index];
    v[i] = value;
    for (; index; --index) {
       mtf[index] = mtf[index - 1];
    }
    mtf[0] = value;
  }
}




int test_ctxtmap_cycle(void* data)
{
  struct tcmplxA_ctxtmap* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_ctxtmap_new(0,0);
  reedkiln_assert(ptr[0]);
  tcmplxA_ctxtmap_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_ctxtmap_setup(void* user_data) {
  return tcmplxA_ctxtmap_new
    (testfont_rand_size_range(1,256), testfont_rand_int_range(0,1)?4:64);
}

void test_ctxtmap_teardown(void* fixture) {
  tcmplxA_ctxtmap_destroy((struct tcmplxA_ctxtmap*)fixture);
  return;
}

int test_ctxtmap_item(void* data)
{
  struct tcmplxA_ctxtmap* const p = (struct tcmplxA_ctxtmap*)data;
  if (p == NULL)
    return Reedkiln_IGNORE;
  /* */{
    unsigned char* data_ptr = tcmplxA_ctxtmap_data(p);
    size_t j;
    reedkiln_assert(data_ptr == tcmplxA_ctxtmap_data_c(p));
    for (j = 0; j < tcmplxA_ctxtmap_block_types(p); ++j) {
      size_t i;
      for (i = 0; i < tcmplxA_ctxtmap_contexts(p); ++i, ++data_ptr) {
        unsigned char const x =
          (unsigned char)(testfont_rand_int_range(0,255));
        tcmplxA_ctxtmap_set(p,j,i,x);
        reedkiln_assert(x == tcmplxA_ctxtmap_get(p,j,i));
        reedkiln_assert(x == *data_ptr);
      }
    }
  }
  return Reedkiln_OK;
}

int test_ctxtmap_distcontext(void* data)
{
  unsigned long int len = testfont_rand_int_range(0,10);
  int ctxt;
  /* query the context */{
    ctxt = tcmplxA_ctxtmap_distance_context(len);
    if (len < 2) {
      reedkiln_assert(ctxt < 0);
    }
  }
  /* inspect the result */if (len >= 2) {
    switch (len) {
    case 2: reedkiln_assert(ctxt == 0); break;
    case 3: reedkiln_assert(ctxt == 1); break;
    case 4: reedkiln_assert(ctxt == 2); break;
    default: reedkiln_assert(ctxt == 3); break;
    }
  }
  return Reedkiln_OK;
}

int test_ctxtmap_litcontext(void* data)
{
  int m = testfont_rand_int_range(0,4);
  int ctxt;
  unsigned char hist[2];
  reedkiln_memrand(hist, sizeof(hist));
  /* query the context */{
    ctxt = tcmplxA_ctxtmap_literal_context(m,hist[0],hist[1]);
    if (m >= 4) {
      reedkiln_assert(ctxt < 0);
    }
  }
  /* inspect the result */if (m < 4) {
    switch (m) {
    case tcmplxA_CtxtMap_LSB6:
      reedkiln_assert(ctxt == (hist[0]&0x3f));
      break;
    case tcmplxA_CtxtMap_MSB6:
      reedkiln_assert(ctxt == (hist[0]>>2));
      break;
    case tcmplxA_CtxtMap_UTF8:
    case tcmplxA_CtxtMap_Signed:
      /* ? */
      break;
    }
  }
  return Reedkiln_OK;
}

int test_ctxtmap_imtf(void* data)
{
  struct tcmplxA_ctxtmap* const p = (struct tcmplxA_ctxtmap*)data;
  unsigned char* mem;
  if (p == NULL)
    return Reedkiln_IGNORE;
  /* */{
    size_t const isize =
      tcmplxA_ctxtmap_contexts(p) * tcmplxA_ctxtmap_block_types(p);
    unsigned char* const data = tcmplxA_ctxtmap_data(p);
    mem = malloc(isize*sizeof(unsigned char));
    if (mem == NULL)
      return Reedkiln_IGNORE;
    reedkiln_memrand(data, isize);
    memcpy(mem, data, isize*sizeof(unsigned char));
    /* apply the inverse transform */{
      tcmplxA_ctxtmap_revert_movetofront(p);
      InverseMoveToFrontTransform(mem, isize);
    }
    reedkiln_assert(memcmp(data, mem, isize) == 0);
  }
  free(mem);
  return Reedkiln_OK;
}
int test_ctxtmap_mtf(void* data)
{
  struct tcmplxA_ctxtmap* const p = (struct tcmplxA_ctxtmap*)data;
  unsigned char* mem;
  if (p == NULL)
    return Reedkiln_IGNORE;
  /* */{
    size_t const isize =
      tcmplxA_ctxtmap_contexts(p) * tcmplxA_ctxtmap_block_types(p);
    unsigned char* const data = tcmplxA_ctxtmap_data(p);
    mem = malloc(isize*sizeof(unsigned char));
    if (mem == NULL)
      return Reedkiln_IGNORE;
    reedkiln_memrand(data, isize);
    memcpy(mem, data, isize*sizeof(unsigned char));
    /* apply the inverse transform */{
      tcmplxA_ctxtmap_apply_movetofront(p);
      InverseMoveToFrontTransform(data, isize);
    }
    reedkiln_assert(memcmp(data, mem, isize) == 0);
  }
  free(mem);
  return Reedkiln_OK;
}

int test_ctxtmap_modes(void* data)
{
  struct tcmplxA_ctxtmap* const p = (struct tcmplxA_ctxtmap*)data;
  if (!p)
    return Reedkiln_IGNORE;
  /* */{
    size_t blocks = tcmplxA_ctxtmap_block_types(p);
    int mode = testfont_rand_int_range(0,3);
    size_t choice = testfont_rand_size_range(0,blocks-1);
    tcmplxA_ctxtmap_set_mode(p, choice, mode);
    reedkiln_assert(tcmplxA_ctxtmap_get_mode(p, choice) == mode);
  }
  return Reedkiln_OK;
}



int main(int argc, char **argv) {
  return reedkiln_main(tests_ctxtmap, argc, argv, NULL);
}
