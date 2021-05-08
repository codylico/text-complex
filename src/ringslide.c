/**
 * @file src/ringslide.c
 * @brief Sliding window of past bytes
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/ringslide.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"

struct tcmplxA_ringslide {
  tcmplxA_uint32 n;
  tcmplxA_uint32 pos;
  size_t cap;
  unsigned char* p;
};

/**
 * @brief Initialize a slide ring.
 * @param x the slide ring to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ringslide_init
  (struct tcmplxA_ringslide* x, tcmplxA_uint32 n);
/**
 * @brief Close a slide ring.
 * @param x the slide ring to close
 */
static void tcmplxA_ringslide_close(struct tcmplxA_ringslide* x);

/* BEGIN slide ring / static */
int tcmplxA_ringslide_init(struct tcmplxA_ringslide* x, tcmplxA_uint32 n) {
  if (n > 0x1000000)
    return tcmplxA_ErrParam;
  else {
    x->n = n;
    x->pos = 0u;
    x->cap = 0u;
    x->p = NULL;
    return tcmplxA_Success;
  }
}

void tcmplxA_ringslide_close(struct tcmplxA_ringslide* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->cap = 0u;
  x->pos = 0u;
  x->n = 0u;
  return;
}
/* END   slide ring / static */

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
/* END   slide ring / public */
