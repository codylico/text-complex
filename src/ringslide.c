/**
 * @file src/ringslide.c
 * @brief Sliding window of past bytes
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ringslide_p.h"
#include "text-complex/access/ringslide.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>



/* BEGIN slide ring / private */
int tcmplxA_ringslide_init(struct tcmplxA_ringslide* x, tcmplxA_uint32 n) {
  if (n > 0x1000000)
    return tcmplxA_ErrParam;
  else {
    x->n = n;
    x->pos = 0u;
    x->cap = 0u;
    x->sz = 0u;
    x->p = NULL;
    return tcmplxA_Success;
  }
}

void tcmplxA_ringslide_close(struct tcmplxA_ringslide* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->cap = 0u;
  x->pos = 0u;
  x->sz = 0u;
  x->n = 0u;
  return;
}
/* END   slide ring / private */

/* BEGIN slide ring / public */
struct tcmplxA_ringslide* tcmplxA_ringslide_new(tcmplxA_uint32 n) {
  struct tcmplxA_ringslide* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_ringslide));
  if (out != NULL
  &&  tcmplxA_ringslide_init(out, n) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_ringslide_destroy(struct tcmplxA_ringslide* x) {
  if (x != NULL) {
    tcmplxA_ringslide_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

tcmplxA_uint32 tcmplxA_ringslide_extent(struct tcmplxA_ringslide const* x) {
  return x->n;
}

int tcmplxA_ringslide_add(struct tcmplxA_ringslide* x, unsigned int v) {
  /* precondition: x->pos < x->n */;
  tcmplxA_uint32 const pos = x->pos;
  if (pos >= x->cap) {
    /* try to expand capacity */
    if (x->cap > ((size_t)-1)/2u)
      return tcmplxA_ErrMemory;
    else {
      size_t const ncap = (x->cap == 0u) ? 1u : x->cap*2u;
      unsigned char* const ptr =
        (unsigned char*)tcmplxA_util_malloc(ncap*sizeof(unsigned char));
      if (ptr == NULL) {
        return tcmplxA_ErrMemory;
      } else {
        memcpy(ptr, x->p, x->cap*sizeof(unsigned char));
        tcmplxA_util_free(x->p);
        x->p = ptr;
        x->cap = ncap;
      }
    }
  }
  x->p[pos] = (unsigned char)v;
  if (pos+1u >= x->n) {
    x->sz = x->n;
    x->pos = 0u;
  } else {
    x->pos = pos+1u;
    x->sz = (x->pos > x->sz) ? x->pos : x->sz;
  }
  return tcmplxA_Success;
}

tcmplxA_uint32 tcmplxA_ringslide_size(struct tcmplxA_ringslide const* x) {
  return x->sz;
}

unsigned int tcmplxA_ringslide_peek
  (struct tcmplxA_ringslide const* x, tcmplxA_uint32 i)
{
  if (i >= x->sz)
    return 0u;
  else if (i >= x->pos)
    return x->p[x->n-(i-x->pos)-1u];
  else return x->p[x->pos-i-1u];
}
/* END   slide ring / public */
