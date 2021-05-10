/**
 * @file src/hashchain.c
 * @brief Duplicate lookup hash chain
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ringslide_p.h"
#include "text-complex/access/hashchain.h"
#include "text-complex/access/ringslide.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <limits.h>
#include <string.h>

struct tcmplxA_hashchain {
  struct tcmplxA_ringslide sr;
  unsigned char last_count;
  unsigned char last_bytes[3];
  tcmplxA_uint32 counter;
  tcmplxA_uint32* chains;
  size_t* positions;
  size_t chain_length;
};

enum tcmplxA_hashchain_const {
  tcmplxA_HashChain_Max = (((size_t)-1)/sizeof(tcmplxA_uint32))/251u
};

/**
 * @brief Initialize a hash chain.
 * @param n maximum sliding window size
 * @param x the hash chain to initialize
 * @param chain_length run-time parameter limiting hash chain length
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_hashchain_init
  (struct tcmplxA_hashchain* x, tcmplxA_uint32 n, size_t chain_length);
/**
 * @brief Close a hash chain.
 * @param x the hash chain to close
 */
static void tcmplxA_hashchain_close(struct tcmplxA_hashchain* x);

/* BEGIN hash chain / static */
int tcmplxA_hashchain_init
  (struct tcmplxA_hashchain* x, tcmplxA_uint32 n, size_t chain_length)
{
  int const res = tcmplxA_ringslide_init(&x->sr, n);
  if (res != tcmplxA_Success)
    return res;
  else {
    tcmplxA_uint32 *new_chains;
    size_t *new_positions;
    if (chain_length >= tcmplxA_HashChain_Max) {
      tcmplxA_ringslide_close(&x->sr);
      return tcmplxA_ErrMemory;
    }
    new_chains = tcmplxA_util_malloc(chain_length*251u*sizeof(tcmplxA_uint32));
    new_positions = tcmplxA_util_malloc(251u*sizeof(size_t));
    if (new_chains == NULL || new_positions == NULL) {
      tcmplxA_util_free(new_positions);
      tcmplxA_util_free(new_chains);
      tcmplxA_ringslide_close(&x->sr);
      return tcmplxA_ErrMemory;
    }
    x->last_count = 0u;
    memset(x->last_bytes, 0, sizeof(unsigned char)*3u);
    x->counter = 0u;
    x->chain_length = chain_length;
    return tcmplxA_Success;
  }
}

void tcmplxA_hashchain_close(struct tcmplxA_hashchain* x) {
  tcmplxA_util_free(x->positions);
  x->positions = NULL;
  tcmplxA_util_free(x->chains);
  x->chains = NULL;
  tcmplxA_ringslide_close(&x->sr);
  return;
}
/* END   hash chain / static */

/* BEGIN hash chain / public */
struct tcmplxA_hashchain* tcmplxA_hashchain_new
  (tcmplxA_uint32 n, size_t chain_length)
{
  struct tcmplxA_hashchain* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_hashchain));
  if (out != NULL
  &&  tcmplxA_hashchain_init(out,n,chain_length) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_hashchain_destroy(struct tcmplxA_hashchain* x) {
  if (x != NULL) {
    tcmplxA_hashchain_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

tcmplxA_uint32 tcmplxA_hashchain_extent(struct tcmplxA_hashchain const* x) {
  return tcmplxA_ringslide_extent(&x->sr);
}

int tcmplxA_hashchain_add(struct tcmplxA_hashchain* x, unsigned int v) {
  return tcmplxA_ringslide_add(&x->sr, v);
}

tcmplxA_uint32 tcmplxA_hashchain_size(struct tcmplxA_hashchain const* x) {
  return tcmplxA_ringslide_size(&x->sr);
}

unsigned int tcmplxA_hashchain_peek
  (struct tcmplxA_hashchain const* x, tcmplxA_uint32 i)
{
  return tcmplxA_ringslide_peek(&x->sr, i);
}
/* END   hash chain / public */
