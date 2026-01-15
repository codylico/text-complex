/**
 * @file src/blockbuf.c
 * @brief DEFLATE block buffer
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "blockbuf_p.h"
#include "text-complex/access/blockbuf.h"
#include "text-complex/access/hashchain.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>


/**
 * @internal
 * Format: @verbatim
  Each command represented as a sequence of bytes
    [X][...]
  where
    X<128 represents an insert command, and
    X>=128 represents a copy command.
  Either way, the length is encoded as such:
    (X&64) == 0 -> length is (X&63)
    (X&64) == 64 -> length is ((X&63)<<8) + (Y&255) + 64
  where Y is the byte immediately following X.

  Insert commands are made up of an [X] or [X][Y] sequence
  followed by the literal bytes. A copy command is made up of
  an [X] or [X][Y] sequence followed by an encoded distance
  value.

  Distance values are represented as a sequence of bytes
    [R][...]
  where
    R<128 represents a bdict reference,
    (R&192) == 128 represents a 14-bit distance
    R>=192 represents a 30-bit distance.
  A bdict reference uses two extra bytes:
    [R][B1][B2]
  The filter is encoded in R as (R&127), the word index as (B1<<8)+B2,
  and the word length is the copy length.

  The 14-bit distance can be extracted from a two-byte sequence:
    [R][Q]
  as ((R&63)<<8) + (Q&255).

  The 30-bit distance can be extracted from a four-byte sequence:
    [R][S1][S2][S3]
  as ((R&63)<<24) + ((S1&255)<<16) + ((S2&255)<<8) + (S3&255) + 16384.
 @endverbatim
 *
 * Examples (values in hexadecimal): @verbatim
 (03)(41)(62)(63)         -> "Abc"
 (83)(80)(01)             -> copy 3 bytes, distance 1
 (01)(54)(83)(80)(01)     -> "TTTT"
 (C0)(05)(90)(02)         -> copy 69 bytes, distance 4098
 (C0)(06)(C0)(00)(00)(03) -> copy 70 bytes, distance 16387
 (84)(05)(00)(02)         -> "life the " (bdict length 4 filter 5 word 2)
 @endverbatim
 */

enum tcmplxA_blockstr_uconst {
  tcmplxA_BlockBuf_SizeMax =
    ((size_t)(-1) > 0x7FffFFffu) ? 0x7FffFFffu : (size_t)(-1),
  tcmplxA_BlockBuf_NPos = (tcmplxA_uint32)(-1),
  /**
   * @internal
   * @brief Maximum number to encode as an insert or copy length.
   */
  tcmplxA_BlockBuf_MaxOutCode = 16447u
};

struct tcmplxA_blockbuf {
  struct tcmplxA_hashchain* chain;
  struct tcmplxA_blockstr input;
  struct tcmplxA_blockstr output;
  int bdict_tf;
  tcmplxA_uint32 input_block_size;
};

/**
 * @brief Add a copy command to an output buffer.
 * @param x the output buffer
 * @param match_size the copy length
 * @param v backward distance
 * @return tcmplxA_Success on success
 */
static int tcmplxA_blockstr_add_copy
  (struct tcmplxA_blockstr* x, tcmplxA_uint32 match_size, tcmplxA_uint32 v);
/**
 * @brief Update an insert command in an output buffer.
 * @param x the output buffer
 * @param b the byte to add
 * @param[in,out] j location of insert command
 * @return tcmplxA_Success on success
 */
static int tcmplxA_blockstr_update_literal
  (struct tcmplxA_blockstr* x, unsigned int b, tcmplxA_uint32* j);

/**
 * @brief Initialize a block buffer.
 * @param x the block buffer to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_blockbuf_init
  ( struct tcmplxA_blockbuf* x, tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length, int bdict_tf);
/**
 * @brief Close a block buffer.
 * @param x the block buffer to close
 */
static void tcmplxA_blockbuf_close(struct tcmplxA_blockbuf* x);

/* BEGIN block buffer / static */
int tcmplxA_blockstr_init
  (struct tcmplxA_blockstr* x, tcmplxA_uint32 cap)
{
  x->cap = 0u;
  x->sz = 0u;
  x->p = NULL;
  if (cap > 0u) {
    int const res = tcmplxA_blockstr_reserve(x, cap);
    if (res != tcmplxA_Success)
      return res;
  }
  return tcmplxA_Success;
}

