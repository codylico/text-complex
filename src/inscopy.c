/**
 * @file text-complex/access/inscopy.c
 * @brief table for insert and copy lengths
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/inscopy.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>


/**
 * @brief Type of insert copy table row.
 */
enum tcmplxA_inscopy_type_ex {
  /**
   * @brief Match either an insert, an insert-copy, or a block count row.
   */
  tcmplxA_InsCopy_RowInsertAny = 255u
};

struct tcmplxA_inscopy {
  struct tcmplxA_inscopy_row* p;
  size_t n;
};

/**
 * @brief Fill an insert-copy table with the DEFLATE
 *   literal-length alphabet.
 * @param r array of 286 rows
 */
static void tcmplxA_inscopy_1951_fill(struct tcmplxA_inscopy_row* r);
/**
 * @brief Fill an insert-copy table with the Brotli
 *   length alphabet cross product.
 * @param r array of 704 rows
 */
static void tcmplxA_inscopy_7932_fill(struct tcmplxA_inscopy_row* r);
/**
 * @brief Fill an insert-copy table with the Brotli block code alphabet.
 * @param r array of 26 rows
 */
static void tcmplxA_inscopy_7932B_fill(struct tcmplxA_inscopy_row* r);
/**
 * @brief Initialize a insert copy table.
 * @param x the insert copy table to initialize
 * @param n desired row count
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_inscopy_init(struct tcmplxA_inscopy* x, size_t n);
/**
 * \brief Resize an insert copy table.
 * \param x the table to resize
 * \param sz number of rows in the table
 * \return zero on success, nonzero otherwise
 */
static int tcmplxA_inscopy_resize(struct tcmplxA_inscopy* x, size_t sz);
/**
 * @brief Close a insert copy table.
 * @param x the insert copy table to close
 */
static void tcmplxA_inscopy_close(struct tcmplxA_inscopy* x);
/**
 * @brief Compare the codes of two table rows.
 * @param a left insert copy row
 * @param b right insert copy row
 * @return -1,0,+1 for `a<b`,`a==b`,`a>b`
 */
static int tcmplxA_inscopy_code_cmp(void const* a, void const* b);
/**
 * @brief Compare the starting lengths for two table rows.
 * @param a left insert copy row
 * @param b right insert copy row
 * @return -1,0,+1 for `a<b`,`a==b`,`a>b`
 */
static int tcmplxA_inscopy_length_cmp(void const* a, void const* b);
/**
 * @brief Compare the starting lengths for two table rows.
 * @param k search key
 * @param icr table insert copy row
 * @return -1,0,+1 for `a<b`,`a==b`,`a>b`
 */
static int tcmplxA_inscopy_encode_cmp(void const* k, void const* icr);


static
struct { void (*f)(struct tcmplxA_inscopy_row*); size_t n; }
const tcmplxA_inscopy_ps[] = {
  { tcmplxA_inscopy_1951_fill, 286u },
  { tcmplxA_inscopy_7932_fill, 704u },
  { tcmplxA_inscopy_7932B_fill, 26u }
};



/* BEGIN insert copy table / static */
int tcmplxA_inscopy_init(struct tcmplxA_inscopy* x, size_t n) {
  x->p = NULL;
  x->n = 0u;
  return tcmplxA_inscopy_resize(x,n);
}

