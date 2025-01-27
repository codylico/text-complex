/**
 * @file src/brcvt.c
 * @brief Brotli conversion state
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "blockbuf_p.h"
#include "fixlist_p.h"
#include "text-complex/access/brcvt.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include "text-complex/access/blockbuf.h"
#include "text-complex/access/fixlist.h"
#include "text-complex/access/inscopy.h"
#include "text-complex/access/ringdist.h"
#include "text-complex/access/zutil.h"
#include "text-complex/access/brmeta.h"
#include "text-complex/access/ctxtspan.h"
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
  tcmplxA_brcvt_CLenExtent = 18,
  tcmplxA_BrCvt_Margin = 16,
  tcmplxA_brcvt_BlockCountBits = 5,
  tcmplxA_brcvt_NoSkip = SHRT_MAX,
};

enum tcmplxA_brcvt_istate {
  tcmplxA_BrCvt_WBits = 0,
  tcmplxA_BrCvt_MetaStart = 1,
  tcmplxA_BrCvt_MetaLength = 2,
  tcmplxA_BrCvt_MetaText = 3,
  tcmplxA_BrCvt_LastCheck = 4,
  tcmplxA_BrCvt_Nibbles = 5,
  tcmplxA_BrCvt_InputLength = 6,
  tcmplxA_BrCvt_Done = 7,
  tcmplxA_BrCvt_CompressCheck = 8,
  tcmplxA_BrCvt_Uncompress = 9,
  tcmplxA_BrCvt_BlockTypesL = 16,
  tcmplxA_BrCvt_BlockTypesLAlpha = 17,
  tcmplxA_BrCvt_BlockCountLAlpha = 18,
  tcmplxA_BrCvt_BlockStartL = 19,
  tcmplxA_BrCvt_BlockTypesI = 20,
  tcmplxA_BrCvt_BlockTypesIAlpha = 21,
  tcmplxA_BrCvt_BlockCountIAlpha = 22,
  tcmplxA_BrCvt_BlockStartI = 23,
  tcmplxA_BrCvt_BlockTypesD = 24,
  tcmplxA_BrCvt_BlockTypesDAlpha = 25,
  tcmplxA_BrCvt_BlockCountDAlpha = 26,
  tcmplxA_BrCvt_BlockStartD = 27,
  tcmplxA_BrCvt_NPostfix = 28,
};

/** @brief Treety machine states. */
enum tcmplxA_brcvt_tstate {
  tcmplxA_BrCvt_TComplex = 0,
  tcmplxA_BrCvt_TSimpleCount = 1,
  tcmplxA_BrCvt_TSimpleAlpha = 2,
  tcmplxA_BrCvt_TDone = 3,
  tcmplxA_BrCvt_TSimpleFour = 4,
  tcmplxA_BrCvt_TSymbols = 5,
  tcmplxA_BrCvt_TRepeatStop = 14,
  tcmplxA_BrCvt_TRepeat = 16,
  tcmplxA_BrCvt_TZeroes = 17,
  tcmplxA_BrCvt_TNineteen = 19,
};

struct tcmplxA_brcvt_treety {
  /** @brief Number of symbols in the tree. */
  unsigned short count;
  /** @brief Index of current symbol. */
  unsigned short index;
  /** @brief Partial bit sequence. */
  unsigned short bits;
  /** @brief Machine state. */
  unsigned char state;
  /** @brief Number of bits captured or remaining. */
  unsigned char bit_length;
  /** @brief Code length check. */
  unsigned short len_check;
  /** @brief Count of nonzero code lengths. */
  unsigned char nonzero;
  /** @brief Most recently used code length. */
  unsigned char last_len;
  /** @brief The only code length usable. */
  unsigned char singular;
  /** @brief Previous nonzero code length. */
  unsigned char last_nonzero;
  /** @brief Last repeat count. */
  unsigned short last_repeat;
  /** @brief Internal prefix code. */
  struct tcmplxA_fixlist nineteen;
  /** @brief Tree description sequence. */
  struct tcmplxA_blockstr sequence_list;
};

struct tcmplxA_brcvt {
  /**
   * @brief ...
   * @note Using a buffer with a slide ring extent greater than 32768
   *   can cause the output sanity check to fail.
   */
  struct tcmplxA_blockbuf* buffer;
  /** @brief Block type prefix code for literals. */
  struct tcmplxA_fixlist literal_blocktype;
  /** @brief Block count prefix code for literals. */
  struct tcmplxA_fixlist literal_blockcount;
  /** @brief Block type prefix code for insert-and-copy. */
  struct tcmplxA_fixlist insert_blocktype;
  /** @brief Block count prefix code for insert-and-copy. */
  struct tcmplxA_fixlist insert_blockcount;
  /** @brief Block type prefix code for distances. */
  struct tcmplxA_fixlist distance_blocktype;
  /** @brief Block count prefix code for distances. */
  struct tcmplxA_fixlist distance_blockcount;
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
  /** @brief Block count insert-copy table */
  struct tcmplxA_inscopy* blockcounts;
  /** @brief ... */
  struct tcmplxA_ringdist* ring;
  /** @brief ... */
  struct tcmplxA_ringdist* try_ring;
  /** @brief Check for large blocks. */
  tcmplxA_uint32* histogram;
  /** @brief ... */
  unsigned short int bits;
  /** @brief Read count for bits used after a Huffman code. */
  unsigned short int extra_length;
  /** @brief ... */
  unsigned char bit_length;
  /**
   * @brief End indicator.
   * @note AND 2 = requested; AND 1 = granted.
   */
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
  /** @brief Partial byte stored aside for later. */
  unsigned char write_scratch;
  /** @brief Which value to use for WBITS. */
  unsigned char wbits_select;
  /** @brief Whether to insert an empty metadata block. */
  unsigned char emptymeta;
  /** @brief Size of alphabet used by the treety. */
  unsigned char alphabits;
  /** @brief Output internal bit count. */
  tcmplxA_uint32 bit_cap;
  /** @brief Nonzero metadata block storage. */
  struct tcmplxA_brmeta* metadata;
  /** @brief Number of metadata posted so far. */
  size_t meta_index;
  /** @brief Text of current metadata to post. */
  unsigned char* metatext;
  /** @brief Prefix code tree marshal. */
  struct tcmplxA_brcvt_treety treety;
  /** @brief Context span guess for outflow. */
  struct tcmplxA_ctxtspan guesses;
  /** @brief Context mode offset for outflow. */
  unsigned char guess_offset;
  /** @brief Current literal block type. */
  unsigned char blocktypeL_index;
  /** @brief Maximum literal block type. */
  unsigned char blocktypeL_max;
  /** @brief Current insert-and-copy block type. */
  unsigned char blocktypeI_index;
  /** @brief Maximum insert-and-copy block type. */
  unsigned char blocktypeI_max;
  /** @brief Current distances block type. */
  unsigned char blocktypeD_index;
  /** @brief Maximum distances block type. */
  unsigned char blocktypeD_max;
  /** @brief Context span effective lengths for outflow. */
  tcmplxA_uint32 guess_lengths[tcmplxA_CtxtSpan_Size];
  /** @brief Remaining items under the current literal blocktype. */
  tcmplxA_uint32 blocktypeL_remaining;
  /** @brief Remaining items under the current insert-and-copy blocktype. */
  tcmplxA_uint32 blocktypeI_remaining;
  /** @brief Remaining items under the current distance blocktype. */
  tcmplxA_uint32 blocktypeD_remaining;
  /** @brief Literal type skip code. */
  unsigned short blocktypeL_skip;
  /** @brief Literal count skip code. */
  unsigned short blockcountL_skip;
  /** @brief Insert-and-copy type skip code. */
  unsigned short blocktypeI_skip;
  /** @brief Insert-and-copy count skip code. */
  unsigned short blockcountI_skip;
  /** @brief Distance type skip code. */
  unsigned short blocktypeD_skip;
  /** @brief Distance count skip code. */
  unsigned short blockcountD_skip;
  /**
   * @brief Built-in tree type for block type outflow.
   * @todo Test for removal.
   */
  unsigned char blocktype_simple
  __attribute__((__deprecated__));
};

