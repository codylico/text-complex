/**
 * @file src/brcvt.c
 * @brief Brotli conversion state
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "blockbuf_p.h"
#include "text-complex/access/brcvt.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include "text-complex/access/blockbuf.h"
#include "text-complex/access/fixlist.h"
#include "text-complex/access/inscopy.h"
#include "text-complex/access/ringdist.h"
#include "text-complex/access/zutil.h"
#include "text-complex/access/brmeta.h"
#include <string.h>
#include <limits.h>
#include <assert.h>



enum tcmplxA_brcvt_uconst {
  tcmplxA_brcvt_LitHistoSize = 288u,
  tcmplxA_brcvt_DistHistoSize = 32u,
  tcmplxA_brcvt_SeqHistoSize = 19u,
  tcmplxA_brcvt_HistogramSize =
      tcmplxA_brcvt_LitHistoSize
    + tcmplxA_brcvt_DistHistoSize
    + tcmplxA_brcvt_SeqHistoSize,
  tcmplxA_brcvt_MetaHeaderLen = 6,
};

enum tcmplxA_brcvt_istate {
  tcmplxA_BrCvt_WBits = 0,
  tcmplxA_BrCvt_MetaStart = 1,
  tcmplxA_BrCvt_MetaLength = 2,
  tcmplxA_BrCvt_MetaText = 3,
  tcmplxA_BrCvt_LastCheck = 4,
  tcmplxA_BrCvt_Done = 7,
};

struct tcmplxA_brcvt {
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
  /** @brief Fixed window size Huffman code table. */
  struct tcmplxA_fixlist* wbits;
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
  /** @brief Maximum metadatum length to store aside. */
  tcmplxA_uint32 max_len_meta;
  /** @brief Backward distance value. */
  tcmplxA_uint32 backward;
  /** @brief Byte count for the active state. */
  tcmplxA_uint32 count;
  /** @brief Byte index for the active state. */
  tcmplxA_uint32 index;
  /** @brief [deprecated] Checksum value. */
  unsigned short checksum
  __attribute__((__deprecated__))
  ;
  /** @brief Which value to use for WBITS. */
  unsigned char wbits_select;
  /** @brief Output internal bit count. */
  tcmplxA_uint32 bit_cap;
  /** @brief Nonzero metadata block storage. */
  struct tcmplxA_brmeta* metadata;
  /** @brief Number of metadata posted so far. */
  size_t meta_index;
  /** @brief Text of current metadata to post. */
  unsigned char* metatext;
};

