/**
 * @file tests/testfont.c
 * @brief Test fonts.
 */
#include "testfont.h"
#include <limits.h>
#include <stdlib.h>
#include "../mmaptwo/mmaptwo.h"

struct tcmplxAtest_mmtp {
  struct mmaptwo_page_i base;
  size_t len;
  size_t off;
  void* data;
};
struct tcmplxAtest_mmt {
  struct mmaptwo_i base;
  size_t len;
  int gen;
  unsigned int seed;
};

static
void tcmplxAtest_mmt_dtor(struct mmaptwo_i* m);
static
struct mmaptwo_page_i* tcmplxAtest_mmt_acquire
  (struct mmaptwo_i* m, size_t siz, size_t off);
static
size_t tcmplxAtest_mmt_length(struct mmaptwo_i const* m);
static
size_t tcmplxAtest_mmt_offset(struct mmaptwo_i const* m);

static
void tcmplxAtest_mmtp_dtor(struct mmaptwo_page_i* m);
static
void* tcmplxAtest_mmtp_get(struct mmaptwo_page_i* m);
static
void const* tcmplxAtest_mmtp_getconst(struct mmaptwo_page_i const* m);
static
size_t tcmplxAtest_mmtp_length(struct mmaptwo_page_i const* m);
static
size_t tcmplxAtest_mmtp_offset(struct mmaptwo_page_i const* m);



int tcmplxAtest_gen_datum(int n, size_t i, unsigned int seed) {
  switch (n) {
  case tcmplxAtest_Zero:
    return 0u;
  case tcmplxAtest_Ascending:
    return (unsigned char)((i+seed)&255u);
  case tcmplxAtest_Descending:
    return (unsigned char)(255u-((i+seed)&255u));
  case tcmplxAtest_Pseudorandom:
    {
      unsigned int const addend = (seed&255u);
      unsigned int const factor = ((seed>>8)&255u);
      unsigned int const count = (unsigned int)(i&511u);
      unsigned int i;
#if UINT_MAX <= 65535u
      unsigned int x = 0u;
#else
      unsigned int x = seed>>16u;
#endif /*UINT_MAX*/
      for (i = 0; i < count; ++i) {
        x = (x*factor)+addend;
      }
      return (unsigned char)(x&255);
    }
  default:
    return -1;
  }
}

size_t tcmplxAtest_gen_size(int n) {
  switch (n) {
  case tcmplxAtest_Zero:
  case tcmplxAtest_Ascending:
  case tcmplxAtest_Descending:
  case tcmplxAtest_Pseudorandom:
    return ~(size_t)0u;
  default:
    return 0u;
  }
}

void tcmplxAtest_mmt_dtor(struct mmaptwo_i* m) {
  free(m);
  return;
}

struct mmaptwo_page_i* tcmplxAtest_mmt_acquire
  (struct mmaptwo_i* m, size_t siz, size_t off)
{
  struct tcmplxAtest_mmt* const tm = (struct tcmplxAtest_mmt*)m;
  if (off >= tm->len
  ||  siz > (tm->len-off))
  {
    return NULL;
  } else {
    size_t i;
    unsigned char* data = (unsigned char*)malloc(siz);
    if (data == NULL)
      return NULL;
    for (i = 0; i < siz; ++i) {
      data[i] = (unsigned char)tcmplxAtest_gen_datum
          (tm->gen, i+off, tm->seed);
    }
    /* */{
      struct tcmplxAtest_mmtp* const out =
        malloc(sizeof(struct tcmplxAtest_mmtp));
      if (out != NULL) {
        out->len = siz;
        out->off = off;
        out->data = data;
        out->base.mmtp_dtor = tcmplxAtest_mmtp_dtor;
        out->base.mmtp_get = tcmplxAtest_mmtp_get;
        out->base.mmtp_getconst = tcmplxAtest_mmtp_getconst;
        out->base.mmtp_length = tcmplxAtest_mmtp_length;
        out->base.mmtp_offset = tcmplxAtest_mmtp_offset;
      } else free(data);
      return (struct mmaptwo_page_i*)out;
    }
  }
}

size_t tcmplxAtest_mmt_length(struct mmaptwo_i const* m) {
  struct tcmplxAtest_mmt* const tm = (struct tcmplxAtest_mmt*)m;
  return tm->len;
}
size_t tcmplxAtest_mmt_offset(struct mmaptwo_i const* m) {
  (void)m;
  return 0u;
}

void tcmplxAtest_mmtp_dtor(struct mmaptwo_page_i* m) {
  struct tcmplxAtest_mmtp* const tp = (struct tcmplxAtest_mmtp*)m;
  free(tp->data);
  free(tp);
  return;
}
void* tcmplxAtest_mmtp_get(struct mmaptwo_page_i* m) {
  struct tcmplxAtest_mmtp* const tp = (struct tcmplxAtest_mmtp*)m;
  return tp->data;
}
void const* tcmplxAtest_mmtp_getconst(struct mmaptwo_page_i const* m) {
  struct tcmplxAtest_mmtp const* const tp = (struct tcmplxAtest_mmtp const*)m;
  return tp->data;
}
size_t tcmplxAtest_mmtp_length(struct mmaptwo_page_i const* m) {
  struct tcmplxAtest_mmtp const* const tp = (struct tcmplxAtest_mmtp const*)m;
  return tp->len;
}
size_t tcmplxAtest_mmtp_offset(struct mmaptwo_page_i const* m) {
  struct tcmplxAtest_mmtp const* const tp = (struct tcmplxAtest_mmtp const*)m;
  return tp->off;
}

struct mmaptwo_i* tcmplxAtest_gen_maptwo
    (int n, size_t maxsize, unsigned int seed)
{
  struct tcmplxAtest_mmt* const out = malloc(sizeof(struct tcmplxAtest_mmt));
  if (out != NULL) {
    out->len =
      (maxsize>tcmplxAtest_gen_size(n) ? tcmplxAtest_gen_size(n) : maxsize);
    out->gen = n;
    out->seed = seed;
    out->base.mmt_dtor = tcmplxAtest_mmt_dtor;
    out->base.mmt_acquire = tcmplxAtest_mmt_acquire;
    out->base.mmt_length = tcmplxAtest_mmt_length;
    out->base.mmt_offset = tcmplxAtest_mmt_offset;
  }
  return (struct mmaptwo_i*)out;
}
