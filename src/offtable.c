/**
 * \file text-complex/access/offtable.c
 * \brief Access point for TrueType files
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/offtable.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <limits.h>
#include <string.h>

struct tcmplxA_offtable {
  struct tcmplxA_offline* p;
  size_t n;
};

/**
 * \brief Initialize an offset table.
 * \param x the table to initialize
 * \param sz number of lines in the table
 * \return zero on success, nonzero otherwise
 */
static int tcmplxA_offtable_init(struct tcmplxA_offtable* x, size_t sz);
/**
 * \brief Resize an offset table.
 * \param x the table to resize
 * \param sz number of lines in the table
 * \return zero on success, nonzero otherwise
 */
static int tcmplxA_offtable_resize(struct tcmplxA_offtable* x, size_t sz);
/**
 * \brief Close an offset table.
 * \param x the table to close
 */
static void tcmplxA_offtable_close(struct tcmplxA_offtable* x);

/* BEGIN offset table / static */
int tcmplxA_offtable_init(struct tcmplxA_offtable* x, size_t sz) {
  x->p = NULL;
  x->n = 0u;
  return tcmplxA_offtable_resize(x,sz);
}

int tcmplxA_offtable_resize(struct tcmplxA_offtable* x, size_t sz) {
  struct tcmplxA_offline *ptr;
  if (sz == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
    }
    x->p = NULL;
    x->n = 0u;
    return tcmplxA_Success;
  } else if (sz >= UINT_MAX/sizeof(struct tcmplxA_offline)) {
    return tcmplxA_ErrMemory;
  }
  ptr = (struct tcmplxA_offline*)tcmplxA_util_malloc
          (sizeof(struct tcmplxA_offline)*sz);
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

void tcmplxA_offtable_close(struct tcmplxA_offtable* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->n = 0u;
  return;
}
/* END   offset table / static */

/* BEGIN offset table / public */
struct tcmplxA_offtable* tcmplxA_offtable_new(size_t n) {
  struct tcmplxA_offtable* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_offtable));
  if (out != NULL
  &&  tcmplxA_offtable_init(out,n) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_offtable_destroy(struct tcmplxA_offtable* x) {
  if (x != NULL) {
    tcmplxA_offtable_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

size_t tcmplxA_offtable_size(struct tcmplxA_offtable const* x) {
  return x->n;
}

struct tcmplxA_offline* tcmplxA_offtable_at
  (struct tcmplxA_offtable* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

struct tcmplxA_offline const* tcmplxA_offtable_at_c
  (struct tcmplxA_offtable const* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

int tcmplxA_offtable_copy
  (struct tcmplxA_offtable* dst, struct tcmplxA_offtable const* src)
{
  int res;
  if (dst == src) {
    return tcmplxA_Success;
  }
  res = tcmplxA_offtable_resize(dst, src->n);
  if (res != tcmplxA_Success) {
    return res;
  }
  memcpy(dst->p, src->p, sizeof(struct tcmplxA_offline)*dst->n);
  return tcmplxA_Success;
}
/* END   offset table / public */
