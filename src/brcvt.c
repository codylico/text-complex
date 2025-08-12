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
#include "text-complex/access/ctxtmap.h"
#include "text-complex/access/ringdist.h"
#include "text-complex/access/zutil.h"
#include "text-complex/access/brmeta.h"
#include "text-complex/access/ctxtspan.h"
#include "text-complex/access/gaspvec.h"
#include "text-complex/access/bdict.h"
#include <string.h>
#include <limits.h>
#include <assert.h>

#if (!defined NDEBUG) && (defined tcmplxA_BrCvt_LogErr)
#include <stdio.h>
#include <stdarg.h>

#if (__STDC_VERSION__ >= 201112L && !(defined __STDC_NO_THREADS__))
_Thread_local
#endif //__STDC_NO_THREADS__
static unsigned long long int tcmplxA_brcvt_counter = 0;

static void tcmplxA_brcvt_countbits(unsigned int bits, unsigned int bit_count,
  const char* description, ...)
#if (defined __GNUC__)
  __attribute__((format(printf,3,4)))
#endif //__GNUC__
  ;
void tcmplxA_brcvt_countbits(unsigned int bits, unsigned int bit_count,
  const char* description, ...)
{
  unsigned long long int const mark = tcmplxA_brcvt_counter;
  tcmplxA_brcvt_counter += bit_count;
  fprintf(stderr, "[%#6llx,%u %#10x] ", mark/8, (unsigned)mark%8, bits);
  va_list arg;
  va_start(arg, description);
  vfprintf(stderr, description, arg);
  va_end(arg);
  fputc('\n', stderr);
  return;
}
#else
static void tcmplxA_brcvt_countbits(unsigned int bits, unsigned int bit_count,
  const char* description, ...) { return; }
#endif //NDEBUG && tcmplxA_BrCvt_LogErr

enum tcmplxA_brcvt_uconst {
  tcmplxA_brcvt_LitHistoSize = 256u,
  tcmplxA_brcvt_DistHistoSize = 68u,
  tcmplxA_brcvt_InsHistoSize = 704u,
  tcmplxA_brcvt_HistogramSize =
      tcmplxA_brcvt_LitHistoSize*4u
    + tcmplxA_brcvt_DistHistoSize
    + tcmplxA_brcvt_InsHistoSize,
  tcmplxA_brcvt_MetaHeaderLen = 6,
  tcmplxA_brcvt_CLenExtent = 18,
  tcmplxA_BrCvt_Margin = 16,
  tcmplxA_brcvt_BlockCountBits = 5,
  tcmplxA_brcvt_NoSkip = SHRT_MAX,
  tcmplxA_brcvt_TreetyOutflowMax = 4096,
  tcmplxA_brcvt_RepeatBit = 128,
  tcmplxA_brcvt_ZeroBit = 64,
  tcmplxA_brcvt_ContextHistogram = 10,
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
  tcmplxA_BrCvt_BadToken = 10,
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
  tcmplxA_BrCvt_ContextTypesL = 29,
  tcmplxA_BrCvt_TreeCountL = 30,
  tcmplxA_BrCvt_ContextRunMaxL = 31,
  tcmplxA_BrCvt_ContextPrefixL = 32,
  tcmplxA_BrCvt_ContextValuesL = 33,
  tcmplxA_BrCvt_ContextRepeatL = 34,
  tcmplxA_BrCvt_ContextInvertL = 35,
  tcmplxA_BrCvt_TreeCountD = 38,
  tcmplxA_BrCvt_ContextRunMaxD = 39,
  tcmplxA_BrCvt_ContextPrefixD = 40,
  tcmplxA_BrCvt_ContextValuesD = 41,
  tcmplxA_BrCvt_ContextRepeatD = 42,
  tcmplxA_BrCvt_ContextInvertD = 43,
  tcmplxA_BrCvt_GaspVectorL = 44,
  tcmplxA_BrCvt_GaspVectorI = 45,
  tcmplxA_BrCvt_GaspVectorD = 46,
  tcmplxA_BrCvt_DataInsertCopy = 47,
  tcmplxA_BrCvt_DoCopy = 48,
  tcmplxA_BrCvt_DataInsertExtra = 49,
  tcmplxA_BrCvt_DataCopyExtra = 50,

