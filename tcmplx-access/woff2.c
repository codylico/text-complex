/**
 * @file tcmplx-access/woff2.c
 * @brief WOFF2 file utility API
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "woff2.h"
#include "api.h"
#include "util.h"
#include <string.h>
#include "../mmaptwo/mmaptwo.h"
#include "seq.h"
#include "offtable.h"

/* TODO optimize */
static struct {
  unsigned int n;
  unsigned char tag[4];
} tcmplxA_woff2_tag_table[64] = {
   {0, "cmap"},	{16, "EBLC"},	{32, "CBDT"},	{48, "gvar"},
   {1, "head"},	{17, "gasp"},	{33, "CBLC"},	{49, "hsty"},
   {2, "hhea"},	{18, "hdmx"},	{34, "COLR"},	{50, "just"},
   {3, "hmtx"},	{19, "kern"},	{35, "CPAL"},	{51, "lcar"},
   {4, "maxp"},	{20, "LTSH"},	{36, "SVG "},	{52, "mort"},
   {5, "name"},	{21, "PCLT"},	{37, "sbix"},	{53, "morx"},
   {6, "OS/2"},	{22, "VDMX"},	{38, "acnt"},	{54, "opbd"},
   {7, "post"},	{23, "vhea"},	{39, "avar"},	{55, "prop"},
   {8, "cvt "},	{24, "vmtx"},	{40, "bdat"},	{56, "trak"},
   {9, "fpgm"},	{25, "BASE"},	{41, "bloc"},	{57, "Zapf"},
  {10, "glyf"},	{26, "GDEF"},	{42, "bsln"},	{58, "Silf"},
  {11, "loca"},	{27, "GPOS"},	{43, "cvar"},	{59, "Glat"},
  {12, "prep"},	{28, "GSUB"},	{44, "fdsc"},	{60, "Gloc"},
  {13, "CFF "},	{29, "EBSC"},	{45, "feat"},	{61, "Feat"},
  {14, "VORG"},	{30, "JSTF"},	{46, "fmtx"},	{62, "Sill"},
  {15, "EBDT"},	{31, "MATH"},	{47, "fvar"},	{63, {0,0,0,0}}
};

struct tcmplxA_woff2 {
  struct mmaptwo_i *fh;
  struct tcmplxA_offtable *offsets;
};

/**
 * @brief Initialize a woff2.
 * @param x the woff2 to initialize
 * @param seq sequential to parse
 * @param sane_tf sanitize flag
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_woff2_initparse
  (struct tcmplxA_woff2* x, struct tcmplxA_seq *seq, int sane_tf);
/**
 * @brief Initialize a woff2.
 * @param x the woff2 to initialize
 * @param fh file access interface
 * @param sane_tf sanitize flag
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_woff2_init
  (struct tcmplxA_woff2* x, struct mmaptwo_i *fh, int sane_tf);
/**
 * @brief Close a woff2.
 * @param x the woff2 to close
 */
static void tcmplxA_woff2_close(struct tcmplxA_woff2* x);
/**
 * @brief Parse out a 16-bit unsigned integer.
 * @param s from here
 * @return the integer
 */
static unsigned short tcmplxA_woff2_read_u16be(void const* s);
/**
 * @brief Parse out a tag from a WOFF2 tag table.
 * @param seq for this sequential
 * @param[out] tag_text to this tag buffer
 * @param[out] enc_path encode path taken for this chunk
 * @return zero on success, ErrSanitize otherwise
 */
static int tcmplxA_woff2_read_tag
  (struct tcmplxA_seq* seq, unsigned char tag_text[], unsigned int *enc_path);

/**
 * @brief Parse out a variable-length-encoding 32-bit unsigned integer.
 * @param s from here
 * @param[out] v to here
 * @throw api_exception on sanitize check failure
 */
static int tcmplxA_woff2_read_uv128
  (struct tcmplxA_seq* seq, tcmplxA_uint32* v);
