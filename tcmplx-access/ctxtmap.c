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
/* END   context map / public */