int tcmplxA_inscopy_resize(struct tcmplxA_inscopy* x, size_t sz) {
  struct tcmplxA_inscopy_row *ptr;
  if (sz == 0u) {
    if (x->p != NULL) {
      tcmplxA_util_free(x->p);
    }
    x->p = NULL;
    x->n = 0u;
    return tcmplxA_Success;
  } else if (sz >= UINT_MAX/sizeof(struct tcmplxA_inscopy_row)) {
    return tcmplxA_ErrMemory;
  }
  ptr = (struct tcmplxA_inscopy_row*)tcmplxA_util_malloc
          (sizeof(struct tcmplxA_inscopy_row)*sz);
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

void tcmplxA_inscopy_close(struct tcmplxA_inscopy* x) {
  tcmplxA_util_free(x->p);
  x->p = NULL;
  x->n = 0u;
  return;
}

void tcmplxA_inscopy_1951_fill(struct tcmplxA_inscopy_row* r) {
  size_t i;
  unsigned short first_copy = 3u;
  unsigned short bits = 0u;
  /* put literals */for (i = 0u; i < 256u; ++i) {
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_Literal;
    r[i].zero_distance_tf = 0;
    r[i].copy_bits = 0;
    r[i].insert_bits = 0;
    r[i].copy_first = 0;
    r[i].insert_first = 0;
  }
  /* put stop code */{ /* i=256; */
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_Stop;
    r[i].zero_distance_tf = 0;
    r[i].copy_bits = 0;
    r[i].insert_bits = 0;
    r[i].copy_first = 0;
    r[i].insert_first = 0;
    ++i;
  }
  /* put some zero-bit insert codes */for (; i < 261u; ++i) {
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_Copy;
    r[i].zero_distance_tf = 0;
    r[i].copy_bits = 0;
    r[i].insert_bits = 0;
    r[i].copy_first = first_copy;
    r[i].insert_first = 0;
    first_copy += 1u;
  }
  /* put most of the other insert codes */for (; i < 285u; ++i) {
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_Copy;
    r[i].zero_distance_tf = 0;
    r[i].copy_bits = bits;
    r[i].insert_bits = 0;
    r[i].copy_first = first_copy;
    r[i].insert_first = 0;
    first_copy += (1u<<bits);
    if ((i%4) == 0) {
      bits += 1u;
    }
  }
  /* adjust 284 */{
    r[284].type = tcmplxA_InsCopy_CopyMinus1;
    first_copy -= 1u;
  }
  /* put code 285 */{ /* i=285; */
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_Copy;
    r[i].zero_distance_tf = 0;
    r[i].copy_bits = 0;
    r[i].insert_bits = 0;
    r[i].copy_first = first_copy;
    r[i].insert_first = 0;
    /* ++i; */
  }
  return;
}

void tcmplxA_inscopy_7932B_fill(struct tcmplxA_inscopy_row* r) {
  size_t i;
  unsigned short first = 1u;
  static unsigned char const bits[26] = {
      2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 6,6, 7, 8, 9, 10, 11, 12, 13, 24
    };
  for (i = 0u; i < 26u; ++i) {
    r[i].code = (unsigned short)i;
    r[i].type = tcmplxA_InsCopy_BlockCount;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = bits[i];
    r[i].copy_bits = 0;
    r[i].insert_first = first;
    r[i].copy_first = 0;
    first = (unsigned short)(first + (1u << bits[i]));
  }
  return;
}

void tcmplxA_inscopy_7932_fill(struct tcmplxA_inscopy_row* r) {
  size_t i;
  struct tab {
    unsigned char bits;
    unsigned short first;
  };
  static struct {
    unsigned char zero_dist_tf : 1;
    unsigned short insert_start : 5;
    unsigned short copy_start : 5;
  } const lookup_matrix[11] = {
    {1, 0, 0}, {1, 0, 8},
    {0, 0, 0}, {0, 0, 8}, /*      */
    {0, 8, 0}, {0, 8, 8}, /*      */
    /*      */ /*      */ /*      */

    /*      */ /*      */ {0, 0,16},
    /*      */ /*      */ /*      */
    {0,16, 0}, /*      */ /*      */

    /*      */ /*      */ /*      */
    /*      */ /*      */ {0, 8,16},
    /*      */ {0,16, 8}, {0,16,16}
  };
  static struct tab const insert_tabs[24] = {
    {0,  0}, {0,  1}, {0,   2}, {0,   3}, {0,   4}, {0,    5},
    {1,  6}, {1,  8}, {2,  10}, {2,  14}, {3,  18}, {3,   26},
    {4, 34}, {4, 50}, {5,  66}, {5,  98}, {6, 130}, {7,  194},
    {8,322}, {9,578},{10,1090},{12,2114},{14,6210},{24,22594}
  };
  static struct tab const copy_tabs[24] = {
    {0,  2}, {0,  3}, {0,   4}, {0,   5}, {0,   6}, {0,    7},
    {0,  8}, {0,  9}, {1,  10}, {1,  12}, {2,  14}, {2,   18},
    {3, 22}, {3, 30}, {4,  38}, {4,  54}, {5,  70}, {5,  102},
    {6,134}, {7,198}, {8, 326}, {9, 582},{10,1094},{24, 2118}
  };
  for (i = 0u; i < 704u; ++i) {
    /* decouple insert code from copy code */
    size_t const j = i/64u;
    unsigned int const insert_code =
      lookup_matrix[j].insert_start + ((i>>3)&7);
    unsigned int const copy_code =
      lookup_matrix[j].copy_start + (i&7);
    /* fill table row */
    r[i].code = (unsigned short)(i);
    r[i].type = tcmplxA_InsCopy_InsertCopy;
    r[i].zero_distance_tf = lookup_matrix[j].zero_dist_tf;
    r[i].insert_bits = insert_tabs[insert_code].bits;
    r[i].insert_first = insert_tabs[insert_code].first;
    r[i].copy_bits = copy_tabs[copy_code].bits;
    r[i].copy_first = copy_tabs[copy_code].first;
  }
  return;
}

int tcmplxA_inscopy_code_cmp(void const* a, void const* b) {
  struct tcmplxA_inscopy_row const*const a_row =
    (struct tcmplxA_inscopy_row const*)a;
  struct tcmplxA_inscopy_row const*const b_row =
    (struct tcmplxA_inscopy_row const*)b;
  if (a_row->code < b_row->code)
    return -1;
  else if (a_row->code > b_row->code)
    return +1;
  else return 0;
}

int tcmplxA_inscopy_length_cmp(void const* a, void const* b) {
  struct tcmplxA_inscopy_row const*const a_row =
    (struct tcmplxA_inscopy_row const*)a;
  struct tcmplxA_inscopy_row const*const b_row =
    (struct tcmplxA_inscopy_row const*)b;
  int const a_zero_tf = a_row->zero_distance_tf ? 1 : 0;
  int const b_zero_tf = b_row->zero_distance_tf ? 1 : 0;
  unsigned int const a_type = a_row->type&127u;
  unsigned int const b_type = b_row->type&127u;
  if (a_type < b_type)
    return -1;
  else if (a_type > b_type)
    return +1;
  else if (a_zero_tf < b_zero_tf)
    return -1;
  else if (a_zero_tf > b_zero_tf)
    return +1;
  else if (a_row->insert_first < b_row->insert_first)
    return -1;
  else if (a_row->insert_first > b_row->insert_first)
    return +1;
  else if (a_row->copy_first < b_row->copy_first)
    return -1;
  else return a_row->copy_first > b_row->copy_first;
}

int tcmplxA_inscopy_encode_cmp(void const* k, void const* icr) {
  /*
   * NOTE The corresponding function in the C++ implementation
   *   (`static ...::inscopy_encode_cmp`) has the key and table row
   *   arguments reversed, and thus the comparison result is reversed.
   */
  struct tcmplxA_inscopy_row const*const key =
    (struct tcmplxA_inscopy_row const*)k;
  struct tcmplxA_inscopy_row const*const ic_row =
    (struct tcmplxA_inscopy_row const*)icr;
  int const icr_zero_tf = ic_row->zero_distance_tf ? 1 : 0;
  if (tcmplxA_InsCopy_Copy > ic_row->type)
    return +1;
  else if (key->zero_distance_tf < icr_zero_tf)
    return -1;
  else if (key->zero_distance_tf > icr_zero_tf)
    return +1;
  else if (key->insert_first < ic_row->insert_first)
    return -1;
  else {
    unsigned long int const insert_end =
      ic_row->insert_first + (1UL<<ic_row->insert_bits);
    if (key->insert_first >= insert_end)
      return +1;
    else if (key->copy_first < ic_row->copy_first)
      return -1;
    else {
      unsigned long int const copy_end =
          ic_row->copy_first + (1UL<<ic_row->copy_bits)
        - ((ic_row->type&128u) ? 1u : 0u);
      if (key->copy_first >= copy_end)
        return +1;
      else return 0;
    }
  }
}
/* END   insert copy table / static */

/* BEGIN insert copy table / public */
struct tcmplxA_inscopy* tcmplxA_inscopy_new(size_t n) {
  struct tcmplxA_inscopy* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_inscopy));
  if (out != NULL
  &&  tcmplxA_inscopy_init(out, n) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_inscopy_destroy(struct tcmplxA_inscopy* x) {
  if (x != NULL) {
    tcmplxA_inscopy_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

size_t tcmplxA_inscopy_size(struct tcmplxA_inscopy const* x) {
  return x->n;
}

struct tcmplxA_inscopy_row const* tcmplxA_inscopy_at_c
  (struct tcmplxA_inscopy const* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

struct tcmplxA_inscopy_row* tcmplxA_inscopy_at
  (struct tcmplxA_inscopy* x, size_t i)
{
  if (i >= x->n)
    return NULL;
  else return &x->p[i];
}

int tcmplxA_inscopy_copy
  (struct tcmplxA_inscopy* dst, struct tcmplxA_inscopy const* src)
{
  int res;
  if (dst == src) {
    return tcmplxA_Success;
  }
  res = tcmplxA_inscopy_resize(dst, src->n);
  if (res != tcmplxA_Success) {
    return res;
  }
  memcpy(dst->p, src->p, sizeof(struct tcmplxA_inscopy_row)*dst->n);
  return tcmplxA_Success;
}

int tcmplxA_inscopy_preset(struct tcmplxA_inscopy* dst, int t) {
  size_t const n = sizeof(tcmplxA_inscopy_ps)/sizeof(tcmplxA_inscopy_ps[0]);
  if (t < 0 || ((size_t)t) >= n)
    return tcmplxA_ErrParam;
  /* resize */{
    size_t const sz = tcmplxA_inscopy_ps[t].n;
    int const res =
      tcmplxA_inscopy_resize(dst, sz);
    if (res != tcmplxA_Success) {
      return res;
    }
    (*tcmplxA_inscopy_ps[t].f)(dst->p);
  }
  return tcmplxA_Success;
}

int tcmplxA_inscopy_codesort(struct tcmplxA_inscopy* ict) {
  qsort(ict->p, ict->n, sizeof(struct tcmplxA_inscopy_row),
    tcmplxA_inscopy_code_cmp);
  /*
   * NOTE This function has a return value to preserve consistency with
   *   the C++ version of this function. (See `...::inscopy_codesort`.)
   */
  return tcmplxA_Success;
}

int tcmplxA_inscopy_lengthsort(struct tcmplxA_inscopy* ict) {
  qsort(ict->p, ict->n, sizeof(struct tcmplxA_inscopy_row),
    tcmplxA_inscopy_length_cmp);
  /*
   * NOTE This function has a return value to preserve consistency with
   *   the C++ version of this function. (See `...::inscopy_lengthsort`.)
   */
  return tcmplxA_Success;
}

size_t tcmplxA_inscopy_encode
  ( struct tcmplxA_inscopy const* ict, unsigned long int i,
    unsigned long int c, int z_tf)
{
  struct tcmplxA_inscopy_row const key = {
      /*type=*/ tcmplxA_InsCopy_RowInsertAny,
      /*zero_distance_tf=*/ z_tf?1u:0u,
      /*insert_bits=*/ 0,
      /*copy_bits=*/ 0,
      /*insert_first=*/ i>USHRT_MAX ? USHRT_MAX : (unsigned short int)i,
      /*copy_first=*/ c>USHRT_MAX ? USHRT_MAX : (unsigned short int)c,
      /*code=*/ ~(unsigned short int)0u
    };
  struct tcmplxA_inscopy_row const* out = bsearch
      (&key, ict->p, ict->n, sizeof(struct tcmplxA_inscopy_row),
        tcmplxA_inscopy_encode_cmp);
  /* range check */if (out != NULL) {
    if (key.zero_distance_tf != (out->zero_distance_tf!=0)
    ||  i < out->insert_first
    ||  i >= out->insert_first+(1UL<<out->insert_bits)
    ||  c < out->copy_first
    ||  c >= out->copy_first+(1UL<<out->copy_bits))
      return ~(size_t)0u;
    else return (size_t)(out - ict->p);
  } else return ~(size_t)0u;
}
/* END   insert copy table / public */
