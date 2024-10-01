/**
 * @file text-complex/access/fixlist.c
 * @brief Prefix code list
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/fixlist.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>


/**
 * @internal
 * @brief Back reference to prefix list.
 */
struct tcmplxA_fixlist_ref {
  /** @internal @brief Left child. */
  unsigned int index;
  /** @internal @brief Right child. */
  tcmplxA_uint32 freq;
};


/**
 * @internal
 * @brief Min-heap item for code tree building.
 */
struct tcmplxA_fixlist_heapitem {
  /** @internal @brief histogram value. */
  tcmplxA_uint32 freq;
  /** @internal @brief block width. */
  tcmplxA_uint32 total_frac;
  /** @internal @brief block width below midline. */
  tcmplxA_uint32 small_frac;
  /** @internal @brief number of cells on the midline. */
  tcmplxA_uint32 midline;
};
/**
 * @internal
 * @brief Arguments for length generator recursive call.
 */
struct tcmplxA_fixlist_arg {
  /** @internal Reference index, first reference. */
  unsigned int begin;
  /** @internal Reference index, one-past-last reference. */
  unsigned int end;
  /** @internal block fraction upper bound. */
  int upper_bound;
  /** @internal block fraction lower bound. */
  int lower_bound;
  /** @internal @brief goal block width. */
  tcmplxA_uint32 total_frac;
};
/**
 * @internal
 * @brief Data for length generator recursive call.
 */
struct tcmplxA_fixlist_state {
  struct tcmplxA_fixlist_arg args;
  struct tcmplxA_fixlist_heapitem item;
};

struct tcmplxA_fixlist {
  struct tcmplxA_fixline* p;
  size_t n;
};

