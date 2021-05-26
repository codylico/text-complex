/**
 * @file src/zcvt.c
 * @brief zlib conversion state
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/zcvt.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include "text-complex/access/blockbuf.h"
#include "text-complex/access/fixlist.h"
#include "text-complex/access/inscopy.h"
#include "text-complex/access/ringdist.h"
#include "text-complex/access/zutil.h"

struct tcmplxA_zcvt {
  /** @brief ... */
  struct tcmplxA_blockbuf* buffer;
  /** @brief ... */
  struct tcmplxA_fixlist* literals;
  /** @brief ... */
  struct tcmplxA_fixlist* distances;
  /** @brief ... */
  struct tcmplxA_fixlist* sequence;
  /** @brief ... */
  struct tcmplxA_inscopy* values;
  /** @brief ... */
  struct tcmplxA_ringdist* ring;
  /** @brief ... */
  unsigned short int bits;
  /** @brief Read count for bits used after a Huffman code. */
  unsigned short int extra_length;
  /** @brief ... */
  unsigned char bit_length;
  /** @brief End indicator. */
  unsigned char h_end;
  /** @brief ... */
  unsigned char state;
  /** @brief Bit position in the stream. */
  unsigned char bit_index;
  /** @brief Backward distance value. */
  tcmplxA_uint32 backward;
  /** @brief Byte count for the active state. */
  tcmplxA_uint32 count;
  /** @brief Byte index for the active state. */
  tcmplxA_uint32 index;
  /** @brief Checksum value. */
  tcmplxA_uint32 checksum;
};

