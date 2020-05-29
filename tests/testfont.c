/**
 * @file tests/testfont.c
 * @brief Test fonts.
 */
#include "testfont.h"
#include <limits.h>
#include <stdlib.h>
#include "../mmaptwo/mmaptwo.h"
#include "munit/munit.h"
#include <stdio.h>

static
bool tcmplxAtest_arg_fp_parse
  ( const MunitSuite* suite, void* user_data, int* arg, int argc,
    char* const argv[]);
static
void tcmplxAtest_arg_fp_help(const MunitArgument* argument, void* user_data);
static
int testfont_clamp_size(size_t z);
static
int testfont_clamp_uint(unsigned int z);

MunitArgument const tcmplxAtest_arglist[] = {
  { "font-path", tcmplxAtest_arg_fp_parse, tcmplxAtest_arg_fp_help },
  { NULL, NULL, NULL }
};


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

int tcmplxAtest_strdup(char** out, char const* arg) {
  if (arg != NULL) {
    size_t const len = strlen(arg);
    char* const p = calloc(len+1u,sizeof(char));
    if (p == NULL) {
      return -1;
    } else {
      memcpy(p, arg, len);
      *(p+len) = '\0';
      *out = p;
      return 0;
    }
  } else {
    *out = NULL;
    return 0;
  }
}

int tcmplxAtest_arg_init(struct tcmplxAtest_arg* tfa) {
  tfa->font_path = NULL;
  return 0;
}

void tcmplxAtest_arg_close(struct tcmplxAtest_arg* tfa) {
  free(tfa->font_path);
  tfa->font_path = NULL;
  return;
}

struct MunitArgument_ const* tcmplxAtest_get_args(void) {
  return tcmplxAtest_arglist;
}

bool tcmplxAtest_arg_fp_parse
  ( const MunitSuite* suite, void* user_data, int* arg, int argc,
    char* const argv[])
{
  struct tcmplxAtest_arg *const tfa = (struct tcmplxAtest_arg*)user_data;
  ++(*arg);
  if ((*arg) < argc) {
    char* dup;
    int const res = tcmplxAtest_strdup(&dup, argv[*arg]);
    if (res != 0) {
      fprintf(stdout, "File name broke --font-path.\n");
      return false;
    } else {
      free(tfa->font_path);
      tfa->font_path = dup;
      return true;
    }
  } else {
    fprintf(stdout, "Missing file name for --font-path.\n");
    return false;
  }
}

void tcmplxAtest_arg_fp_help(const MunitArgument* argument, void* user_data) {
  fprintf(stdout, " --font-path\n"
    "           Path of font file against which to test.\n");
  return;
}

int tcmplxAtest_fixlist_lex_start
  (struct tcmplxAtest_fixlist_lex *x, char const* s)
{
  x->p = NULL;
  x->total = 0u;
  x->left = 0u;
  x->prefix_len = -2;
  if (s == NULL) return -1;
  else {
    char const* q;
    for (q = s; *q != '\0'; ++q) {
      char* ep;
      size_t r_count = 1u;
      unsigned long int n = strtoul(q, &ep, 0);
      if ((*ep) != '\0' && (*ep) != ',' && (*ep) != '*') {
        return -1;
      } else if (n > (unsigned long)(INT_MAX)) {
        /* overflow, so */return -2;
      } else {
        /* this is the prefix_len */
      }
      q = ep;
      if ((*q) == '*') {
        unsigned long int new_r;
        ++q;
        new_r = strtoul(q, &ep, 0);
        if (((*ep) != '\0' && (*ep) != ',')) {
          return -1;
        } else if (new_r > ((~(size_t)0u) - x->total)) {
          /* overflow, so */return -2;
        } else if (new_r > 0u) {
          /* this is the repeat_count */
          r_count = (size_t)new_r;
        }
        q = ep;
      }
      x->total += r_count;
      if (*q == '\0') break;
    }
  }
  x->p = s;
  return 0;
}
int tcmplxAtest_fixlist_lex_next(struct tcmplxAtest_fixlist_lex *x) {
  if (x->prefix_len == -2 || x->left == 0u) {
    /* start */
    char const* q = x->p;
    char* ep;
    size_t r_count = 1u;
    if (q == NULL || (*q) == '\0')
      return -1;
    x->prefix_len = (int)strtoul(q, &ep, 0);
    q = ep;
    if ((*q) == '*') {
      unsigned long int new_r;
      ++q;
      new_r = strtoul(q, &ep, 0);
      if (new_r > 0u) {
        r_count = (size_t)new_r;
      }
      q = ep;
    }
    x->left = r_count;
    if (*q == ',') ++q;
    x->p = q;
  }
  x->left -= 1u;
  return x->prefix_len;
}

int testfont_rand_int_range(int a, int b) {
  if (a == b)
    return a;
  else return munit_rand_int_range(a,b);
}

int testfont_clamp_size(size_t z) {
  if (z > (unsigned int)(INT_MAX)) {
    return INT_MAX;
  } else return (int)(z);
}

int testfont_clamp_uint(unsigned int z) {
  if (z > (unsigned int)(INT_MAX)) {
    return INT_MAX;
  } else return (int)(z);
}

size_t testfont_rand_size_range(size_t a, size_t b) {
  if (a == b)
    return a;
  else return munit_rand_int_range(
      testfont_clamp_size(a), testfont_clamp_size(b)
    );
}

unsigned int testfont_rand_uint_range(unsigned int a, unsigned int b) {
  if (a == b)
    return a;
  else return munit_rand_int_range(
      testfont_clamp_uint(a), testfont_clamp_uint(b)
    );
}