void tcmplxA_blockstr_close(struct tcmplxA_blockstr* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->sz = 0u;
  x->cap = 0u;
  return;
}

int tcmplxA_blockstr_reserve
  (struct tcmplxA_blockstr* x, tcmplxA_uint32 sz)
{
  if (sz <= x->cap)
    return tcmplxA_Success;
  else {
    unsigned char* const input_data =
      (unsigned char*)tcmplxA_util_malloc
          ((size_t)(sz)*sizeof(unsigned char));
    if (input_data != NULL) {
      memcpy(input_data, x->p, x->sz);
      tcmplxA_util_free(x->p);
      x->cap = sz;
      x->p = input_data;
      return tcmplxA_Success;
    } else return tcmplxA_ErrMemory;
  }
}

int tcmplxA_blockstr_append
  (struct tcmplxA_blockstr* x, unsigned char const* buf, size_t n)
{
  size_t i;
  tcmplxA_uint32 const old_sz = x->sz;
  for (i = 0u; i < n; ) {
    tcmplxA_uint32 rem = x->cap - x->sz;
    for (; rem-- > 0u && i < n; ++i) {
      x->p[x->sz++] = buf[i];
    }
    /* expand */if (i < n) {
      tcmplxA_uint32 const new_cap = (x->cap == 0u) ? 1u : x->cap*2u;
      int const reserve_res = tcmplxA_blockstr_reserve(x, new_cap);
      if (reserve_res != tcmplxA_Success) {
        x->sz = old_sz;
        return reserve_res;
      }
    }
  }
  return tcmplxA_Success;
}

int tcmplxA_blockstr_add_copy
  (struct tcmplxA_blockstr* x, tcmplxA_uint32 match_size, tcmplxA_uint32 v)
{
  unsigned char buf[6];
  unsigned int i;
  if (match_size >= 64u) {
    tcmplxA_uint32 const t = match_size-64u;
    buf[0] = (unsigned char)((t&63u)>>8)|192u;
    buf[1] = (unsigned char)(t&255u);
    i = 2u;
  } else {
    buf[0] = (unsigned char)(match_size | 128u);
    i = 1u;
  }
  if (v >= 16384u) {
    tcmplxA_uint32 const t = v-16384u;
    buf[i++] = (unsigned char)(((t>>24)&63u) | 192u);
    buf[i++] = (unsigned char)((t>>16)&255u);
    buf[i++] = (unsigned char)((t>>8)&255u);
    buf[i++] = (unsigned char)(t&255u);
  } else {
    buf[i++] = (unsigned char)(((v>>8)&63u) | 128u);
    buf[i++] = (unsigned char)(v&255u);
  }
  return tcmplxA_blockstr_append(x, buf, i);
}
int tcmplxA_blockstr_update_literal
  (struct tcmplxA_blockstr* x, unsigned int b, tcmplxA_uint32* jptr)
{
  tcmplxA_uint32 const j = *jptr;
  int res;
  unsigned char bytes[2];
  bytes[0] = 1u;
  bytes[1] = (unsigned char)b;
  if (j == x->sz) {
    /* add a new literal */
    res = tcmplxA_blockstr_append(x, bytes, 2u);
  } else if (x->p[j] == 63u) {
    /* expand */
    res = tcmplxA_blockstr_append(x, bytes, 2u);
    if (res != tcmplxA_Success)
      return res;
    memmove(x->p+j+2, x->p+j+1, 63u*sizeof(unsigned char));
    x->p[j] = 64u;
    x->p[j+1u] = 0u;
  } else if (x->p[j] >= 64u) {
    /* long sequence */
    unsigned short int len = (x->p[j]<<8)|x->p[j+1];
    res = tcmplxA_blockstr_append(x, bytes+1u, 1u);
    len += 1u;
    x->p[j] = (unsigned char)(len>>8);
    x->p[j+1u] = (unsigned char)(len&255u);
    if (len == 0x7Fff)
      *jptr = x->sz;
  } else {
    res = tcmplxA_blockstr_append(x, bytes+1u, 1u);
    x->p[j] += 1u;
  }
  return res;
}