  tcmplxA_BrCvt_TempGap = 55,
  tcmplxA_BrCvt_Literal = 56,
  tcmplxA_BrCvt_Distance = 57,
  tcmplxA_BrCvt_LiteralRestart = 58,
  tcmplxA_BrCvt_BDict = 59,
  tcmplxA_BrCvt_InsertRestart = 60,
  tcmplxA_BrCvt_DistanceRestart = 61,
  tcmplxA_BrCvt_DataDistanceExtra = 62,
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

struct tcmplxA_brcvt_token {
  tcmplxA_uint32 first;
  unsigned short second;
  unsigned char state;
};

/**
 * @brief Demand-driven output bit negotiator.
 * @note This structure allows meta-blocks to emit zero bytes.
 */
struct tcmplxA_brcvt_forward {
  size_t i;
  tcmplxA_uint32 literal_i;
  tcmplxA_uint32 literal_total;
  tcmplxA_uint32 pos;
  /**
   * @note Used as temporary storage of copy length during
   *   inflow of Literal and LiteralRestart.
   */
  tcmplxA_uint32 stop;
  tcmplxA_uint32 accum;
  unsigned short command_span;
  unsigned char ostate;
  /**
   * @note Used as temporary storage of zero distance flag.
   */
  unsigned char ctxt_i;
  unsigned char bstore[38];
  unsigned char literal_ctxt[2];
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
  /** @brief Context transcode prefixes. */
  struct tcmplxA_fixlist context_tree;
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
  /** @brief Context map for dancing through the literals' Huffman forest. */
  struct tcmplxA_ctxtmap* literals_map;
  /** @brief The literals' Huffman forest. */
  struct tcmplxA_gaspvec* literals_forest;
  /** @brief Context map for dancing through the distances' Huffman forest. */
  struct tcmplxA_ctxtmap* distance_map;
  /** @brief The distances' Huffman forest. */
  struct tcmplxA_gaspvec* distance_forest;
  /** @brief The Huffman forest for insert-and-copy. */
  struct tcmplxA_gaspvec* insert_forest;
  /** @brief Histogram storage used for generating data trees. */
  tcmplxA_uint32* histogram;
  /** @brief ... */
  tcmplxA_uint32 bits;
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
  /**
   * @brief Block length.
   * @note[intent] Backward distance value.
   * @todo Rename this field and create a new `backward` with the original intent.
   */
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
  /** @brief Field for context map transcoding. */
  unsigned char rlemax;
  /** @brief Field for context map encoding. */
  struct tcmplxA_blockstr context_encode;
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
  /** @brief Skip code for literals. */
  unsigned short literal_skip;
  /** @brief Skip code for insert-and-copy. */
  unsigned short insert_skip;
  /** @brief Skip code for distances. */
  unsigned short distance_skip;
  /** @brief Context map prefix tree skip code. */
  unsigned short context_skip;
  /** @brief Token forwarding. */
  struct tcmplxA_brcvt_forward fwd;
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
static struct tcmplxA_brcvt_forward const tcmplxA_brcvt_fwd_zero = {0};

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
/**
 * @brief Encode a nonzero entry in a context map using run-length encoding.
 * @param[out] buffer storage of intermediate encoding
 * @param zeroes number of preceding zeroes
 * @param map_datum if nonzero, the entry to encode
 * @param[in,out] rlemax_ptr pointer to integer holding maximum RLE keyword used
 * @return Success on success, nonzero on allocation failure
 */
static int tcmplxA_brcvt_encode_map(struct tcmplxA_blockstr* buffer, size_t zeroes,
  int map_datum, unsigned* rlemax_ptr);
/**
 * @brief Prepare for the inflow of a compressed stream.
 * @param ps state to prepare
 */
static void tcmplxA_brcvt_reset_compress(struct tcmplxA_brcvt* ps);
/**
 * @brief Apply a histogram to a prefix list.
 * @param tree prefix list to replace and update
 * @param histogram item frequencies
 * @param histogram_size size of frequency array, also number of
 *   entries in the prefix list
 * @param[in,out] ae error code on failure; fast-quits if set
 * @return the number of bits that would be used to encode the prefix list
 *   @em and the stream that uses the new prefix list
 */
static size_t tcmplxA_brcvt_apply_histogram(struct tcmplxA_fixlist* tree,
  tcmplxA_uint32 const* histogram, size_t histogram_size,
  int *ae);
/**
 * @brief Get the active forest.
 * @param ps state to inspect
 * @return a pointer to a forest if available
 */
static struct tcmplxA_gaspvec* tcmplxA_brcvt_active_forest(struct tcmplxA_brcvt const* ps);
/**
 * @brief Get the active skip code.
 * @param ps state to inspect
 * @return a pointer to a skip code if available
 */
static unsigned short* tcmplxA_brcvt_active_skip(struct tcmplxA_brcvt* ps);
/**
 * @brief Generate a nonzero token to emit to output.
 * @param fwd token forwarding structure
 * @param guesses literal block switch tracker
 * @param data output data in block buffer format
 * @param size length of output data in bytes
 * @param wbits_select window size bits indirectly selected by user
 * @return a token
 */
struct tcmplxA_brcvt_token tcmplxA_brcvt_next_token
  (struct tcmplxA_brcvt_forward* fwd, struct tcmplxA_ctxtspan const* guesses,
    unsigned char const* data, size_t size, unsigned char wbits_select);
/**
 * @brief Process skip frameworks.
 * @param ps state to update
 * @param[in,out] ret write position of output buffer
 * @param[out] dst start of output buffer
 * @param[out] dstsz size of output buffer
 * @return error code or success code
 */
static int tcmplxA_brcvt_handle_inskip(struct tcmplxA_brcvt* ps,
  size_t* ret, unsigned char* dst, size_t dstsz);
/**
 * @brief Bring in the next insert command.
 * @param ps state to update
 * @param insert insert code
 * @return success code or error code
 */
static int tcmplxA_brcvt_inflow_insert(struct tcmplxA_brcvt* ps, unsigned insert);
/**
 * @brief Apply a literal byte to output.
 * @param ps state to update
 * @param ch byte value to apply
 * @param[in,out] ret write position of output buffer
 * @param[out] dst start of output buffer
 * @param[out] dstsz size of output buffer
 * @return error code or success code
 */
static int tcmplxA_brcvt_inflow_literal(struct tcmplxA_brcvt* ps, unsigned ch,
  size_t* ret, unsigned char* dst, size_t dstsz);
/**
 * @brief Bring in the next distance command.
 * @param ps state to update
 * @param distance distance code to apply
 * @return success code or error code
 */
static int tcmplxA_brcvt_inflow_distance(struct tcmplxA_brcvt* ps, unsigned distance);
/**
 * @brief Bring in the extra bits of the distance command.
 * @param ps state to update
 * @return success code or error code
 */
static int tcmplxA_brcvt_inflow_distextra(struct tcmplxA_brcvt* ps);
/**
 * @brief Try to find a value by bit string.
 * @param ps state to update with the new bit for the string
 * @param tree prefix tree to check
 * @param x next bit in the string to check
 * @return a value on success, `UINT_MAX` otherwise
 */
static unsigned tcmplxA_brcvt_inflow_lookup(struct tcmplxA_brcvt* ps,
  struct tcmplxA_fixlist const* tree, unsigned x);

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
    x->values = tcmplxA_inscopy_new(704u);
    if (x->values == NULL)
      res = tcmplxA_ErrMemory;
    else {
      int const preset_ae =
        tcmplxA_inscopy_preset(x->values, tcmplxA_InsCopy_BrotliIC);
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
    int const context_tree_res = tcmplxA_fixlist_init(&x->context_tree,0);
    x->treety = tcmplxA_brcvt_treety_zero;
    memset(x->guess_lengths, 0, sizeof(x->guess_lengths));
    assert(bltypesl_res == tcmplxA_Success);
    assert(blcountl_res == tcmplxA_Success);
    assert(bltypesi_res == tcmplxA_Success);
    assert(blcounti_res == tcmplxA_Success);
    assert(bltypesd_res == tcmplxA_Success);
    assert(blcountd_res == tcmplxA_Success);
    assert(context_tree_res == tcmplxA_Success);
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
    tcmplxA_blockstr_init(&x->context_encode, 0);
    x->literals_map = NULL;
    x->literals_forest = NULL;
    x->distance_map = NULL;
    x->distance_forest = NULL;
    x->insert_forest = NULL;
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
    x->blocktypeD_index = 0u;
    x->blocktypeD_max = 0u;
    x->blocktypeD_remaining = 0u;
    x->rlemax = 0u;
    x->blocktypeL_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountL_skip = tcmplxA_brcvt_NoSkip;
    x->blocktypeI_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountI_skip = tcmplxA_brcvt_NoSkip;
    x->blocktypeD_skip = tcmplxA_brcvt_NoSkip;
    x->blockcountD_skip = tcmplxA_brcvt_NoSkip;
    x->literal_skip = tcmplxA_brcvt_NoSkip;
    x->insert_skip = tcmplxA_brcvt_NoSkip;
    x->distance_skip = tcmplxA_brcvt_NoSkip;
    x->context_skip = tcmplxA_brcvt_NoSkip;
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
  tcmplxA_fixlist_close(&x->context_tree);
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
  tcmplxA_ctxtmap_destroy(x->literals_map);
  tcmplxA_gaspvec_destroy(x->literals_forest);
  tcmplxA_ctxtmap_destroy(x->distance_map);
  tcmplxA_gaspvec_destroy(x->distance_forest);
  tcmplxA_gaspvec_destroy(x->insert_forest);
  tcmplxA_blockstr_close(&x->context_encode);
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
  x->literal_skip = tcmplxA_brcvt_NoSkip;
  x->insert_skip = tcmplxA_brcvt_NoSkip;
  x->distance_skip = tcmplxA_brcvt_NoSkip;
  x->context_skip = tcmplxA_brcvt_NoSkip;
  x->literals_map = NULL;
  x->literals_forest = NULL;
  x->distance_map = NULL;
  x->distance_forest = NULL;
  x->insert_forest = NULL;
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

void tcmplxA_brcvt_reset_compress(struct tcmplxA_brcvt* ps) {
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
  ps->literal_skip = tcmplxA_brcvt_NoSkip;
  ps->insert_skip = tcmplxA_brcvt_NoSkip;
  ps->distance_skip = tcmplxA_brcvt_NoSkip;
}

struct tcmplxA_gaspvec* tcmplxA_brcvt_active_forest(struct tcmplxA_brcvt const* ps) {
  switch (ps->state) {
  case tcmplxA_BrCvt_GaspVectorI: return ps->insert_forest;
  case tcmplxA_BrCvt_GaspVectorD: return ps->distance_forest;
  case tcmplxA_BrCvt_GaspVectorL:
  default: return ps->literals_forest;
  }
}
unsigned short* tcmplxA_brcvt_active_skip(struct tcmplxA_brcvt* ps) {
  switch (ps->state) {
  case tcmplxA_BrCvt_GaspVectorI: return &ps->insert_skip;
  case tcmplxA_BrCvt_GaspVectorD: return &ps->distance_skip;
  case tcmplxA_BrCvt_GaspVectorL:
  default: return &ps->literal_skip;
  }
}

unsigned tcmplxA_brcvt_inflow_lookup(struct tcmplxA_brcvt* ps,
  struct tcmplxA_fixlist const* tree, unsigned x)
{
  size_t line_index = 0;
  struct tcmplxA_fixline const* line = NULL;
  if (ps->bit_length >= 15) {
    ps->state = tcmplxA_BrCvt_BadToken;
    return UINT_MAX;
  }
  ps->bits = (ps->bits<<1)|x;
  ps->bit_length += 1;
  line_index = tcmplxA_fixlist_codebsearch(tree, ps->bit_length, ps->bits);
  if (line_index >= tcmplxA_fixlist_size(tree))
    return UINT_MAX;
  return (unsigned)tcmplxA_fixlist_at_c(tree, line_index)->value;
}

int tcmplxA_brcvt_inflow_insert(struct tcmplxA_brcvt* ps, unsigned insert) {
  struct tcmplxA_inscopy_row const* const row = tcmplxA_inscopy_at_c(ps->values, insert);
  ps->blocktypeI_remaining -= 1;
  if (!row)
    return tcmplxA_ErrSanitize;
  ps->state = ((ps->blocktypeL_remaining || (row->insert_first==0))
    ? tcmplxA_BrCvt_Literal : tcmplxA_BrCvt_LiteralRestart);
  ps->extra_length = row->copy_bits;
  if (row->copy_bits)
    ps->state = tcmplxA_BrCvt_DataCopyExtra;
  if (row->insert_bits) {
    ps->extra_length = (ps->extra_length<<5) | row->insert_bits;
    ps->state = tcmplxA_BrCvt_DataInsertExtra;
  }
  ps->bits = 0;
  ps->bit_length = 0;
  ps->count = 0;
  ps->fwd.literal_i = 0;
  ps->fwd.literal_total = row->insert_first;
  ps->fwd.stop = row->copy_first;
  ps->fwd.ctxt_i = row->zero_distance_tf;
  return tcmplxA_Success;
}
int tcmplxA_brcvt_inflow_distextra(struct tcmplxA_brcvt* ps) {
  size_t const window = (size_t)((1ul<<ps->wbits_select)-16u);
  size_t const cutoff = (ps->fwd.accum > window ? window : ps->fwd.accum);
  ps->state = tcmplxA_BrCvt_DoCopy;
  ps->fwd.pos = tcmplxA_ringdist_decode(ps->ring, ps->fwd.pos, ps->bits, (tcmplxA_uint32)cutoff);
  if (ps->fwd.pos > cutoff) {
    ps->state = tcmplxA_BrCvt_BDict;
    /* RFC-7932 Section 8: */
    unsigned size = (unsigned)ps->fwd.literal_total;
    unsigned const word_id = ps->fwd.pos - (cutoff + 1);
    unsigned const word_count = tcmplxA_bdict_word_count(size);
    unsigned index;
    unsigned transform;
    unsigned char const* text;
    if (word_count == 0)
      return tcmplxA_ErrSanitize;
    index = word_id % word_count;
    transform = word_id / word_count;
    text = tcmplxA_bdict_get_word(size, index);
    if (!text)
      return tcmplxA_ErrSanitize;
    memcpy(ps->fwd.bstore, text, size);
    if (tcmplxA_bdict_transform(ps->fwd.bstore, &size, transform) != tcmplxA_Success)
      return tcmplxA_ErrSanitize;
    ps->fwd.literal_total = size;
  }
  return tcmplxA_ErrPartial;
}

int tcmplxA_brcvt_inflow_distance(struct tcmplxA_brcvt* ps, unsigned distance) {
  ps->fwd.pos = distance;
  ps->extra_length = tcmplxA_ringdist_bit_count(ps->ring, distance);
  ps->bit_length = 0;
  ps->bits = 0;
  ps->count = 0;
  if (ps->extra_length > 0) {
    ps->state = tcmplxA_BrCvt_DataDistanceExtra;
    return tcmplxA_Success;
  }
  return tcmplxA_brcvt_inflow_distextra(ps);
}

int tcmplxA_brcvt_inflow_literal(struct tcmplxA_brcvt* ps, unsigned ch,
  size_t* ret, unsigned char* dst, size_t dstsz)
{
  unsigned char const ch_byte = (unsigned char)ch;
  dst[*ret] = (unsigned char)ch;
  ps->fwd.accum += 1;
  tcmplxA_blockbuf_bypass(ps->buffer, &ch_byte, 1);
  (*ret)++;
  ps->fwd.literal_ctxt[0] = ps->fwd.literal_ctxt[1];
  ps->fwd.literal_ctxt[1] = (unsigned char)ch;
  return tcmplxA_Success;
}


int tcmplxA_brcvt_handle_inskip(struct tcmplxA_brcvt* ps,
  size_t* ret, unsigned char* dst, size_t dstsz)
{
  int skip = 1;
  long int repeat;
  struct tcmplxA_brcvt_forward *const fwd = &ps->fwd;
  for (repeat = 0; repeat < 134217728L && skip; ++repeat) {
    switch (ps->state) {
    case tcmplxA_BrCvt_DataInsertCopy:
      if (ps->insert_skip != tcmplxA_brcvt_NoSkip) {
        int const res = tcmplxA_brcvt_inflow_insert(ps, ps->insert_skip);
        tcmplxA_brcvt_countbits(0, 0, "[[INSCOPY %u]]", ps->insert_skip);
        if (res != tcmplxA_Success)
          return res;
        continue;
      } else return tcmplxA_Success;
    case tcmplxA_BrCvt_Literal:
      if (ps->fwd.literal_i >= ps->fwd.literal_total) {
        ps->state = (ps->blocktypeD_remaining
          ? tcmplxA_BrCvt_Distance : tcmplxA_BrCvt_DistanceRestart);
        ps->fwd.literal_i = 0;
        ps->fwd.literal_total = ps->fwd.stop;
        ps->fwd.stop = 0;
        ps->bit_length = 0;
        ps->bits = 0;
        tcmplxA_brcvt_countbits(0, 0, "[[-> distance%s]]",
          ps->state == tcmplxA_BrCvt_Distance ? "" : "-restart");
        continue;
      } else if (ps->literal_skip != tcmplxA_brcvt_NoSkip) {
        tcmplxA_brcvt_inflow_literal(ps, ps->literal_skip, ret, dst, dstsz);
        tcmplxA_brcvt_countbits(0, 0, "[[LITERAL %u]]", ps->literal_skip);
        continue;
      } else return tcmplxA_Success;
    case tcmplxA_BrCvt_Distance:
      if (ps->fwd.ctxt_i) {
        int const res = tcmplxA_brcvt_inflow_distance(ps, 0);
        ps->fwd.ctxt_i = 0;
        tcmplxA_brcvt_countbits(0, 0, "[[DIST -]]");
        if (res == tcmplxA_Success)
          return res;
        continue;
      } else if (ps->distance_skip != tcmplxA_brcvt_NoSkip) {
        int const res = tcmplxA_brcvt_inflow_distance(ps, ps->distance_skip);
        ps->blocktypeD_remaining -= 1;
        tcmplxA_brcvt_countbits(0, 0, "[[DIST %u]]", ps->distance_skip);
        if (res == tcmplxA_Success)
          return res;
        continue;
      } else return tcmplxA_Success;
    case tcmplxA_BrCvt_DoCopy:
      for (; fwd->literal_i < fwd->literal_total; ++fwd->literal_i) {
        unsigned char ch_byte = 0;
        if (*ret >= dstsz)
          return tcmplxA_ErrPartial;
        ch_byte = tcmplxA_blockbuf_peek(ps->buffer, ps->fwd.pos-1u);
        tcmplxA_brcvt_inflow_literal(ps, ch_byte, ret, dst, dstsz);
      }
      ps->state = (ps->blocktypeI_remaining ? tcmplxA_BrCvt_DataInsertCopy
        : tcmplxA_BrCvt_InsertRestart);
      break;
    case tcmplxA_BrCvt_BDict:
      if (fwd->literal_total > sizeof(fwd->bstore))
        return tcmplxA_ErrSanitize;
      for (; fwd->literal_i < fwd->literal_total; ++fwd->literal_i) {
        if (*ret >= dstsz)
          return tcmplxA_ErrPartial;
        tcmplxA_brcvt_inflow_literal(ps, fwd->bstore[fwd->literal_i], ret, dst, dstsz);
      }
      ps->state = (ps->blocktypeI_remaining ? tcmplxA_BrCvt_DataInsertCopy
        : tcmplxA_BrCvt_InsertRestart);
      break;
    case tcmplxA_BrCvt_DataInsertExtra:
    case tcmplxA_BrCvt_DataCopyExtra:
    case tcmplxA_BrCvt_DataDistanceExtra:
      return tcmplxA_Success;
    default:
      tcmplxA_brcvt_countbits(0, 0, "[[sanitize-fail %i]]", ps->state);
      return tcmplxA_ErrSanitize;
    }
  }
  return tcmplxA_Success;
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
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "WBITS %i", ps->wbits_select);
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
        /* prevent wraparound */
        if (ps->fwd.accum >= 16777216)
          ps->fwd.accum = 16777216;
        tcmplxA_brcvt_countbits(x, 1, "ISLAST %i", ps->h_end);
        ps->bit_length = x?4:3;
        ps->count = 0;
        ps->bits = 0;
      }
      if (ps->count == 1 && ps->h_end) {
        tcmplxA_brcvt_countbits(x, 1, "ISLASTEMPTY %u", x);
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
          tcmplxA_brcvt_countbits(ps->bits, 2, "MNIBBLES 0");
          ps->state = tcmplxA_BrCvt_MetaStart;
          ps->bit_length = 0;
        } else {
          tcmplxA_brcvt_countbits(ps->bits, 2, "MNIBBLES %i", ps->bits+4);
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
        tcmplxA_brcvt_countbits(ps->backward, ps->bit_length, "MLEN-1 %lu", (long unsigned)ps->backward);
        ps->bit_length = 0;
        ps->count = 0;
        if (ps->bit_length > 16 && (ps->backward>>(ps->bit_length-4))==0)
          ae = tcmplxA_ErrSanitize;
        ps->backward += 1;
        if (ps->h_end)
          tcmplxA_brcvt_reset_compress(ps);
        else
          ps->state = tcmplxA_BrCvt_CompressCheck;
      } break;
    case tcmplxA_BrCvt_CompressCheck:
      tcmplxA_brcvt_countbits(x, 1, "ISUNCOMPRESSED %u", x);
      if (x) {
        ps->state = tcmplxA_BrCvt_Uncompress;
      } else {
        tcmplxA_brcvt_reset_compress(ps);
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
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESL 1");
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->literal_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeL_skip = 0;
          ps->blocktypeL_max = 0;
          ps->blocktypeL_remaining = (tcmplxA_uint32)(~0ul);
          ps->bit_length = 0;
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESL %u", alphasize);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeL_max = (unsigned char)(alphasize-1u);
        }
        tcmplxA_ctxtmap_destroy(ps->literals_map);
        ps->literals_map = tcmplxA_ctxtmap_new(ps->treety.count, 64);
        if (!ps->literals_map) {
          ae = tcmplxA_ErrMemory;
          break;
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
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "literal.Block_count_code %lu", line->value);
        ps->blocktypeL_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "literal.extra_bits %u", ps->bits);
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
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESI 1");
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->insert_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeI_skip = 0;
          ps->blocktypeI_max = 0;
          ps->blocktypeI_remaining = (tcmplxA_uint32)(~0ul);
          ps->bit_length = 0;
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESI %u", alphasize);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeI_max = (unsigned char)(alphasize-1u);
        }
        tcmplxA_gaspvec_destroy(ps->insert_forest);
        ps->insert_forest = tcmplxA_gaspvec_new(ps->treety.count);
        if (!ps->insert_forest) {
          ae = tcmplxA_ErrMemory;
          break;
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
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "insert.Block_count_code %lu", line->value);
        ps->blocktypeI_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "insert.extra_bits %u", ps->bits);
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
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESD 1");
          ps->treety.count = 1;
          tcmplxA_fixlist_preset(&ps->distance_blocktype, tcmplxA_FixList_BrotliSimple1);
          ps->state += 4;
          ps->blocktypeD_skip = 0;
          ps->blocktypeD_max = 0;
          ps->blocktypeD_remaining = (tcmplxA_uint32)(~0ul);
          ps->bit_length = 0;
        } else {
          unsigned const alphasize = ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
          tcmplxA_brcvt_countbits(ps->bits, ps->count, "NBLTYPESD %u", alphasize);
          ps->treety.count = (unsigned short)alphasize;
          ps->alphabits = (unsigned char)tcmplxA_util_bitwidth(alphasize+1u); /* BITWIDTH(NBLTYPESx + 2)*/
          ps->state += 1;
          ps->blocktypeI_max = (unsigned char)(alphasize-1u);
        }
        tcmplxA_ctxtmap_destroy(ps->distance_map);
        ps->distance_map = tcmplxA_ctxtmap_new(ps->treety.count, 4);
        if (!ps->distance_map) {
          ae = tcmplxA_ErrMemory;
          break;
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
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "distance.Block_count_code %lu", line->value);
        ps->blocktypeD_remaining = tcmplxA_brcvt_config_count(ps, line->value, ps->state + 1);
      } else if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<< ps->bit_length++);
        if (ps->bit_length >= ps->extra_length) {
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "distance.extra_bits %u", ps->bits);
          ps->blocktypeD_remaining += ps->bits;
          ps->bits = 0;
          ps->bit_length = 0;
          ps->state += 1;
        }
      } else ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_NPostfix:
      if (ps->bit_length == 0)
        ps->bits = 0;
      if (ps->bit_length < 6) {
        ps->bits |= (x<<ps->bit_length);
        ps->bit_length += 1;
      }
      if (ps->bit_length >= 6) {
        unsigned const postfix = ps->bits&3;
        unsigned const direct = (ps->bits>>2)<<postfix;
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "NPOSTFIX %u NDIRECT %u", postfix, direct);
        struct tcmplxA_ringdist* const tryring = tcmplxA_ringdist_new(1,direct,postfix);
        if (!tryring) {
          tcmplxA_ringdist_destroy(tryring);
          ae = tcmplxA_ErrMemory;
          break;
        }
        ps->state = tcmplxA_BrCvt_ContextTypesL;
        tcmplxA_ringdist_destroy(ps->try_ring);
        tcmplxA_ringdist_copy(ps->ring, tryring);
        ps->try_ring = tryring;
        ps->bit_length = 0;
        ps->bits = 0;
        assert(ps->literals_map);
        ps->count = (tcmplxA_uint32)tcmplxA_ctxtmap_block_types(ps->literals_map);
        ps->index = 0;
      } break;
    case tcmplxA_BrCvt_ContextTypesL:
      if (ps->bit_length < 2) {
        ps->bits |= (x<<ps->bit_length);
        ps->bit_length += 1;
      }
      if (ps->bit_length >= 2) {
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "literal.context_mode[%lu] %u", (long unsigned)ps->index, ps->bits);
        tcmplxA_ctxtmap_set_mode(ps->literals_map, ps->index, (int)ps->bits);
        ps->index += 1;
        ps->bits = 0;
        ps->bit_length = 0;
      }
      if (ps->index >= ps->count) {
        ps->state = tcmplxA_BrCvt_TreeCountL;
        ps->bit_length = 0;
      } break;
    case tcmplxA_BrCvt_TempGap:
      if (ps->bit_length < 3)
        ps->bit_length += 1;
      if (ps->bit_length >= 3) {
        ps->state = tcmplxA_BrCvt_TreeCountL;
        ps->bit_length = 0;
      }
      break;
    case tcmplxA_BrCvt_TreeCountL:
    case tcmplxA_BrCvt_TreeCountD:
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
        int const literal = (ps->state == tcmplxA_BrCvt_TreeCountL);
        struct tcmplxA_gaspvec** const forest_ptr =
          (literal ? &ps->literals_forest : &ps->distance_forest);
        unsigned const alphasize = (ps->count == 1) ? 1u
          : ((ps->bits>>4)+(1u<<(ps->count-4))+1u);
        if (alphasize > 256 || alphasize == 0) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "NTREES%c %u", literal?'L':'D', alphasize);
        tcmplxA_gaspvec_destroy(*forest_ptr);
        *forest_ptr = tcmplxA_gaspvec_new(alphasize);
        if (!*forest_ptr) {
          ae = tcmplxA_ErrMemory;
          break;
        } else if (alphasize == 1) {
          struct tcmplxA_ctxtmap* const map =
            (literal ? ps->literals_map : ps->distance_map);
          memset(tcmplxA_ctxtmap_data(map), 0,
            tcmplxA_ctxtmap_block_types(map) * tcmplxA_ctxtmap_contexts(map));
          ps->state = (literal ? tcmplxA_BrCvt_TreeCountD : tcmplxA_BrCvt_GaspVectorL);
          ps->index = 0;
          ps->bit_length = 0;
        } else {
          ps->state += 1;
          ps->bit_length = 0;
          ps->rlemax = 0;
        }
      } break;
    case tcmplxA_BrCvt_ContextRunMaxL:
    case tcmplxA_BrCvt_ContextRunMaxD:
      if (ps->bit_length == 0) {
        ps->count = 1;
        ps->bits = x;
        ps->bit_length = (x ? 5 : 1);
      } else if (ps->count < ps->bit_length) {
        ps->bits |= ((x&1u)<<(ps->count++));
      }
      if (ps->count >= ps->bit_length) {
        struct tcmplxA_gaspvec const* const forest = (ps->state == tcmplxA_BrCvt_ContextRunMaxL)
          ? ps->literals_forest : ps->distance_forest;
        size_t const ntrees = forest ? tcmplxA_gaspvec_size(forest) : 0;
        ps->rlemax = (ps->bits ? (ps->bits>>1)+1u : 0u);
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "context.RLEMAX %u", ps->rlemax);
        ps->bit_length = 0;
        ps->state += 1;
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->treety.count = (unsigned short)(ps->rlemax + ntrees);
        ps->alphabits = (unsigned char)(ps->rlemax + ntrees);
      } break;
    case tcmplxA_BrCvt_ContextPrefixL:
    case tcmplxA_BrCvt_ContextPrefixD:
      {
        int const res = tcmplxA_brcvt_inflow19(&ps->treety, &ps->context_tree, x,
          ps->alphabits);
        if (res == tcmplxA_EOF) {
          struct tcmplxA_ctxtmap* const map = (ps->state == tcmplxA_BrCvt_ContextPrefixL)
            ? ps->literals_map : ps->distance_map;
          ps->context_skip = tcmplxA_brcvt_resolve_skip(&ps->context_tree);
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->bit_length = 0;
          ps->bits = 0;
          ps->index = 0;
          ps->count = (!map) ? 0
            : tcmplxA_ctxtmap_block_types(map)*tcmplxA_ctxtmap_contexts(map);
          if (ps->context_skip == tcmplxA_brcvt_NoSkip) {
            ps->state += 1;
          } else if (ps->context_skip == 0 || ps->context_skip > ps->rlemax) {
            unsigned char const fill = (ps->context_skip ?
              ps->context_skip - ps->rlemax : 0);
            if (map)
              memset(tcmplxA_ctxtmap_data(map), fill, ps->count*sizeof(char));
            ps->state += 3;
          } else {
            ps->extra_length = ps->context_skip;
            ps->state += 2;
          }
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_ContextValuesL:
    case tcmplxA_BrCvt_ContextValuesD:
      if (ps->bit_length < 16) {
        struct tcmplxA_ctxtmap* const map = (ps->state == tcmplxA_BrCvt_ContextValuesL)
          ? ps->literals_map : ps->distance_map;
        size_t line_index = 0;
        struct tcmplxA_fixline const* line = NULL;
        ps->bits = (ps->bits<<1)|x;
        ps->bit_length += 1;
        line_index = tcmplxA_fixlist_codebsearch(&ps->context_tree, ps->bit_length, ps->bits);
        if (line_index >= tcmplxA_fixlist_size(&ps->context_tree))
          break;
        line = tcmplxA_fixlist_at_c(&ps->context_tree, line_index);
        if (line->value == 0 || line->value > ps->rlemax) {
          /* single value */
          unsigned char const value = (unsigned char)(line->value ? line->value-ps->rlemax : 0);
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "context.value[%lu] (%lu)->%u",
            (long unsigned)ps->index, line->value, value);
          tcmplxA_ctxtmap_data(map)[ps->index] = value;
          ps->index += 1;
          if (ps->index >= ps->count)
            ps->state += 2;
        } else {
          tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "context.value[%lu..] (%lu)->zero",
            (long unsigned)ps->index, line->value);
          ps->extra_length = line->value;
          ps->state += 1;
        }
        ps->bits = 0;
        ps->bit_length = 0;
      }
      if (ps->bit_length >= 16)
        ae = tcmplxA_ErrSanitize;
      break;
    case tcmplxA_BrCvt_ContextRepeatL:
    case tcmplxA_BrCvt_ContextRepeatD:
      if (ps->bit_length < ps->extra_length) {
        ps->bits |= (x<<ps->bit_length);
        ps->bit_length += 1;
      }
      if (ps->bit_length >= ps->extra_length) {
        struct tcmplxA_ctxtmap* const map = (ps->state == tcmplxA_BrCvt_ContextRepeatL)
          ? ps->literals_map : ps->distance_map;
        size_t const total = ps->bits | (1ul<<ps->extra_length);
        tcmplxA_brcvt_countbits(ps->bits, ps->extra_length, "... x%u", (unsigned)total);
        if (total > ps->count - ps->index) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        memset(tcmplxA_ctxtmap_data(map)+ps->index, 0, total*sizeof(char));
        ps->index += total;
        ps->bits = 0;
        ps->bit_length = 0;
        if (ps->index >= ps->count)
          ps->state += 1; /* IMTF bit */
        else if (ps->context_skip == tcmplxA_brcvt_NoSkip)
          ps->state -= 1; /* next */
        else/* stay put and */break;
      } break;
    case tcmplxA_BrCvt_ContextInvertL:
    case tcmplxA_BrCvt_ContextInvertD:
      tcmplxA_brcvt_countbits(x, 1, "context.IMTF %u", x);
      {
        int const literals = (ps->state == tcmplxA_BrCvt_ContextInvertL);
        if (x) {
          struct tcmplxA_ctxtmap* const map = literals
            ? ps->literals_map : ps->distance_map;
          tcmplxA_ctxtmap_revert_movetofront(map);
        }
        ps->state = (literals ? tcmplxA_BrCvt_TreeCountD : tcmplxA_BrCvt_GaspVectorL);
        ps->index = 0;
        ps->bits = 0;
        ps->bit_length = 0;
      } break;
    case tcmplxA_BrCvt_GaspVectorL:
    case tcmplxA_BrCvt_GaspVectorI:
    case tcmplxA_BrCvt_GaspVectorD:
      if (ps->bit_length == 0) {
        ps->bit_length = 1;
        tcmplxA_brcvt_reset19(&ps->treety);
        if (ps->state == tcmplxA_BrCvt_GaspVectorD) {
          ps->treety.count = (16 + tcmplxA_ringdist_get_direct(ps->ring)
            + (48 << tcmplxA_ringdist_get_postfix(ps->ring)));
        } else ps->treety.count = ((ps->state == tcmplxA_BrCvt_GaspVectorL) ? 256 : 704);
        ps->alphabits = tcmplxA_util_bitwidth(ps->treety.count-1);
      }
      if (ps->bit_length > 0) {
        struct tcmplxA_brcvt_treety* const treety = &ps->treety;
        struct tcmplxA_gaspvec* const forest = tcmplxA_brcvt_active_forest(ps);
        struct tcmplxA_fixlist* const tree = tcmplxA_gaspvec_at(forest, ps->index);
        int const res = tcmplxA_brcvt_inflow19(treety, tree, x, ps->alphabits);
        if (res == tcmplxA_EOF) {
          unsigned short const noskip = tcmplxA_brcvt_resolve_skip(tree);
          tcmplxA_gaspvec_set_skip(forest, ps->index, noskip);
          if (ps->index == 0)
            *tcmplxA_brcvt_active_skip(ps) = noskip;
          ps->bit_length = 0;
          ps->index += 1;
          ps->bits = 0;
          if (ps->index >= tcmplxA_gaspvec_size(forest)) {
            tcmplxA_uint32 const old_accum = ps->fwd.accum;
            ps->state += 1;
            ps->index = 0;
            ps->fwd = tcmplxA_brcvt_fwd_zero;
            ps->fwd.accum = old_accum;
            if (ps->state != tcmplxA_BrCvt_DataInsertCopy || ps->insert_skip == tcmplxA_brcvt_NoSkip)
              break;
            ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
          }
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_DataInsertCopy:
      {
        unsigned const line = tcmplxA_brcvt_inflow_lookup(ps,
          tcmplxA_gaspvec_at(ps->insert_forest, ps->blocktypeI_index), x);
        if (line >= 704)
          break;
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "insert-and-copy %u", line);
        tcmplxA_brcvt_inflow_insert(ps, line);
        ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
      } break;
    case tcmplxA_BrCvt_DataInsertExtra:
      if (ps->count < (ps->extra_length&31)) {
        ps->bits |= (((tcmplxA_uint32)x)<<ps->count);
        ps->count++;
      }
      if (ps->count >= (ps->extra_length&31)) {
        tcmplxA_brcvt_countbits(ps->bits, ps->extra_length&31u, "insert-extra %u", ps->bits);
        ps->fwd.literal_total += ps->bits;
        ps->extra_length >>= 5;
        ps->bits = 0;
        ps->count = 0;
        if (ps->extra_length > 0)
          ps->state = tcmplxA_BrCvt_DataCopyExtra;
        else {
          ps->state = (ps->blocktypeL_remaining
            ? tcmplxA_BrCvt_Literal : tcmplxA_BrCvt_LiteralRestart);
          ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
        }
      } break;
    case tcmplxA_BrCvt_DataCopyExtra:
      if (ps->count < ps->extra_length) {
        ps->bits |= (((tcmplxA_uint32)x)<<ps->count);
        ps->count++;
      }
      if (ps->count >= ps->extra_length) {
        tcmplxA_brcvt_countbits(ps->bits, ps->extra_length, "copy-extra %u", ps->bits);
        ps->fwd.stop += ps->bits;
        ps->bits = 0;
        ps->state = (ps->blocktypeL_remaining
          ? tcmplxA_BrCvt_Literal : tcmplxA_BrCvt_LiteralRestart);
        ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
      } break;
    case tcmplxA_BrCvt_Literal:
      {
        int const mode = tcmplxA_ctxtmap_get_mode(ps->literals_map, ps->blocktypeL_index);
        int const column = tcmplxA_ctxtmap_literal_context(mode, ps->fwd.literal_ctxt[1],
          ps->fwd.literal_ctxt[0]);
        int const index = tcmplxA_ctxtmap_get(ps->literals_map, ps->blocktypeL_index, column);
        unsigned const line = tcmplxA_brcvt_inflow_lookup(ps,
          tcmplxA_gaspvec_at(ps->literals_forest, index), x);
        if (line >= 256)
          break;
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "literal %u (tree %i)", line, index);
        tcmplxA_brcvt_inflow_literal(ps, line, &ret_out, dst, dstsz);
        ps->fwd.literal_i ++;
        ps->bit_length = 0;
        ps->bits = 0;
        ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
      } break;
    case tcmplxA_BrCvt_Distance:
      {
        int const column = tcmplxA_ctxtmap_distance_context(ps->fwd.literal_total);
        int const index = tcmplxA_ctxtmap_get(ps->distance_map, ps->blocktypeD_index, column);
        unsigned const line = tcmplxA_brcvt_inflow_lookup(ps,
          tcmplxA_gaspvec_at(ps->distance_forest, index), x);
        int res;
        if (line >= 520)
          break;
        tcmplxA_brcvt_countbits(ps->bits, ps->bit_length, "distance-code %u", line);
        res = tcmplxA_brcvt_inflow_distance(ps, line);
        if (res == tcmplxA_ErrPartial)
          ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
      } break;
    case tcmplxA_BrCvt_DataDistanceExtra:
      if (ps->count < ps->extra_length) {
        ps->bits |= (((tcmplxA_uint32)x)<<ps->count);
        ps->count++;
      }
      if (ps->count >= ps->extra_length) {
        tcmplxA_brcvt_countbits(ps->bits, ps->extra_length, "distance-extra %u", ps->bits);
        tcmplxA_brcvt_inflow_distextra(ps);
        ps->extra_length = 0;
        ps->bit_length = 0;
        ps->bits = 0;
        ps->count = 0;
        ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
      } break;
    case tcmplxA_BrCvt_DoCopy:
    case tcmplxA_BrCvt_BDict:
      ae = tcmplxA_brcvt_handle_inskip(ps, &ret_out, dst, dstsz);
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
            ps->backward = tcmplxA_ringdist_decode(ps->ring, alpha, 0u, 0);
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
          (ps->ring, ps->backward, ps->bits, 0);
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
      tcmplxA_brcvt_countbits(treety->bits, treety->bit_length, "prefix.HSKIP %u", treety->bits);
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
      tcmplxA_brcvt_countbits(treety->bits, treety->bit_length, "prefix.NSYM %u", new_count);
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
      tcmplxA_brcvt_countbits(treety->bits, treety->bit_length, "prefix.symbol %u", treety->bits);
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
    tcmplxA_brcvt_countbits(x, 1, "prefix.tree_select %u", x);
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
      tcmplxA_brcvt_countbits(treety->bits, treety->bit_length, "prefix.Nineteen_length[[%u]->%u] %u",
        treety->index, tcmplxA_brcvt_clen[treety->index], len);
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
      tcmplxA_brcvt_countbits(treety->bits, treety->bit_length, "... x%u", treety->bits);
      if (res != tcmplxA_Success)
        return res;
      treety->state = tcmplxA_BrCvt_TSymbols;
      treety->bit_length = 0;
      treety->bits = 0;
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
      if (code_index < tcmplxA_fixlist_size(&treety->nineteen)) {
        long unsigned const value =
          tcmplxA_fixlist_at_c(&treety->nineteen, code_index)->value;
        unsigned const bits = treety->bits;
        unsigned const bit_length = treety->bit_length;
        unsigned const index = treety->index;
        int const res = tcmplxA_brcvt_post19(treety, prefixes, value);
        tcmplxA_brcvt_countbits(bits, bit_length, "prefix.Symbol[%u] = %lu", index, value);
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
int tcmplxA_brcvt_zerofill(struct tcmplxA_brcvt_treety* treety,
  struct tcmplxA_fixlist* prefixes)
{
  unsigned i;
  for (i = treety->index; i < treety->count; ++i) {
    struct tcmplxA_fixline* const line = tcmplxA_fixlist_at(prefixes, i);
    line->len = 0;
    line->value = i;
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
      treety->bits = 0;
      treety->bit_length = 0;
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
    int const fill = tcmplxA_brcvt_zerofill(treety, prefixes);
    int const sort_res = tcmplxA_fixlist_valuesort(prefixes);
    int const res = tcmplxA_fixlist_gen_codes(prefixes);
    int const code_res = tcmplxA_fixlist_codesort(prefixes);
    treety->state = tcmplxA_BrCvt_TDone;
    if (sort_res != tcmplxA_Success)
      return sort_res;
    else if (res != tcmplxA_Success)
      return res;
    return (code_res == tcmplxA_Success) ? tcmplxA_EOF : (code_res);
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

struct tcmplxA_brcvt_token tcmplxA_brcvt_next_token
  (struct tcmplxA_brcvt_forward* fwd, struct tcmplxA_ctxtspan const* guesses,
    unsigned char const* data, size_t size, unsigned char wbits_select)
{
  struct tcmplxA_brcvt_token out = {0};
  if (fwd->ostate == 0) {
    fwd->stop = (tcmplxA_uint32)(guesses->count > 1
      ? guesses->offsets[1] : guesses->total_bytes);
    fwd->ostate = tcmplxA_BrCvt_DataInsertCopy;
  }
  if (fwd->i >= size)
    return out;
  else if (fwd->pos >= fwd->stop && fwd->i < size) {
    fwd->ctxt_i += 1;
    for (; fwd->ctxt_i < guesses->count; ++fwd->ctxt_i) {
      fwd->stop = (tcmplxA_uint32)(fwd->ctxt_i+1 < guesses->count
        ? guesses->offsets[fwd->ctxt_i+1] : guesses->total_bytes);
      if (fwd->pos < fwd->stop) {
        out.state = tcmplxA_BrCvt_LiteralRestart;
        return out;
      }
    }
  }
  switch (fwd->ostate) {
  case tcmplxA_BrCvt_DataInsertCopy:
    {
      /* acquire all inserts */
      tcmplxA_uint32 total = 0;
      size_t next_i;
      tcmplxA_uint32 literals = 0;
      tcmplxA_uint32 first_literals = 0;
      out.state = tcmplxA_BrCvt_DataInsertCopy;
      /* compose insert length */
      for (next_i = fwd->i; next_i < size; ++next_i) {
        unsigned short next_span = 0;
        unsigned char const ch = data[next_i];
        if (literals) {
          literals -= 1;
          continue;
        } else if (ch & 128u)
          break;
        next_span = (ch & 63u);
        if (ch & 64u) {
          assert(next_i < size-1u);
          next_i += 1;
          next_span = (next_span << 8) + data[next_i] + 64u;
        }
        if (next_span == 0 && first_literals == 0) {
          fwd->i = next_i+1;
          if (fwd->i >= size) {
            out.state = 0;
            fwd->ostate = tcmplxA_BrCvt_Done;
            return out;
          }
          continue;
        }
        if (!first_literals)
          first_literals = next_span;
        literals = next_span;
        assert(literals <= 16777216-total);
        total += literals;
      }
      out.first = total;
      fwd->accum += total;
      if (fwd->accum >= 16777200u)
        fwd->accum = 16777200u;
      if (next_i >= size)
        break;
      /* parse copy length */{
        unsigned char const ch = data[next_i];
        unsigned short next_span = 0;
        assert(ch & 128u);
        next_span = (ch & 63u);
        if (ch & 64u) {
          assert(next_i < size-1u);
          next_i += 1;
          next_span = (next_span << 8) + data[next_i] + 64u;
        }
        out.second = next_span;
      }
      fwd->i += (1 + ((data[fwd->i]&64u)!=0));
      if (out.first == 0)
        fwd->ostate = tcmplxA_BrCvt_Distance;
      else
        fwd->ostate = tcmplxA_BrCvt_Literal;
      fwd->literal_i = 0;
      fwd->command_span = (unsigned short)first_literals;
      fwd->literal_total = total;
    } break;
  case tcmplxA_BrCvt_Literal:
    out.state = tcmplxA_BrCvt_Literal;
    out.first = data[fwd->i];
    fwd->i += 1;
    fwd->literal_i += 1;
    if (fwd->i >= size)
      fwd->ostate = tcmplxA_BrCvt_Done;
    else if (fwd->literal_i >= fwd->literal_total) {
      /* check for copy count */
      unsigned char const ch = data[fwd->i];
      unsigned short next_span = (ch & 63u);
      if (ch & 64u) {
        assert(fwd->i < size-1u);
        fwd->i += 1;
        next_span = (next_span << 8) + data[fwd->i] + 64u;
      }
      fwd->i += 1;
      fwd->ostate = tcmplxA_BrCvt_Distance;
      fwd->pos += next_span;
      fwd->command_span = next_span;
    } else if (fwd->literal_i >= fwd->command_span) {
      size_t next_i;
      assert(fwd->command_span <= fwd->literal_total);
      fwd->literal_total -= fwd->command_span;
      fwd->literal_i = 0;
      for (next_i = fwd->i; next_i < size; ++next_i) {
        unsigned short next_span = 0;
        unsigned char const ch = data[next_i];
        if (ch & 128u) {
          out.state = tcmplxA_BrCvt_BadToken;
          return out;
        } else if ((ch & 63u) == 0u)
          continue;
        next_span = (ch & 63u);
        if (ch & 64u) {
          assert(next_i < size-1u);
          next_i += 1;
          next_span = (next_span << 8) + data[next_i] + 64u;
        }
        fwd->i = next_i+1;
        fwd->command_span = next_span;
        break;
      }
    } break;
  case tcmplxA_BrCvt_Distance:
    {
      unsigned char const root = data[fwd->i];
      if (root < 128) {
        /* bdict command */
        tcmplxA_uint32 const past_window = (tcmplxA_uint32)((1ul<<wbits_select)-16ul);
        unsigned const n_words = tcmplxA_bdict_word_count(fwd->command_span);
        unsigned short const filter = root & 127u;
        tcmplxA_uint32 word_id = filter * n_words;
        tcmplxA_uint32 past_counter = 0;
        unsigned selector = 0;
        if (n_words == 0 || size < 3 || fwd->i > size-3) {
          out.state = tcmplxA_BrCvt_BadToken;
          break;
        }
        fwd->i += 1;
        selector = (data[fwd->i]<<8)|data[fwd->i+1];
        if (selector >= n_words) {
          out.state = tcmplxA_BrCvt_BadToken;
          break;
        }
        fwd->i += 2;
        word_id += selector;
        past_counter = (fwd->accum > past_window) ? past_window : fwd->accum;
        out.state = tcmplxA_BrCvt_BDict;
        out.first = past_counter + word_id;
      } else {
        tcmplxA_uint32 distance = 0;
        unsigned const byte_count = (root&64u) ? 4 : 2;
        unsigned j;
        if (size < byte_count || fwd->i > size-byte_count) {
          out.state = tcmplxA_BrCvt_BadToken;
          break;
        }
        for (j = 0; j < byte_count; ++j, ++fwd->i) {
          unsigned const digit = data[fwd->i] & (j ? 255u : 63u);
          distance = (distance<<8) | digit;
        }
        out.state = tcmplxA_BrCvt_Distance;
        out.first = distance + ((root&64u)<<8); /* +16384 when 30-bit sequence */
      }
      fwd->accum += fwd->command_span;
      fwd->ostate = (fwd->i >= size ? tcmplxA_BrCvt_Done : tcmplxA_BrCvt_DataInsertCopy);
    } break;
  default:
    out.state = tcmplxA_BrCvt_BadToken;
    break;
  }
  return out;
}

int tcmplxA_brcvt_check_compress(struct tcmplxA_brcvt* ps) {
  unsigned int ctxt_i;
  int guess_nonzero = 0;
  size_t accum = 0;
  size_t btypes = 0;
  size_t btype_j;
  size_t try_bit_count = 0;
  int blocktype_tree = tcmplxA_FixList_BrotliComplex;
  /* calculate the guesses */
  tcmplxA_uint32 ctxt_histogram[4] = {0};
  if (tcmplxA_inscopy_lengthsort(ps->values) != tcmplxA_Success)
    return tcmplxA_ErrInit;
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
  btypes = tcmplxA_fixlist_size(&ps->literal_blocktype);
  accum += (blocktype_tree >= tcmplxA_FixList_BrotliSimple3 ? 3 : 2) * btypes;
  accum += (blocktype_tree >= tcmplxA_FixList_BrotliSimple4A);
  if (ps->literals_map && tcmplxA_ctxtmap_block_types(ps->literals_map)!=btypes) {
    tcmplxA_ctxtmap_destroy(ps->literals_map);
    ps->literals_map = tcmplxA_ctxtmap_new(btypes, 64);
    if (!ps->literals_map)
      return tcmplxA_ErrMemory;
  }
  for (btype_j = 0; btype_j < btypes; ++btype_j) {
    struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(&ps->literal_blocktype, btype_j);
    tcmplxA_ctxtmap_set_mode(ps->literals_map, btype_j, (int)line->value);
    for (ctxt_i = 0; ctxt_i < 64; ++ctxt_i)
      tcmplxA_ctxtmap_set(ps->literals_map, btype_j, ctxt_i, (int)btype_j);
  }
  ps->context_encode.sz = 0;
  /* prepare the fixed-size forests */{
    if (!ps->insert_forest) {
      ps->insert_forest = tcmplxA_gaspvec_new(1);
      if (!ps->insert_forest)
        return tcmplxA_ErrMemory;
    }
    if (!ps->distance_forest) {
      ps->distance_forest = tcmplxA_gaspvec_new(1);
      if (!ps->distance_forest)
        return tcmplxA_ErrMemory;
    }
  }
  /* prepare the variable-size forest */{
    if ((!ps->literals_forest) || tcmplxA_gaspvec_size(ps->literals_forest) != btypes) {
      tcmplxA_gaspvec_destroy(ps->literals_forest);
      ps->literals_forest = tcmplxA_gaspvec_new(btypes);
      if (!ps->literals_forest)
        return tcmplxA_ErrMemory;
    }
  }
  /* fill the histograms */{
    tcmplxA_uint32 *const insert_histogram = ps->histogram;
    tcmplxA_uint32 *const distance_histogram = insert_histogram+tcmplxA_brcvt_InsHistoSize;
    tcmplxA_uint32 *literal_histograms[4] = {NULL};
    tcmplxA_uint32 literal_lengths[tcmplxA_CtxtSpan_Size+1] = {0};
    struct tcmplxA_brcvt_forward try_fwd = {0};
    size_t const size = tcmplxA_blockbuf_output_size(ps->buffer);
    size_t i = 0;
    tcmplxA_uint32 literal_counter = 0;
    tcmplxA_uint32 next_copy = 0;
    unsigned char const* const data = tcmplxA_blockbuf_output_data(ps->buffer);
    int ae = tcmplxA_Success;
    //TODO: try_fwd.accum = ps->fwd.accum;
    /* compute histogram addresses for literals */{
      tcmplxA_uint32 *const literal_start = distance_histogram+tcmplxA_brcvt_DistHistoSize;
      for (btype_j = 0; btype_j < 4u; ++btype_j)
        literal_histograms[btype_j] = literal_start+tcmplxA_brcvt_LitHistoSize*btype_j;
    }
    ctxt_i = 0;
    memset(ps->histogram, 0, sizeof(tcmplxA_uint32)*tcmplxA_brcvt_HistogramSize);
    for (i = 0; i < size; ++i) {
      struct tcmplxA_brcvt_token next =
        tcmplxA_brcvt_next_token(&try_fwd, &ps->guesses, data, size, ps->wbits_select);
      if (try_fwd.i <= i)
        return tcmplxA_ErrSanitize;
      i = try_fwd.i-1;
      switch (next.state) {
      case tcmplxA_BrCvt_DataInsertCopy:
        /* */{
          size_t const icv = tcmplxA_inscopy_encode(ps->values, next.first,
            next.second ? next.second : 2, 0);
          struct tcmplxA_inscopy_row const* const icv_row =
            tcmplxA_inscopy_at_c(ps->values, icv);
          if (!icv_row)
            return tcmplxA_ErrSanitize;
          assert(icv_row->code < 704u);
          insert_histogram[icv_row->code] += 1;
          try_bit_count += icv_row->insert_bits;
          try_bit_count += icv_row->copy_bits;
          next_copy = next.second;
        } break;
      case tcmplxA_BrCvt_LiteralRestart:
        assert(ctxt_i < tcmplxA_CtxtSpan_Size);
        literal_lengths[ctxt_i] = literal_counter;
        literal_counter = 0;
        ctxt_i = try_fwd.ctxt_i;
        break;
      case tcmplxA_BrCvt_Literal:
        /* */{
          tcmplxA_uint32* const hist = literal_histograms[ps->guesses.modes[ctxt_i]];
          hist[next.first&255u] += 1;
        } break;
      case tcmplxA_BrCvt_Distance:
      case tcmplxA_BrCvt_BDict:
        /* */{
          int const to_record = (next.state==tcmplxA_BrCvt_Distance);
          tcmplxA_uint32 extra = 0;
          // TODO: use `to_record`
          unsigned const cmd = tcmplxA_ringdist_encode(ps->try_ring, next.first, &extra,
            to_record ? 0xFFffFFff : 0);
          if (cmd >= tcmplxA_brcvt_DistHistoSize)
            return tcmplxA_ErrSanitize;
          try_bit_count += extra;
          distance_histogram[cmd] += 1;
        } break;
      default:
        return tcmplxA_ErrSanitize;
      }
    }
    assert(ctxt_i <= tcmplxA_CtxtSpan_Size);
    literal_lengths[ctxt_i] = literal_counter;
    /* apply histograms to the trees */
    try_bit_count += tcmplxA_brcvt_apply_histogram(
      tcmplxA_gaspvec_at(ps->distance_forest,0), distance_histogram,
      tcmplxA_brcvt_DistHistoSize, &ae);
    try_bit_count += tcmplxA_brcvt_apply_histogram(
      tcmplxA_gaspvec_at(ps->insert_forest,0), insert_histogram,
      tcmplxA_brcvt_InsHistoSize, &ae);
    for (btype_j = 0; btype_j < btypes; ++btype_j) {
      int const btype = tcmplxA_ctxtmap_get_mode(ps->literals_map, btype_j);
      try_bit_count += tcmplxA_brcvt_apply_histogram(
        tcmplxA_gaspvec_at(ps->literals_forest,btype_j),
        literal_histograms[btype],
        tcmplxA_brcvt_LitHistoSize, &ae);
    }
  }
  if (try_bit_count/8+1 > tcmplxA_blockbuf_input_size(ps->buffer))
    return tcmplxA_ErrBlockOverflow;
  return tcmplxA_Success;
}

size_t tcmplxA_brcvt_apply_histogram(struct tcmplxA_fixlist* tree,
  tcmplxA_uint32 const* histogram, size_t histogram_size,
  int *ae)
{
  size_t i;
  size_t bit_count;
  unsigned const alphabits = tcmplxA_util_bitwidth((unsigned)(histogram_size-1));
  struct tcmplxA_brcvt_treety attempt = {0};
  if (*ae)
    return 0;
  if (tcmplxA_fixlist_size(tree) != histogram_size) {
    *ae = tcmplxA_fixlist_resize(tree, histogram_size);
    if (*ae)
      return 0;
  }
  for (i = 0; i < histogram_size; ++i) {
    struct tcmplxA_fixline* const line = tcmplxA_fixlist_at(tree, i);
    line->value = (unsigned)i;
  }
  *ae = tcmplxA_fixlist_gen_lengths(tree, histogram, 15);
  if (*ae)
    return 0;
  *ae = tcmplxA_fixlist_gen_codes(tree);
  if (*ae)
    return 0;
  for (i = 0; i < histogram_size; ++i) {
    struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(tree, i);
    bit_count += line->len * (size_t)histogram[i];
  }
  for (i = 0; i < tcmplxA_brcvt_TreetyOutflowMax; ++i) {
    unsigned sink = 0;
    int const res = tcmplxA_brcvt_outflow19(&attempt, tree, &sink, alphabits);
    if (res == tcmplxA_EOF)
      break;
    else if (res != tcmplxA_Success) {
      *ae = res;
      tcmplxA_brcvt_close19(&attempt);
      return 0;
    } else bit_count += 1;
  }
  tcmplxA_brcvt_close19(&attempt);
  return bit_count;
}


int tcmplxA_brcvt_encode_map(struct tcmplxA_blockstr* buffer, size_t zeroes,
    int map_datum, unsigned* rlemax_ptr)
{
  unsigned char code[3] = {0};
  int len = 0;
  if (zeroes > 0) {
    code[len] = tcmplxA_util_bitwidth((unsigned)zeroes)-1u;
    len += 1;
    if (zeroes > 1) {
      if (code[0] > *rlemax_ptr)
        *rlemax_ptr = code[0];
      code[0] |= tcmplxA_brcvt_ZeroBit;
      code[len] = (unsigned char)(((1u<<code[0])-1u)&zeroes);
      len += 1;
    }
  }
  if (map_datum) {
    code[len] = map_datum;
    len += 1;
  }
  return tcmplxA_blockstr_append(buffer, code, len);
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
#if (!defined NDEBUG) && (defined tcmplxA_BrCvt_LogErr)
        tcmplxA_brcvt_counter = 0;
#endif //NDEBUG && tcmplxA_BrCvt_LogErr
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
          //TODO: ps->fwd.accum += ps->backward;
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
    case tcmplxA_BrCvt_NPostfix:
      if (ps->bit_length == 0) {
        unsigned const postfix = tcmplxA_ringdist_get_postfix(ps->ring);
        unsigned const direct = tcmplxA_ringdist_get_direct(ps->ring);
        if (postfix > 3 || (direct & ((1u<<postfix)-1u))) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        ps->bits = postfix|(direct>>postfix);
      }
      if (ps->bit_length < 6) {
        x = (ps->bits>>ps->bit_length)&1u;
        ps->bit_length += 1;
      }
      if (ps->bit_length >= 6) {
        ps->state = tcmplxA_BrCvt_ContextTypesL;
        ps->bit_length = 0;
        ps->count = 0;
      } break;
    case tcmplxA_BrCvt_ContextTypesL:
      if (ps->bit_length == 0) {
        size_t const contexts = tcmplxA_fixlist_size(&ps->literal_blocktype);
        size_t i;
        for (i = 0; i < contexts; ++i) {
          struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(&ps->literal_blocktype, i);
          ps->bits |= ((line->value&3u)<<ps->bit_length);
          ps->bit_length += 2;
        }
      }
      if (ps->count < ps->bit_length) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        ps->state = tcmplxA_BrCvt_TreeCountL;
        ps->bit_length = 0;
        ps->count = 0;
      } break;
    case tcmplxA_BrCvt_TreeCountL:
      if (ps->bit_length == 0) {
        size_t const btypes = tcmplxA_fixlist_size(&ps->literal_blocktype);
        ps->count = 0;
        switch (btypes) {
        case 1:
          ps->bits = 0;
          ps->bit_length = 1;
          break;
        case 2:
          ps->bits = 1;
          ps->bit_length = 4;
          break;
        default:
          ps->bits = 3|(((btypes&1u)^1u)<<4);
          ps->bit_length = 5;
          break;
        }
      }
      if (ps->count < ps->bit_length) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        if (ps->count == 1)
          ps->state = tcmplxA_BrCvt_TreeCountD;
        else {
          ps->state = tcmplxA_BrCvt_ContextRunMaxL;
        }
        ps->bit_length = 0;
      } break;
    case tcmplxA_BrCvt_ContextRunMaxL:
      if (ps->bit_length == 0) {
        struct tcmplxA_ctxtmap* const map = ps->literals_map;
        size_t const total = tcmplxA_ctxtmap_block_types(map)
          * tcmplxA_ctxtmap_contexts(map);
        size_t j = 0;
        size_t zeroes = 0;
        unsigned int rlemax = 0;
        unsigned char const* const map_data = tcmplxA_ctxtmap_data_c(map);
        tcmplxA_ctxtmap_apply_movetofront(map);
        ps->context_encode.sz = 0;
        for (j = 0; j < total && ae == tcmplxA_Success; ++j) {
          if (map_data[j] || zeroes >= 63) {
            ae = tcmplxA_brcvt_encode_map(&ps->context_encode, zeroes, map_data[j], &rlemax);
            zeroes = (map_data[j]==0);
          } else zeroes += 1;
        }
        if (zeroes > 0 && ae == tcmplxA_Success)
          ae = tcmplxA_brcvt_encode_map(&ps->context_encode, zeroes, 0, &rlemax);
#ifndef NDEBUG
        tcmplxA_ctxtmap_revert_movetofront(map);
#endif //NDEBUG
        if (ae != tcmplxA_Success)
          break;
        ps->rlemax = (unsigned char)rlemax;
        ps->count = 0;
        if (ps->rlemax == 0) {
          ps->bits = 0;
          ps->bit_length = 1;
        } else {
          ps->bits = 1u | ((rlemax-1)<<1u);
          ps->bit_length = 5;
        }
      }
      if (ps->count < ps->bit_length) {
        x = (ps->bits>>ps->count)&1u;
        ps->count += 1;
      }
      if (ps->count >= ps->bit_length) {
        size_t const btypes = tcmplxA_fixlist_size(&ps->literal_blocktype);
        tcmplxA_uint32 histogram[tcmplxA_brcvt_ContextHistogram] = {0};
        size_t j;
        unsigned int const rlemax = ps->rlemax;
        unsigned char const alphabits = (unsigned char)(rlemax+btypes);
        /* calculate prefix tree */
        ae = tcmplxA_fixlist_resize(&ps->context_tree, alphabits);
        if (ae != tcmplxA_Success)
          break;
        for (j = 0; j < ps->context_encode.sz; ++j) {
          unsigned char const ch = ps->context_encode.p[j];
          if (ch < tcmplxA_brcvt_ZeroBit) {
            if (ch == 0)
              histogram[0] += 1;
            else {
              assert(ch+rlemax < tcmplxA_brcvt_ContextHistogram);
              histogram[ch+rlemax] += 1;
            }
          } else if (!(ch & tcmplxA_brcvt_RepeatBit)) {
            histogram[ch&(tcmplxA_brcvt_ZeroBit-1)] += 1;
          } else continue;
        }
        ae = tcmplxA_fixlist_gen_lengths(&ps->context_tree, histogram, 8);
        if (ae != tcmplxA_Success)
          break;
        ae = tcmplxA_fixlist_gen_codes(&ps->context_tree);
        if (ae != tcmplxA_Success)
          break;
        tcmplxA_brcvt_reset19(&ps->treety);
        ps->alphabits = alphabits;
        ps->state += 1;
      } break;
    case tcmplxA_BrCvt_ContextPrefixL:
      {
        int const res = tcmplxA_brcvt_outflow19(&ps->treety, &ps->context_tree, &x, ps->alphabits);
        if (res == tcmplxA_EOF) {
          ps->bit_length = 0;
          ps->index = 0;
          ps->count = (tcmplxA_uint32)ps->context_encode.sz;
          tcmplxA_brcvt_reset19(&ps->treety);
          ps->state += 1;
          ae = tcmplxA_fixlist_valuesort(&ps->context_tree);
        } else if (res != tcmplxA_Success)
          ae = res;
      } break;
    case tcmplxA_BrCvt_ContextValuesL:
      if (ps->bit_length == 0) {
        unsigned char const code = ps->context_encode.p[ps->index];
        unsigned int const extra = (code&tcmplxA_brcvt_ZeroBit)
          ? code & (tcmplxA_brcvt_ZeroBit-1u) : 0;
        unsigned int const value = extra ? extra : (code?code+ps->rlemax:0);
        size_t const line_index = tcmplxA_fixlist_valuebsearch(&ps->context_tree, value);
        struct tcmplxA_fixline const* const line = tcmplxA_fixlist_at_c(&ps->context_tree, line_index);
        if (!line) {
          ae = tcmplxA_ErrSanitize;
          break;
        }
        ps->bits = line->code;
        ps->bit_length = (unsigned char)line->len;
        ps->extra_length = extra;
      }
      if (ps->bit_length > 0)
        x = (ps->bits>>(--ps->bit_length))&1u;
      if (ps->bit_length == 0) {
        ps->index += 1;
        ps->bits = 0;
        if (ps->index >= ps->count)
          ps->state += 2;
        else if (ps->extra_length > 0)
          ps->state += 1;
      } break;
    case tcmplxA_BrCvt_ContextRepeatL:
      if (ps->bit_length == 0) {
        assert(ps->index < ps->context_encode.sz);
        ps->bits = ps->context_encode.p[ps->index];
      }
      if (ps->bit_length < ps->extra_length) {
        x = (ps->bits>>ps->bit_length)&1u;
        ps->bit_length += 1;
      }
      if (ps->bit_length >= ps->extra_length) {
        ps->index += 1;
        ps->bits = 0;
        ps->bit_length = 0;
        if (ps->index >= ps->count)
          ps->state += 1;
        else
          ps->state -= 1;
      } break;
    case tcmplxA_BrCvt_ContextInvertL:
      x = 1;
      ps->state = tcmplxA_BrCvt_TreeCountD;
      break;
    case tcmplxA_BrCvt_TreeCountD:
      x = 0;
      ps->state = tcmplxA_BrCvt_GaspVectorL;
      break;
    case tcmplxA_BrCvt_GaspVectorL:
      /* TODO this state */
      break;
    case tcmplxA_BrCvt_ContextRunMaxD:
    case tcmplxA_BrCvt_ContextPrefixD:
    case tcmplxA_BrCvt_ContextValuesD:
    case tcmplxA_BrCvt_ContextRepeatD:
    case tcmplxA_BrCvt_ContextInvertD:
      ae = tcmplxA_ErrSanitize;
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
    case tcmplxA_BrCvt_NPostfix:
    case tcmplxA_BrCvt_ContextTypesL:
    case tcmplxA_BrCvt_TreeCountL:
    case tcmplxA_BrCvt_ContextRunMaxL:
    case tcmplxA_BrCvt_ContextPrefixL:
    case tcmplxA_BrCvt_ContextValuesL:
    case tcmplxA_BrCvt_ContextRepeatL:
    case tcmplxA_BrCvt_ContextInvertL:
    case tcmplxA_BrCvt_TreeCountD:
    case tcmplxA_BrCvt_ContextRunMaxD:
    case tcmplxA_BrCvt_ContextPrefixD:
    case tcmplxA_BrCvt_ContextValuesD:
    case tcmplxA_BrCvt_ContextRepeatD:
    case tcmplxA_BrCvt_ContextInvertD:
    case tcmplxA_BrCvt_GaspVectorL:
    case tcmplxA_BrCvt_GaspVectorI:
    case tcmplxA_BrCvt_GaspVectorD:
    case tcmplxA_BrCvt_Literal:
    case tcmplxA_BrCvt_DataInsertCopy:
    case tcmplxA_BrCvt_DataInsertExtra:
    case tcmplxA_BrCvt_DataCopyExtra:
    case tcmplxA_BrCvt_LiteralRestart:
    case tcmplxA_BrCvt_Distance:
    case tcmplxA_BrCvt_DistanceRestart:
    case tcmplxA_BrCvt_InsertRestart:
    case tcmplxA_BrCvt_DataDistanceExtra:
    case tcmplxA_BrCvt_DoCopy:
    case tcmplxA_BrCvt_BDict:
    case tcmplxA_BrCvt_TempGap:
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
    case tcmplxA_BrCvt_NPostfix:
    case tcmplxA_BrCvt_ContextTypesL:
    case tcmplxA_BrCvt_TreeCountL:
    case tcmplxA_BrCvt_ContextRunMaxL:
    case tcmplxA_BrCvt_ContextPrefixL:
    case tcmplxA_BrCvt_ContextValuesL:
    case tcmplxA_BrCvt_ContextRepeatL:
    case tcmplxA_BrCvt_ContextInvertL:
    case tcmplxA_BrCvt_TreeCountD:
    case tcmplxA_BrCvt_ContextRunMaxD:
    case tcmplxA_BrCvt_ContextPrefixD:
    case tcmplxA_BrCvt_ContextValuesD:
    case tcmplxA_BrCvt_ContextRepeatD:
    case tcmplxA_BrCvt_ContextInvertD:
    case tcmplxA_BrCvt_GaspVectorL:
    case tcmplxA_BrCvt_GaspVectorI:
    case tcmplxA_BrCvt_GaspVectorD:
    case tcmplxA_BrCvt_DataInsertCopy:
    case tcmplxA_BrCvt_DataInsertExtra:
    case tcmplxA_BrCvt_DataCopyExtra:
    case tcmplxA_BrCvt_Literal:
    case tcmplxA_BrCvt_Distance:
    case tcmplxA_BrCvt_LiteralRestart:
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
