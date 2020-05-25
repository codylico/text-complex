/**
 * @file tcmplx-access/fixlist.c
 * @brief Prefix code list
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "fixlist.h"
#include "api.h"
#include "util.h"
#include <string.h>

struct tcmplxA_fixlist {
  struct tcmplxA_fixline* p;
  size_t n;
};

/**
 * @brief Initialize a prefix list.
 * @param x the prefix list to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_fixlist_init(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Resize a prefix list.
 * @param x the list to resize
 * @param sz number of lines in the list
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_fixlist_resize(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Close a prefix list.
 * @param x the prefix list to close
 */
static void tcmplxA_fixlist_close(struct tcmplxA_fixlist* x);

/* BEGIN prefix list / static */
int tcmplxA_fixlist_init(struct tcmplxA_fixlist* x, size_t sz) {
  x->p = NULL;
  x->n = 0u;
  return tcmplxA_fixlist_resize(x,sz);
}

int tcmplxA_fixlist_resize(struct tcmplxA_fixlist* x, size_t sz) {
  struct tcmplxA_fixline *ptr;
  if (sz == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
    }
    x->p = NULL;
    x->n = 0u;
    return tcmplxA_Success;
  } else if (sz >= UINT_MAX/sizeof(struct tcmplxA_fixline)) {
    return tcmplxA_ErrMemory;
  }
  ptr = (struct tcmplxA_fixline*)tcmplxA_util_malloc
          (sizeof(struct tcmplxA_fixline)*sz);
  if (ptr == NULL) {
    return tcmplxA_ErrMemory;
  }
  if (x->p != NULL) {
    tcmplxA_util_free(x->p);
  }
  x->p = ptr;
  x->n = sz;
  return tcmplxA_Success;
}

void tcmplxA_fixlist_close(struct tcmplxA_fixlist* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->n = 0u;
  return;
}
/* END   prefix list / static */

/* BEGIN prefix list / public */
struct tcmplxA_fixlist* tcmplxA_fixlist_new(size_t sz) {
  struct tcmplxA_fixlist* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_fixlist));
  if (out != NULL
  &&  tcmplxA_fixlist_init(out, sz) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_fixlist_destroy(struct tcmplxA_fixlist* x) {
  if (x != NULL) {
    tcmplxA_fixlist_close(x);
    tcmplxA_util_free(x);
  }
  return;
}


size_t tcmplxA_fixlist_size(struct tcmplxA_fixlist const* x) {
  return x->n;
}

struct tcmplxA_fixline* tcmplxA_fixlist_at
  (struct tcmplxA_fixlist* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

struct tcmplxA_fixline const* tcmplxA_fixlist_at_c
  (struct tcmplxA_fixlist const* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

int tcmplxA_fixlist_copy
  (struct tcmplxA_fixlist* dst, struct tcmplxA_fixlist const* src)
{
  int res;
  if (dst == src) {
    return tcmplxA_Success;
  }
  res = tcmplxA_fixlist_resize(dst, src->n);
  if (res != tcmplxA_Success) {
    return res;
  }
  memcpy(dst->p, src->p, sizeof(struct tcmplxA_fixline)*dst->n);
  return tcmplxA_Success;
}
/* END   prefix list / public */