int tcmplxA_blockbuf_init
  ( struct tcmplxA_blockbuf* x, tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length, int bdict_tf)
{
  x->chain = NULL;
  x->bdict_tf = bdict_tf;
  /* truncate lengths */{
    if (block_size > tcmplxA_BlockBuf_SizeMax/2u) {
      block_size = (tcmplxA_uint32)(tcmplxA_BlockBuf_SizeMax/2u);
    }
  }
  /* allocate things */{
    struct tcmplxA_hashchain* const chain =
      tcmplxA_hashchain_new(n, chain_length);
    int const input_res = tcmplxA_blockstr_init(&x->input, block_size);
    int const output_res = tcmplxA_blockstr_init(&x->output, block_size);
    if (chain == NULL || input_res != tcmplxA_Success
        || output_res != tcmplxA_Success)
    {
      tcmplxA_blockstr_close(&x->output);
      tcmplxA_blockstr_close(&x->input);
      tcmplxA_hashchain_destroy(chain);
      return tcmplxA_ErrMemory;
    } else {
      x->chain = chain;
      x->input_block_size = block_size;
    }
  }
  return tcmplxA_Success;
}

void tcmplxA_blockbuf_close(struct tcmplxA_blockbuf* x) {
  if (x->chain != NULL) {
    tcmplxA_hashchain_destroy(x->chain);
    x->chain = NULL;
  }
  tcmplxA_blockstr_close(&x->input);
  tcmplxA_blockstr_close(&x->output);
  return;
}
/* END   block buffer / static */

