/**
 * @file text-complex/access/ctxtmap.c
 * @brief Context map for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ctxtmap_p.h"
#include "text-complex/access/ctxtmap.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>
#include <assert.h>

struct tcmplxA_ctxtmap {
  unsigned char *p;
  size_t btypes;
  size_t ctxts;
};

/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut0[256] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  4,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   8, 12, 16, 12, 12, 20, 12, 16, 24, 28, 12, 12, 32, 12, 36, 12,
  44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 32, 32, 24, 40, 28, 12,
  12, 48, 52, 52, 52, 48, 52, 52, 52, 48, 52, 52, 52, 52, 52, 48,
  52, 52, 52, 52, 52, 48, 52, 52, 52, 52, 52, 24, 12, 28, 12, 12,
  12, 56, 60, 60, 60, 56, 60, 60, 60, 56, 60, 60, 60, 60, 60, 56,
  60, 60, 60, 60, 60, 56, 60, 60, 60, 60, 60, 24, 12, 28, 12,  0,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
   2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3
 };
/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut1[256] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
   1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
   1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
  };
/* NOTE from RFC7932 */
static unsigned char tcmplxA_ctxtmap_lut2[256] = {
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7
  };
/* NOTE adapted from RFC7932 */
/* NOTE just a sequence from 0 to 255 */
static
unsigned char const tcmplxA_ctxtmap_imtf_init[256] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
    8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u,
    16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u,
    24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u,
    32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u,
    40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u,
    48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u,
    56u, 57u, 58u, 59u, 60u, 61u, 62u, 63u,
    64u, 65u, 66u, 67u, 68u, 69u, 70u, 71u,
    72u, 73u, 74u, 75u, 76u, 77u, 78u, 79u,
    80u, 81u, 82u, 83u, 84u, 85u, 86u, 87u,
    88u, 89u, 90u, 91u, 92u, 93u, 94u, 95u,
    96u, 97u, 98u, 99u, 100u, 101u, 102u, 103u,
    104u, 105u, 106u, 107u, 108u, 109u, 110u, 111u,
    112u, 113u, 114u, 115u, 116u, 117u, 118u, 119u,
    120u, 121u, 122u, 123u, 124u, 125u, 126u, 127u,
    128u, 129u, 130u, 131u, 132u, 133u, 134u, 135u,
    136u, 137u, 138u, 139u, 140u, 141u, 142u, 143u,
    144u, 145u, 146u, 147u, 148u, 149u, 150u, 151u,
    152u, 153u, 154u, 155u, 156u, 157u, 158u, 159u,
    160u, 161u, 162u, 163u, 164u, 165u, 166u, 167u,
    168u, 169u, 170u, 171u, 172u, 173u, 174u, 175u,
    176u, 177u, 178u, 179u, 180u, 181u, 182u, 183u,
    184u, 185u, 186u, 187u, 188u, 189u, 190u, 191u,
    192u, 193u, 194u, 195u, 196u, 197u, 198u, 199u,
    200u, 201u, 202u, 203u, 204u, 205u, 206u, 207u,
    208u, 209u, 210u, 211u, 212u, 213u, 214u, 215u,
    216u, 217u, 218u, 219u, 220u, 221u, 222u, 223u,
    224u, 225u, 226u, 227u, 228u, 229u, 230u, 231u,
    232u, 233u, 234u, 235u, 236u, 237u, 238u, 239u,
    240u, 241u, 242u, 243u, 244u, 245u, 246u, 247u,
    248u, 249u, 250u, 251u, 252u, 253u, 254u, 255u
  };

/**
 * @brief Initialize a context map.
 * @param x the context map to initialize
 * @param btypes number of block types
 * @param ctxts number of contexts
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ctxtmap_init
    (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts);
/**
 * @brief Close a context map.
 * @param x the context map to close
 */
static void tcmplxA_ctxtmap_close(struct tcmplxA_ctxtmap* x);
/**
 * @brief Resize a context map.
 * @param x the context map to resize
 * @param btypes number of block types
 * @param ctxts number of contexts
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ctxtmap_resize
    (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts);

/* BEGIN context map / static */
int tcmplxA_ctxtmap_init
      (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts)
{
  x->p = NULL;
  x->btypes = 0u;
  x->ctxts = 0u;
  return tcmplxA_ctxtmap_resize(x, btypes, ctxts);
}

void tcmplxA_ctxtmap_close(struct tcmplxA_ctxtmap* x) {
  if (x->p != NULL) {
    tcmplxA_util_free(x->p);
    x->p = NULL;
  }
  x->btypes = 0u;
  x->ctxts = 0u;
  return;
}

