/**
 * @file src/zcvt.c
 * @brief zlib conversion state
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "blockbuf_p.h"
#include "text-complex/access/zcvt.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include "text-complex/access/blockbuf.h"
#include "text-complex/access/fixlist.h"
#include "text-complex/access/inscopy.h"
#include "text-complex/access/ringdist.h"
#include "text-complex/access/zutil.h"
#include <string.h>
#include <limits.h>



enum tcmplxA_zcvt_uconst {
  tcmplxA_ZCvt_LitHistoSize = 288u,
  tcmplxA_ZCvt_DistHistoSize = 32u,
  tcmplxA_ZCvt_SeqHistoSize = 19u,
  tcmplxA_ZCvt_HistogramSize =
      tcmplxA_ZCvt_LitHistoSize
    + tcmplxA_ZCvt_DistHistoSize
    + tcmplxA_ZCvt_SeqHistoSize,
  tcmplxA_ZCvt_LitDynamicConst = 286u,
  tcmplxA_ZCvt_DistDynamicConst = 30u,
};

struct tcmplxA_zcvt {
  /**
   * @brief ...
   * @note Using a buffer with a slide ring extent greater than 32768
   *   can cause the output sanity check to fail.
   */
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
  struct tcmplxA_ringdist* try_ring;
  /** @brief Check for large blocks. */
  tcmplxA_uint32* histogram;
  /** @brief Tree description sequence. */
  struct tcmplxA_blockstr sequence_list;
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
  /** @brief Output internal bit count. */
  tcmplxA_uint32 bit_cap;
  /** @brief Partial byte stored aside for later. */
  unsigned char write_scratch;
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
/**
 * @brief Calculate a compression info value.
 * @param window_size window size
 * @return an info value
 */
static tcmplxA_uint32 tcmplxA_zcvt_cinfo(tcmplxA_uint32 window_size);
/**
 * @brief Check whether the user set a dictionary.
 * @param ps conversion state
 * @return nonzero if set, zero otherwise
 */
static int tcmplxA_zcvt_out_has_dict(struct tcmplxA_zcvt const* ps);
/**
 * @brief Choose a nonzero value.
 * @param a one choice
 * @param b another choice
 * @return either `a` or `b`, whichever is nonzero
 */
static int tcmplxA_zcvt_nonzero(int a, int b);
/**
 * @brief Choose a nonzero value.
 * @param a one choice
 * @param b another choice
 * @param c another choice
 * @return either `a` or `b` or `c`, whichever is nonzero
 */
static int tcmplxA_zcvt_nonzero3(int a, int b, int c);
/**
 * @brief Make a code length sequence.
 * @param x the zcvt state
 * @return tcmplxA_Success on success, nonzero otherwise
 */
static int tcmplxA_zcvt_make_sequence(struct tcmplxA_zcvt* x);
/**
 * @brief Add a code length sequence.
 * @param s sequence list
 * @param len code length
 * @param count repeat count
 * @return tcmplxA_Success on success, nonzero otherwise
 */
static int tcmplxA_zcvt_post_sequence
  (struct tcmplxA_blockstr* s, unsigned int len, unsigned int count);
/**
 * @brief Bit iteration.
 * @param x the zcvt state
 * @param[out] y bits to iterate
 * @param[in,out] src pointer to source bytes to deflate
 * @param src_end pointer to end of source buffer
 * @return tcmplxA_Success on success, nonzero otherwise
 * @note The conversion state referred to by `ps` is updated based
 *   on the conversion result, whether succesful or failed.
 * @return tcmplxA_Success on success, tcmplxA_ErrPartial on incomplete
 *   bit sequence
 */
static int tcmplxA_zcvt_strrtozs_bits
  ( struct tcmplxA_zcvt* x, unsigned char* y,
    unsigned char const** src, unsigned char const* src_end);
/**
 * @brief Move from an output noconvert block to next state.
 * @param ps the zcvt state to update
 */