/* BEGIN block buffer / public */
struct tcmplxA_blockbuf* tcmplxA_blockbuf_new
  ( tcmplxA_uint32 block_size, tcmplxA_uint32 n, size_t chain_length,
    int bdict_tf)
{
  struct tcmplxA_blockbuf* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_blockbuf));
  if (out != NULL
  &&  tcmplxA_blockbuf_init(out,block_size,n,chain_length,bdict_tf)
        != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_blockbuf_destroy(struct tcmplxA_blockbuf* x) {
  if (x != NULL) {
    tcmplxA_blockbuf_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

int tcmplxA_blockbuf_gen_block(struct tcmplxA_blockbuf* x) {
  int const ae = tcmplxA_blockbuf_try_block(x);
  if (ae == tcmplxA_Success)
    tcmplxA_blockbuf_clear_input(x);
  return ae;
}

int tcmplxA_blockbuf_noconv_block(struct tcmplxA_blockbuf* x) {
  return tcmplxA_blockstr_append(&x->output, x->input.p, x->input.sz);
}

int tcmplxA_blockbuf_try_block(struct tcmplxA_blockbuf* x) {
  int res = tcmplxA_Success;
  tcmplxA_uint32 const n = 51u;
  tcmplxA_uint32 j = x->output.sz;
  tcmplxA_uint32 i;
  tcmplxA_uint32 const input_sz = x->input.sz;
  tcmplxA_uint32 other_v = 0u;
  tcmplxA_uint32 v = 0u;
  unsigned char state = 0;
  unsigned char skipped_byte = 0u;
  tcmplxA_uint32 other_match_size = 0u;
  tcmplxA_uint32 match_size = 0u;
  for (i = 0u; i < input_sz && res == tcmplxA_Success; ++i) {
    switch (state) {
    case 2:
      /* two things */{
        unsigned int const q = tcmplxA_hashchain_peek(x->chain, v);
        unsigned int const other_q = tcmplxA_hashchain_peek(x->chain, other_v);
        if (q != x->input.p[i]) {
          res = tcmplxA_blockstr_update_literal(&x->output, skipped_byte, &j);
          v = other_v;
          match_size = other_match_size;
          state = 1;
        } else if (other_q != x->input.p[i]
            || match_size >= tcmplxA_BlockBuf_MaxOutCode)
        {
          state = 1;
        } else {
          match_size += 1u;
          other_match_size += 1u;
          res = tcmplxA_hashchain_add(x->chain, x->input.p[i]);
          break;
        }
      }
      if (res != tcmplxA_Success)
        break;
      /* [[fallthrough]] */;
    case 1: /* one thing */
      {
        unsigned int const q = tcmplxA_hashchain_peek(x->chain, v);
        if (q == x->input.p[i] && match_size < tcmplxA_BlockBuf_MaxOutCode) {
          match_size += 1u;
          res = tcmplxA_hashchain_add(x->chain, x->input.p[i]);
          break;
        } else {
          /* close the match */
          res = tcmplxA_blockstr_add_copy(&x->output, match_size, v);
          if (res != tcmplxA_Success)
            break;
          else j = x->output.sz;
          state = 0;
        }
      }
      /* [[fallthrough]] */;
    case 0:
      if (i <= input_sz-4u) {
        v = tcmplxA_hashchain_find(x->chain, x->input.p+i, 0);
        if (v != tcmplxA_BlockBuf_NPos) {
          other_v = tcmplxA_hashchain_find(x->chain, x->input.p+i+1, 0);
          if (other_v != tcmplxA_BlockBuf_NPos) {
            other_match_size = 2u;
            state = 2;
            other_v += 1u;
          } else {
            other_match_size = 0u;
            state = 1;
          }
          match_size = 3u;
          skipped_byte = x->input.p[i];
          res = tcmplxA_hashchain_add(x->chain, x->input.p[i]);
          if (res != tcmplxA_Success)
            break;
          res = tcmplxA_hashchain_add(x->chain, x->input.p[i+1u]);
          if (res != tcmplxA_Success)
            break;
          res = tcmplxA_hashchain_add(x->chain, x->input.p[i+2u]);
          i += 2u;
          break;
        }
      }
      /* literal */{
        res = tcmplxA_blockstr_update_literal(&x->output, x->input.p[i], &j);
        if (res != tcmplxA_Success)
          break;
        res = tcmplxA_hashchain_add(x->chain, x->input.p[i]);
      } break;
    }
  }
  if (res == tcmplxA_Success && state > 0) {
    /* close the match */
    res = tcmplxA_blockstr_add_copy(&x->output, match_size, v);
  }
  return tcmplxA_Success;
}

int tcmplxA_blockbuf_write
  (struct tcmplxA_blockbuf* x, unsigned char const* buf, size_t sz)
{
  if (sz > x->input_block_size - x->input.sz) {
    return tcmplxA_ErrBlockOverflow;
  } else return tcmplxA_blockstr_append(&x->input, buf, sz);
}

tcmplxA_uint32 tcmplxA_blockbuf_input_size(struct tcmplxA_blockbuf const* x) {
  return x->input.sz;
}

tcmplxA_uint32 tcmplxA_blockbuf_capacity(struct tcmplxA_blockbuf const* x) {
  return x->input_block_size;
}

tcmplxA_uint32 tcmplxA_blockbuf_output_size(struct tcmplxA_blockbuf const* x) {
  return x->output.sz;
}

unsigned char const* tcmplxA_blockbuf_input_data
  (struct tcmplxA_blockbuf const* x)
{
  return x->input.p;
}

unsigned char const* tcmplxA_blockbuf_output_data
  (struct tcmplxA_blockbuf const* x)
{
  return x->output.p;
}

void tcmplxA_blockbuf_clear_output(struct tcmplxA_blockbuf* x) {
  x->output.sz = 0u;
  return;
}

size_t tcmplxA_blockbuf_bypass
  (struct tcmplxA_blockbuf* x, unsigned char const* buf, size_t sz)
{
  int chain_ae = tcmplxA_Success;
  size_t i;
  for (i = 0u; i < sz && chain_ae == tcmplxA_Success; ++i) {
    chain_ae = tcmplxA_hashchain_add(x->chain, buf[i]);
  }
  return i;
}

unsigned int tcmplxA_blockbuf_peek
  (struct tcmplxA_blockbuf const* x, tcmplxA_uint32 i)
{
  return tcmplxA_hashchain_peek(x->chain, i);
}

tcmplxA_uint32 tcmplxA_blockbuf_extent(struct tcmplxA_blockbuf const* x) {
  return tcmplxA_hashchain_extent(x->chain);
}

tcmplxA_uint32 tcmplxA_blockbuf_ring_size(struct tcmplxA_blockbuf const* x) {
  return tcmplxA_hashchain_size(x->chain);
}

void tcmplxA_blockbuf_clear_input(struct tcmplxA_blockbuf* x) {
  x->input.sz = 0u;
  return;
}
/* END   block buffer / public */
