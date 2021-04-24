/*
 * \file text-complex/access/bdict_cb.c
 * \brief Built-in dictionary
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "bdict_p.h"
#include "text-complex/access/bdict.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>





/**
 * @internal
 * @brief Transform callback.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
typedef void (*tcmplxA_bdict_cb_fn)
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Identity transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_identity
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Single ferment transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_brew_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Complete ferment transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_brew_all
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first one transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first two transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_two
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first three transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_three
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first four transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_four
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first five transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_five
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first six transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_six
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first seven transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_seven
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first eight transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_eight
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit first nine transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropfront_nine
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last one transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last two transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_two
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last three transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_three
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last four transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_four
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last five transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_five
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last six transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_six
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last seven transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_seven
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last eight transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_eight
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Omit last nine transform.
 * @param dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen source length
 */
static
void tcmplxA_bdict_cb_dropback_nine
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen);
/**
 * @internal
 * @brief Ferment some bytes.
 * @param[out] dst destination buffer
 * @param[in,out] dstlen destination length
 * @param src source buffer
 * @param srclen length of source buffer
 * @param[in,out] srci read position
 * @return advance distance
 */
static
unsigned int tcmplxA_bdict_brew
  ( unsigned char* dst, unsigned int dstlen,
    unsigned char const* src, unsigned int srclen, unsigned int srci);

/**
 * @internal
 * @brief List of callbacks for word transforms.
 */
static
tcmplxA_bdict_cb_fn const tcmplxA_bdict_cblist[21u] = {
  /*  0 */ tcmplxA_bdict_cb_identity,
  /*  1 */ tcmplxA_bdict_cb_brew_one,
  /*  2 */ tcmplxA_bdict_cb_brew_all,
  /*  3 */ tcmplxA_bdict_cb_dropfront_one,
  /*  4 */ tcmplxA_bdict_cb_dropfront_two,
  /*  5 */ tcmplxA_bdict_cb_dropfront_three,
  /*  6 */ tcmplxA_bdict_cb_dropfront_four,
  /*  7 */ tcmplxA_bdict_cb_dropfront_five,
  /*  8 */ tcmplxA_bdict_cb_dropfront_six,
  /*  9 */ tcmplxA_bdict_cb_dropfront_seven,
  /* 10 */ tcmplxA_bdict_cb_dropfront_eight,
  /* 11 */ tcmplxA_bdict_cb_dropfront_nine,
  /* 12 */ tcmplxA_bdict_cb_dropback_one,
  /* 13 */ tcmplxA_bdict_cb_dropback_two,
  /* 14 */ tcmplxA_bdict_cb_dropback_three,
  /* 15 */ tcmplxA_bdict_cb_dropback_four,
  /* 16 */ tcmplxA_bdict_cb_dropback_five,
  /* 17 */ tcmplxA_bdict_cb_dropback_six,
  /* 18 */ tcmplxA_bdict_cb_dropback_seven,
  /* 19 */ tcmplxA_bdict_cb_dropback_eight,
  /* 20 */ tcmplxA_bdict_cb_dropback_nine
};


/* BEGIN built-in dictionary / static */
unsigned int tcmplxA_bdict_brew
  ( unsigned char* dst, unsigned int wpos,
    unsigned char const* src, unsigned int srclen, unsigned int rpos)
{
  if (src[rpos] < 192u) {
    dst[wpos] = (src[rpos] >= 97u && src[rpos] <= 122u)
      ? src[rpos]^32u : src[rpos];
    return 1u;
  } else if (src[rpos] < 224u) {
    dst[wpos] = src[rpos];
    if (rpos + 1u < srclen) {
      dst[wpos + 1u] = src[rpos + 1u] ^ 32u;
      return 2u;
    } else return 1u;
  } else /* src[rpos] <= 255u */{
    dst[wpos] = src[rpos];
    if (rpos + 1u < srclen) {
      dst[wpos+1u] = src[rpos+1u];
      if (rpos + 2u < srclen) {
        dst[wpos + 2u] = src[rpos + 2u] ^ 5u;
        return 3u;
      } else return 2u;
    } else return 1u;
  }
}

void tcmplxA_bdict_cb_identity
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  unsigned int const oldlen = (*dstlen);
  memcpy(dst+oldlen, src, srclen*sizeof(unsigned char));
  (*dstlen) = oldlen+srclen;
  return;
}

void tcmplxA_bdict_cb_brew_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  unsigned int dstpos = *dstlen;
  if (srclen > 0u && dstpos < 34u) {
    unsigned int const srcxpos =
      tcmplxA_bdict_brew(dst, dstpos, src, srclen, 0u);
    dstpos += srcxpos;
    /* */{
      unsigned int const dstdiff = 37u-dstpos;
      unsigned int const srcdiff = srclen - srcxpos;
      unsigned int const len = (dstdiff < srcdiff) ? dstdiff : srcdiff;
      memcpy(dst+dstpos, src+srcxpos, len);
      dstpos += len;
    }
    *dstlen = dstpos;
  }
  return;
}

void tcmplxA_bdict_cb_brew_all
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  unsigned int dstpos = *dstlen;
  unsigned int i;
  for (i = 0u; i < srclen && dstpos < 34u; ) {
    unsigned int const next = tcmplxA_bdict_brew(dst, dstpos, src, srclen, i);
    i += next;
    dstpos += next;
  }
  *dstlen = dstpos;
  return;
}

void tcmplxA_bdict_cb_dropfront_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 1u) {
    unsigned int const omitlen = srclen-1u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+1u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_two
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 2u) {
    unsigned int const omitlen = srclen-2u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+2u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_three
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 3u) {
    unsigned int const omitlen = srclen-3u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+3u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_four
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 4u) {
    unsigned int const omitlen = srclen-4u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+4u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_five
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 5u) {
    unsigned int const omitlen = srclen-5u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+5u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_six
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 6u) {
    unsigned int const omitlen = srclen-6u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+6u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_seven
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 7u) {
    unsigned int const omitlen = srclen-7u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+7u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_eight
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 8u) {
    unsigned int const omitlen = srclen-8u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+8u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropfront_nine
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 9u) {
    unsigned int const omitlen = srclen-9u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src+9u, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_one
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 1u) {
    unsigned int const omitlen = srclen-1u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_two
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 2u) {
    unsigned int const omitlen = srclen-2u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_three
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 3u) {
    unsigned int const omitlen = srclen-3u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_four
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 4u) {
    unsigned int const omitlen = srclen-4u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_five
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 5u) {
    unsigned int const omitlen = srclen-5u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_six
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 6u) {
    unsigned int const omitlen = srclen-6u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_seven
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 7u) {
    unsigned int const omitlen = srclen-7u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_eight
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 8u) {
    unsigned int const omitlen = srclen-8u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}

void tcmplxA_bdict_cb_dropback_nine
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen)
{
  if (srclen >= 9u) {
    unsigned int const omitlen = srclen-9u;
    unsigned int const oldlen = (*dstlen);
    memcpy(dst+oldlen, src, omitlen*sizeof(unsigned char));
    (*dstlen) = oldlen+omitlen;
  }
  return;
}
/* END   built-in dictionary / static */

/* BEGIN built-in dictionary / private */
void tcmplxA_bdict_cb_do
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen, unsigned int k)
{
  (*tcmplxA_bdict_cblist[k])(dst,dstlen,src,srclen);
  return;
}
/* END   built-in dictionary / public */