unsigned char tcmplxA_brcvt_clen[tcmplxA_brcvt_CLenExtent] =
  {1, 2, 3, 4, 0, 5, 17, 6, 16, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static struct tcmplxA_brcvt_treety const tcmplxA_brcvt_treety_zero = {0};
static struct tcmplxA_ctxtspan const tcmplxA_brcvt_guess_zero = {0};

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
 * @param x the mini-state for tree conversion
 * @param literals primary tree to encode
 * @return tcmplxA_Success on success, nonzero otherwise
 */
static int tcmplxA_brcvt_make_sequence(struct tcmplxA_brcvt_treety* x, struct tcmplxA_fixlist const* literals);
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
/**
 * @brief Determine if it's safe to add input.
 * @param ps compressor state to check
 * @return nonzero if safe, zero if unsafe
 */
static int tcmplxA_brcvt_can_add_input(struct tcmplxA_brcvt const* ps);
/**
 * @brief Process a single bit of a prefix list description.
 * @param[in,out] treety mini-state for description parsing
 * @param[out] prefixes prefix list to update
 * @param x bit to process
 * @param alphabits minimum number of bits needed to represent all
 *   applicable symbols
 * @return EOF at the end of a prefix list, Sanitize on
 *   parse failure, otherwise Success
 * @note At EOF, the extracted prefix list `prefixes` is sorted
 *   and ready to use.
 */
static int tcmplxA_brcvt_inflow19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned x, unsigned alphabits);
/**
 * @brief Reset a mini-state.
 * @param treety mini-state to reset
 */
static void tcmplxA_brcvt_reset19(struct tcmplxA_brcvt_treety* treety);
/**
 * @brief CLean up a mini-state.
 * @param treety mini-state to clean
 */
static void tcmplxA_brcvt_close19(struct tcmplxA_brcvt_treety* treety);
/**
 * @brief Emit a single bit of a prefix list description.
 * @param[in,out] treety mini-state for description parsing
 * @param[in,out] prefixes prefix list to update
 * @param[out] x bit to emit
 * @param alphabits minimum number of bits needed to represent all
 *   applicable symbols
 * @return EOF at the end of a prefix list, Sanitize or Memory on
 *   generator failure, otherwise Success
 */
static int tcmplxA_brcvt_outflow19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned* x, unsigned alphabits);
/**
 * @brief Transfer values for a simple prefix tree.
 * @param[out] prefixes tree to compose
 * @param nineteen value storage
 * @return EOF on success, nonzero otherwise
 */
static int tcmplxA_brcvt_transfer19(struct tcmplxA_fixlist* prefixes,
  struct tcmplxA_fixlist const* nineteen);
/**
 * @brief Post a code length to a prefix list.
 * @param[in,out] treety state machine
 * @param[out] prefixes prefix tree to update
 * @param value value to post
 * @return Success on success, Sanitize otherwise
 */
static int tcmplxA_brcvt_post19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned short value);
/**
 * @brief Resolve skip values.
 * @param prefixes prefix tree that may participate in code skipping
 * @return the value with the single nonzero code length, or NoSkip if unavailable
 */
static unsigned short tcmplxA_brcvt_resolve_skip(struct tcmplxA_fixlist const* prefixes);
/**
 * @brief Configure the literal block count acquisition.
 * @param[out] ps Brotli conversion state to update
 * @param value block code [0, 26)
 * @param next_state follow-up state in case of zero-length extra bits
 * @return base block count
 */
static tcmplxA_uint32 tcmplxA_brcvt_config_count
  (struct tcmplxA_brcvt* ps, unsigned long value, unsigned char next_state);
/**
 * @brief Check whether to emit compression.
 * @param ps Brotli conversion state
 * @return Success to proceed with compression, nonzero to emit uncompressed
 */
static int tcmplxA_brcvt_check_compress(struct tcmplxA_brcvt* ps);

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
  if (block_size >= 16777200u)
    block_size = 16777200u;
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
  /* blockcounts */{
    x->blockcounts = tcmplxA_inscopy_new(26u);
    if (x->blockcounts == NULL)
      res = tcmplxA_ErrMemory;
    else {
      int const preset_ae =
        tcmplxA_inscopy_preset(x->blockcounts, tcmplxA_InsCopy_BrotliBlock);
      if (preset_ae != tcmplxA_Success)
        res = preset_ae;
      else tcmplxA_inscopy_codesort(x->blockcounts);
    }
  }
  /* fixed prefix code ensemble */{
    int const bltypesl_res = tcmplxA_fixlist_init(&x->literal_blocktype,0);
    int const blcountl_res = tcmplxA_fixlist_init(&x->literal_blockcount,0);
    int const bltypesi_res = tcmplxA_fixlist_init(&x->insert_blocktype,0);
    int const blcounti_res = tcmplxA_fixlist_init(&x->insert_blockcount,0);
    int const bltypesd_res = tcmplxA_fixlist_init(&x->distance_blocktype,0);
    int const blcountd_res = tcmplxA_fixlist_init(&x->distance_blockcount,0);
    x->treety = tcmplxA_brcvt_treety_zero;
    memset(x->guess_lengths, 0, sizeof(x->guess_lengths));
    assert(bltypesl_res == tcmplxA_Success);
    assert(blcountl_res == tcmplxA_Success);
    assert(bltypesi_res == tcmplxA_Success);
    assert(blcounti_res == tcmplxA_Success);
    assert(bltypesd_res == tcmplxA_Success);
    assert(blcountd_res == tcmplxA_Success);
  }
  if (res != tcmplxA_Success) {
    tcmplxA_inscopy_destroy(x->blockcounts);
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
    x->write_scratch = 0u;
    x->bit_cap = 0u;
    x->emptymeta = 0u;
    x->meta_index = 0u;
    x->metatext = NULL;
    x->blocktypeL_index = 0u;
    x->blocktypeL_max = 0u;
    x->blocktypeL_remaining = 0u;
    x->blocktypeI_index = 0u;
    x->blocktypeI_max = 0u;
    x->blocktypeI_remaining = 0u;
    x->blocktypeL_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountL_skip = tcmplxA_brcvt_NoSkip;
    x->blocktypeI_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountI_skip = tcmplxA_brcvt_NoSkip;
    x->blocktypeD_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountD_skip = tcmplxA_brcvt_NoSkip;
    return tcmplxA_Success;
  }
}

void tcmplxA_brcvt_close(struct tcmplxA_brcvt* x) {
  tcmplxA_brcvt_close19(&x->treety);
  tcmplxA_fixlist_close(&x->literal_blocktype);
  tcmplxA_fixlist_close(&x->literal_blockcount);
  tcmplxA_fixlist_close(&x->insert_blocktype);
  tcmplxA_fixlist_close(&x->insert_blockcount);
  tcmplxA_fixlist_close(&x->distance_blocktype);
  tcmplxA_fixlist_close(&x->distance_blockcount);
  tcmplxA_util_free(x->histogram);
  tcmplxA_ringdist_destroy(x->try_ring);
  tcmplxA_ringdist_destroy(x->ring);
  tcmplxA_inscopy_destroy(x->values);
  tcmplxA_inscopy_destroy(x->blockcounts);
  tcmplxA_fixlist_destroy(x->sequence);
  tcmplxA_fixlist_destroy(x->distances);
  tcmplxA_fixlist_destroy(x->literals);
  tcmplxA_brmeta_destroy(x->metadata);
  tcmplxA_fixlist_destroy(x->wbits);
  tcmplxA_blockbuf_destroy(x->buffer);
#ifndef NDEBUG
  x->treety = tcmplxA_brcvt_treety_zero;
  x->metatext = NULL;
  x->meta_index = 0u;
  x->emptymeta = 0u;
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
  x->blocktypeL_skip = tcmplxA_brcvt_NoSkip;
  x->blockcountL_skip = tcmplxA_brcvt_NoSkip;
  x->blocktypeI_skip = tcmplxA_brcvt_NoSkip;
  x->blockcountI_skip = tcmplxA_brcvt_NoSkip;
  x->blocktypeD_skip = tcmplxA_brcvt_NoSkip;
  x->blockcountD_skip = tcmplxA_brcvt_NoSkip;
#endif /*NDEBUG*/
  return;
}

