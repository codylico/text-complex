/**
 * @file text-complex/access/brmeta.c
 * @brief Brotli metadata storage
 */
#include "text-complex/access/brmeta.h"
#include "text-complex/access/util.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>


struct tcmplxA_brline;

enum tcmplxA_brmeta_const {
    tcmplxA_BrMeta_CapMax = INT_MAX/sizeof(struct tcmplxA_brline*),
    tcmplxA_BrMeta_CapInitMax = (63>tcmplxA_BrMeta_CapMax
        ? tcmplxA_BrMeta_CapMax : 63)
};

struct tcmplxA_brline {
    tcmplxA_uint32 len;
#if (defined __STDC_VERSION__) && (__STDC_VERSION__>=199901L)
    unsigned char data[];
#else
    unsigned char data[1];
#endif /*__STDC_VERSION__*/
};

struct tcmplxA_brmeta {
    struct tcmplxA_brline** lines;
    size_t n;
    size_t cap;
};
/**
 * @brief Initialize a metadata storage.
 * @param b storage to initialize
 * @param cap desired initial capacity
 * @return an error code
 */
static
int tcmplxA_brmeta_init(struct tcmplxA_brmeta* b, size_t cap);

/**
 * @brief Ensure space for one more slot.
 * @param b storage to check
 * @return Success if successful, nonzero otherwise
 */
static
int tcmplxA_brmeta_ensure(struct tcmplxA_brmeta* b);

/* BEGIN brmeta storage / static */
int tcmplxA_brmeta_init(struct tcmplxA_brmeta* b, size_t cap) {
  b->n = 0;
  b->cap = 0;
  b->lines = NULL;
  if (cap > tcmplxA_BrMeta_CapInitMax)
    cap = tcmplxA_BrMeta_CapInitMax;
  if (cap > 0) {
    struct tcmplxA_brline** const ptr =
      tcmplxA_util_malloc(cap*sizeof(struct tcmplxA_brline*));
    if (!ptr)
      return tcmplxA_ErrMemory;
#ifndef NDEBUG
    {
      size_t i;
      for (i = 0; i < b->cap; ++i)
        b->lines[i] = NULL;
    }
#endif /*NDEBUG*/
    b->lines = ptr;
    b->cap = cap;
  }
  return tcmplxA_Success;
}

int tcmplxA_brmeta_ensure(struct tcmplxA_brmeta* b) {
  struct tcmplxA_brline** ptr = NULL;
  size_t i;
  size_t const new_cap = (b->cap*2+1);
  if (b->n < b->cap)
    return tcmplxA_Success;
  else if (b->cap >= tcmplxA_BrMeta_CapMax/2)
    return tcmplxA_ErrMemory;
  ptr = tcmplxA_util_malloc(new_cap*sizeof(struct tcmplxA_brline*));
  if (!ptr)
    return tcmplxA_ErrMemory;
  for (i = 0; i < b->n; ++i) {
    ptr[i] = b->lines[i];
#ifndef NDEBUG
    b->lines[i] = NULL;
#endif /*NDEBUG*/
  }
  tcmplxA_util_free(b->lines);
  b->lines = ptr;
  b->cap = new_cap;
  return tcmplxA_Success;
}
/* END   brmeta storage / static */

/* BEGIN brmeta storage / public */
struct tcmplxA_brmeta* tcmplxA_brmeta_new(size_t cap) {
  struct tcmplxA_brmeta* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_brmeta));
  if (out != NULL
  &&  tcmplxA_brmeta_init(out, cap) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_brmeta_destroy(struct tcmplxA_brmeta* p) {
  size_t i;
  if (!p)
    return;
  for (i = 0; i < p->n; ++i) {
    if (p->lines[i])
      tcmplxA_util_free(p->lines[i]);
#ifndef NDEBUG
    p->lines[i] = NULL;
#endif /*NDEBUG*/
  }
#ifndef NDEBUG
  for (i = p->n; i < p->cap; ++i) {
    assert(!p->lines[i]);
  }
#endif /*NDEBUG*/
  tcmplxA_util_free(p->lines);
#ifndef NDEBUG
  p->n = 0;
  p->cap = 0;
#endif /*NDEBUG*/
  tcmplxA_util_free(p);
  return;
}


int tcmplxA_brmeta_emplace(struct tcmplxA_brmeta* b, size_t n) {
  int const ensure_res = tcmplxA_brmeta_ensure(b);
  if (ensure_res != tcmplxA_Success)
    return ensure_res;
  else if (n > 16777216)
    return tcmplxA_ErrMemory;
  else if (n == 0)
    return tcmplxA_ErrParam;
  else {
    struct tcmplxA_brline* const ptr =
      tcmplxA_util_malloc(sizeof(struct tcmplxA_brline)+n+1);
    if (ptr == NULL)
      return tcmplxA_ErrMemory;
    memset(ptr->data, 0, n+1);
    ptr->len = n;
    b->lines[b->n++] = ptr;
    return tcmplxA_Success;
  }
}

size_t tcmplxA_brmeta_size(struct tcmplxA_brmeta const* b) {
  return b->n;
}

size_t tcmplxA_brmeta_itemsize(struct tcmplxA_brmeta const* b, size_t i) {
  if (i >= b->n)
    return 0;
  assert(b->lines && b->lines[i]);
  return b->lines[i]->len;
}


unsigned char const* tcmplxA_brmeta_itemdata_c
    (struct tcmplxA_brmeta const* b, size_t i)
{
  if (i >= b->n)
    return NULL;
  assert(b->lines && b->lines[i]);
  return b->lines[i]->data;
}

unsigned char* tcmplxA_brmeta_itemdata
    (struct tcmplxA_brmeta const* b, size_t i)
{
  if (i >= b->n)
    return NULL;
  assert(b->lines && b->lines[i]);
  return b->lines[i]->data;
}
/* END   brmeta storage / public */