/**
 * @brief Check if a WOFF2 table/encoding combination requires
 *   a transform length in the table directory.
 * @param tag_text tag name
 * @param enc_path encoding path
 * @return nonzero for true, zero for false
 */
static int tcmplxA_woff2_req_twolen
  (unsigned char const tag_text[], unsigned int enc_path);

/* BEGIN woff2 / static */
unsigned short tcmplxA_woff2_read_u16be(void const* s) {
  unsigned char const* c = (unsigned char const*)(s);
  return (((unsigned short)c[0])<<8)|c[1];
}

int tcmplxA_woff2_read_tag
  (struct tcmplxA_seq* seq, unsigned char tag_text[], unsigned int *enc_path)
{
  int const q_path = tcmplxA_seq_get_byte(seq);
  if (q_path < 0)
    return tcmplxA_ErrSanitize;
  if ((q_path&63) == 63) {
    int i;
    for (i = 0; i < 4; ++i) {
      int const ch = tcmplxA_seq_get_byte(seq);
      if (ch < 0)
        return tcmplxA_ErrSanitize;
      else tag_text[i] = (unsigned char)(ch&255);
    }
  } else {
    memcpy(tag_text, tcmplxA_woff2_tag_fromi(q_path&63), 4);
  }
  (*enc_path) = ((unsigned int)(q_path>>6)&3u);
  return tcmplxA_Success;
}

int tcmplxA_woff2_read_uv128(struct tcmplxA_seq* seq, tcmplxA_uint32* v) {
  tcmplxA_uint32 out = 0u;
  int i;
  for (i = 0; i < 5; ++i) {
    int const ch = tcmplxA_seq_get_byte(seq);
    if (ch < 0)
      return tcmplxA_ErrSanitize;
    else if (out == 0u && ch == 0x80)
      return tcmplxA_ErrSanitize;
    else if (i == 4 && ((ch&0x80) || (out > 0x1ffFFff))) {
      return tcmplxA_ErrSanitize;
    } else {
      out = ((out<<7)|(tcmplxA_uint32)(ch&0x7f));
      if (!(ch&0x80))
        break;
    }
  }
  (*v) = out;
  return tcmplxA_Success;
}

int tcmplxA_woff2_req_twolen
  (unsigned char const tag_text[], unsigned int enc_path)
{
  static unsigned char const glyf[4] = {0x67,0x6c,0x79,0x66};
  static unsigned char const loca[4] = {0x6c,0x6f,0x63,0x61};
  static unsigned char const hmtx[4] = {0x68,0x6d,0x74,0x78};
  if (enc_path == 0
  &&  (  (memcmp(tag_text, glyf, 4) == 0)
      || (memcmp(tag_text, loca, 4) == 0)))
  {
    return 1;
  }	else if (enc_path == 1
  &&  memcmp(tag_text, hmtx, 4) == 0)
  {
    return 1;
  } else return 0;
}