tcmplxA_uint32 tcmplxA_brcvt_config_count
  (struct tcmplxA_brcvt* ps, unsigned long value, unsigned char next_state)
{
  struct tcmplxA_inscopy_row const* const row = tcmplxA_inscopy_at_c(ps->blockcounts, (size_t)value);
  assert(row);
  ps->extra_length = row->insert_bits;
  ps->bits = 0;
  ps->bit_length = 0;
  if (!ps->extra_length)
    ps->state = next_state;
  return row->insert_first;
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
          ps->state = tcmplxA_BrCvt_InputLength;
          ps->backward = 0;
          ps->count = 0;
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
    case tcmplxA_BrCvt_InputLength:
      if (ps->count < ps->bit_length) {
        ps->backward |= (((tcmplxA_uint32)x)<<ps->count);
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->bit_length = 0;
        ps->count = 0;
        if (ps->bit_length > 16 && (ps->backward>>(ps->bit_length-4))==0)
          ae = tcmplxA_ErrSanitize;
        ps->backward += 1;
        ps->state = tcmplxA_BrCvt_CompressCheck;
      } break;
    case tcmplxA_BrCvt_CompressCheck:
      if (x) {
        ps->state = tcmplxA_BrCvt_Uncompress;
      } else {
        tcmplxA_inscopy_codesort(ps->blockcounts);
        ps->state = tcmplxA_BrCvt_BlockTypesL;
        ps->bit_length = 0;
        ps->count = 0;
        ps->bits = 0;
        ps->blocktypeL_skip = tcmplxA_brcvt_NoSkip;
        ps->blockcountL_skip = tcmplxA_brcvt_NoSkip;
        ps->blocktypeI_skip = tcmplxA_brcvt_NoSkip;
        ps->blockcountI_skip = tcmplxA_brcvt_NoSkip;
        ps->blocktypeD_skip = tcmplxA_brcvt_NoSkip;
        ps->blockcountD_skip = tcmplxA_brcvt_NoSkip;
      } break;
    case tcmplxA_BrCvt_Uncompress:
      if (x)
        ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_BlockTypesL:
      if (ps->bit_length == 0) {
        ps->count = 1;
        ps->bits = x;
        ps->bit_length = (x ? 4 : 1);
      } else if (ps->count < ps->bit_length) {
        ps->bits |= ((x&1u)<<(ps->count++));
        if (ps->count == 4 && (ps->bits&14u))
          ps->bit_length += (ps->bits>>1);
      }
      if (ps->count >= ps->bit_length) {
        /* extract encoded count */
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->blocktypeL_index = 0;
        if (ps->count == 1) {
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->literal_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeL_skip = 0;
          ps->blocktypeL_max = 0;
          ps->blocktypeL_remaining = (tcmplxA_uint32)(~0ul);
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeL_max = (unsigned char)(alphasize-1u);
        }
      } break;
    case tcmplxA_BrCvt_BlockTypesLAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->literal_blocktype, x,
          ps->alphabits);
        if (res == tcmplxA_EOF) {
          ps->blocktypeL_skip = tcmplxA_brcvt_resolve_skip(&ps->literal_blocktype);
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockCountLAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->literal_blockcount, x, 5);
        if (res == tcmplxA_EOF) {
          ps->blockcountL_skip = tcmplxA_brcvt_resolve_skip(&ps->literal_blockcount);
          ps->state += 1;
          ps->bit_length = 0;
          ps->bits = 0;
          ps->count = 0;
          ps->extra_length = 0;
          ps->blocktypeL_index = 0;
          ps->blocktypeL_remaining = 0;
          tcmplxA_fixlist_codesort(&ps->literal_blockcount);
          if (ps->blockcountL_skip != tcmplxA_brcvt_NoSkip)
            ps->blocktypeL_remaining = tcmplxA_brcvt_config_count(ps, ps->blockcountL_skip, ps->state + 1);
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockStartL:
      if (ps->extra_length == 0) {
        size_t code_index = ~0;
        struct tcmplxA_fixline const* line = NULL;
        ps->bits = (ps->bits<<1) | x;
        ps->bit_length += 1;
        code_index = tcmplxA_fixlist_codebsearch(&ps->literal_blockcount, ps->bit_length, ps->bits);
        if (code_index >= 26) {
          if (ps->bit_length >= 15)
            ae = tcmplxA_ErrSanitize;
          break;
        }
        line = tcmplxA_fixlist_at_c(&ps->literal_blockcount, code_index);
        assert(line);
        ps->blocktypeL_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          ps->blocktypeL_remaining += ps->bits;
          ps->bits = 0;
          ps->bit_length = 0;
          ps->state += 1;
        }
      } else ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_BlockTypesI:
      if (ps->bit_length == 0) {
        ps->count = 1;
        ps->bits = x;
        ps->bit_length = (x ? 4 : 1);
      } else if (ps->count < ps->bit_length) {
        ps->bits |= ((x&1u)<<(ps->count++));
        if (ps->count == 4 && (ps->bits&14u))
          ps->bit_length += (ps->bits>>1);
      }
      if (ps->count >= ps->bit_length) {
        /* extract encoded count */
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->blocktypeI_index = 0;
        if (ps->count == 1) {
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->insert_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeI_skip = 0;
          ps->blocktypeI_max = 0;
          ps->blocktypeI_remaining = (tcmplxA_uint32)(~0ul);
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeI_max = (unsigned char)(alphasize-1u);
        }
      } break;
    case tcmplxA_BrCvt_BlockTypesIAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->insert_blocktype, x,
          ps->alphabits);
        if (res == tcmplxA_EOF) {
          ps->blocktypeI_skip = tcmplxA_brcvt_resolve_skip(&ps->insert_blocktype);
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockCountIAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->insert_blockcount, x, 5);
        if (res == tcmplxA_EOF) {
          ps->blockcountI_skip = tcmplxA_brcvt_resolve_skip(&ps->insert_blockcount);
          ps->state += 1;
          ps->bit_length = 0;
          ps->bits = 0;
          ps->count = 0;
          ps->extra_length = 0;
          ps->blocktypeI_index = 0;
          ps->blocktypeI_remaining = 0;
          tcmplxA_fixlist_codesort(&ps->insert_blockcount);
          if (ps->blockcountI_skip != tcmplxA_brcvt_NoSkip)
            ps->blocktypeI_remaining = tcmplxA_brcvt_config_count(ps, ps->blockcountI_skip, ps->state + 1);
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockStartI:
      if (ps->extra_length == 0) {
        size_t code_index = ~0;
        struct tcmplxA_fixline const* line = NULL;
        ps->bits = (ps->bits<<1) | x;
        ps->bit_length += 1;
        code_index = tcmplxA_fixlist_codebsearch(&ps->insert_blockcount, ps->bit_length, ps->bits);
        if (code_index >= 26) {
          if (ps->bit_length >= 15)
            ae = tcmplxA_ErrSanitize;
          break;
        }
        line = tcmplxA_fixlist_at_c(&ps->insert_blockcount, code_index);
        assert(line);
        ps->blocktypeI_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          ps->blocktypeI_remaining += ps->bits;
          ps->bits = 0;
          ps->bit_length = 0;
          ps->state += 1;
        }
      } else ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_BlockTypesD:
      if (ps->bit_length == 0) {
        ps->count = 1;
        ps->bits = x;
        ps->bit_length = (x ? 4 : 1);
      } else if (ps->count < ps->bit_length) {
        ps->bits |= ((x&1u)<<(ps->count++));
        if (ps->count == 4 && (ps->bits&14u))
          ps->bit_length += (ps->bits>>1);
      }
      if (ps->count >= ps->bit_length) {
        /* extract encoded count */
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->blocktypeD_index = 0;
        if (ps->count == 1) {
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->distance_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeD_skip = 0;
          ps->blocktypeD_max = 0;
          ps->blocktypeD_remaining = (tcmplxA_uint32)(~0ul);
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeI_max = (unsigned char)(alphasize-1u);
        }
      } break;
    case tcmplxA_BrCvt_BlockTypesDAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->distance_blocktype, x,
          ps->alphabits);
        if (res == tcmplxA_EOF) {
          ps->blocktypeD_skip = tcmplxA_brcvt_resolve_skip(&ps->distance_blocktype);
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockCountDAlpha:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->distance_blockcount, x, 5);
        if (res == tcmplxA_EOF) {
          ps->blockcountD_skip = tcmplxA_brcvt_resolve_skip(&ps->distance_blockcount);
          ps->state += 1;
          ps->bit_length = 0;
          ps->bits = 0;
          ps->count = 0;
          ps->extra_length = 0;
          ps->blocktypeD_index = 0;
          ps->blocktypeD_remaining = 0;
          tcmplxA_fixlist_codesort(&ps->distance_blockcount);
          if (ps->blockcountD_skip != tcmplxA_brcvt_NoSkip)
            ps->blocktypeD_remaining = tcmplxA_brcvt_config_count(ps, ps->blockcountD_skip, ps->state + 1);
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockStartD:
      if (ps->extra_length == 0) {
        size_t code_index = ~0;
        struct tcmplxA_fixline const* line = NULL;
        ps->bits = (ps->bits<<1) | x;
        ps->bit_length += 1;
        code_index = tcmplxA_fixlist_codebsearch(&ps->distance_blockcount, ps->bit_length, ps->bits);
        if (code_index >= 26) {
          if (ps->bit_length >= 15)
            ae = tcmplxA_ErrSanitize;
          break;
        }
        line = tcmplxA_fixlist_at_c(&ps->distance_blockcount, code_index);
        assert(line);
        ps->blocktypeD_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          ps->blocktypeD_remaining += ps->bits;
          ps->bits = 0;
          ps->bit_length = 0;
          ps->state += 1;
        }
      } else ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_Done: /* end of stream */
      ae = tcmplxA_EOF;
      break;
    case 5000019: /* generate code trees */
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
    case 5000020: /* alpha bringback */
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
    case 5000016: /* copy previous code length */
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
    case 5000017: /* copy zero length */
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
    case 5000018: /* copy zero length + 11 */
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

