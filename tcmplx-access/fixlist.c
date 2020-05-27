/**
 * @file tcmplx-access/fixlist.c
 * @brief Prefix code list
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "fixlist.h"
#include "api.h"
#include "util.h"
#include <string.h>

struct tcmplxA_fixlist {
  struct tcmplxA_fixline* p;
  size_t n;
};

static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliComplex[] = {
  {  0    /*0*/, 2u, 0 },
  {0xe /*1110*/, 4u, 0 },
  {0x6  /*110*/, 3u, 0 },
  {0x1   /*01*/, 2u, 0 },
  {0x2   /*10*/, 2u, 0 },
  {0xf /*1111*/, 4u, 0 }
};

static
struct {
  size_t n;
  struct tcmplxA_fixline const* v;
} const tcmplxA_fixlist_ps[] = {
  { 6u, tcmplxA_fixlist_ps_BrotliComplex },
};

/**
 * @brief Initialize a prefix list.
 * @param x the prefix list to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_fixlist_init(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Resize a prefix list.
 * @param x the list to resize
 * @param sz number of lines in the list
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_fixlist_resize(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Close a prefix list.
 * @param x the prefix list to close
 */
static void tcmplxA_fixlist_close(struct tcmplxA_fixlist* x);

/* BEGIN prefix list / static */
int tcmplxA_fixlist_init(struct tcmplxA_fixlist* x, size_t sz) {
  x->p = NULL;
  x->n = 0u;
  return tcmplxA_fixlist_resize(x,sz);
}

int tcmplxA_fixlist_resize(struct tcmplxA_fixlist* x, size_t sz) {
  struct tcmplxA_fixline *ptr;
  if (sz == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
    }
    x->p = NULL;
    x->n = 0u;
    return tcmplxA_Success;
  } else if (sz >= UINT_MAX/sizeof(struct tcmplxA_fixline)) {
    return tcmplxA_ErrMemory;
  }
  ptr = (struct tcmplxA_fixline*)tcmplxA_util_malloc
          (sizeof(struct tcmplxA_fixline)*sz);
  if (ptr == NULL) {
    return tcmplxA_ErrMemory;
  }
  if (x->p != NULL) {
    tcmplxA_util_free(x->p);
  }
  x->p = ptr;
  x->n = sz;
  return tcmplxA_Success;
}

void tcmplxA_fixlist_close(struct tcmplxA_fixlist* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->n = 0u;
  return;
}
/* END   prefix list / static */

/* BEGIN prefix list / public */
struct tcmplxA_fixlist* tcmplxA_fixlist_new(size_t sz) {
  struct tcmplxA_fixlist* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_fixlist));
  if (out != NULL
  &&  tcmplxA_fixlist_init(out, sz) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_fixlist_destroy(struct tcmplxA_fixlist* x) {
  if (x != NULL) {
    tcmplxA_fixlist_close(x);
    tcmplxA_util_free(x);
  }
  return;
}


size_t tcmplxA_fixlist_size(struct tcmplxA_fixlist const* x) {
  return x->n;
}

struct tcmplxA_fixline* tcmplxA_fixlist_at
  (struct tcmplxA_fixlist* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

struct tcmplxA_fixline const* tcmplxA_fixlist_at_c
  (struct tcmplxA_fixlist const* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

int tcmplxA_fixlist_copy
  (struct tcmplxA_fixlist* dst, struct tcmplxA_fixlist const* src)
{
  int res;
  if (dst == src) {
    return tcmplxA_Success;
  }
  res = tcmplxA_fixlist_resize(dst, src->n);
  if (res != tcmplxA_Success) {
    return res;
  }
  memcpy(dst->p, src->p, sizeof(struct tcmplxA_fixline)*dst->n);
  return tcmplxA_Success;
}

int tcmplxA_fixlist_gen_codes(struct tcmplxA_fixlist* dst) {
  size_t counts[16] = {0u};
  unsigned int code_mins[16] = {0u};
  /* step 1. compute histogram */{
    size_t i;
    for (i = 0u; i < dst->n; ++i) {
      unsigned short const len = dst->p[i].len;
      if (len >= 16u)
        return tcmplxA_ErrFixLenRange;
      else counts[len] += 1u;
    }
  }
  /* step 2. find the minimum code for each length */{
    unsigned int cap_tracker = 1u;
    unsigned int next_code = 0u;
    int j;
    /* ignore length zero */
    for (j = 1; j < 16; ++j) {
      next_code <<= 1;
      cap_tracker <<= 1;
      code_mins[j] = next_code;
      if (counts[j] > cap_tracker-next_code) {
        return tcmplxA_ErrFixCodeAlloc;
      } else next_code += counts[j];
    }
  }
  /* step 3. "allocate" codes in order */{
    size_t i;
    for (i = 0u; i < dst->n; ++i) {
      struct tcmplxA_fixline* const line = dst->p + i;
      unsigned short const len = line->len;
      if (len > 0) {
        line->code = code_mins[len];
        code_mins[len] += 1u;
      } else line->code = 0u;
    }
  }
  return tcmplxA_Success;
}

int tcmplxA_fixlist_preset(struct tcmplxA_fixlist* dst, unsigned int i) {
  size_t const n = sizeof(tcmplxA_fixlist_ps)/sizeof(tcmplxA_fixlist_ps[0]);
  if (i >= n)
    return tcmplxA_ErrParam;
  /* resize */{
    size_t const sz = tcmplxA_fixlist_ps[i].n;
    int const res =
      tcmplxA_fixlist_resize(dst, sz);
    if (res != tcmplxA_Success) {
      return res;
    }
    memcpy(dst->p, tcmplxA_fixlist_ps[i].v, sizeof(struct tcmplxA_fixlist)*sz);
  }
  return tcmplxA_Success;
}
/* END   prefix list / public */