unsigned char tcmplxA_zcvt_clen[19] =
  {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/**
 * @brief Initialize a zcvt state.
 * @param x the zcvt state to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_zcvt_init
  ( struct tcmplxA_zcvt* x, tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length);
/**
 * @brief Close a zcvt state.
 * @param x the zcvt state to close
 */
static void tcmplxA_zcvt_close(struct tcmplxA_zcvt* x);
/**
 * @brief Bit iteration.
 * @param x the zcvt state
 * @param y bits to iterate
 * @param[out] ret number of (inflated) destination bytes written
 * @param dst destination buffer
 * @param dstsz size of destination buffer
 * @return tcmplxA_Success on success, tcmplxA_ErrPartial on incomplete
 *   bit sequence
 */
static int tcmplxA_zcvt_zsrtostr_bits
  ( struct tcmplxA_zcvt* x, unsigned int y,
    size_t* ret, unsigned char* dst, size_t dstsz);

/* BEGIN zcvt state / static */
int tcmplxA_zcvt_init
    ( struct tcmplxA_zcvt* x, tcmplxA_uint32 block_size,
      tcmplxA_uint32 n, size_t chain_length)
{
  int res = tcmplxA_Success;
  /* buffer */{
    x->buffer = tcmplxA_blockbuf_new(block_size,n,chain_length, 0);
    if (x->buffer == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* literals */{
    x->literals = tcmplxA_fixlist_new(288u);
    if (x->literals == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* distances */{
    x->distances = tcmplxA_fixlist_new(32u);
    if (x->distances == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* sequence */{
    x->sequence = tcmplxA_fixlist_new(19u);
    if (x->sequence == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* ring */{
    x->ring = tcmplxA_ringdist_new(0,4u,0u);
    if (x->ring == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* values */{
    x->values = tcmplxA_inscopy_new(286u);
    if (x->values == NULL)
      res = tcmplxA_ErrMemory;
    else {
      int const preset_ae =
        tcmplxA_inscopy_preset(x->values, tcmplxA_InsCopy_Deflate);
      if (preset_ae != tcmplxA_Success)
        res = preset_ae;
      else tcmplxA_inscopy_codesort(x->values);
    }
  }
  if (res != tcmplxA_Success) {
    tcmplxA_inscopy_destroy(x->values);
    tcmplxA_ringdist_destroy(x->ring);
    tcmplxA_fixlist_destroy(x->sequence);
    tcmplxA_fixlist_destroy(x->distances);
    tcmplxA_fixlist_destroy(x->literals);
    tcmplxA_blockbuf_destroy(x->buffer);
    return res;
  } else {
    x->bits = 0u;
    x->bit_length = 0u;
    x->state = 0u;
    x->bit_index = 0u;
    x->backward = 0u;
    x->count = 0u;
    x->checksum = 0u;
    return tcmplxA_Success;
  }
}

void tcmplxA_zcvt_close(struct tcmplxA_zcvt* x) {
  tcmplxA_ringdist_destroy(x->ring);
  tcmplxA_inscopy_destroy(x->values);
  tcmplxA_fixlist_destroy(x->sequence);
  tcmplxA_fixlist_destroy(x->distances);
  tcmplxA_fixlist_destroy(x->literals);
  tcmplxA_blockbuf_destroy(x->buffer);
#ifndef NDEBUG
  x->checksum = 0u;
  x->count = 0u;
  x->backward = 0u;
  x->bit_index = 0u;
  x->state = 0u;
  x->bit_length = 0u;
  x->bits = 0u;
  x->sequence = NULL;
  x->distances = NULL;
  x->literals = NULL;
  x->buffer = NULL;
#endif /*NDEBUG*/
  return;
}

int tcmplxA_zcvt_zsrtostr_bits
  ( struct tcmplxA_zcvt* ps, unsigned int y,
    size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned int i;
  int ae = tcmplxA_Success;
  size_t ret_out = *ret;
  for (i = ps->bit_index; i < 8u && ae == tcmplxA_Success; ++i) {
    unsigned int x = (y>>i)&1u;
    switch (ps->state) {
    case 3:
      if (ps->count < 3u) {
        ps->bits |= (x<<ps->count);
        ps->count += 1u;
      }
      if (ps->count >= 3u) {
        unsigned int const end = ps->bits&1u;
        unsigned int const btype = (ps->bits>>1)&3u;
        if (btype == 3u) {
          ae = tcmplxA_ErrSanitize;
        } else {
          ps->h_end = end;
          if (btype == 0u) {
            ps->state = 4;
          } else if (btype == 1u) {
            ps->state = 8;
            /* fixed Huffman codes */{
              size_t i;
              for (i = 0u; i < 288u; ++i) {
                struct tcmplxA_fixline *const line =
                  tcmplxA_fixlist_at(ps->literals, i);
                line->value = (unsigned long)i;
                if (i < 144u)
                  line->len = 8u;
                else if (i < 256u)
                  line->len = 9u;
                else if (i < 280u)
                  line->len = 7u;
                else line->len = 8u;
              }
              for (i = 0u; i < 32u; ++i) {
                struct tcmplxA_fixline *const line =
                  tcmplxA_fixlist_at(ps->distances, i);
                line->code = (unsigned short)i;
                line->len = 5u;
                line->value = (unsigned long int)i;
              }
              tcmplxA_fixlist_gen_codes(ps->literals);
              tcmplxA_fixlist_codesort(ps->literals);
            }
          } else ps->state = 13;
          ps->count = 0u;
          ps->bits = 0u;
          ps->backward = 0u;
          ps->bit_length = 0u;
        }
      }
      break;
    case 4: /* no compression: LEN and NLEN */
    case 6: /* */
      break;
    case 19: /* generate code trees */
      {
        ae = tcmplxA_fixlist_gen_codes(ps->literals);
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_gen_codes(ps->distances);
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_codesort(ps->literals);
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_codesort(ps->distances);
        if (ae != tcmplxA_Success)
          break;
        ps->state = 8;
      } /*[[fallthrough]]*/;
    case 8: /* decode */
      if (ps->bit_length < 15u) {
        size_t j;
        ps->bits = (ps->bits<<1) | x;
        j = tcmplxA_fixlist_codebsearch
          (ps->literals, ps->bit_length+1u, ps->bits);
        if (j < ((size_t)-1)) {
          unsigned const alpha =
            (unsigned)tcmplxA_fixlist_at_c(ps->literals, j)->value;
          struct tcmplxA_inscopy_row const* row =
            tcmplxA_inscopy_at_c(ps->values, alpha);
          if (row->type == tcmplxA_InsCopy_Stop) {
            if (ps->h_end)
              ps->state = 6;
            else ps->state = 3;
            ps->count = 0u;
          } else if (row->type == tcmplxA_InsCopy_Literal) {
            if (ret_out < dstsz) {
              unsigned char const byt = (unsigned char)alpha;
              dst[ret_out] = byt;
              ps->checksum = tcmplxA_zutil_adler32(1u, &byt, ps->checksum);
              ret_out += 1u;
              /* */{
                size_t const z = tcmplxA_blockbuf_bypass(ps->buffer, &byt, 1u);
                if (z != 1u) {
                  ae = tcmplxA_ErrMemory;
                  break;
                }
              }
            } else {
              ps->state = 20;
              ps->bits = (unsigned char)alpha;
              ae = tcmplxA_ErrPartial;
              break;
            }
          } else if ((row->type&127) == tcmplxA_InsCopy_Copy) {
            if (row->copy_bits > 0u) {
              ps->state = 9;
              ps->extra_length = row->copy_bits;
            } else ps->state = 10;
            ps->count = row->copy_first;
          } else ae = tcmplxA_ErrSanitize;
          ps->bit_length = 0u;
          ps->backward = 0u;
          ps->bits = 0u;
          break;
        }
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 15u) {
        ae = tcmplxA_ErrSanitize;
      } break;
    case 20: /* alpha bringback */
      if (ret_out < dstsz) {
        unsigned char const byt = (unsigned char)(ps->bits);
        dst[ret_out] = byt;
        ps->checksum = tcmplxA_zutil_adler32(1u, &byt, ps->checksum);
        ret_out += 1u;
        /* */{
          size_t const z = tcmplxA_blockbuf_bypass(ps->buffer, &byt, 1u);
          if (z != 1u) {
            ae = tcmplxA_ErrMemory;
            break;
          }
        }
        ps->state = 8;
        ps->bits = 0u;
        ps->bit_length = 0u;
        ps->backward = 0u;
      } else {
        ae = tcmplxA_ErrPartial;
      } break;
    case 9: /* copy bits */
      if (ps->bit_length < ps->extra_length) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->count += ps->bits;
        ps->bits = 0u;
        ps->bit_length = 0u;
        ps->state = 10;
      } break;
    case 10: /* backward */
      if (ps->bit_length < 15u) {
        size_t j;
        ps->bits = (ps->bits<<1) | x;
        j = tcmplxA_fixlist_codebsearch
          (ps->distances, ps->bit_length+1u, ps->bits);
        if (j < ((size_t)-1)) {
          unsigned const alpha =
            (unsigned)tcmplxA_fixlist_at_c(ps->distances, j)->value;
          unsigned int const extra_length =
            tcmplxA_ringdist_bit_count(ps->ring, alpha);
          ps->index = 0u;
          if (extra_length > 0u) {
            ps->extra_length = extra_length;
            ps->state = 11;
            ps->backward = alpha;
          } else {
            ps->state = 12;
            ps->backward = tcmplxA_ringdist_decode(ps->ring, alpha, 0u);
            if (ps->backward == 0u) {
              ae = tcmplxA_ErrSanitize;
              break;
            } else ps->backward -= 1u;
            for (; ps->index < ps->count && ret_out < dstsz; ++ps->index) {
              unsigned char const byt =
                tcmplxA_blockbuf_peek(ps->buffer, ps->backward);
              dst[ret_out] = byt;
              ps->checksum = tcmplxA_zutil_adler32(1u, &byt, ps->checksum);
              ret_out += 1u;
              /* */{
                size_t const z = tcmplxA_blockbuf_bypass(ps->buffer, &byt, 1u);
                if (z != 1u) {
                  ae = tcmplxA_ErrMemory;
                  break;
                }
              }
            }
            if (ps->index >= ps->count) {
              ps->state = 8;
              ps->count = 0u;
            } else ae = tcmplxA_ErrPartial;
          }
          ps->bit_length = 0u;
          ps->bits = 0u;
          break;
        }
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 15u) {
        ae = tcmplxA_ErrSanitize;
      } break;
    case 11: /* distance bits */
      if (ps->bit_length < ps->extra_length) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->backward = tcmplxA_ringdist_decode
          (ps->ring, ps->backward, ps->bits);
        if (ps->backward == 0u) {
          ae = tcmplxA_ErrSanitize;
          break;
        } else ps->backward -= 1u;
        ps->bits = 0u;
        ps->bit_length = 0u;
        ps->state = 12;
      } else break;
      /* [[fallthrough]] */;
    case 12:
      for (; ps->index < ps->count && ret_out < dstsz; ++ps->index) {
        unsigned char const byt =
          tcmplxA_blockbuf_peek(ps->buffer, ps->backward);
        dst[ret_out] = byt;
        ps->checksum = tcmplxA_zutil_adler32(1u, &byt, ps->checksum);
        ret_out += 1u;
        /* */{
          size_t const z = tcmplxA_blockbuf_bypass(ps->buffer, &byt, 1u);
          if (z != 1u) {
            ae = tcmplxA_ErrMemory;
            break;
          }
        }
      }
      if (ps->index >= ps->count) {
        ps->state = 8;
        ps->bits = 0u;
        ps->bit_length = 0u;
        ps->count = 0u;
      } else ae = tcmplxA_ErrPartial;
      break;
    case 13: /* hcounts */
      if (ps->bit_length < 14u) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 14u) {
        unsigned int const hclen = ((ps->bits>>10)&15u) + 4u;
        ps->backward = (ps->bits&1023u);
        ps->bits = 0u;
        ps->bit_length = 0u;
        ps->count = hclen;
        ps->state = 14;
        ps->index = 0u;
        /* */{
          size_t j;
          for (j = 0u; j < 19u; ++j) {
            struct tcmplxA_fixline* line = tcmplxA_fixlist_at(ps->sequence, j);
            line->value = (unsigned long int)j;
            line->len = 0u;
          }
        }
      } break;
    case 14: /* code lengths code lengths */
      if (ps->bit_length < 3u) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 3u) {
        tcmplxA_fixlist_at(
            ps->sequence, tcmplxA_zcvt_clen[ps->index]
          )->len = (ps->bits&7u);
        ps->index += 1u;
        ps->bits = 0u;
        ps->bit_length = 0u;
      }
      if (ps->index >= ps->count) {
        ps->index = 0u;
        ps->state = 15;
        ps->count = (ps->backward&31u)+257u;
        ps->backward = ((ps->backward>>5)&31u) + 1u + ps->count;
        ps->extra_length = 0u;
        /* dynamic Huffman codes */{
          size_t j;
          for (j = 0u; j < 288u; ++j) {
            struct tcmplxA_fixline *const line =
              tcmplxA_fixlist_at(ps->literals, j);
            line->len = 0u;
            line->value = (unsigned long int)j;
          }
          for (j = 0u; j < 32u; ++j) {
            struct tcmplxA_fixline *const line =
              tcmplxA_fixlist_at(ps->distances, j);
            line->len = 0u;
            line->value = (unsigned long int)j;
          }
        }
        ae = tcmplxA_fixlist_gen_codes(ps->sequence);
        if (ae != tcmplxA_Success)
          break;
        else ae = tcmplxA_fixlist_codesort(ps->sequence);
      } break;
    case 15: /* literals and distances */
      if (ps->bit_length < 15u) {
        size_t j;
        ps->bits = (ps->bits<<1) | x;
        j = tcmplxA_fixlist_codebsearch
          (ps->sequence, ps->bit_length+1u, ps->bits);
        if (j < ((size_t)-1)) {
          unsigned const alpha =
            (unsigned)tcmplxA_fixlist_at_c(ps->sequence, j)->value;
          if (alpha <= 15u) {
            if (ps->index >= ps->count)
              tcmplxA_fixlist_at(ps->distances, ps->index-ps->count)->len
                = alpha;
            else tcmplxA_fixlist_at(ps->literals, ps->index)->len = alpha;
            ps->extra_length = alpha;
            ps->index += 1u;
          } else ps->state = alpha;
          ps->bit_length = 0u;
          ps->bits = 0u;
        } else ps->bit_length += 1u;
      }
      if (ps->bit_length >= 15u) {
        ae = tcmplxA_ErrSanitize;
      } else if (ps->index >= ps->backward) {
        ps->state = 19;
        ps->count = 0u;
        ps->index = 0u;
        ps->backward = 0u;
        ps->extra_length = 0u;
      } break;
    case 16: /* copy previous code length */
      if (ps->bit_length < 2u) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 2u) {
        unsigned int const n = ps->bits+3u;
        unsigned int j;
        for (j = 0u; j < n && ps->index < ps->backward; ++j, ++ps->index) {
          unsigned int const alpha = ps->extra_length;
          if (ps->index >= ps->count)
            tcmplxA_fixlist_at(ps->distances, ps->index-ps->count)->len
              = alpha;
          else tcmplxA_fixlist_at(ps->literals, ps->index)->len = alpha;
        }
        if (j < n)
          ae = tcmplxA_ErrSanitize;
        else if (ps->index >= ps->backward)
          ps->state = 19;
        else ps->state = 15;
        ps->bits = 0u;
        ps->bit_length = 0u;
      } break;
    case 17: /* copy zero length */
      if (ps->bit_length < 3u) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 3u) {
        unsigned int const n = ps->bits+3u;
        unsigned int j;
        for (j = 0u; j < n && ps->index < ps->backward; ++j, ++ps->index) {
          if (ps->index >= ps->count)
            tcmplxA_fixlist_at(ps->distances, ps->index-ps->count)->len = 0u;
          else tcmplxA_fixlist_at(ps->literals, ps->index)->len = 0u;
        }
        ps->extra_length = 0u;
        if (j < n)
          ae = tcmplxA_ErrSanitize;
        else if (ps->index >= ps->backward)
          ps->state = 19;
        else ps->state = 15;
        ps->bits = 0u;
        ps->bit_length = 0u;
      } break;
    case 18: /* copy zero length + 11 */
      if (ps->bit_length < 7u) {
        ps->bits = (ps->bits | (x<<ps->bit_length));
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 7u) {
        unsigned int const n = ps->bits+11u;
        unsigned int j;
        for (j = 0u; j < n && ps->index < ps->backward; ++j, ++ps->index) {
          if (ps->index >= ps->count)
            tcmplxA_fixlist_at(ps->distances, ps->index-ps->count)->len = 0u;
          else tcmplxA_fixlist_at(ps->literals, ps->index)->len = 0u;
        }
        ps->extra_length = 0u;
        if (j < n)
          ae = tcmplxA_ErrSanitize;
        else if (ps->index >= ps->backward)
          ps->state = 19;
        else ps->state = 15;
        ps->bits = 0u;
        ps->bit_length = 0u;
      } break;
    }
    if (ae > tcmplxA_Success)
      /* halt the read position here: */break;
  }
  ps->bit_index = i&7u;
  *ret = ret_out;
  return ae;
}
/* END   zcvt state / static */

/* BEGIN zcvt state / public */
struct tcmplxA_zcvt* tcmplxA_zcvt_new
  ( tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length)
{
  struct tcmplxA_zcvt* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_zcvt));
  if (out != NULL
  &&  tcmplxA_zcvt_init(out, block_size, n, chain_length) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_zcvt_destroy(struct tcmplxA_zcvt* x) {
  if (x != NULL) {
    tcmplxA_zcvt_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

int tcmplxA_zcvt_zsrtostr
  ( struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end)
{
  int ae = tcmplxA_Success;
  unsigned char const* p;
  size_t ret_out = 0u;
  for (p = *src; p < src_end && ae == tcmplxA_Success; ++p) {
    switch (ps->state) {
    case 0: /* initial state */
      if (ps->count < 2u) {
        ps->backward = (ps->backward<<8) | (*p);
        ps->count += 1u;
      }
      if (ps->count >= 2u) {
        unsigned int const cm = (ps->backward>>8)&15u;
        unsigned int const cinfo = (ps->backward>>12)&15u;
        if (ps->backward % 31u != 0u) {
          ae = tcmplxA_ErrSanitize;
        } else if (cm != 8u || cinfo > 7u) {
          ae = tcmplxA_ErrSanitize;
        } else if (ps->backward & 32u) {
          ae = tcmplxA_Success;
          ps->state = 1;
          ps->count = 0;
          ps->backward = ps->checksum;
          ps->checksum = 1u;
        } else {
          ps->state = 3;
          ps->count = 0u;
          ps->bits = 0u;
          ps->checksum = 1u;
        }
      } break;
    case 1: /* dictionary checksum */
      if (ps->count < 4u) {
        ps->checksum = (ps->checksum<<8) | (*p);
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        ps->state = 2;
        ae = tcmplxA_ErrZDictionary;
      } break;
    case 2: /* check dictionary checksum */
    case 3: /* block start */
      if (ps->state == 2) {
        if (ps->backward != ps->checksum) {
          ae = tcmplxA_ErrSanitize;
          break;
        } else {
          ps->state = 3;
          ps->count = 0u;
          ps->bits = 0u;
          ps->checksum = 1u;
        }
      }
      ae = tcmplxA_zcvt_zsrtostr_bits(ps, (*p), &ret_out, dst, dstsz);
      break;
    case 4: /* no compression: LEN and NLEN */
      if (ps->count < 4u) {
        ps->backward = (ps->backward<<8) | (*p);
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        unsigned int const len = (ps->backward>>16)&65535u;
        unsigned int const nlen = (~ps->backward)&65535u;
        if (len != nlen) {
          ae = tcmplxA_ErrSanitize;
        } else {
          ps->backward = len;
          ps->state = 5;
          ps->count = 0u;
        }
      } break;
    case 5: /* no compression: copy bytes */
      if (ps->count < ps->backward) {
        if (ret_out < dstsz) {
          dst[ret_out] = (*p);
          ps->checksum = tcmplxA_zutil_adler32(1u, p, ps->checksum);
          ret_out += 1u;
          /* */{
            size_t const z = tcmplxA_blockbuf_bypass(ps->buffer, p, 1u);
            if (z != 1u)
              ae = tcmplxA_ErrMemory;
          }
        } else {
          ae = tcmplxA_ErrPartial;
          break;
        }
        ps->count += 1u;
      }
      if (ps->count >= ps->backward) {
        if (ps->h_end)
          ps->state = 6;
        else ps->state = 3;
        ps->count = 0u;
        ps->backward = 0u;
      } break;
    case 6: /* end-of-stream checksum */
      if (ps->count < 4u) {
        ps->backward = (ps->backward<<8) | (*p);
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        if (ps->checksum != ps->backward) {
          ae = tcmplxA_ErrSanitize;
        } else ps->state = 7;
      } break;
    case 7:
      ae = tcmplxA_EOF;
      break;
    case 8: /* decode */
    case 9: /* copy bits */
    case 10: /* backward */
    case 11: /* distance bits */
    case 12: /* output from backward */
    case 13: /* hcounts */
    case 14: /* code lengths code lengths */
    case 15: /* literals and distances */
    case 16: /* copy previous code length */
    case 17: /* copy zero length */
    case 18: /* copy zero length + 11 */
    case 19: /* generate code trees */
    case 20: /* alpha bringback */
      ae = tcmplxA_zcvt_zsrtostr_bits(ps, (*p), &ret_out, dst, dstsz);
      break;
    }
    if (ae > tcmplxA_Success)
      /* halt the read position here: */break;
  }
  *src = p;
  *ret = ret_out;
  return ae;
}

tcmplxA_uint32 tcmplxA_zcvt_checksum(struct tcmplxA_zcvt const* x) {
  return x->checksum;
}

size_t tcmplxA_zcvt_bypass
  (struct tcmplxA_zcvt* x, unsigned char const* buf, size_t sz)
{
  if (x->state >= 2u) {
    return 0u;
  } else {
    size_t const n = tcmplxA_blockbuf_bypass(x->buffer, buf, sz);
    if (x->state == 0u) {
      /* strrtozs */
      x->checksum = tcmplxA_zutil_adler32(sz, buf, x->checksum);
    } else {
      /* zsrtostr */
      x->backward = tcmplxA_zutil_adler32(sz, buf, x->backward);
    }
    return n;
  }
}
/* END   zcvt state / public */