unsigned short tcmplxA_brcvt_resolve_skip(struct tcmplxA_fixlist const* prefixes) {
  size_t i;
  size_t const count = tcmplxA_fixlist_size(prefixes);
  unsigned long last_nonzero = ULONG_MAX;
  if (count == 1) {
    return (unsigned short)tcmplxA_fixlist_at_c(prefixes, 0)->value;
  } else for (i = 0; i < count; ++i) {
    struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(prefixes, i);
    if (line->len == 0)
      continue;
    else if (last_nonzero != ULONG_MAX)
      return tcmplxA_brcvt_NoSkip;
    else last_nonzero = line->value;
  }
  return last_nonzero == ULONG_MAX ? tcmplxA_brcvt_NoSkip : (unsigned short)last_nonzero;
}

int tcmplxA_brcvt_transfer19(struct tcmplxA_fixlist* prefixes,
  struct tcmplxA_fixlist const* nineteen)
{
  size_t const n = tcmplxA_fixlist_size(nineteen);
  size_t i;
  for (i = 0; i < n; ++i) {
    tcmplxA_fixlist_at(prefixes, i)->value = tcmplxA_fixlist_at_c(nineteen, i)->value;
  }
  tcmplxA_fixlist_valuesort(prefixes);
  tcmplxA_fixlist_gen_codes(prefixes);
  return tcmplxA_EOF;
}

void tcmplxA_brcvt_reset19(struct tcmplxA_brcvt_treety* treety) {
  struct tcmplxA_fixlist nineteen = treety->nineteen;
  struct tcmplxA_blockstr compress = treety->sequence_list;
  *treety = tcmplxA_brcvt_treety_zero;
  treety->sequence_list = compress;
  treety->sequence_list.sz = 0;
  treety->nineteen = nineteen;
}

void tcmplxA_brcvt_close19(struct tcmplxA_brcvt_treety* treety) {
  tcmplxA_fixlist_close(&treety->nineteen);
  tcmplxA_blockstr_close(&treety->sequence_list);
}

int tcmplxA_brcvt_inflow19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned x, unsigned alphabits)
{
  switch (treety->state) {
  case tcmplxA_BrCvt_TComplex:
    treety->bits |= (x<<(treety->bit_length++));
    if (treety->bit_length == 2) {
      if (treety->bits == 1) {
        treety->state = tcmplxA_BrCvt_TSimpleCount;
        treety->bits = 0;
        treety->bit_length = 0;
      } else {
        size_t i;
        int const res = tcmplxA_fixlist_resize(&treety->nineteen, tcmplxA_brcvt_CLenExtent);
        int const prefix_res = tcmplxA_fixlist_resize(prefixes, treety->count);
        if (res != tcmplxA_Success)
          return res;
        else if (prefix_res != tcmplxA_Success)
          return prefix_res;
        for (i = 0; i < tcmplxA_brcvt_CLenExtent; ++i) {
          struct tcmplxA_fixline* const line = tcmplxA_fixlist_at(&treety->nineteen, i);
          line->value = tcmplxA_brcvt_clen[i];
          line->code = 0;
          line->len = 0;
        }
        treety->index = treety->bits;
        treety->state = tcmplxA_BrCvt_TNineteen;
        treety->bits = 0;
        treety->bit_length = 0;
        treety->nonzero = 0;
        treety->len_check = 0;
        treety->last_len = 255;
      }
    } break;
  case tcmplxA_BrCvt_TSimpleCount:
    treety->bits |= (x<<(treety->bit_length++));
    if (treety->bit_length == 2) {
      unsigned short const new_count = treety->bits+1;
      int const res = tcmplxA_fixlist_resize(&treety->nineteen, new_count);
      if (res != tcmplxA_Success)
        return res;
      treety->count = new_count;
      treety->index = 0;
      treety->bit_length = 0;
      treety->bits = 0;
      treety->state = tcmplxA_BrCvt_TSimpleAlpha;
    } break;
  case tcmplxA_BrCvt_TSimpleAlpha:
    treety->bits |= (x<<(treety->bit_length++));
    if (treety->bit_length == alphabits) {
      tcmplxA_fixlist_at(&treety->nineteen, treety->index)->value = treety->bits;
      treety->index += 1;
      treety->bits = 0;
      treety->bit_length = 0;
      if (treety->index >= treety->count) {
        if (treety->count == 4) {
          treety->state = tcmplxA_BrCvt_TSimpleFour;
          break;
        }
        treety->state = tcmplxA_BrCvt_TDone;
        return tcmplxA_fixlist_preset(prefixes, treety->count) == tcmplxA_Success
          ? tcmplxA_brcvt_transfer19(prefixes, &treety->nineteen) : tcmplxA_ErrMemory;
      }
    } break;
  case tcmplxA_BrCvt_TSimpleFour:
    treety->state = tcmplxA_BrCvt_TDone;
    return tcmplxA_fixlist_preset(prefixes, 4+x) == tcmplxA_Success
      ? tcmplxA_brcvt_transfer19(prefixes, &treety->nineteen) : tcmplxA_ErrMemory;
  case tcmplxA_BrCvt_TNineteen:
    treety->bits = (treety->bits<<1)|x;
    treety->bit_length += 1;
    if (treety->bit_length > 4)
      return tcmplxA_ErrSanitize;
    else {
      unsigned short len = 255;
      switch (treety->bit_length) {
      case 2:
        if (treety->bits < 3u) {
          len = (treety->bits>0)?2+treety->bits:0;
        } break;
      case 3:
        if (treety->bits == 6u) {
          len = 2;
        } break;
      case 4:
        len = (treety->bits-14u)*4u + 1u;
        break;
      }
      if (len > 5)
        return tcmplxA_Success;
      treety->bits = 0;
      treety->bit_length = 0;
      if (len > 0) {
        treety->len_check += (32 >> len);
        treety->nonzero += 1;
        if (treety->len_check > 32)
          return tcmplxA_ErrSanitize;
      }
      if (treety->nonzero == 1)
        treety->singular = tcmplxA_brcvt_clen[treety->index];
      tcmplxA_fixlist_at(&treety->nineteen, treety->index++)->len = len;
      if (treety->index >= tcmplxA_brcvt_CLenExtent || treety->len_check >= 32) {
        if (treety->nonzero > 1 && treety->len_check != 32)
          return tcmplxA_ErrSanitize;
        treety->state = tcmplxA_BrCvt_TSymbols;
        treety->len_check = 0;
        treety->last_len = 8;
        treety->last_nonzero = 8;
        treety->bits = 0;
        treety->index = 0;
        treety->bit_length = 0;
        treety->last_repeat = 0;
        if (treety->nonzero == 1) {
          unsigned stop;
          for (stop = 0; stop < 704 && treety->state == tcmplxA_BrCvt_TSymbols; ++stop) {
            int const res = tcmplxA_brcvt_post19(treety, prefixes, treety->singular);
            if (res != tcmplxA_Success)
              return res;
          }
        } else {
          int res;
          tcmplxA_fixlist_valuesort(&treety->nineteen);
          res = tcmplxA_fixlist_gen_codes(&treety->nineteen);
          if (res != tcmplxA_Success)
            return tcmplxA_ErrSanitize;
          tcmplxA_fixlist_codesort(&treety->nineteen);
        }
      }
    } break;
  case tcmplxA_BrCvt_TRepeat:
  case tcmplxA_BrCvt_TZeroes:
    treety->bits |= (x<<(treety->bit_length++));
    if (treety->bit_length < treety->state-tcmplxA_BrCvt_TRepeatStop)
      break;
    else {
      int const res = tcmplxA_brcvt_post19(treety, prefixes, treety->bits);
      if (res != tcmplxA_Success)
        return res;
    }
    if (treety->nonzero != 1)
      break;
  case tcmplxA_BrCvt_TSymbols:
    if (treety->nonzero == 1) {
      unsigned stop;
      for (stop = 0; stop < 704 && treety->state == tcmplxA_BrCvt_TSymbols; ++stop) {
        int const res = tcmplxA_brcvt_post19(treety, prefixes, treety->singular);
        if (res != tcmplxA_Success)
          return res;
      }
    } else {
      size_t code_index;
      treety->bits = (treety->bits<<1)|x;
      treety->bit_length += 1;
      code_index = tcmplxA_fixlist_codebsearch(&treety->nineteen, treety->bit_length, treety->bits);
      if (code_index < treety->count) {
        int const res = tcmplxA_brcvt_post19(treety, prefixes,
          tcmplxA_fixlist_at_c(&treety->nineteen, code_index)->value);
        if (res != tcmplxA_Success)
          return res;
      } else if (treety->bit_length > 5)
        return tcmplxA_ErrSanitize;
    } break;
  default:
    return tcmplxA_ErrSanitize;
  }
  return tcmplxA_Success;
}

