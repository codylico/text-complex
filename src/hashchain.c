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
/**
 * @brief Calculate a hash.
 * @param b three bytes to use as input
 * @return a hash value
 */
static unsigned int tcmplxA_hashchain_hash(unsigned char const* b);
/**
 * @brief Fetch some bytes from the past.
 * @param sr slide ring to use
 * @param[out] b three bytes to use as output
 * @param x backward distance to inspect
 */
static void tcmplxA_hashchain_fetch
    (struct tcmplxA_ringslide const* sr, unsigned char* b, tcmplxA_uint32 x);

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
    size_t const chain_size = chain_length*251u*sizeof(tcmplxA_uint32);
    size_t const positions_size = 251u*sizeof(size_t);
    if (chain_length >= tcmplxA_HashChain_Max) {
      tcmplxA_ringslide_close(&x->sr);
      return tcmplxA_ErrMemory;
    }
    new_chains = tcmplxA_util_malloc(chain_size);
    new_positions = tcmplxA_util_malloc(positions_size);
    if (new_chains == NULL || new_positions == NULL) {
      tcmplxA_util_free(new_positions);
      tcmplxA_util_free(new_chains);
      tcmplxA_ringslide_close(&x->sr);
      return tcmplxA_ErrMemory;
    }
    memset(new_chains, 0, chain_size);
    memset(new_positions, 0, positions_size);
    x->last_count = 0u;
    memset(x->last_bytes, 0, sizeof(unsigned char)*3u);
    x->counter = 0u;
    x->chain_length = chain_length;
    x->chains = new_chains;
    x->positions = new_positions;
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

unsigned int tcmplxA_hashchain_hash(unsigned char const* b) {
  return ((unsigned int)((b[0]<<6)+(b[1]<<3)+b[2]))%251u;
}

void tcmplxA_hashchain_fetch
    (struct tcmplxA_ringslide const* sr, unsigned char* b, tcmplxA_uint32 x)
{
  b[2] = (unsigned char)tcmplxA_ringslide_peek(sr, x);
  b[1] = (unsigned char)tcmplxA_ringslide_peek(sr, x+1u);
  b[0] = (unsigned char)tcmplxA_ringslide_peek(sr, x+2u);
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
  /* rotate the bytes */{
    unsigned char* const bytes = x->last_bytes;
    bytes[0] = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = (unsigned char)v;
  }
  /* add to hash table */{
    unsigned int const i = tcmplxA_hashchain_hash(x->last_bytes);
    size_t const pos = x->positions[i];
    tcmplxA_uint32 *const chain = x->chains+(i*x->chain_length);
    chain[pos] = x->counter++;
    x->positions[i] = (pos+1u >= x->chain_length ? 0u : pos+1u);
  }
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

tcmplxA_uint32 tcmplxA_hashchain_find
  ( struct tcmplxA_hashchain const* x, unsigned char const* b,
    tcmplxA_uint32 pos)
{
  if (tcmplxA_ringslide_size(&x->sr) < 3u)
    return ((tcmplxA_uint32)-1);
  else {
    unsigned int const i = tcmplxA_hashchain_hash(b);
    size_t chain_i = x->positions[i];
    tcmplxA_uint32 *const chain = x->chains+(i*x->chain_length);
    tcmplxA_uint32 const here = x->counter;
    tcmplxA_uint32 const size = tcmplxA_ringslide_size(&x->sr)-2u;
    size_t j;
    for (j = 0u; j < x->chain_length; ++j) {
      if (chain_i == 0u)
        chain_i = x->chain_length;
      /* */{
        tcmplxA_uint32 const y = here-chain[--chain_i]-1u;
        if (y < pos)
          continue;
        else if (y >= size)
          return ((tcmplxA_uint32)-1);
        else {
          unsigned char tmp[3];
          tcmplxA_hashchain_fetch(&x->sr, tmp, y);
          if (memcmp(tmp, b, 3u*sizeof(unsigned char)) == 0)
              return y+2u;
        }
      }
    }
    return ((tcmplxA_uint32)-1);
  }
}
/* END   hash chain / public */