int tcmplxA_ctxtmap_resize
  (struct tcmplxA_ctxtmap* x, size_t btypes, size_t ctxts)
{
  if (ctxts > 0u && btypes >= (~(size_t)0u)/ctxts) {
    return tcmplxA_ErrMemory;
  } else if (ctxts == 0u || btypes == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
      x->p = NULL;
    }
  } else {
    unsigned char* const np = tcmplxA_util_malloc(btypes*ctxts);
    if (np == NULL) {
      return tcmplxA_ErrMemory;
    } else {
      if (x->p != NULL) {
        tcmplxA_util_free(x->p);
      }
    }
    x->p = np;
  }
  x->btypes = btypes;
  x->ctxts = ctxts;
  return tcmplxA_Success;
}
/* END   context map / static */

/* BEGIN context map / internal */
unsigned char tcmplxA_ctxtmap_getlut2(int i) {
  assert(i >= 0 && i < sizeof(tcmplxA_ctxtmap_lut2));
  return tcmplxA_ctxtmap_lut2[i];
}
/* END   context map / internal */

/* BEGIN context map / public */
struct tcmplxA_ctxtmap* tcmplxA_ctxtmap_new(size_t btypes, size_t ctxts) {
  struct tcmplxA_ctxtmap* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_ctxtmap));
  if (out != NULL
  &&  tcmplxA_ctxtmap_init(out, btypes, ctxts) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_ctxtmap_destroy(struct tcmplxA_ctxtmap* x) {
  if (x != NULL) {
    tcmplxA_ctxtmap_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

size_t tcmplxA_ctxtmap_block_types(struct tcmplxA_ctxtmap const* x) {
  return x->btypes;
}

size_t tcmplxA_ctxtmap_contexts(struct tcmplxA_ctxtmap const* x) {
  return x->ctxts;
}

unsigned char* tcmplxA_ctxtmap_data(struct tcmplxA_ctxtmap* x) {
  return x->p;
}

unsigned char const* tcmplxA_ctxtmap_data_c(struct tcmplxA_ctxtmap const* x) {
  return x->p;
}

int tcmplxA_ctxtmap_get(struct tcmplxA_ctxtmap const* x, size_t i, size_t j) {
#ifndef NDEBUG
  if (i >= x->btypes || j >= x->ctxts)
    return -1;
#endif /*NDEBUG*/
  return x->p[i*x->ctxts + j];
}

void tcmplxA_ctxtmap_set
  (struct tcmplxA_ctxtmap* x, size_t i, size_t j, int v)
{
#ifndef NDEBUG
  if (i >= x->btypes || j >= x->ctxts)
    return;
#endif /*NDEBUG*/
  x->p[i*x->ctxts + j] = (unsigned char)v;
  return;
}

int tcmplxA_ctxtmap_distance_context(unsigned long int copylen) {
  if (copylen < 2)
    return -1;
  else switch (copylen) {
  case 2: return 0;
  case 3: return 1;
  case 4: return 2;
  default: return 3;
  }
}

int tcmplxA_ctxtmap_literal_context
    (int mode, unsigned int p1, unsigned int p2)
{
  switch (mode) {
  case tcmplxA_CtxtMap_LSB6:
    return p1&63u;
  case tcmplxA_CtxtMap_MSB6:
    return (p1>>2)&63u;
  case tcmplxA_CtxtMap_UTF8:
    return tcmplxA_ctxtmap_lut0[p1&255] | tcmplxA_ctxtmap_lut1[p2&255];
  case tcmplxA_CtxtMap_Signed:
    return (tcmplxA_ctxtmap_lut2[p1&255]<<3) | tcmplxA_ctxtmap_lut2[p2&255];
  default:
    return -1;
  }
}

void tcmplxA_ctxtmap_apply_movetofront(struct tcmplxA_ctxtmap* x) {
  size_t const len = x->btypes * x->ctxts;
  unsigned char *const data = x->p;
  unsigned char mtf_refs[256];
  memcpy(mtf_refs, tcmplxA_ctxtmap_imtf_init, 256u*sizeof(char));
  /* */{
    size_t i;
    for (i = 0u; i < len; ++i) {
      unsigned char const v = data[i];
      unsigned short const j = (unsigned short)(
            (unsigned char*)memchr(mtf_refs, v, 256u) - mtf_refs
          );
      data[i] = j;
      memmove(mtf_refs+1u, mtf_refs, j*sizeof(char));
      mtf_refs[0u] = v;
    }
  }
  return;
}

void tcmplxA_ctxtmap_revert_movetofront(struct tcmplxA_ctxtmap* x) {
  size_t const len = x->btypes * x->ctxts;
  unsigned char *const data = x->p;
  unsigned char mtf_refs[256];
  memcpy(mtf_refs, tcmplxA_ctxtmap_imtf_init, 256u*sizeof(char));
  /* */{
    size_t i;
    for (i = 0u; i < len; ++i) {
      unsigned short const j = data[i];
      unsigned char const v = mtf_refs[j];
      data[i] = v;
      memmove(mtf_refs+1u, mtf_refs, j*sizeof(char));
      mtf_refs[0u] = v;
    }
  }
  return;
}
/* END   context map / public */