int tcmplxA_brcvt_post19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned short value)
{
  if (treety->state == tcmplxA_BrCvt_TRepeat) {
    unsigned short const repeat_total =
      ((treety->last_repeat > 0) ? 4*(treety->last_repeat-2) : 0) + (value+3);
    unsigned short const repeat_gap = repeat_total - treety->last_repeat;
    unsigned i;
    if (repeat_gap > treety->count - treety->index)
      return tcmplxA_ErrSanitize;
    treety->last_repeat = repeat_total;
    for (i = 0; i < repeat_gap; ++i) {
      unsigned short const push = (32768>>treety->last_nonzero);
      if (push > 32768-treety->len_check)
        return tcmplxA_ErrSanitize;
      tcmplxA_fixlist_at(prefixes, treety->index)->value = treety->index;
      tcmplxA_fixlist_at(prefixes, treety->index)->len = treety->last_nonzero;
      treety->len_check += push;
      treety->index += 1;
    }
    treety->last_len = treety->last_nonzero;
  } else if (treety->state == tcmplxA_BrCvt_TZeroes) {
    unsigned short const repeat_total =
      ((treety->last_repeat > 0) ? 8*(treety->last_repeat-2) : 0) + (value+3);
    unsigned short const repeat_gap = repeat_total - treety->last_repeat;
    unsigned i;
    if (repeat_gap > treety->count - treety->index)
      return tcmplxA_ErrSanitize;
    treety->last_repeat = repeat_total;
    for (i = 0; i < repeat_gap; ++i) {
      tcmplxA_fixlist_at(prefixes, treety->index)->value = treety->index;
      tcmplxA_fixlist_at(prefixes, treety->index)->len = 0;
      treety->index += 1;
    }
    treety->last_len = 0;
  } else if (treety->state == tcmplxA_BrCvt_TSymbols) {
    if (value < 16) {
      tcmplxA_fixlist_at(prefixes, treety->index)->value = treety->index;
      tcmplxA_fixlist_at(prefixes, treety->index)->len = value;
      treety->index += 1;
      treety->last_repeat = 0;
      treety->last_len = value;
      if (value) {
        unsigned short const push = (32768>>value);
        if (push > 32768-treety->len_check)
          return tcmplxA_ErrSanitize;
        treety->last_nonzero = (unsigned char)value;
        treety->len_check += push;
      }
    } else if (value == 16) {
      if (treety->last_len == 0)
        treety->last_repeat = 0;
      treety->state = tcmplxA_BrCvt_TRepeat;
      treety->bits = 0;
      treety->bit_length = 0;
    } else if (value == 17) {
      if (treety->last_len != 0)
        treety->last_repeat = 0;
      treety->state = tcmplxA_BrCvt_TZeroes;
      treety->bits = 0;
      treety->bit_length = 0;
    } else return tcmplxA_ErrSanitize;
  } else return tcmplxA_ErrSanitize;
  if (treety->index >= treety->count || treety->len_check >= 32768) {
    int const sort_res = tcmplxA_fixlist_valuesort(prefixes);
    int const res = tcmplxA_fixlist_gen_codes(prefixes);
    treety->state = tcmplxA_BrCvt_TDone;
    return (res == tcmplxA_Success && sort_res == tcmplxA_Success)
      ? tcmplxA_EOF : (res?res:sort_res);
  }
  return tcmplxA_Success;
}

