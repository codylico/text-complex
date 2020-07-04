/**
 * @file tcmplx-access/ctxtmap.c
 * @brief Context map for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ctxtmap.h"
#include "api.h"
#include "util.h"

struct tcmplxA_ctxtmap {
  unsigned char *p;
  size_t btypes;
  size_t ctxts;
};

/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut0[256] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  4,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   8, 12, 16, 12, 12, 20, 12, 16, 24, 28, 12, 12, 32, 12, 36, 12,
  44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 32, 32, 24, 40, 28, 12,
  12, 48, 52, 52, 52, 48, 52, 52, 52, 48, 52, 52, 52, 52, 52, 48,
  52, 52, 52, 52, 52, 48, 52, 52, 52, 52, 52, 24, 12, 28, 12, 12,
  12, 56, 60, 60, 60, 56, 60, 60, 60, 56, 60, 60, 60, 60, 60, 56,
  60, 60, 60, 60, 60, 56, 60, 60, 60, 60, 60, 24, 12, 28, 12,  0,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3
 };
/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut1[256] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
   1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
   1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
  };
/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut2[256] = {
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7
  };

/**
 * @brief Initialize a context map.
 * @param x the context map to initialize
 * @param btypes number of block types
 * @param ctxts number of contexts
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ctxtmap_init
    (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts);
/**
 * @brief Close a context map.
 * @param x the context map to close
 */
static void tcmplxA_ctxtmap_close(struct tcmplxA_ctxtmap* x);
/**
 * @brief Resize a context map.
 * @param x the context map to resize
 * @param btypes number of block types
 * @param ctxts number of contexts
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ctxtmap_resize
    (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts);

/* BEGIN context map / static */
int tcmplxA_ctxtmap_init
      (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts)
{
  x->p = NULL;
  x->btypes = 0u;
  x->ctxts = 0u;
  return tcmplxA_ctxtmap_resize(x, btypes, ctxts);
}

void tcmplxA_ctxtmap_close(struct tcmplxA_ctxtmap* x) {
  if (x->p != NULL) {
    tcmplxA_util_free(x->p);
    x->p = NULL;
  }
  x->btypes = 0u;
  x->ctxts = 0u;
  return;
}

int tcmplxA_ctxtmap_resize
  (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts)
{
  if (ctxts > 0u && btypes >= (~(size_t)0u)/ctxts) {
    return tcmplxA_ErrMemory;
  } else if (ctxts == 0u || btypes == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
      x->p = NULL;
    }
  } else {
    unsigned char* const np = tcmplxA_util_malloc(btypes*ctxts);
    if (np == NULL) {
      return tcmplxA_ErrMemory;
    } else {
      if (x->p != NULL) {
        tcmplxA_util_free(x->p);
      }
    }
    x->p = np;
  }
  x->btypes = btypes;
  x->ctxts = ctxts;
  return tcmplxA_Success;
}
/* END   context map / static */

/* BEGIN context map / public */
struct tcmplxA_ctxtmap* tcmplxA_ctxtmap_new(size_t btypes, size_t ctxts) {
  struct tcmplxA_ctxtmap* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_ctxtmap));
  if (out != NULL
  &&  tcmplxA_ctxtmap_init(out, btypes, ctxts) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_ctxtmap_destroy(struct tcmplxA_ctxtmap* x) {
  if (x != NULL) {
    tcmplxA_ctxtmap_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

size_t tcmplxA_ctxtmap_block_types(struct tcmplxA_ctxtmap const* x) {
  return x->btypes;
}

size_t tcmplxA_ctxtmap_contexts(struct tcmplxA_ctxtmap const* x) {
  return x->ctxts;
}

unsigned char* tcmplxA_ctxtmap_data(struct tcmplxA_ctxtmap* x) {
  return x->p;
}

unsigned char const* tcmplxA_ctxtmap_data_c(struct tcmplxA_ctxtmap const* x) {
  return x->p;
}

int tcmplxA_ctxtmap_get(struct tcmplxA_ctxtmap const* x, size_t i, size_t j) {
#ifndef NDEBUG
  if (i >= x->btypes || j >= x->ctxts)
    return -1;
#endif /*NDEBUG*/
  return x->p[i*x->ctxts + j];
}

void tcmplxA_ctxtmap_set
  (struct tcmplxA_ctxtmap* x, size_t i, size_t j, int v)
{
#ifndef NDEBUG
  if (i >= x->btypes || j >= x->ctxts)
    return;
#endif /*NDEBUG*/
  x->p[i*x->ctxts + j] = (unsigned char)v;
  return;
}

int tcmplxA_ctxtmap_distance_context(unsigned long int copylen) {
  if (copylen < 2)
    return -1;
  else switch (copylen) {
  case 2: return 0;
  case 3: return 1;
  case 4: return 2;
  default: return 3;
  }
}

int tcmplxA_ctxtmap_literal_context
    (int mode, unsigned int p1, unsigned int p2)
{
  switch (mode) {
  case tcmplxA_CtxtMap_LSB6:
    return p1&63u;
  case tcmplxA_CtxtMap_MSB6:
    return (p1>>2)&63u;
  case tcmplxA_CtxtMap_UTF8:
    return tcmplxA_ctxtmap_lut0[p1&255] | tcmplxA_ctxtmap_lut1[p2&255];
  case tcmplxA_CtxtMap_Signed:
    return (tcmplxA_ctxtmap_lut2[p1&255]<<3) | tcmplxA_ctxtmap_lut2[p2&255];
  default:
    return -1;
  }
}
/* END   context map / public */