static void tcmplxA_zcvt_noconv_next(struct tcmplxA_zcvt* ps);

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
  /* try_ring */{
    x->try_ring = tcmplxA_ringdist_new(0,4u,0u);
    if (x->try_ring == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* sequence_list */{
    int const ae = tcmplxA_blockstr_init(&x->sequence_list, 286u+30u);
    if (ae != tcmplxA_Success)
      res = ae;
  }
  /* histogram */{
    if (tcmplxA_ZCvt_HistogramSize < ((size_t)-1)/sizeof(tcmplxA_uint32)) {
      x->histogram = tcmplxA_util_malloc
        (tcmplxA_ZCvt_HistogramSize*sizeof(tcmplxA_uint32));
    } else x->histogram = NULL;
    if (x->histogram == NULL)
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
    tcmplxA_blockstr_close(&x->sequence_list);
    tcmplxA_inscopy_destroy(x->values);
    tcmplxA_util_free(x->histogram);
    tcmplxA_ringdist_destroy(x->try_ring);
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
    x->bit_cap = 0u;
    x->write_scratch = 0u;
    return tcmplxA_Success;
  }
}

void tcmplxA_zcvt_close(struct tcmplxA_zcvt* x) {
  tcmplxA_blockstr_close(&x->sequence_list);
  tcmplxA_util_free(x->histogram);
  tcmplxA_ringdist_destroy(x->try_ring);
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
  x->write_scratch = 0u;
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
            ps->backward = tcmplxA_ringdist_decode(ps->ring, alpha, 0u, 0u);
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
          (ps->ring, ps->backward, ps->bits, 0u);
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

unsigned int tcmplxA_zcvt_cinfo(tcmplxA_uint32 window_size) {
  unsigned int out = 0u;
  tcmplxA_uint32 v;
  if (window_size > 32768u)
    return 8u;
  else for (v = 1u; v < window_size; v<<=1, ++out) {
    continue;
  }
  return out > 8u ? out-8u : 0u;
}

int tcmplxA_zcvt_out_has_dict(struct tcmplxA_zcvt const* ps) {
  return tcmplxA_blockbuf_ring_size(ps->buffer) > 0u;
}

int tcmplxA_zcvt_nonzero(int a, int b) {
  return a != tcmplxA_Success ? a : b;
}

int tcmplxA_zcvt_nonzero3(int a, int b, int c) {
  return tcmplxA_zcvt_nonzero(a,tcmplxA_zcvt_nonzero(b,c));
}

int tcmplxA_zcvt_make_sequence(struct tcmplxA_zcvt* x) {
  unsigned int len = UINT_MAX, len_count = 0u;
  size_t i;
  size_t const lit_sz = tcmplxA_fixlist_size(x->literals);
  size_t const dist_sz = tcmplxA_fixlist_size(x->distances);
  x->sequence_list.sz = 0u;
  for (i = 0u; i < tcmplxA_ZCvt_LitDynamicConst; ++i) {
    unsigned int const n = (i >= lit_sz) ? 0
      : tcmplxA_fixlist_at_c(x->literals,i)->len;
    if (len != n) {
      int const ae = tcmplxA_zcvt_post_sequence
        (&x->sequence_list, len, len_count);
      if (ae != tcmplxA_Success)
        return ae;
      len = n;
      len_count = 1u;
    } else len_count += 1u;
  }
  for (i = 0u; i < tcmplxA_ZCvt_DistDynamicConst; ++i) {
    unsigned int const n = (i >= dist_sz) ? 0
      : tcmplxA_fixlist_at_c(x->distances,i)->len;
    if (len != n) {
      int const ae = tcmplxA_zcvt_post_sequence
        (&x->sequence_list, len, len_count);
      if (ae != tcmplxA_Success)
        return ae;
      len = n;
      len_count = 1u;
    } else len_count += 1u;
  }
  return tcmplxA_zcvt_post_sequence(&x->sequence_list, len, len_count);
}

int tcmplxA_zcvt_post_sequence
  (struct tcmplxA_blockstr* s, unsigned int len, unsigned int count)
{
  if (count == 0u)
    return tcmplxA_Success;
  else if ((len==0u&&count<3u) || count < 4u) {
    unsigned char buf[3];
    memset(buf, len, count);
    return tcmplxA_blockstr_append(s, buf, count);
  } else if (len == 0u) {
    unsigned int i;
    int ae = tcmplxA_Success;
    for (i = 0u; i < count && ae == tcmplxA_Success; ) {
      unsigned int const x = count-i;
      if (x > 138u) {
        unsigned char buf[2] = {18u, 127u};
        ae = tcmplxA_blockstr_append(s, buf, 2u);
        i += 138u;
      } else if (x >= 11u) {
        unsigned char buf[2] = {18u};
        buf[1] = x-11u;
        ae = tcmplxA_blockstr_append(s, buf, 2u);
        i += x;
      } else if (x >= 3u) {
        unsigned char buf[2] = {17u};
        buf[1] = x-3u;
        ae = tcmplxA_blockstr_append(s, buf, 2u);
        i += x;
      } else {
        unsigned char buf[1] = {0u};
        ae = tcmplxA_blockstr_append(s, buf, 1u);
        ++i;
      }
    }
    return ae;
  } else {
    unsigned int i;
    int ae;
    /* */{
      unsigned char buf[1];
      buf[0] = (unsigned char)len;
      ae = tcmplxA_blockstr_append(s, buf, 1u);
    }
    for (i = 1u; i < count && ae == tcmplxA_Success; ) {
      unsigned int const x = count-i;
      if (x > 6u) {
        unsigned char buf[2] = {16u, 3u};
        ae = tcmplxA_blockstr_append(s, buf, 2u);
        i += 6u;
      } else if (x >= 3u) {
        unsigned char buf[2] = {16u};
        buf[1] = x-3u;
        ae = tcmplxA_blockstr_append(s, buf, 2u);
        i += x;
      } else {
        unsigned char const buf = (unsigned char)len;
        ae = tcmplxA_blockstr_append(s, &buf, 1u);
        ++i;
      }
    }
    return ae;
  }
}

int tcmplxA_zcvt_strrtozs_bits
  ( struct tcmplxA_zcvt* ps, unsigned char* y,
    unsigned char const** src, unsigned char const* src_end)
{
  unsigned char const* p = *src;
  unsigned int i;
  int ae = tcmplxA_Success;
  /* restore in-progress byte */
  *y = ps->write_scratch;
  ps->write_scratch = 0u;
  for (i = ps->bit_index; i < 8u && ae == tcmplxA_Success; ++i) {
    unsigned int x = 0u;
    if ((!(ps->h_end&1u))/* if end marker not activated yet */
    &&  (ps->state == 3 && ps->count == 0u)/* and not inside a block */)
    {
      tcmplxA_uint32 const input_space =
          tcmplxA_blockbuf_capacity(ps->buffer)
        - tcmplxA_blockbuf_input_size(ps->buffer);
      size_t const src_count = src_end - p;
      size_t const min_count = (input_space < src_count)
          ? (size_t)input_space : src_count;
      ae = tcmplxA_blockbuf_write(ps->buffer, p, min_count);
      if (ae != tcmplxA_Success)
        break;
      ps->checksum = tcmplxA_zutil_adler32(min_count, p, ps->checksum);
      p += min_count;
    }
    switch (ps->state) {
    case 3: /* block start */
      /* flip the switch */if (ps->h_end & 2u) {
        ps->h_end |= 1u;
      }
      /* try compress the data */if (ps->count == 0u) {
        int dynamic_flag = 0;
        if (tcmplxA_blockbuf_input_size(ps->buffer) == 0u && (!ps->h_end)) {
          /* stash the current byte to the side */
          ae = tcmplxA_ErrPartial;
          break;
        } else {
          tcmplxA_blockbuf_clear_output(ps->buffer);
          ae = tcmplxA_blockbuf_try_block(ps->buffer);
        }
        if (ae == tcmplxA_Success) {
          /* histogram */
          unsigned char const* const buffer_str =
            tcmplxA_blockbuf_output_data(ps->buffer);
          tcmplxA_uint32 const buffer_size =
            tcmplxA_blockbuf_output_size(ps->buffer);
          tcmplxA_uint32 const buffer_m1 = buffer_size-1u;
          unsigned long int bit_count = 0u;
          tcmplxA_uint32 *const lit_histogram = ps->histogram;
          tcmplxA_uint32 *const dist_histogram =
            lit_histogram+tcmplxA_ZCvt_LitHistoSize;
          tcmplxA_uint32 *const seq_histogram =
            dist_histogram+tcmplxA_ZCvt_DistHistoSize;
          /* prepare */{
            tcmplxA_ringdist_copy(ps->try_ring, ps->ring);
            memset(lit_histogram, 0u,
                tcmplxA_ZCvt_LitHistoSize*sizeof(tcmplxA_uint32));
            memset(dist_histogram, 0u,
                tcmplxA_ZCvt_DistHistoSize*sizeof(tcmplxA_uint32));
            memset(seq_histogram, 0u,
                tcmplxA_ZCvt_SeqHistoSize*sizeof(tcmplxA_uint32));
            ae = tcmplxA_inscopy_lengthsort(ps->values);
            if (ae != tcmplxA_Success)
              break;
          }
          /* calculate histogram */{
            tcmplxA_uint32 buffer_pos;
            for (buffer_pos = 0u; buffer_pos < buffer_size; ++buffer_pos) {
              unsigned char const byt = buffer_str[buffer_pos];
              int const insert_flag = ((byt&128u) == 0u);
              unsigned short len;
              unsigned short j;
              if (byt&64u) {
                if (buffer_pos == buffer_m1) {
                  ae = tcmplxA_ErrSanitize;
                  break;
                } else {
                  len = (((byt&63u)<<8)|(buffer_str[buffer_pos+1u]&255u))+64u;
                  buffer_pos += 1u;
                }
              } else len = byt&63u;
              if (len == 0u)
                continue;
              else if (!insert_flag) {
                size_t const lit_index =
                  tcmplxA_inscopy_encode(ps->values, 0u, len, 0);
                tcmplxA_uint32 distance = 0u;
                if (lit_index >= tcmplxA_inscopy_size(ps->values)) {
                  ae = tcmplxA_ErrInsCopyMissing;
                  break;
                } else {
                  struct tcmplxA_inscopy_row const* const lit =
                    tcmplxA_inscopy_at_c(ps->values, lit_index);
                  lit_histogram[lit->code] += 1u;
                  bit_count += lit->copy_bits;
                }
                /* distance */switch (buffer_str[buffer_pos+1u] & 192u) {
                case 128u:
                  if (buffer_pos+2u < buffer_size) {
                    distance = ((buffer_str[buffer_pos+1u]&63u)<<8) | buffer_str[buffer_pos+2u];
                    buffer_pos += 2u;
                  } else ae = tcmplxA_ErrBlockOverflow;
                  break;
                case 192u:
                  if (buffer_pos+4u < buffer_size) {
                    distance = ((((tcmplxA_uint32)(buffer_str[buffer_pos+1u]&63u))<<24)
                      | (((tcmplxA_uint32)buffer_str[buffer_pos+2u]) << 16)
                      | (buffer_str[buffer_pos+3u]<<8) | (buffer_str[buffer_pos+4u])) + 16384u;
                    buffer_pos += 4u;
                  } else ae = tcmplxA_ErrBlockOverflow;
                  break;
                default:
                  /* Brotli dictionary not supported in ZLIB stream */
                  ae = tcmplxA_ErrSanitize;
                }
                /* encode distance */if (ae == tcmplxA_Success) {
                  tcmplxA_uint32 extra;
                  unsigned int const dist_code =
                    tcmplxA_ringdist_encode(ps->try_ring, distance, &extra, 0u);
                  if (dist_code == UINT_MAX) {
                    ae = tcmplxA_ErrParam;
                    break;
                  } else {
                    bit_count += extra;
                    dist_histogram[dist_code] += 1u;
                  }
                } else break;
              } else for (j = 0u; j < len && buffer_pos < buffer_m1; ++j, ++buffer_pos) {
                lit_histogram[buffer_str[buffer_pos+1u]] += 1u;
              }
            }
            /* Ensure the stop code. */
            lit_histogram[256] = 1;
            if (ae != tcmplxA_Success)
              break;
            else/* plant two trees */{
              /* dynamic Huffman codes */{
                unsigned int j;
                for (j = 0u; j < 288u; ++j) {
                  tcmplxA_fixlist_at(ps->literals, j)->value = j;
                }
                for (j = 0u; j < 32u; ++j) {
                  tcmplxA_fixlist_at(ps->distances, j)->value = j;
                }
                for (j = 0u; j < 19u; ++j) {
                  tcmplxA_fixlist_at(ps->sequence, j)->value = j;
                }
              }
              /* lengths */{
                int const lit_ae = tcmplxA_fixlist_gen_lengths
                  (ps->literals, lit_histogram, 15u);
                int const dist_ae = tcmplxA_fixlist_gen_lengths
                  (ps->distances, dist_histogram, 15u);
                ae = tcmplxA_zcvt_nonzero(lit_ae, dist_ae);
                if (ae != tcmplxA_Success)
                  break;
                ae = tcmplxA_zcvt_nonzero(tcmplxA_fixlist_valuesort(ps->literals),
                  tcmplxA_fixlist_valuesort(ps->distances));
                if (ae != tcmplxA_Success)
                  break;
              }
              /* guess a sequence */{
                ae = tcmplxA_zcvt_make_sequence(ps);
                if (ae != tcmplxA_Success)
                  break;
              }
              /* the sequence histogram */{
                tcmplxA_uint32 j;
                unsigned char const* const sequence_data =
                  ps->sequence_list.p;
                for (j = 0u; j < ps->sequence_list.sz; ++j) {
                  unsigned int const v = sequence_data[j];
                  if (v >= 16u) {
                    unsigned char const v2[3] = {2u,3u,7u};
                    j += 1u;
                    bit_count += v2[v-16u];
                  }
                  seq_histogram[v] += 1u;
                }
              }
              /* length lengths */{
                ae = tcmplxA_fixlist_gen_lengths
                  (ps->sequence, seq_histogram, 7u);
                if (ae != tcmplxA_Success)
                  break;
              }
            }
            /* estimate the block length */{
              unsigned int j;
              for (j = 0u; j < 288u; ++j) {
                bit_count += lit_histogram[j]
                  * tcmplxA_fixlist_at_c(ps->literals,j)->len;
              }
              for (j = 0u; j < 32u; ++j) {
                bit_count += dist_histogram[j]
                  * tcmplxA_fixlist_at_c(ps->distances,j)->len;
              }
              for (j = 0u; j < 19u; ++j) {
                bit_count += seq_histogram[j]
                  * tcmplxA_fixlist_at_c(ps->sequence,j)->len;
              }
            }
          }
          /* compare to text length */{
            unsigned long int const byte_count = ((bit_count+7u)>>3);
            if (byte_count < tcmplxA_blockbuf_input_size(ps->buffer)) {
              ae = tcmplxA_fixlist_gen_codes(ps->sequence);
              dynamic_flag = 1;
            } else {
              dynamic_flag = 0;
              tcmplxA_blockbuf_clear_output(ps->buffer);
              ae = tcmplxA_blockbuf_noconv_block(ps->buffer);
            }
          }
          if (ae == tcmplxA_Success) {
            ps->bits = (ps->h_end&1u) | (dynamic_flag?4u:0u);
            tcmplxA_blockbuf_clear_input(ps->buffer);
          }
        } else break;
      }
      if (ps->count < 3u) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1u;
      }
      if (ps->count == 3u) {
        if (ps->bits&6u) {
          ps->state = 13u;
          ps->count = 0u;
          ps->bit_length = 0u;
          ps->bits = 0u;
        } else {
          ps->state = 4u;
          ps->backward = tcmplxA_blockbuf_output_size(ps->buffer);
          ps->index = 0u;
          ps->count = 0u;
          ps->bit_index = 0u;
        }
      } break;
    case 4: /* no compression: LEN and NLEN */
    case 6: /* end-of-stream checksum */
      x = 0u;
      break;
    case 19: /* generate code trees */
      /* */{
        int const lit_ae = tcmplxA_fixlist_gen_codes(ps->literals);
        int const dist_ae = tcmplxA_fixlist_gen_codes(ps->distances);
        ae = tcmplxA_zcvt_nonzero(lit_ae, dist_ae);
        if (ae != tcmplxA_Success)
          break;
      }
      /* */{
        int const lit_ae = tcmplxA_fixlist_valuesort(ps->literals);
        int const dist_ae = tcmplxA_fixlist_valuesort(ps->distances);
        ae = tcmplxA_zcvt_nonzero(lit_ae, dist_ae);
        if (ae != tcmplxA_Success)
          break;
        ps->state = 8u;
        ps->backward = tcmplxA_blockbuf_output_size(ps->buffer);
        ps->index = 0u;
        ps->bit_length = 0u;
      } /* [[fallthrough]] */;
    case 8: /* encode */
      if (ps->bit_length == 0u) {
        if (ps->index >= ps->backward) {
          struct tcmplxA_fixline const* const line =
            tcmplxA_fixlist_at_c(ps->literals, 256u);
          ps->bit_cap = line->len;
          ps->bits = line->code;
        } else {
          unsigned char const* const data =
            tcmplxA_blockbuf_output_data(ps->buffer);
          unsigned char buf[2u] = {0u};
          unsigned int len;
          buf[0u] = data[ps->index];
          if ((buf[0u]&64u) && (ps->index+1u < ps->backward)) {
            ps->index += 1u;
            buf[1u] = data[ps->index];
            len = buf[1u] + ((buf[0u]&63u)<<8) + 64u;
          } else len = buf[0u]&63u;
          if ((buf[0u]&128u)==0u) {
            /* insert */
            if (ps->index+1u >= ps->backward || len == 0u) {
              ae = tcmplxA_ErrSanitize;
              break;
            } else {
              unsigned char const alpha = data[ps->index+1u];
              struct tcmplxA_fixline const* const line =
                tcmplxA_fixlist_at_c(ps->literals, alpha);
              ps->state = 20u;
              ps->index += 1u;
              ps->count = len;
              ps->bit_cap = line->len;
              ps->bits = line->code;
            }
          } else {
            /* copy */
            size_t const copy_index = tcmplxA_inscopy_encode
                (ps->values, 0u, len, 0);
            if (copy_index == ((size_t)-1)) {
              ae = tcmplxA_ErrSanitize;
              break;
            } else {
              struct tcmplxA_inscopy_row const* const irow =
                tcmplxA_inscopy_at_c(ps->values, copy_index);
              unsigned short const alpha = irow->code;
              struct tcmplxA_fixline const* const line =
                tcmplxA_fixlist_at_c(ps->literals, alpha);
              ps->bit_cap = line->len;
              ps->bits = line->code;
              ps->count = len - irow->copy_first;
              ps->extra_length = irow->copy_bits;
            }
          }
        }
      }
      if (ps->bit_length < ps->bit_cap) {
        x = (ps->bits>>(ps->bit_cap-1u-ps->bit_length))&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->bit_cap) {
        if (ps->index >= ps->backward) {
          if (ps->h_end & 1u) {
            ps->state = 6u;
          } else {
            ps->state = 3u;
            ps->bits = 0u;
          }
          ps->count = 0u;
        } else if (ps->state == 20u) {
          ps->count -= 1u;
          ps->index += 1u;
          if (ps->count == 0u) {
            ps->state = 8u;
          }
          ps->bit_length = 0u;
        } else {
          ps->bit_length = 0u;
          ps->state = (ps->extra_length>0u ? 9u : 10u);
          ps->index += 1u;
        }
      } break;
    case 20: /* alpha bringback */
      if (ps->bit_length == 0u) {
        if (ps->index < ps->backward) {
          unsigned char const* const data =
            tcmplxA_blockbuf_output_data(ps->buffer);
          unsigned char const alpha = data[ps->index];
          struct tcmplxA_fixline const* const line =
            tcmplxA_fixlist_at_c(ps->literals, alpha);
          ps->bit_cap = line->len;
          ps->bits = line->code;
        } else {
          ae = tcmplxA_ErrSanitize;
          break;
        }
      }
      if (ps->bit_length < ps->bit_cap) {
        x = (ps->bits>>(ps->bit_cap-1u-ps->bit_length))&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->bit_cap) {
        ps->index += 1u;
        ps->count -= 1u;
        if (ps->count == 0u)
          ps->state = 8u;
        ps->bit_length = 0u;
      } break;
    case 9: /* copy bits */
      if (ps->bit_length < ps->extra_length) {
        x = (ps->count>>ps->bit_length)&1u;
        ps->bit_length += 1;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->state = 10u;
        ps->bit_length = 0u;
      }
      break;
    case 10: /* distance Huffman code */
      if (ps->bit_length == 0u) {
        unsigned char buf[4] = {0};
        unsigned long distance = 0;
        unsigned char const* const data =
          tcmplxA_blockbuf_output_data(ps->buffer);
        assert(ps->index < ps->backward);
        buf[0u] = data[ps->index];
        ps->index += 1;
        if ((buf[0]&128u) == 0u) {
          /* zlib stream does not support Brotli references */
          ae = tcmplxA_ErrSanitize;
          break;
        } else if (ps->index >= ps->backward) {
          /* also, index needs to be in range */
          ae = tcmplxA_ErrOutOfRange;
          break;
        }
        if (buf[0u] & 64u) {
          if (ps->backward - ps->index < 3u) {
            ae = tcmplxA_ErrOutOfRange;
            break;
          }
          memcpy(buf+1, data+ps->index, 3);
          ps->index += 2;
          distance = ((buf[0]&63ul)<<24)
            + ((buf[1]&255ul)<<16) + ((buf[2]&255u)<<8)
            + (buf[3]&255u) + 16384ul;
        } else {
          buf[1] = data[ps->index];
          distance = ((buf[0]&63)<<8) + (buf[1]&255);
        }
        if (distance > 32768) {
          /* zlib lacks support for large distances */
          ae = tcmplxA_ErrSanitize;
          break;
        } else {
          tcmplxA_uint32 dist_extra = 0;
          unsigned const dist_index = tcmplxA_ringdist_encode
              (ps->ring, (unsigned)distance, &dist_extra, 0u);
          if (dist_index == ((unsigned)-1)) {
            ae = tcmplxA_ErrSanitize;
            break;
          } else {
            struct tcmplxA_fixline const* const line =
              tcmplxA_fixlist_at_c(ps->distances, dist_index);
            ps->bit_cap = line->len;
            ps->bits = line->code;
            ps->count = dist_extra;
            ps->extra_length =
              tcmplxA_ringdist_bit_count(ps->ring, dist_index);
          }
        }
      }
      if (ps->bit_length < ps->bit_cap) {
        x = (ps->bits>>(ps->bit_cap-1u-ps->bit_length))&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->bit_cap) {
        ps->bit_length = 0u;
        ps->state = (ps->extra_length>0u ? 11u : 8u);
        ps->index += 1u;
      } break;
    case 11: /* distance extra bits */
      if (ps->bit_length < ps->extra_length) {
        x = (ps->count>>ps->bit_length)&1u;
        ps->bit_length += 1;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->state = 8u;
        ps->bit_length = 0u;
      }
      break;
    case 13: /* hcounts */
      if (ps->bit_length == 0u) {
        ps->count = 19u;
        ps->bits = (((ps->count-4u) << 10)
          | ((tcmplxA_ZCvt_DistDynamicConst-1) << 5)
          | (tcmplxA_ZCvt_LitDynamicConst-257));
      }
      if (ps->bit_length < 14u) {
        x = (ps->bits>>ps->bit_length)&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 14u) {
        ps->backward = (ps->bits&1023u);
        ps->state = 14u;
        ps->index = 0u;
        ps->bit_length = 0u;
        ps->bits = 0u;
        ae = tcmplxA_fixlist_valuesort(ps->sequence);
      } break;
    case 14: /* code lengths code lengths */
      if (ps->bit_length == 0u) {
        ps->bits = tcmplxA_fixlist_at_c
          (ps->sequence, tcmplxA_zcvt_clen[ps->index])->len;
      }
      if (ps->bit_length < 3u) {
        x = (ps->bits>>ps->bit_length)&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= 3u) {
        ps->index += 1u;
        ps->bit_length = 0u;
      }
      if (ps->index >= ps->count) {
        ps->state = 15u;
        ps->count = (ps->backward&31u)+257u;
        ps->backward = ((ps->backward>>5)&31u) + 1u + ps->count;
        ps->extra_length = 0u;
        ps->index = 0u;
        ps->bits = 0u;
        ps->bit_length = 0u;
      } break;
    case 15: /* literals and distances */
      if (ps->bit_length == 0u) {
        unsigned int const alpha = ps->sequence_list.p[ps->index];
        struct tcmplxA_fixline const* line =
          tcmplxA_fixlist_at_c(ps->sequence, alpha);
        ps->bits = line->code;
        ps->extra_length = line->len;
      }
      if (ps->bit_length < ps->extra_length) {
        x = (ps->bits>>(ps->extra_length-1u-ps->bit_length))&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->extra_length) {
        unsigned int const alpha = ps->sequence_list.p[ps->index];
        ps->bit_length = 0u;
        ps->index += 1u;
        if (alpha >= 16u && alpha <= 18u) {
          ps->state = (unsigned char)alpha;
        } else if (ps->index >= ps->sequence_list.sz) {
          ps->state = 19u;
        } else ps->state = 15u;
      } break;
    case 16: /* copy previous code length */
    case 17: /* copy zero length */
    case 18: /* copy zero length + 11 */
      if (ps->bit_length == 0u) {
        unsigned char const lens[3] = {2u,3u,7u};
        ps->extra_length = lens[ps->state-16u];
        ps->bits = ps->sequence_list.p[ps->index];
      }
      if (ps->bit_length < ps->extra_length) {
        x = (ps->bits>>ps->bit_length)&1u;
        ps->bit_length += 1u;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->bit_length = 0u;
        ps->index += 1u;
        if (ps->index >= ps->sequence_list.sz) {
          ps->state = 19u;
        } else ps->state = 15u;
      } break;
    default:
      ae = tcmplxA_ErrUnknown;
      break;
    }
    if (ae > tcmplxA_Success) {
      if (ae == tcmplxA_ErrPartial)
        ps->write_scratch = *y; /* save current byte */
      /* halt the read position here: */break;
    }
    else *y |= (x<<i);
  }
  ps->bit_index = i&7u;
  *src = p;
  return ae;
}


void tcmplxA_zcvt_noconv_next(struct tcmplxA_zcvt* ps) {
  if (ps->index < ps->backward) {
    ps->state = 4u;
  } else if (ps->h_end & 1u) {
    ps->state = 6u;
  } else {
    ps->state = 3u;
    ps->bits = 0u;
  }
  ps->count = 0u;
  return;
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

int tcmplxA_zcvt_strrtozs
  ( struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end)
{
  int ae = tcmplxA_Success;
  size_t ret_out = 0u;
  unsigned char const *p = *src;
  for (ret_out = 0u; ret_out < dstsz && ae == tcmplxA_Success; ++ret_out) {
    switch (ps->state) {
    case 0: /* initial state */
      if (ps->count == 0u) {
        /* make a header */
        unsigned int header = 2048u;
        tcmplxA_uint32 const extent = tcmplxA_blockbuf_extent(ps->buffer);
        unsigned int const cinfo = tcmplxA_zcvt_cinfo(extent);
        ae = tcmplxA_inscopy_lengthsort(ps->values);
        if (ae != tcmplxA_Success)
          break;
        if (cinfo > 7u)
          ae = tcmplxA_ErrSanitize;
        header |= ((cinfo<<12)
            | (tcmplxA_zcvt_out_has_dict(ps) ? 32u : 0u));
        header += (31u-(header%31u));
        ps->backward = header;
      }
      if (ps->count < 2u) {
        dst[ret_out] = (unsigned char)(
              (ps->backward>>(8u-ps->count*8u))&255u
            );
        ps->count += 1u;
      }
      if (ps->count >= 2u) {
        unsigned int const fdict = ps->backward&32u;
        if (fdict) {
          ps->state = 1u;
        } else {
          ps->state = 3u;
          ps->checksum = 1u;
        }
        ps->backward = 0u;
        ps->count = 0u;
      } break;
    case 1: /* dictionary checksum */
      if (ps->count < 4u) {
        dst[ret_out] = (unsigned char)(
              (ps->checksum>>(32u-ps->count*8u))&255u
            );
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        ps->state = 3u;
        ps->checksum = 1u;
        ps->count = 0u;
      } break;
    case 2: /* check dictionary checksum */
    case 3: /* block start */
      ps->state = 3u;
      ae = tcmplxA_zcvt_strrtozs_bits(ps, dst+ret_out, &p, src_end);
      break;
    case 4: /* no compression: LEN and NLEN */
      if (ps->count == 0u) {
        tcmplxA_uint32 const diff = ps->backward - ps->index;
        ps->extra_length = (unsigned short)(diff<65535u ? diff : 65535u);
      }
      if (ps->count < 2u) {
        dst[ret_out] = (unsigned char)(ps->extra_length>>(8u*ps->count));
        ps->count += 1u;
      } else if (ps->count < 4u) {
        dst[ret_out] = (unsigned char)(
              ~(ps->extra_length>>(8u*(2u-ps->count)))
            );
        ps->count += 1u;
      }
      if (ps->count == 4u) {
        ps->state = 5u;
        if (ps->extra_length == 0u)
          tcmplxA_zcvt_noconv_next(ps);
      } break;
    case 5: /* no compression: copy bytes */
      if (ps->extra_length > 0u) {
        ps->extra_length -= 1u;
        dst[ret_out] = tcmplxA_blockbuf_output_data(ps->buffer)[ps->index];
        ps->index += 1u;
      }
      if (ps->extra_length == 0u) {
        tcmplxA_zcvt_noconv_next(ps);
      } break;
    case 6: /* end-of-stream checksum */
      if (ps->count < 4u) {
        dst[ret_out] = (ps->checksum>>(24u-8u*ps->count));
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        ps->state = 7u;
        ps->count = 0u;
      } break;
    case 7:
      ae = tcmplxA_EOF;
      break;
    case 8: /* encode */
    case 9:
    case 10:
    case 11:
    case 13: /* hcounts */
    case 14: /* code lengths code lengths */
    case 15: /* literals and distances */
    case 16: /* copy previous code length */
    case 17: /* copy zero length */
    case 18: /* copy zero length + 11 */
    case 19: /* generate code trees */
    case 20:
      ae = tcmplxA_zcvt_strrtozs_bits(ps, dst+ret_out, &p, src_end);
      break;
    }
    if (ae > tcmplxA_Success)
      break;
  }
  *ret = ret_out;
  *src = p;
  return ae;
}

int tcmplxA_zcvt_delimrtozs
  (struct tcmplxA_zcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned char const tmp[1] = {0u};
  unsigned char const* tmp_src = tmp;
  /* set the end flag: */ps->h_end |= 2u;
  return tcmplxA_zcvt_strrtozs(ps, ret, dst, dstsz, &tmp_src, tmp);
}
/* END   zcvt state / public */