int tcmplxA_brcvt_outflow19(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes, unsigned* x, unsigned alphabits)
{
  switch (treety->state) {
  case tcmplxA_BrCvt_TComplex:
    if (treety->bit_length == 0) {
      int const preset = tcmplxA_fixlist_match_preset(prefixes);
      if (preset != tcmplxA_FixList_BrotliComplex) {
        treety->bits = 1;
        treety->count = preset;
      } else {
        tcmplxA_uint32 const lines = sizeof(tcmplxA_brcvt_clen);
        int res = 0;
        tcmplxA_uint32 histogram[sizeof(tcmplxA_brcvt_clen)] = {0};
        tcmplxA_uint32 i;
        tcmplxA_uint32 nonzero = 0;
        if (tcmplxA_brcvt_make_sequence(treety, prefixes) != tcmplxA_Success)
          return tcmplxA_ErrMemory;
        for (i = 0; i < treety->sequence_list.sz; ++i) {
          unsigned char const code = treety->sequence_list.p[i];
          assert(code < sizeof(histogram));
          histogram[code] += 1;
          if (code >= 16)
            i += 1;
        }
        for (i = 0; i < 3; ++i) {
          if (histogram[tcmplxA_brcvt_clen[i]])
            break;
        }
        treety->bits = (i - (i==1));
        if (tcmplxA_fixlist_resize(&treety->nineteen, lines) != tcmplxA_Success)
          return tcmplxA_ErrMemory;
        for (i = 0; i < lines; ++i)
          tcmplxA_fixlist_at(&treety->nineteen, i)->value = i;
        if (tcmplxA_fixlist_gen_lengths(&treety->nineteen, histogram, 5) != tcmplxA_Success)
          return tcmplxA_ErrMemory;
        res = tcmplxA_fixlist_gen_codes(&treety->nineteen);
        assert(res == tcmplxA_Success);
        treety->count = 0;
        treety->nonzero = 0;
        for (i = 0; i < lines; ++i) {
          unsigned char clen = tcmplxA_brcvt_clen[i];
          if (histogram[clen] == 0)
            continue;
          else if (!treety->nonzero)
            treety->nonzero = clen;
          treety->count = i+1;
          nonzero += 1;
        }
        if (nonzero < 2)
          treety->count = (unsigned short)lines;
        else nonzero = 0;
      }
    }
    *x = (treety->bits>>treety->bit_length++)&1u;
    if (treety->bit_length >= 2) {
      treety->bit_length = 0;
      if (treety->bits == 1) {
        treety->state = tcmplxA_BrCvt_TSimpleCount;
        treety->index = 0;
      } else {
        treety->state = tcmplxA_BrCvt_TNineteen;
        treety->index = treety->bits;
      }
    } break;
  case tcmplxA_BrCvt_TSimpleCount:
    if (treety->bit_length == 0) {
      treety->bits = treety->count - 1 - (treety->count==tcmplxA_FixList_BrotliSimple4B);
    }
    *x = (treety->bits>>treety->bit_length++)&1u;
    if (treety->bit_length >= 2) {
      treety->state = tcmplxA_BrCvt_TSimpleAlpha;
      treety->bit_length = 0;
    } break;
  case tcmplxA_BrCvt_TSimpleAlpha:
    if (treety->bit_length == 0) {
      if (treety->index >= tcmplxA_fixlist_size(prefixes))
        return tcmplxA_ErrSanitize;
      treety->bits = (unsigned short)tcmplxA_fixlist_at_c(prefixes, treety->index)->value;
    }
    *x = (treety->bits>>treety->bit_length++)&1u;
    if (treety->bit_length >= alphabits) {
      unsigned const effective = treety->count - (treety->count==tcmplxA_FixList_BrotliSimple4B);
      treety->index += 1;
      treety->bit_length = 0;
      if (treety->index < effective)
        break;
      treety->state = tcmplxA_BrCvt_TDone;
      if (treety->count < tcmplxA_FixList_BrotliSimple4A)
        return tcmplxA_EOF;
      treety->state = tcmplxA_BrCvt_TSimpleFour;
    } break;
  case tcmplxA_BrCvt_TSimpleFour:
    *x = (treety->count==tcmplxA_FixList_BrotliSimple4B);
    treety->state = tcmplxA_BrCvt_TDone;
    return tcmplxA_EOF;
  case tcmplxA_BrCvt_TNineteen:
    if (treety->bit_length == 0) {
      unsigned short const len = tcmplxA_fixlist_at_c(&treety->nineteen, treety->index)->len;
      switch (len) {
      case 0: treety->bit_length = 2; treety->bits = 0u; break;
      case 1: treety->bit_length = 4; treety->bits = 14u; break;
      case 2: treety->bit_length = 3; treety->bits = 6u; break;
      case 3: treety->bit_length = 2; treety->bits = 1u; break;
      case 4: treety->bit_length = 2; treety->bits = 2u; break;
      case 5: treety->bit_length = 2; treety->bits = 15u; break;
      default: return tcmplxA_ErrSanitize;
      }
    }
    if (treety->bit_length > 0)
      *x = (treety->bits>>--treety->bit_length)&1u;
    if (treety->bit_length == 0) {
      treety->index += 1;
      if (treety->index < treety->count)
        break;
      assert(treety->bit_length == 0);
      if (treety->nonzero >= 16) {
        treety->state = treety->nonzero;
        treety->index = 1;
        assert(treety->sequence_list.sz >= 2);
      } else if (treety->nonzero > 0) {
        treety->state = tcmplxA_BrCvt_TDone;
        return tcmplxA_EOF;
      } else {
        treety->index = 0;
        treety->state = tcmplxA_BrCvt_TSymbols;
      }
    } break;
  case tcmplxA_BrCvt_TRepeat:
  case tcmplxA_BrCvt_TZeroes:
    if (treety->bit_length == 0) {
      treety->bits = treety->sequence_list.p[treety->index];
      treety->count = treety->state - 14;
    }
    *x = (treety->bits>>treety->bit_length++)&1u;
    if (treety->bit_length >= treety->count) {
      treety->index += 1;
      if (treety->index >= treety->sequence_list.sz) {
        treety->state = tcmplxA_BrCvt_TDone;
        return tcmplxA_EOF;
      } else if (treety->nonzero) {
        assert(treety->sequence_list.p[treety->index] == treety->nonzero);
        treety->index += 1;
      } else treety->state = tcmplxA_BrCvt_TSymbols;
    } break;
  case tcmplxA_BrCvt_TSymbols:
    if (treety->bit_length == 0) {
      unsigned char const code = treety->sequence_list.p[treety->index];
      struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(&treety->nineteen, code);
      treety->bits = line->code;
      treety->bit_length = line->len;
    }
    if (treety->bit_length > 0)
      *x = (treety->bits>>--treety->bit_length)&1u;
    if (treety->bit_length == 0) {
      unsigned char const code = treety->sequence_list.p[treety->index];
      treety->index += 1;
      if (treety->index >= treety->sequence_list.sz) {
        if (code >= 16)
          return tcmplxA_ErrSanitize;
        treety->state = tcmplxA_BrCvt_TDone;
        return tcmplxA_EOF;
      } else if (code >= 16)
        treety->state = code;
      // otherwise stay put
    } break;
  case tcmplxA_BrCvt_TDone:
    return tcmplxA_EOF;
  default:
    return tcmplxA_ErrSanitize;
  }
  return tcmplxA_Success;
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

int tcmplxA_brcvt_make_sequence(struct tcmplxA_brcvt_treety* x, struct tcmplxA_fixlist const* literals) {
  unsigned int len = UINT_MAX, len_count = 0u;
  size_t i;
  size_t end = 0;
  size_t const lit_sz = tcmplxA_fixlist_size(literals);
  x->sequence_list.sz = 0u;
  for (i = 0u; i < lit_sz; ++i) {
    if (tcmplxA_fixlist_at_c(literals,i)->len > 0)
      end = i+1;
  }
  for (i = 0u; i < end; ++i) {
    unsigned int const n = tcmplxA_fixlist_at_c(literals,i)->len;
    if (len != n) {
      int const ae = tcmplxA_brcvt_post_sequence
        (&x->sequence_list, len, len_count);
      if (ae != tcmplxA_Success)
        return ae;
      len = n;
      len_count = 1u;
    } else len_count += 1u;
  }
  return tcmplxA_brcvt_post_sequence(&x->sequence_list, len, len_count);
}

int tcmplxA_brcvt_post_sequence
  (struct tcmplxA_blockstr* s, unsigned int len, unsigned int count)
{
  int ae = tcmplxA_Success;
  unsigned int i;
  if (count == 0u)
    return tcmplxA_Success;
  else if (count < 4u) {
    unsigned char buf[3];
    memset(buf, len, count);
    return tcmplxA_blockstr_append(s, buf, count);
  } else if (len == 0u) {
    unsigned const recount = count-3;
    int width = (int)tcmplxA_util_bitwidth(recount);
    for (i = width-width%3; i >= 0 && ae == tcmplxA_Success; i -= 3) {
      unsigned int const x = (recount>>i)&7u;
      unsigned char buf[2] = {17u};
      buf[1] = (unsigned char)x;
      ae = tcmplxA_blockstr_append(s, buf, 2u);
    }
  } else {
    /* */{
      unsigned char buf[1];
      buf[0] = (unsigned char)len;
      ae = tcmplxA_blockstr_append(s, buf, 1u);
    }
    unsigned const recount = count-3;
    int width = (int)tcmplxA_util_bitwidth(recount);
    for (i = (int)(width&~1u); i >= 0 && ae == tcmplxA_Success; i -= 2) {
      unsigned int const x = (recount>>i)&3u;
      unsigned char buf[2] = {16u};
      buf[1] = (unsigned char)x;
      ae = tcmplxA_blockstr_append(s, buf, 2u);
    }
  }
  return ae;
}


void tcmplxA_brcvt_next_block(struct tcmplxA_brcvt* ps) {
  if (ps->meta_index < tcmplxA_brmeta_size(ps->metadata))
    ps->state = tcmplxA_BrCvt_MetaStart;
  else if (ps->emptymeta)
    ps->state = tcmplxA_BrCvt_MetaStart;
  else if (!(ps->h_end&1u))
    ps->state = tcmplxA_BrCvt_Nibbles;
  else {
    ps->state = tcmplxA_BrCvt_LastCheck;
    ps->h_end |= 1u;
  }
}

int tcmplxA_brcvt_can_add_input(struct tcmplxA_brcvt const* ps) {
  if (ps->h_end&1u)
    return 0;
  else if (ps->state == tcmplxA_BrCvt_Nibbles)
    return ps->bit_length == 0;
  else return (ps->state >= tcmplxA_BrCvt_MetaStart)
    && (ps->state <= tcmplxA_BrCvt_MetaText);
}


int tcmplxA_brcvt_check_compress(struct tcmplxA_brcvt* ps) {
  unsigned int ctxt_i;
  int guess_nonzero = 0;
  size_t accum = 0;
  int blocktype_tree = tcmplxA_FixList_BrotliComplex;
  /* calculate the guesses */
  tcmplxA_uint32 ctxt_histogram[4] = {0};
  ps->guesses = tcmplxA_brcvt_guess_zero;
  tcmplxA_ctxtspan_subdivide(&ps->guesses,
    tcmplxA_blockbuf_input_data(ps->buffer), tcmplxA_blockbuf_input_size(ps->buffer),
    tcmplxA_BrCvt_Margin);
  tcmplxA_fixlist_resize(&ps->literal_blocktype, 4);
  ps->guess_offset = (ps->guesses.count)
    ? ps->guesses.modes[0] : 0;
  for (ctxt_i = 0; ctxt_i < ps->guesses.count; ++ctxt_i) {
    unsigned const mode = ps->guesses.modes[ctxt_i];
    assert(mode < 4);
    ctxt_histogram[(mode+4u-ps->guess_offset)%4u] += 1;
  }
  for (ctxt_i = 0; ctxt_i < 4u; ++ctxt_i) {
    struct tcmplxA_fixline* const line = tcmplxA_fixlist_at(&ps->literal_blocktype, ctxt_i);
    line->value = ctxt_i;
    if (ctxt_histogram[ctxt_i] > 0)
      guess_nonzero += 1;
  }
  tcmplxA_fixlist_gen_lengths(&ps->literal_blocktype, ctxt_histogram, 3);
  {
    int const res = tcmplxA_fixlist_gen_codes(&ps->literal_blocktype);
    if (res != tcmplxA_Success)
      return res;
  }
  blocktype_tree = tcmplxA_fixlist_match_preset(&ps->literal_blocktype);
  if (blocktype_tree == tcmplxA_FixList_BrotliComplex)
    return tcmplxA_ErrSanitize;
  ps->blocktype_simple = (unsigned char)blocktype_tree;
  accum += 4;
  accum += (blocktype_tree >= tcmplxA_FixList_BrotliSimple3 ? 3 : 2)
     * tcmplxA_fixlist_size(&ps->literal_blocktype);
  accum += (blocktype_tree >= tcmplxA_FixList_BrotliSimple4A);
  /* TODO the rest */
  return tcmplxA_Success;
}

int tcmplxA_brcvt_strrtozs_bits
  ( struct tcmplxA_brcvt* ps, unsigned char* y,
    unsigned char const** src, unsigned char const* src_end)
{
  unsigned char const* p = *src;
  unsigned int i;
  int ae = tcmplxA_Success;
  /* restore in-progress byte */
  *y = ps->write_scratch;
  ps->write_scratch = 0u;
  /* iterate through remaining bits */
  for (i = ps->bit_index; i < 8u && ae == tcmplxA_Success; ++i) {
    unsigned int x = 0u;
    if (tcmplxA_brcvt_can_add_input(ps)) {
      tcmplxA_uint32 const input_space =
          tcmplxA_blockbuf_capacity(ps->buffer)
        - tcmplxA_blockbuf_input_size(ps->buffer);
      size_t const src_count = src_end - p;
      size_t const min_count = (input_space < src_count)
          ? (size_t)input_space : src_count;
      ae = tcmplxA_blockbuf_write(ps->buffer, p, min_count);
      if (ae != tcmplxA_Success)
        break;
      p += min_count;
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
        tcmplxA_brcvt_next_block(ps);
        ps->bit_length = 0;
      }
      break;
    case tcmplxA_BrCvt_MetaStart:
      if (ps->bit_length == 0u) {
        int const actual_meta =
          ps->meta_index < tcmplxA_brmeta_size(ps->metadata);
        size_t const sz = actual_meta
          ? tcmplxA_brmeta_itemsize(ps->metadata, ps->meta_index)
          : 0;
        ps->metatext = actual_meta
          ? tcmplxA_brmeta_itemdata(ps->metadata, ps->meta_index)
          : NULL;
        ps->emptymeta = 0;
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
    case tcmplxA_BrCvt_LastCheck:
      if (ps->bit_length == 0u) {
        ps->bit_length = 2;
        ps->count = 0;
      }
      if (ps->count < ps->bit_length) {
        x = 1;
        ps->count += 1;
      } else x = 0;
      if (ps->count >= ps->bit_length && i==7) {
        ps->state = tcmplxA_BrCvt_Done;
        ps->bit_length = 0;
      }
      break;
    case tcmplxA_BrCvt_Nibbles:
      if (ps->bit_length == 0u) {
        size_t const input_len = tcmplxA_blockbuf_input_size(ps->buffer);
        if (input_len < tcmplxA_blockbuf_capacity(ps->buffer)
        &&  !(ps->h_end&2u))
        {
          ae = tcmplxA_ErrPartial;
          break;
        }
        ps->bit_length = 3;
        ps->backward = (tcmplxA_uint32)input_len-1u;
        if (input_len > 1048576)
          ps->bits = 4;
        else if (input_len > 65536)
          ps->bits = 2;
        else if (input_len > 0)
          ps->bits = 0;
        else if (ps->h_end&2u) { /* convert to end block */
          ps->bits = 3;
          ps->h_end |= 1u;
          ps->state = tcmplxA_BrCvt_LastCheck;
          ps->bit_length = 2;
        } else { /* convert to metadata block*/
          ps->bits = 6;
          ps->state = tcmplxA_BrCvt_MetaStart;
          ps->bit_length = tcmplxA_brcvt_MetaHeaderLen;
          ps->backward = 0;
        }
        ps->count = 0;
      }
      if (ps->count < ps->bit_length) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->state = tcmplxA_BrCvt_InputLength; /* TODO */
        ps->bit_length = (((ps->bits>>1)&3u)|4u)<<2;
        ps->count = 0;
      }
      break;
    case tcmplxA_BrCvt_InputLength:
      if (ps->count < ps->bit_length) {
        x = (ps->backward>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->state = tcmplxA_BrCvt_CompressCheck; /* TODO */
        ps->bit_length = 0;
        ps->count = 0;
      }
      break;
    case tcmplxA_BrCvt_CompressCheck:
      if (ps->bit_length == 0) {
        int const want_compress = tcmplxA_brcvt_check_compress(ps)==tcmplxA_Success;
        if (!want_compress) {
          x = 1;
          tcmplxA_blockbuf_clear_output(ps->buffer);
          ae = tcmplxA_blockbuf_noconv_block(ps->buffer);
          if (ae != tcmplxA_Success)
            break;
          ps->state = tcmplxA_BrCvt_Uncompress;
          ps->backward = tcmplxA_blockbuf_output_size(ps->buffer);
          ps->count = 0;
          assert(ps->backward);
          break;
        }
        ps->bit_length = 0;
        x = 0;
        ps->state = tcmplxA_BrCvt_BlockTypesL;
      } break;
    case tcmplxA_BrCvt_BlockTypesL:
      if (ps->bit_length == 0) {
        ps->count = 0;
        switch (tcmplxA_fixlist_size(&ps->literal_blocktype)) {
        case 0:
        case 1:
          ps->bit_length = 1;
          ps->bits = 0;
          break;
        case 2:
          ps->bit_length = 4;
          ps->bits = 1;
          break;
        case 3:
        case 4:
          ps->bit_length = 5;
          ps->bits = 3 | ((tcmplxA_fixlist_size(&ps->literal_blocktype)-3)<<4);
          break;
        default:
          ae = tcmplxA_ErrSanitize;
          break;
        }
      }
      if (ps->count < ps->bit_length) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->bit_length = 0;
        tcmplxA_fixlist_close(&ps->treety.nineteen);
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->alphabits = 2u+(tcmplxA_fixlist_size(&ps->literal_blocktype)>=3);
        if (tcmplxA_fixlist_size(&ps->literal_blocktype) > 1)
          ps->state += 1;
        else
          ps->state += 4;
      } break;
    case tcmplxA_BrCvt_BlockTypesLAlpha:
      {
        int const res = tcmplxA_brcvt_outflow19(&ps->treety, &ps->literal_blocktype, &x,
          ps->alphabits);
        if (res == tcmplxA_EOF) {
          ps->bit_length = 0;
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockCountLAlpha:
      if (ps->bit_length == 0) {
        ps->bit_length += 1;
        tcmplxA_uint32 histogram[26] = {0};
        size_t const output_len = tcmplxA_blockbuf_output_size(ps->buffer);
        size_t i;
        size_t guess_i = 0;
        size_t total = 0;
        size_t literals = 0;
        size_t coverage = 0;
        unsigned char const* const data = tcmplxA_blockbuf_output_data(ps->buffer);
        // Reparse the text.
        for (i = 0; i < output_len; ++i) {
          int const copy = (data[i]&128u);
          unsigned len = data[i]&63u;
          if (data[i] & 64u && i+1 < output_len) {
            i += 1;
            len = (len<<8) | data[i];
          }
          if (copy && i+i < output_len) {
            unsigned extend = 0;
            i += 1;
            if (data[i] < 128)
              extend = 3;
            else if (data[i] < 192)
              extend = 2;
            else extend = 4;
            if (extend >= output_len - i)
              break;
            i += (extend-1);
          } else if (len > output_len - i) {
            ae = tcmplxA_ErrSanitize;
            break;
          } else i += len;
          for (; guess_i < ps->guesses.count; ++guess_i) {
            size_t const limit = (guess_i >= ps->guesses.count-1)
              ? ps->guesses.total_bytes : ps->guesses.offsets[guess_i+1];
            size_t const remaining = total - limit;
            if (len < remaining) {
              literals += len;
              break;
            }
            len -= remaining;
            literals += remaining;
            ps->guess_lengths[guess_i] = literals;
            literals = 0;
          }
          total += len;
        }
        if (ae != tcmplxA_Success)
          break;
        assert(total == output_len);
        // Populate histogram.
        tcmplxA_inscopy_lengthsort(ps->blockcounts);
        for (i = 0; i < ps->guesses.count; ++i) {
          size_t const v = tcmplxA_inscopy_encode(ps->blockcounts, ps->guess_lengths[i], 0, 0);
          if (v >= 26) {
            ae = tcmplxA_ErrSanitize;
            break;
          }
          histogram[v] += 1;
        }
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_resize(&ps->literal_blockcount, 26);
        if (ae != tcmplxA_Success)
          break;
        for (i = 0; i < 26; ++i) {
          tcmplxA_fixlist_at(&ps->literal_blockcount, i)->value = i;
          coverage += (histogram[i] != 0);
        }
        ae = tcmplxA_fixlist_gen_lengths(&ps->literal_blockcount, histogram, 15);
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_gen_codes(&ps->literal_blockcount);
        assert(ae == tcmplxA_Success);
      }
      /* render tree to output */
      {
        int const res = tcmplxA_brcvt_outflow19(&ps->treety, &ps->literal_blockcount, &x, 5);
        if (res == tcmplxA_EOF) {
          ps->bit_length = 0;
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
          ae = tcmplxA_fixlist_valuesort(&ps->literal_blockcount);
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_BlockStartL:
      if (ps->bit_length == 0) {
        size_t const code_index =
          tcmplxA_inscopy_encode(ps->blockcounts, ps->guess_lengths[0], 0,0);
        struct tcmplxA_inscopy_row const* const row =
          tcmplxA_inscopy_at_c(ps->blockcounts, code_index);
        struct tcmplxA_fixline const* line;
        size_t line_index = 0;
        if (!row) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        line_index = tcmplxA_fixlist_valuebsearch(&ps->literal_blockcount, row->code);
        line = tcmplxA_fixlist_at_c(&ps->literal_blockcount, line_index);
        if (!line) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        ps->count = (ps->guess_lengths[0] - row->insert_first);
        ps->extra_length = row->insert_bits;
        ps->bits = line->code;
        ps->bit_cap = line->len;
      }
      if (ps->bit_length < ps->bit_cap) {
        x = (ps->bits >> (ps->bit_cap - ps->bit_length - 1u))&1u;
      } else {
        x = (ps->count >> (ps->bit_length - ps->bit_cap)) & 1u;
      }
      ps->bit_length += 1;
      if (ps->bit_length >= ps->bit_cap + ps->extra_length) {
        ps->state += 1;
        ps->bit_length = 0;
      } break;
    case tcmplxA_BrCvt_BlockTypesI:
      /* hardcode single insert-and-copy block type */
      x = 0;
      ps->state = tcmplxA_BrCvt_BlockTypesD;
      ps->bit_length = 0;
      break;
    case tcmplxA_BrCvt_BlockTypesD:
      /* hardcode single distance block type */
      x = 0;
      ps->state = tcmplxA_BrCvt_NPostfix;
      ps->bit_length = 0;
      break;
    case tcmplxA_BrCvt_Uncompress:
      x = 0;
      break;
    case tcmplxA_BrCvt_Done: /* end of stream */
      x = 0;
      ae = tcmplxA_EOF;
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
    case tcmplxA_BrCvt_InputLength:
    case tcmplxA_BrCvt_CompressCheck:
    case tcmplxA_BrCvt_BlockTypesL:
    case tcmplxA_BrCvt_BlockTypesLAlpha:
    case tcmplxA_BrCvt_BlockCountLAlpha:
    case tcmplxA_BrCvt_BlockStartL:
    case tcmplxA_BrCvt_BlockTypesI:
    case tcmplxA_BrCvt_BlockTypesIAlpha:
    case tcmplxA_BrCvt_BlockCountIAlpha:
    case tcmplxA_BrCvt_BlockStartI:
    case tcmplxA_BrCvt_BlockTypesD:
    case tcmplxA_BrCvt_BlockTypesDAlpha:
    case tcmplxA_BrCvt_BlockCountDAlpha:
    case tcmplxA_BrCvt_BlockStartD:
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
    case tcmplxA_BrCvt_Uncompress:
      if (ps->count < ps->backward) {
        if (!tcmplxA_blockbuf_bypass(ps->buffer, p, 1))
          ae = tcmplxA_ErrMemory;
        dst[ret_out] = (*p);
        ret_out += 1u;
        ps->count += 1;
      }
      if (ps->count >= ps->backward) {
        ps->metatext = NULL;
        ps->state = (ps->h_end
          ? tcmplxA_BrCvt_Done : tcmplxA_BrCvt_LastCheck);
        if (ps->h_end)
          ae = tcmplxA_EOF;
      } break;
    case 7: /* end of stream */
      ae = tcmplxA_EOF;
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
    case tcmplxA_BrCvt_LastCheck:
    case tcmplxA_BrCvt_Nibbles:
    case tcmplxA_BrCvt_InputLength:
    case tcmplxA_BrCvt_CompressCheck:
    case tcmplxA_BrCvt_BlockTypesL:
    case tcmplxA_BrCvt_BlockTypesLAlpha:
    case tcmplxA_BrCvt_BlockCountLAlpha:
    case tcmplxA_BrCvt_BlockStartL:
    case tcmplxA_BrCvt_BlockTypesI:
    case tcmplxA_BrCvt_BlockTypesIAlpha:
    case tcmplxA_BrCvt_BlockCountIAlpha:
    case tcmplxA_BrCvt_BlockStartI:
    case tcmplxA_BrCvt_BlockTypesD:
    case tcmplxA_BrCvt_BlockTypesDAlpha:
    case tcmplxA_BrCvt_BlockCountDAlpha:
    case tcmplxA_BrCvt_BlockStartD:
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
    case tcmplxA_BrCvt_Uncompress:
      if (ps->count < ps->backward)
        dst[ret_out] = tcmplxA_blockbuf_output_data(ps->buffer)[ps->count++];
      ae = tcmplxA_Success;
      if (ps->count >= ps->backward) {
        ps->metatext = NULL;
        tcmplxA_blockbuf_clear_input(ps->buffer);
        tcmplxA_brcvt_next_block(ps);
        ps->bit_length = 0;
      } break;
    case tcmplxA_BrCvt_Done:
      ae = tcmplxA_EOF;
      break;
    default:
      ae = tcmplxA_ErrSanitize;
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
int tcmplxA_brcvt_flush
  (struct tcmplxA_brcvt* ps, size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned char const tmp[1] = {0u};
  unsigned char const* tmp_src = tmp;
  /* set the flush flag: */ps->emptymeta = 1;
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