int tcmplxA_woff2_initparse
  (struct tcmplxA_woff2* x, struct tcmplxA_seq *seq, int sane_tf)
{
  /* acquire the header */{
    unsigned char wheader[48];
    int i;
    unsigned short table_count;
    for (i = 0; i < 48; ++i) {
      int ch = tcmplxA_seq_get_byte(seq);
      if (ch < 0)
        break;
      else wheader[i] = (unsigned char)(ch&255);
    }
    if (i < 48) {
      return tcmplxA_ErrSanitize;
    }
    /* inspect the signature */{
      unsigned char sig[4] = {0x77, 0x4F, 0x46, 0x32};
      if (memcmp(sig, wheader, 4) != 0) {
        return tcmplxA_ErrSanitize;
      }
    }
    /* parse out the table count */{
      table_count = tcmplxA_woff2_read_u16be(wheader+12);
      if (table_count > ~(size_t)0u) {
        return tcmplxA_ErrMemory;
      } else {
        x->offsets = tcmplxA_offtable_new(table_count);
        if (x->offsets == NULL)
          return tcmplxA_ErrMemory;
      }
    }
    /* iterate through the table directory */{
      unsigned short table_i;
      tcmplxA_uint32 next_offset = 0u;
      for (table_i = 0u; table_i < table_count; ++table_i) {
        /* TODO put into an offset table */
        unsigned char tag_text[4];
        unsigned int enc_path;
        tcmplxA_uint32 len, use_len;
        /* read the tag */{
          int const tag_res = tcmplxA_woff2_read_tag(seq, tag_text, &enc_path);
          if (tag_res != tcmplxA_Success)
            break;
        }
        /* read the original length */{
          int const len_res = tcmplxA_woff2_read_uv128(seq, &len);
          if (len_res != tcmplxA_Success)
            break;
        }
        /* read the transform length */
        if (tcmplxA_woff2_req_twolen(tag_text,enc_path)) {
          int const len_res = tcmplxA_woff2_read_uv128(seq, &use_len);
          if (len_res != tcmplxA_Success)
            break;
        } else use_len = len;
        /* add entry to offset table */{
          struct tcmplxA_offline *const line =
              tcmplxA_offtable_at(x->offsets, table_i);
          memcpy(line->tag, tag_text, 4);
          line->checksum = 0u;
          line->offset = next_offset;
          line->length = use_len;
          if (use_len > 0xFFffFFff-next_offset) {
            break;
          }
          next_offset += use_len;
        }
      }
      if (table_i < table_count) {
        return tcmplxA_ErrSanitize;
      }
    }
  }
  if (sane_tf) {
    /* TODO sanitize the rest of the file */
    if (0) {
      return tcmplxA_ErrSanitize;
    }
  }
  return tcmplxA_Success;
}

int tcmplxA_woff2_init
    (struct tcmplxA_woff2* x, struct mmaptwo_i *fh, int sane_tf)
{
  struct tcmplxA_seq* seq;
  int res;
  x->fh = fh;
  x->offsets = NULL;
  seq = tcmplxA_seq_new(fh);
  if (seq == NULL) {
    return tcmplxA_ErrInit;
  }
  /* parse */{
    res = tcmplxA_woff2_initparse(x,seq,sane_tf);
  }
  tcmplxA_seq_destroy(seq);
  if (res != tcmplxA_Success) {
    tcmplxA_offtable_destroy(x->offsets);
  }
  return res;
}

void tcmplxA_woff2_close(struct tcmplxA_woff2* x) {
  if (x->fh != NULL) {
    mmaptwo_close(x->fh);
    x->fh = NULL;
  }
  return;
}
/* END   woff2 / static */

/* BEGIN woff2 utilities / public */
unsigned int tcmplxA_woff2_tag_toi(unsigned char const* s) {
  unsigned int i;
  for (i = 0; i < 63; ++i) {
    if (memcmp(s,tcmplxA_woff2_tag_table[i].tag,4) == 0) {
      return tcmplxA_woff2_tag_table[i].n;
    }
  }
  return 63;
}

unsigned char const* tcmplxA_woff2_tag_fromi(unsigned int x) {
  unsigned int check_value = x&63;
  int i;
  if (x == 63)
    return NULL;
  else for (i = 0; i < 63; ++i) {
    if (check_value == tcmplxA_woff2_tag_table[i].n) {
      return tcmplxA_woff2_tag_table[i].tag;
    }
  }
  return NULL;
}
/* END   woff2 utilities / public */

/* BEGIN woff2 / public */
struct tcmplxA_woff2* tcmplxA_woff2_new(struct mmaptwo_i* xfh, int sane_tf) {
  struct tcmplxA_woff2* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_woff2));
  if (out != NULL
  &&  tcmplxA_woff2_init(out,xfh,sane_tf) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_woff2_destroy(struct tcmplxA_woff2* x) {
  if (x != NULL) {
    tcmplxA_woff2_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

struct tcmplxA_offtable const* tcmplxA_woff2_get_offsets
  (struct tcmplxA_woff2 const* x)
{
  return x->offsets;
}
/* END   woff2 / public */