unsigned char tcmplxA_brcvt_clen[19] =
  {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/**
 * @brief Initialize a zcvt state.
 * @param x the zcvt state to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_brcvt_init
  ( struct tcmplxA_brcvt* x, tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length);
/**
 * @brief Close a zcvt state.
 * @param x the zcvt state to close
 */
static void tcmplxA_brcvt_close(struct tcmplxA_brcvt* x);
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
static int tcmplxA_brcvt_zsrtostr_bits
  ( struct tcmplxA_brcvt* x, unsigned int y,
    size_t* ret, unsigned char* dst, size_t dstsz);
/**
 * @brief Calculate a compression info value.
 * @param window_size window size
 * @return an info value
 */
static tcmplxA_uint32 tcmplxA_brcvt_cinfo(tcmplxA_uint32 window_size);
/**
 * @brief Check whether the user set a dictionary.
 * @param ps conversion state
 * @return nonzero if set, zero otherwise
 */
static int tcmplxA_brcvt_out_has_dict(struct tcmplxA_brcvt const* ps);
/**
 * @brief Choose a nonzero value.
 * @param a one choice
 * @param b another choice
 * @return either `a` or `b`, whichever is nonzero
 */
static int tcmplxA_brcvt_nonzero(int a, int b);
/**
 * @brief Choose a nonzero value.
 * @param a one choice
 * @param b another choice
 * @param c another choice
 * @return either `a` or `b` or `c`, whichever is nonzero
 */
static int tcmplxA_brcvt_nonzero3(int a, int b, int c);
/**
 * @brief Make a code length sequence.
 * @param x the zcvt state
 * @return tcmplxA_Success on success, nonzero otherwise
 */
static int tcmplxA_brcvt_make_sequence(struct tcmplxA_brcvt* x);
/**
 * @brief Add a code length sequence.
 * @param s sequence list
 * @param len code length
 * @param count repeat count
 * @return tcmplxA_Success on success, nonzero otherwise
 */
static int tcmplxA_brcvt_post_sequence
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
static int tcmplxA_brcvt_strrtozs_bits
  ( struct tcmplxA_brcvt* x, unsigned char* y,
    unsigned char const** src, unsigned char const* src_end);
/**
 * @brief Start the next block of output.
 * @param ps Brotli state
 */
static void tcmplxA_brcvt_next_block(struct tcmplxA_brcvt* ps);

/* BEGIN zcvt state / static */
int tcmplxA_brcvt_init
    ( struct tcmplxA_brcvt* x, tcmplxA_uint32 block_size,
      tcmplxA_uint32 n, size_t chain_length)
{
  int res = tcmplxA_Success;
  if (n > 16777200u)
    n = 16777200u;
  x->wbits_select = 24;
  for (unsigned char i = 10; i < 24; ++i) {
    if (n <= (1ul<<i)-16) {
      x->wbits_select = i;
      break;
    }
  }
  /* buffer */{
    x->buffer = tcmplxA_blockbuf_new(block_size,n,chain_length, 0);
    if (x->buffer == NULL)
      res = tcmplxA_ErrMemory;
  }
  /* window size Huffman codes */{
    x->wbits = tcmplxA_fixlist_new(15);
    if (x->wbits == NULL)
      res = tcmplxA_ErrMemory;
    else
      tcmplxA_fixlist_preset(x->wbits, tcmplxA_FixList_BrotliWBits);
  }
  /* metadata storage */{
    x->metadata = tcmplxA_brmeta_new(0);
    if (x->metadata == NULL)
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
    if (tcmplxA_brcvt_HistogramSize < ((size_t)-1)/sizeof(tcmplxA_uint32)) {
      x->histogram = tcmplxA_util_malloc
        (tcmplxA_brcvt_HistogramSize*sizeof(tcmplxA_uint32));
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
    tcmplxA_brmeta_destroy(x->metadata);
    tcmplxA_fixlist_destroy(x->wbits);
    tcmplxA_blockbuf_destroy(x->buffer);
    return res;
  } else {
    x->max_len_meta = 1024;
    x->bits = 0u;
    x->h_end = 0;
    x->bit_length = 0u;
    x->state = tcmplxA_BrCvt_WBits;
    x->bit_index = 0u;
    x->backward = 0u;
    x->count = 0u;
    x->checksum = 0u;
    x->bit_cap = 0u;
    x->meta_index = 0u;
    x->metatext = NULL;
    return tcmplxA_Success;
  }
}

void tcmplxA_brcvt_close(struct tcmplxA_brcvt* x) {
  tcmplxA_blockstr_close(&x->sequence_list);
  tcmplxA_util_free(x->histogram);
  tcmplxA_ringdist_destroy(x->try_ring);
  tcmplxA_ringdist_destroy(x->ring);
  tcmplxA_inscopy_destroy(x->values);
  tcmplxA_fixlist_destroy(x->sequence);
  tcmplxA_fixlist_destroy(x->distances);
  tcmplxA_fixlist_destroy(x->literals);
  tcmplxA_brmeta_destroy(x->metadata);
  tcmplxA_fixlist_destroy(x->wbits);
  tcmplxA_blockbuf_destroy(x->buffer);
#ifndef NDEBUG
  x->metatext = NULL;
  x->meta_index = 0u;
  x->checksum = 0u;
  x->count = 0u;
  x->backward = 0u;
  x->bit_index = 0u;
  x->state = 0u;
  x->bit_length = 0u;
  x->bits = 0u;
  x->h_end = 0;
  x->sequence = NULL;
  x->distances = NULL;
  x->literals = NULL;
  x->buffer = NULL;
#endif /*NDEBUG*/
  return;
}

int tcmplxA_brcvt_zsrtostr_bits
  ( struct tcmplxA_brcvt* ps, unsigned int y,
    size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned int i;
  int ae = tcmplxA_Success;
  size_t ret_out = *ret;
  for (i = ps->bit_index; i < 8u && ae == tcmplxA_Success; ++i) {
    unsigned int x = (y>>i)&1u;
    switch (ps->state) {
    case tcmplxA_BrCvt_WBits: /* WBITS */
      if (ps->bit_length == 0) {
        tcmplxA_fixlist_codesort(ps->wbits);
      }
      if (ps->bit_length < 7u) {
        size_t j;
        ps->bits = (ps->bits<<1) | x;
        ps->bit_length += 1;
        j = tcmplxA_fixlist_codebsearch
          (ps->wbits, ps->bit_length, ps->bits);
        if (j < 16) {
          ps->wbits_select =
            (unsigned char)tcmplxA_fixlist_at(ps->wbits, j)->value;
          ps->state = tcmplxA_BrCvt_LastCheck;
          ps->bit_length = 0;
          break;
        }
      }
      if (ps->bit_length >= 7) {
        ae = tcmplxA_ErrSanitize;
      }
      break;
    case tcmplxA_BrCvt_LastCheck:
      if (ps->bit_length == 0) {
        ps->h_end = (x!=0);
        ps->bit_length = x?4:3;
        ps->count = 0;
        ps->bits = 0;
      }
      if (ps->count == 1 && ps->h_end) {
        if (x) {
          ps->state = tcmplxA_BrCvt_Done;
          ae = tcmplxA_EOF;
        }
      } else if (ps->count >= ps->bit_length-2) {
        ps->bits |= (x<<(ps->count-2));
      }
      if (ps->count < ps->bit_length)
        ps->count += 1;
      if (ps->count >= ps->bit_length) {
        if (ps->bits == 3) {
          ps->state = tcmplxA_BrCvt_MetaStart;
          ps->bit_length = 0;
        } else {
          ps->bit_length = (ps->bits+4)*4;
          ps->state = 99; /* TODO handle data */
        }
      }
      break;
    case tcmplxA_BrCvt_MetaStart:
      if (ps->bit_length == 0) {
        if (x != 0)
         ae = tcmplxA_ErrSanitize;
        else {
          ps->count = 4;
          ps->bit_length = tcmplxA_brcvt_MetaHeaderLen;
          ps->bits = 0;
        }
      } else if (ps->count < tcmplxA_brcvt_MetaHeaderLen) {
        ps->bits |= (x << (ps->count-4));
        ps->count += 1;
      }
      if (ps->count >= tcmplxA_brcvt_MetaHeaderLen) {
        ps->bit_length = ps->bits*8;
        ps->state = (ps->bits
          ? tcmplxA_BrCvt_MetaLength : tcmplxA_BrCvt_MetaText);
        ps->count = 0;
        ps->backward = 0;
      }
      break;
    case tcmplxA_BrCvt_MetaLength:
      if (ps->count < ps->bit_length) {
        ps->backward |= (((tcmplxA_uint32)x&1u)<<ps->count);
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        if (!(ps->backward>>(ps->count-8)))
          ae = tcmplxA_ErrSanitize;
        ps->count = 0;
        ps->state = tcmplxA_BrCvt_MetaText;
      }
      break;
    case tcmplxA_BrCvt_MetaText:
      if (x)
        ae = tcmplxA_ErrSanitize;
      else if (ps->backward == 0 && i == 7) {
        ps->state = (ps->h_end
          ? tcmplxA_BrCvt_Done : tcmplxA_BrCvt_LastCheck);
        if (ps->h_end)
          ae = tcmplxA_EOF;
      }
      break;
    case tcmplxA_BrCvt_Done: /* end of stream */
      ae = tcmplxA_EOF;
      break;
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
            ps->sequence, tcmplxA_brcvt_clen[ps->index]
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

unsigned int tcmplxA_brcvt_cinfo(tcmplxA_uint32 window_size) {
  unsigned int out = 0u;
  tcmplxA_uint32 v;
  if (window_size > 32768u)
    return 8u;
  else for (v = 1u; v < window_size; v<<=1, ++out) {
    continue;
  }
  return out > 8u ? out-8u : 0u;
}

int tcmplxA_brcvt_out_has_dict(struct tcmplxA_brcvt const* ps) {
  return tcmplxA_blockbuf_ring_size(ps->buffer) > 0u;
}

int tcmplxA_brcvt_nonzero(int a, int b) {
  return a != tcmplxA_Success ? a : b;
}

int tcmplxA_brcvt_nonzero3(int a, int b, int c) {
  return tcmplxA_brcvt_nonzero(a,tcmplxA_brcvt_nonzero(b,c));
}

int tcmplxA_brcvt_make_sequence(struct tcmplxA_brcvt* x) {
  unsigned int len = UINT_MAX, len_count = 0u;
  size_t i;
  size_t const lit_sz = tcmplxA_fixlist_size(x->literals);
  size_t const dist_sz = tcmplxA_fixlist_size(x->distances);
  x->sequence_list.sz = 0u;
  for (i = 0u; i < lit_sz; ++i) {
    unsigned int const n = tcmplxA_fixlist_at_c(x->literals,i)->len;
    if (len != n) {
      int const ae = tcmplxA_brcvt_post_sequence
        (&x->sequence_list, len, len_count);
      if (ae != tcmplxA_Success)
        return ae;
      else {
        len = n;
        len_count = 1u;
      }
    }
  }
  for (i = 0u; i < dist_sz; ++i) {
    unsigned int const n = tcmplxA_fixlist_at_c(x->distances,i)->len;
    if (len != n) {
      int const ae = tcmplxA_brcvt_post_sequence
        (&x->sequence_list, len, len_count);
      if (ae != tcmplxA_Success)
        return ae;
      else {
        len = n;
        len_count = 1u;
      }
    }
  }
  return tcmplxA_brcvt_post_sequence(&x->sequence_list, len, len_count);
}

int tcmplxA_brcvt_post_sequence
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
    for (i = 0u; i < count && ae == tcmplxA_Success; ) {
      unsigned int const x = len-i;
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


void tcmplxA_brcvt_next_block(struct tcmplxA_brcvt* ps) {
  if (ps->meta_index < tcmplxA_brmeta_size(ps->metadata))
    ps->state = tcmplxA_BrCvt_MetaStart;
  else if (ps->h_end)
    ps->state = tcmplxA_BrCvt_Done; /* TODO emit end-of-stream mark */
  else
    ps->state = tcmplxA_BrCvt_Done; /* TODO output data */
}

int tcmplxA_brcvt_strrtozs_bits
  ( struct tcmplxA_brcvt* ps, unsigned char* y,
    unsigned char const** src, unsigned char const* src_end)
{
  unsigned char const* p = *src;
  unsigned int i;
  int ae = tcmplxA_Success;
  for (i = ps->bit_index; i < 8u && ae == tcmplxA_Success; ++i) {
    unsigned int x = 0u;
    if ((!(ps->h_end&1u))/* if end marker not activated yet */
    &&  (ps->state == 3)/* and not inside a block */)
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
      else ps->checksum = tcmplxA_zutil_adler32(min_count, p, ps->checksum);
    }
    switch (ps->state) {
    case tcmplxA_BrCvt_WBits: /* WBITS */
      if (ps->bit_length == 0u) {
        struct tcmplxA_fixlist* const wbits = ps->wbits;
        tcmplxA_fixlist_valuesort(wbits);
        assert(ps->wbits_select >= 10 && ps->wbits_select <= 24);
        {
          struct tcmplxA_fixline const* const line =
            tcmplxA_fixlist_at_c(wbits, ps->wbits_select-10);
          ps->bit_length = line->len;
          ps->count = 1u;
          ps->bits = line->code;
        }
      }
      if (ps->count <= ps->bit_length) {
        x = (ps->bits>>(ps->bit_length-ps->count))&1u;
        ps->count += 1u;
      }
      if (ps->count > ps->bit_length) {
        if (ps->meta_index < tcmplxA_brmeta_size(ps->metadata))
          ps->state = tcmplxA_BrCvt_MetaStart;
        else
          ps->state = tcmplxA_BrCvt_Done;
        ps->bit_length = 0;
        ae = tcmplxA_EOF;
      }
      break;
    case tcmplxA_BrCvt_MetaStart:
      if (ps->bit_length == 0u) {
        size_t const sz = tcmplxA_brmeta_itemsize(ps->metadata, ps->meta_index);
        ps->metatext = tcmplxA_brmeta_itemdata(ps->metadata, ps->meta_index);
        ps->bits = 6;
        ps->count = 0;
        ps->bit_length = tcmplxA_brcvt_MetaHeaderLen;
        ps->backward = (tcmplxA_uint32)sz;
        if (sz > 65536)
          ps->bits |= 48;
        else if (sz > 256)
          ps->bits |= 32;
        else if (sz > 0)
          ps->bits |= 16;
      }
      if (ps->count < tcmplxA_brcvt_MetaHeaderLen) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1u;
      }
      if (ps->count >= tcmplxA_brcvt_MetaHeaderLen) {
        if (ps->backward)
          ps->state = tcmplxA_BrCvt_MetaLength;
        else if (i == 7)
          tcmplxA_brcvt_next_block(ps);
        else
          ps->state = tcmplxA_BrCvt_MetaText;
        ps->bit_length = 0;
      }
      break;
    case tcmplxA_BrCvt_MetaLength:
      if (ps->bit_length == 0u) {
        ps->bit_length = (ps->bits>>5)*8;
        ps->backward -= 1;
        ps->count = 0;
      }
      if (ps->count < ps->bit_length) {
        x = (ps->backward>>ps->count)&1;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->state = tcmplxA_BrCvt_MetaText;
        ps->backward += 1;
        ps->count = 0;
      }
      break;
    case tcmplxA_BrCvt_MetaText:
      x = 0;
      if (i == 7 && !ps->backward)
          tcmplxA_brcvt_next_block(ps);
      break;
    case tcmplxA_BrCvt_Done: /* end of stream */
      x = 0;
      ae = tcmplxA_EOF;
      break;
    }
    if (ae > tcmplxA_Success)
      /* halt the read position here: */break;
    else *y |= (x<<i);
  }
  ps->bit_index = i&7u;
  *src = p;
  return ae;
}
/* END   zcvt state / static */

/* BEGIN zcvt state / public */
struct tcmplxA_brcvt* tcmplxA_brcvt_new
  ( tcmplxA_uint32 block_size,
    tcmplxA_uint32 n, size_t chain_length)
{
  struct tcmplxA_brcvt* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_brcvt));
  if (out != NULL
  &&  tcmplxA_brcvt_init(out, block_size, n, chain_length) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_brcvt_destroy(struct tcmplxA_brcvt* x) {
  if (x != NULL) {
    tcmplxA_brcvt_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

int tcmplxA_brcvt_zsrtostr
  ( struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end)
{
  int ae = tcmplxA_Success;
  unsigned char const* p;
  size_t ret_out = 0u;
  for (p = *src; p < src_end && ae == tcmplxA_Success; ++p) {
    switch (ps->state) {
    case tcmplxA_BrCvt_WBits: /* initial state */
    case tcmplxA_BrCvt_LastCheck:
    case tcmplxA_BrCvt_MetaStart:
    case tcmplxA_BrCvt_MetaLength:
      ae = tcmplxA_brcvt_zsrtostr_bits(ps, (*p), &ret_out, dst, dstsz);
      break;
    case tcmplxA_BrCvt_MetaText:
      if (ps->count == 0) {
        /* allocate */
        size_t const use_backward = (ps->backward >= ps->max_len_meta)
          ? ps->max_len_meta : ps->backward;
        ps->meta_index = tcmplxA_brmeta_size(ps->metadata);
        ae = tcmplxA_brmeta_emplace(ps->metadata, use_backward);
        if (ae != tcmplxA_Success)
          break;
        ps->metatext = tcmplxA_brmeta_itemdata(ps->metadata, ps->meta_index);
        ps->index = tcmplxA_brmeta_itemsize(ps->metadata, ps->meta_index);
      }
      if (ps->count < ps->backward) {
        if (ps->count < ps->index)
          ps->metatext[ps->count] = (*p);
        ps->count += 1;
      }
      if (ps->count >= ps->backward) {
        ps->metatext = NULL;
        ps->state = (ps->h_end
          ? tcmplxA_BrCvt_Done : tcmplxA_BrCvt_LastCheck);
        if (ps->h_end)
          ae = tcmplxA_EOF;
      }
      break;
    case 9904: /* no compression: LEN and NLEN */
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
    case 7: /* end of stream */
      ae = tcmplxA_EOF;
      break;
    case 8: /* decode */
      ae = tcmplxA_brcvt_zsrtostr_bits(ps, (*p), &ret_out, dst, dstsz);
      break;
    }
    if (ae > tcmplxA_Success)
      /* halt the read position here: */break;
  }
  *src = p;
  *ret = ret_out;
  return ae;
}

size_t tcmplxA_brcvt_bypass
  (struct tcmplxA_brcvt* x, unsigned char const* buf, size_t sz)
{
  // TODO (this function)
  return 0u;
}

int tcmplxA_brcvt_strrtozs
  ( struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz,
    unsigned char const** src, unsigned char const* src_end)
{
  int ae = tcmplxA_Success;
  size_t ret_out = 0u;
  unsigned char const *p = *src;
  for (ret_out = 0u; ret_out < dstsz && ae == tcmplxA_Success; ++ret_out) {
    switch (ps->state) {
    case tcmplxA_BrCvt_WBits: /* initial state */
    case tcmplxA_BrCvt_MetaStart:
    case tcmplxA_BrCvt_MetaLength:
      ae = tcmplxA_brcvt_strrtozs_bits(ps, dst+ret_out, &p, src_end);
      break;
    case tcmplxA_BrCvt_MetaText:
      assert(ps->metatext);
      if (ps->count < ps->backward)
        dst[ret_out] = ps->metatext[ps->count++];
      ae = tcmplxA_Success;
      if (ps->count >= ps->backward) {
        ps->metatext = NULL;
        tcmplxA_brcvt_next_block(ps);
        ps->bit_length = 0;
      }
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
      } break;
    case 5: /* no compression: copy bytes */
      if (ps->extra_length > 0u) {
        ps->extra_length -= 1u;
        dst[ret_out] = tcmplxA_blockbuf_output_data(ps->buffer)[ps->index];
        ps->index += 1u;
      }
      if (ps->extra_length == 0u) {
        if (ps->index < ps->backward) {
          ps->state = 4u;
        } else if (ps->h_end & 1u) {
          ps->state = 6u;
        } else {
          ps->state = 3u;
          ps->bits = 0u;
        }
        ps->count = 0u;
      } break;
    case 6: /* end-of-stream checksum */
      if (ps->count < 4u) {
        dst[ret_out] = (ps->checksum>>(8u*ps->count));
        ps->count += 1u;
      }
      if (ps->count >= 4u) {
        ps->state = 7u;
        ps->count = 0u;
      } break;
    case tcmplxA_BrCvt_Done:
      ae = tcmplxA_EOF;
      break;
    case 8: /* encode */
      ae = tcmplxA_brcvt_strrtozs_bits(ps, dst+ret_out, &p, src_end);
      break;
    }
    if (ae > tcmplxA_Success)
      break;
  }
  *ret = ret_out;
  *src = p;
  return ae;
}

int tcmplxA_brcvt_delimrtozs
  (struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned char const tmp[1] = {0u};
  unsigned char const* tmp_src = tmp;
  /* set the end flag: */ps->h_end |= 2u;
  return tcmplxA_brcvt_strrtozs(ps, ret, dst, dstsz, &tmp_src, tmp);
}


struct tcmplxA_brmeta* tcmplxA_brcvt_metadata(struct tcmplxA_brcvt* ps) {
  return ps->metadata;
}
struct tcmplxA_brmeta const* tcmplxA_brcvt_metadata_c
  (struct tcmplxA_brcvt const* ps)
{
  return ps->metadata;
}
/* END   zcvt state / public */