enum tcmplxA_fixlist_uconst {
  /** @internal @brief Code comparison maximum length difference. */
  tcmplxA_FixList_CodeCmpMax = (CHAR_BIT)*sizeof(unsigned short)
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
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliS1[] = {
  {  0    /*-*/, 0u, 0 },
};
static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliS2[] = {
  {  0    /*0*/, 1u, 0 },
  {0x1    /*1*/, 1u, 0 }
};
static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliS3[] = {
  {  0    /*0*/, 1u, 0 },
  {0x2   /*10*/, 2u, 0 },
  {0x3   /*11*/, 2u, 0 }
};
static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliS4A[] = {
  {  0   /*00*/, 2u, 0 },
  {0x1   /*01*/, 2u, 0 },
  {0x2   /*10*/, 2u, 0 },
  {0x3   /*11*/, 2u, 0 }
};
static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliS4B[] = {
  {  0    /*0*/, 1u, 0 },
  {0x2   /*10*/, 2u, 0 },
  {0x6  /*110*/, 3u, 0 },
  {0x7  /*111*/, 3u, 0 }
};
static
struct tcmplxA_fixline const tcmplxA_fixlist_ps_BrotliWBits[] = {
  {   0        /*0*/, 1u, 16 },
  {0x40  /*1000000*/, 7u, 17 },
  {0x41  /*1000001*/, 7u, 12 },
  {0x42  /*1000010*/, 7u, 10 },
  {0x43  /*1000011*/, 7u, 14 },
  /* - */
  {0x45  /*1000101*/, 7u, 13 },
  {0x46  /*1000110*/, 7u, 11 },
  {0x47  /*1000111*/, 7u, 15 },
  { 0x9     /*1001*/, 4u, 21 },
  { 0xA     /*1010*/, 4u, 19 },
  { 0xB     /*1011*/, 4u, 23 },
  { 0xC     /*1100*/, 4u, 18 },
  { 0xD     /*1101*/, 4u, 22 },
  { 0xE     /*1110*/, 4u, 20 },
  { 0xF     /*1111*/, 4u, 24 },
};

static
struct {
  size_t n;
  struct tcmplxA_fixline const* v;
} const tcmplxA_fixlist_ps[] = {
  { 6u, tcmplxA_fixlist_ps_BrotliComplex },
  { 1u, tcmplxA_fixlist_ps_BrotliS1 },
  { 2u, tcmplxA_fixlist_ps_BrotliS2 },
  { 3u, tcmplxA_fixlist_ps_BrotliS3 },
  { 4u, tcmplxA_fixlist_ps_BrotliS4A },
  { 4u, tcmplxA_fixlist_ps_BrotliS4B },
  {16u, tcmplxA_fixlist_ps_BrotliWBits }
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
/**
 * @brief Push a heap item onto a heap.
 * @param h heap represented by an array
 * @param[in,out] n size of heap
 * @param top maximum size of the heap
 * @param nh the item to push
 */
static int tcmplxA_fixlist_heappush
  ( struct tcmplxA_fixlist_heapitem* h, size_t *n, size_t top,
    struct tcmplxA_fixlist_heapitem const* nh);
/**
 * @brief Swap two heap items.
 * @param a one heap item
 * @param b the other heap item
 */
static void tcmplxA_fixlist_heapswap
  (struct tcmplxA_fixlist_heapitem* a, struct tcmplxA_fixlist_heapitem* b);
/**
 * @brief Compare two heap items.
 * @param a one item
 * @param b another item
 * @return negative if `a` < `b`; zero if equal; positive otherwise
 */
static int tcmplxA_fixlist_heapcmp
  (struct tcmplxA_fixlist_heapitem* a, struct tcmplxA_fixlist_heapitem* b);
/**
 * @brief Remove an item from a heap.
 * @param h heap represented by an array
 * @param[in,out] n size of heap
 * @param[out] nh the item removed from the stack
 */
static int tcmplxA_fixlist_heappop
  ( struct tcmplxA_fixlist_heapitem* h, size_t *n,
    struct tcmplxA_fixlist_heapitem* nh);
/**
 * @brief Sum two heap items together.
 * @param[out] sum heap item
 * @param b one addend
 * @param a another addend
 */
static void tcmplxA_fixlist_heap_add
  ( struct tcmplxA_fixlist_heapitem* c,
    struct tcmplxA_fixlist_heapitem const* a,
    struct tcmplxA_fixlist_heapitem const* b);
/**
 * @brief Add two numbers, clamping the result.
 * @param a addend
 * @param b addend
 * @return a clamped sum
 */
static tcmplxA_uint32 tcmplxA_fixlist_addclamp
  (tcmplxA_uint32 a, tcmplxA_uint32 b);
/**
 * @internal
 * @brief Compare two prefix list references.
 * @param a one reference
 * @param b another reference
 * @return positive if `a`'s frequency is less than `b`'s,
 *   zero if the frequencies are equal,
 *   negative otherwise
 */
static
int tcmplxA_fixlist_ref_cmp(void const* a, void const* b);
/**
 * @internal
 * @brief Find the least significant set bit.
 * @param x input
 * @return a number with only that set bit
 */
static
tcmplxA_uint32 tcmplxA_fixlist_min_frac(tcmplxA_uint32 x);
/**
 * @brief Check if the heap can provide packable items.
 * @param h heap represented by an array
 * @param n size of heap
 * @param frac fraction to meet
 * @return nonzero if two items can fit in such
 */
static int tcmplxA_fixlist_heap_packable
  ( struct tcmplxA_fixlist_heapitem* h, size_t n, tcmplxA_uint32 frac);
/**
 * @internal
 * @brief Compare two prefix lines by bit code.
 * @param a one line
 * @param b another line
 * @return positive if `a`'s code is "less" than `b`'s,
 *   zero if the codes are equal,
 *   negative otherwise
 */
static
int tcmplxA_fixline_codecmp(void const* a, void const* b);
/**
 * @internal
 * @brief Compare two prefix lines by alphabet value.
 * @param a one line
 * @param b another line
 * @return positive if `a`'s value is "less" than `b`'s,
 *   zero if the value are equal,
 *   negative otherwise
 */
static
int tcmplxA_fixline_valuecmp(void const* a, void const* b);




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

int tcmplxA_fixlist_heappush
  ( struct tcmplxA_fixlist_heapitem* h, size_t *n, size_t top,
    struct tcmplxA_fixlist_heapitem const* nh)
{
  size_t const i = *n;
  if (i >= top)
    return tcmplxA_ErrMemory;
  else {
    h[i] = *nh;
    *n = i+1u;
    /* raise */{
      size_t j;
      for (j = i+1u; j > 1u; j>>=1) {
        size_t const here = j-1u;
        size_t const parent = (j>>1)-1u;
        if (tcmplxA_fixlist_heapcmp(&h[here], &h[parent]) < 0) {
          tcmplxA_fixlist_heapswap(h+here, h+parent);
        } else break;
      }
    }
    return tcmplxA_Success;
  }
}

void tcmplxA_fixlist_heapswap
  (struct tcmplxA_fixlist_heapitem* a, struct tcmplxA_fixlist_heapitem* b)
{
  struct tcmplxA_fixlist_heapitem const tmp = *a;
  *a = *b;
  *b = tmp;
  return;
}

int tcmplxA_fixlist_heappop
  ( struct tcmplxA_fixlist_heapitem* h, size_t *n,
    struct tcmplxA_fixlist_heapitem* nh)
{
  size_t const new_size = *n - 1u;
  if (*n == 0) {
    return tcmplxA_ErrOutOfRange;
  } else {
    *nh = h[0u];
    *n = new_size;
    h[0u] = h[new_size];
    /* lower */{
      size_t j;
      for (j = 1u; (j<<1) <= new_size; ) {
        size_t const left = (j<<1)-1u;
        size_t const right = (j<<1);
        size_t const choice =
            ( right >= new_size ||
              tcmplxA_fixlist_heapcmp(&h[left], &h[right]) < 0)
          ? left : right;
        if (tcmplxA_fixlist_heapcmp(&h[choice], &h[j-1u]) < 0) {
          tcmplxA_fixlist_heapswap(&h[choice], &h[j-1u]);
          j = choice+1u;
        } else break;
      }
    }
    return tcmplxA_Success;
  }
}

tcmplxA_uint32 tcmplxA_fixlist_addclamp(tcmplxA_uint32 a, tcmplxA_uint32 b) {
  return a > 0xFFffFFff-b ? 0xFFffFFff : a+b;
}

void tcmplxA_fixlist_heap_add
  ( struct tcmplxA_fixlist_heapitem* c,
    struct tcmplxA_fixlist_heapitem const* a,
    struct tcmplxA_fixlist_heapitem const* b)
{
  c->freq = tcmplxA_fixlist_addclamp(a->freq, b->freq);
  c->total_frac = tcmplxA_fixlist_addclamp(a->total_frac, b->total_frac);
  c->small_frac = tcmplxA_fixlist_addclamp(a->small_frac, b->small_frac);
  c->midline = tcmplxA_fixlist_addclamp(a->midline, b->midline);
  return;
}

int tcmplxA_fixlist_ref_cmp(void const* pa, void const* pb) {
  struct tcmplxA_fixlist_ref const* const a =
    (struct tcmplxA_fixlist_ref const*)pa;
  struct tcmplxA_fixlist_ref const* const b =
    (struct tcmplxA_fixlist_ref const*)pb;
  if (a->freq > b->freq)
    return -1;
  else if (a->freq < b->freq)
    return +1;
  else return 0;
}

int tcmplxA_fixlist_heapcmp
  (struct tcmplxA_fixlist_heapitem* a, struct tcmplxA_fixlist_heapitem* b)
{
  if (a->total_frac < b->total_frac)
    return -1;
  else if (a->total_frac > b->total_frac)
    return +1;
  else if (a->freq < b->freq)
    return -1;
  else if (a->freq > b->freq)
    return +1;
  else return 0;
}

tcmplxA_uint32 tcmplxA_fixlist_min_frac(tcmplxA_uint32 x) {
  unsigned int n;
  if (x == 0u)
    return 0u;
  else for (n = 0u; (x & 1u) == 0u; x >>= 1, ++n) {
    continue;
  }
  return ((tcmplxA_uint32)1u)<<n;
}

int tcmplxA_fixlist_heap_packable
  (struct tcmplxA_fixlist_heapitem* h, size_t n, tcmplxA_uint32 frac)
{
  if (n < 2u)
    return 0;
  else if (h[0].total_frac > frac)
    return 0;
  else {
    if (h[1].total_frac <= frac)
      return 1;
    else if (n < 3u)
      return 0;
    else return (h[2].total_frac <= frac);
  }
}

int tcmplxA_fixline_codecmp(void const* pa, void const* pb) {
  struct tcmplxA_fixline const* const a =
    (struct tcmplxA_fixline const*)pa;
  struct tcmplxA_fixline const* const b =
    (struct tcmplxA_fixline const*)pb;
  if (a->len < b->len)
    return -1;
  else if (a->len > b->len)
    return +1;
  else if (a->code < b->code)
    return -1;
  else return (a->code > b->code);
}

int tcmplxA_fixline_valuecmp(void const* pa, void const* pb) {
  struct tcmplxA_fixline const* const a =
    (struct tcmplxA_fixline const*)pa;
  struct tcmplxA_fixline const* const b =
    (struct tcmplxA_fixline const*)pb;
  if (a->value < b->value)
    return -1;
  else return (a->value > b->value);
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

int tcmplxA_fixlist_gen_lengths
  ( struct tcmplxA_fixlist* dst, tcmplxA_uint32 const* table,
    unsigned int max_bits)
{
  if (dst->n > 32768u || max_bits > 15u)
    return tcmplxA_ErrFixLenRange;
  else if (dst->n > ((size_t)-1)/sizeof(struct tcmplxA_fixlist_ref)
    ||  dst->n > ((size_t)-1)/sizeof(struct tcmplxA_fixlist_heapitem))
  {
    return tcmplxA_ErrMemory;
  } else {
    /* allocate the nodes */
    struct tcmplxA_fixlist_ref* const nodes =
      tcmplxA_util_malloc(dst->n*sizeof(struct tcmplxA_fixlist_ref));
    size_t heap_top = dst->n*2u;
    struct tcmplxA_fixlist_heapitem* const heap =
      tcmplxA_util_malloc(heap_top*sizeof(struct tcmplxA_fixlist_heapitem));
    size_t heap_count = 0u;
    size_t node_count = 0u;
    if (nodes == NULL || heap == NULL) {
      tcmplxA_util_free(heap);
      tcmplxA_util_free(nodes);
      return tcmplxA_ErrMemory;
    }
    /* */{
      size_t i;
      for (i = 0u; i < dst->n; ++i) {
        if (table[i] > 0u) {
          struct tcmplxA_fixlist_ref ref;
          ref.index = i;
          ref.freq = table[i];
          nodes[node_count++] = ref;
        } else {
          dst->p[i].len = 0u;
        }
      }
      if (node_count <= 2u) {
        if (max_bits > 0u) {
          for (i = 0u; i < node_count; ++i)
            dst->p[nodes[i].index].len = 1u;
          tcmplxA_util_free(heap);
          tcmplxA_util_free(nodes);
          return tcmplxA_Success;
        } else {
          tcmplxA_util_free(heap);
          tcmplxA_util_free(nodes);
          return tcmplxA_Success;
        }
      }
      qsort(nodes, node_count, sizeof(struct tcmplxA_fixlist_ref),
          &tcmplxA_fixlist_ref_cmp);
    }
    /* do the algorithm */{
      struct tcmplxA_fixlist_state states[16];
      size_t current_state = 0u;
      size_t state_total = 0u;
#ifndef NDEBUG
      memset(states, 0, sizeof(states));
#endif /*NDEBUG*/
      /* initialize */{
        states[0].args.begin = 0u;
        states[0].args.end = node_count;
        states[0].args.upper_bound = -1;
        states[0].args.lower_bound = -(int)max_bits;
        states[0].args.total_frac = ((tcmplxA_uint32)node_count-1u)<<max_bits;
        state_total += 1u;
      }
      /* run */while (current_state < state_total) {
        struct tcmplxA_fixlist_state st = states[current_state];
        int l;
        unsigned long int rem_frac = st.args.total_frac;
        int const midline = st.args.upper_bound +
              (st.args.lower_bound-st.args.upper_bound-1)/2;
        /* */{
          memset(&st.item, 0, sizeof(struct tcmplxA_fixlist_heapitem));
        }
        for (l = st.args.lower_bound; rem_frac > 0u; ++l) {
          tcmplxA_uint32 const frac = 1u<<((int)max_bits+l);
          tcmplxA_uint32 const min_frac = tcmplxA_fixlist_min_frac(rem_frac);
          /* add some items */if (l <= st.args.upper_bound) {
            size_t i;
            for (i = st.args.begin; i < st.args.end; ++i) {
              struct tcmplxA_fixlist_heapitem it;
              int res;
              it.freq = nodes[i].freq;
              it.total_frac = frac;
              it.small_frac = (l < midline ? frac : 0u);
              it.midline = (l==midline ? 1u : 0u);
              res = tcmplxA_fixlist_heappush(heap, &heap_count, heap_top, &it);
              if (/*unlikely*/res != tcmplxA_Success) {
                tcmplxA_util_free(heap);
                tcmplxA_util_free(nodes);
                return res;
              }
            }
          }
          /* inspect */
          if (heap_count == 0u || frac > min_frac) {
            tcmplxA_util_free(heap);
            tcmplxA_util_free(nodes);
            return tcmplxA_ErrFixLenRange;
          } else if (frac == min_frac) {
            if (heap[0].total_frac != min_frac) {
              tcmplxA_util_free(heap);
              tcmplxA_util_free(nodes);
              return tcmplxA_ErrFixLenRange;
            } else {
              struct tcmplxA_fixlist_heapitem addend;
              rem_frac -= min_frac;
              tcmplxA_fixlist_heappop(heap, &heap_count, &addend);
              tcmplxA_fixlist_heap_add(&st.item, &st.item, &addend);
            }
          }
          /* pack the rest */{
            while (tcmplxA_fixlist_heap_packable(heap, heap_count, frac)) {
              struct tcmplxA_fixlist_heapitem item1;
              struct tcmplxA_fixlist_heapitem item2;
              tcmplxA_fixlist_heappop(heap, &heap_count, &item1);
              tcmplxA_fixlist_heappop(heap, &heap_count, &item2);
              tcmplxA_fixlist_heap_add(&item1, &item1, &item2);
              tcmplxA_fixlist_heappush(heap, &heap_count, heap_top, &item1);
            }
            if (heap_count > 0u && heap[0].total_frac <= frac) {
              /* discard */
              struct tcmplxA_fixlist_heapitem it;
              tcmplxA_fixlist_heappop(heap, &heap_count, &it);
            }
          }
        }
        states[current_state] = st;
        /* add new states */{
          int const bound_span =
            (st.args.upper_bound-st.args.lower_bound);
          if (bound_span >= 3) {
            /* construct "D" span */{
              struct tcmplxA_fixlist_state d_state = {{0}};
              d_state.args.begin = st.args.end-st.item.midline;
              d_state.args.end = st.args.end;
              d_state.args.upper_bound = midline-1;
              d_state.args.lower_bound = st.args.lower_bound;
              d_state.args.total_frac = st.item.small_frac;
              states[state_total] = d_state;
              state_total += 1u;
            }
            /* construct "A" span */{
              int const imax_bits = (int)max_bits;
              struct tcmplxA_fixlist_state a_state = {{0}};
              a_state.args.begin = st.args.begin;
              a_state.args.end = st.args.end-st.item.midline;
              a_state.args.upper_bound = st.args.upper_bound;
              a_state.args.lower_bound = midline+1;
              a_state.args.total_frac =
                  st.item.total_frac
                - st.item.small_frac
                -   ( (1u<<(imax_bits+st.args.upper_bound+1))
                    - (1u<<(imax_bits+midline)))
                  * st.item.midline;
              states[state_total] = a_state;
              state_total += 1u;
            }
          }
        }
        current_state += 1u;
        heap_count = 0u;
      }
      /* parse out the lengths */{
        size_t i;
        tcmplxA_uint32 lengths[16] = {0u};
        int const imax_bits = (int)max_bits;
        for (i = 0u; i < state_total; ++i) {
          struct tcmplxA_fixlist_state st = states[i];
          tcmplxA_uint32 const offset = (tcmplxA_uint32)node_count-st.args.end;
          int const bound_span = st.args.upper_bound-st.args.lower_bound;
          int const midline = st.args.lower_bound+(bound_span/2);
          lengths[-midline] = offset+st.item.midline;
          if (bound_span <= 2) {
            if (st.args.upper_bound > midline) {
              tcmplxA_uint32 const x =
                  st.item.total_frac
                - (st.item.midline<<(imax_bits+midline))
                - st.item.small_frac;
              lengths[-st.args.upper_bound] =
                (x>>(imax_bits+st.args.upper_bound)) + offset;
            }
            if (st.args.lower_bound < midline) {
              tcmplxA_uint32 const x = st.item.small_frac;
              lengths[-st.args.lower_bound] =
                (x>>(imax_bits+st.args.lower_bound)) + offset;
            }
          }
        }
        /* post to the prefix list */{
          size_t i = node_count;
          size_t j;
          for (j = 15u; j > 0u; --j) {
            size_t const front = node_count-lengths[j];
            for (; i > front; --i) {
              dst->p[nodes[i-1u].index].len = j;
            }
          }
        }
      }
    }
    tcmplxA_util_free(heap);
    tcmplxA_util_free(nodes);
    return tcmplxA_Success;
  }
}

int tcmplxA_fixlist_codesort(struct tcmplxA_fixlist* dst) {
  qsort
    ( dst->p, dst->n,
      sizeof(struct tcmplxA_fixline), tcmplxA_fixline_codecmp);
  return tcmplxA_Success;
}

size_t tcmplxA_fixlist_codebsearch
  (struct tcmplxA_fixlist const* dst, unsigned int n, unsigned int bits)
{
  struct tcmplxA_fixline key;
  key.len = (unsigned short)n;
  key.code = (unsigned short)bits;
  /* */{
    struct tcmplxA_fixline* x = bsearch
      ( &key, dst->p, dst->n, sizeof(struct tcmplxA_fixline),
        tcmplxA_fixline_codecmp);
    return x!=NULL ? (size_t)(x-dst->p) : ((size_t)-1);
  }
}

int tcmplxA_fixlist_valuesort(struct tcmplxA_fixlist* dst) {
  qsort
    ( dst->p, dst->n,
      sizeof(struct tcmplxA_fixline), tcmplxA_fixline_valuecmp);
  return tcmplxA_Success;
}

size_t tcmplxA_fixlist_valuebsearch
  (struct tcmplxA_fixlist const* dst, unsigned long int value)
{
  struct tcmplxA_fixline key;
  key.value = value;
  /* */{
    struct tcmplxA_fixline* x = bsearch
      ( &key, dst->p, dst->n, sizeof(struct tcmplxA_fixline),
        tcmplxA_fixline_valuecmp);
    return x!=NULL ? (size_t)(x-dst->p) : ((size_t)-1);
  }
}
/* END   prefix list / public */
