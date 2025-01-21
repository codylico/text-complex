/**
 * @file text-complex/access/gaspvec.c
 * @brief Forest of prefix trees
 */
#include "fixlist_p.h"
#include <text-complex/access/gaspvec.h>
#include <text-complex/access/util.h>


struct tcmplxA_gaspvec {
  struct tcmplxA_fixlist* trees;
  unsigned count;
  unsigned cap;
};


/**
 * @internal
 * @brief Initialize a prefix gasp vector.
 * @param x the prefix gasp vector to initialize
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_gaspvec_init(struct tcmplxA_gaspvec* x, size_t sz);
/**
 * @internal
 * @brief Resize a prefix gasp vector.
 * @param x the vector to resize
 * @param sz number of trees in the list
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_gaspvec_resize(struct tcmplxA_gaspvec* x, size_t sz);
/**
 * @internal
 * @brief Close a prefix gasp vector.
 * @param x the prefix gasp vector to close
 */
void tcmplxA_gaspvec_close(struct tcmplxA_gaspvec* x);

#pragma region("gaspvec / static")
int tcmplxA_gaspvec_init(struct tcmplxA_gaspvec* x, size_t sz) {
  x->trees = NULL;
  x->count = 0u;
  x->cap = 0u;
  return tcmplxA_gaspvec_resize(x,sz);
}

int tcmplxA_gaspvec_resize(struct tcmplxA_gaspvec* x, size_t sz) {
  struct tcmplxA_fixlist *ptr = NULL;
  if (sz >= UINT_MAX/sizeof(struct tcmplxA_fixlist)) {
    return tcmplxA_ErrMemory;
  } else if (sz > 0 && sz > x->cap) {
    size_t i;
    ptr = (struct tcmplxA_fixlist*)tcmplxA_util_malloc
            (sizeof(struct tcmplxA_fixlist)*sz);
    if (!ptr)
      return tcmplxA_ErrMemory;
    if (x->trees) {
      for (i = 0; i < x->count; ++i)
        tcmplxA_fixlist_close(&x->trees[i]);
      tcmplxA_util_free(x->trees);
    }
    for (i = 0; i < sz; ++i)
      tcmplxA_fixlist_init(&ptr[i], 0);
    x->trees = ptr;
    x->cap = sz;
  }
  x->count = sz;
  return tcmplxA_Success;
}

void tcmplxA_gaspvec_close(struct tcmplxA_gaspvec* x) {
  unsigned i;
  for (i = 0; i < x->count; ++i)
    tcmplxA_fixlist_close(&x->trees[i]);
  tcmplxA_util_free(x->trees);
  x->trees = NULL;
  x->count = 0u;
  x->cap = 0u;
  return;
}
#pragma endregion

#pragma region("gaspvec / functions")
struct tcmplxA_gaspvec* tcmplxA_gaspvec_new(size_t n) {
  struct tcmplxA_gaspvec* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_gaspvec));
  if (out != NULL
  &&  tcmplxA_gaspvec_init(out, n) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_gaspvec_destroy(struct tcmplxA_gaspvec* x) {
  if (!x)
    return;
  tcmplxA_gaspvec_close(x);
  tcmplxA_util_free(x);
  return;
}

size_t tcmplxA_gaspvec_size(struct tcmplxA_gaspvec const* x) {
  return x->count;
}

struct tcmplxA_fixlist* tcmplxA_gaspvec_at
  (struct tcmplxA_gaspvec* x, size_t i)
{
  if (i >= x->count)
    return NULL;
  else return &x->trees[i];
}

struct tcmplxA_fixlist const* tcmplxA_gaspvec_at_c
  (struct tcmplxA_gaspvec const* x, size_t i)
{
  if (i >= x->count)
    return NULL;
  else return &x->trees[i];
}
#pragma endregion
