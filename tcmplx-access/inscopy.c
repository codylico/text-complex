/**
 * @file tcmplx-access/inscopy.c
 * @brief table for insert and copy lengths
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "inscopy.h"
#include "api.h"
#include "util.h"
#include <string.h>

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
 * @brief Initialize a insert copy table.
 * @param x the insert copy table to initialize
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_inscopy_init(struct tcmplxA_inscopy* x, int t);
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

/* BEGIN insert copy table / static */
int tcmplxA_inscopy_init(struct tcmplxA_inscopy* x, int t) {
  x->p = NULL;
  x->n = 0u;
  switch (t) {
  case tcmplxA_InsCopy_Deflate:
    /* */ {
      int const resize_res = tcmplxA_inscopy_resize(x,286);
      if (resize_res != tcmplxA_Success) {
        return resize_res;
      }
      tcmplxA_inscopy_1951_fill(x->p);
    }break;
  case tcmplxA_InsCopy_Brotli:
    /* */ {
      int const resize_res = tcmplxA_inscopy_resize(x,704);
      if (resize_res != tcmplxA_Success) {
        return resize_res;
      }
      tcmplxA_inscopy_7932_fill(x->p);
    }break;
  default:
    return tcmplxA_ErrParam;
  }
  return tcmplxA_Success;
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
  unsigned short first_insert = 3u;
  unsigned short bits = 0u;
  /* put literals */for (i = 0u; i < 256u; ++i) {
    r[i].type = tcmplxA_InsCopy_Literal;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = 0;
    r[i].copy_bits = 0;
    r[i].insert_first = 0;
    r[i].copy_first = 0;
  }
  /* put stop code */{ /* i=256; */
    r[i].type = tcmplxA_InsCopy_Stop;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = 0;
    r[i].copy_bits = 0;
    r[i].insert_first = 0;
    r[i].copy_first = 0;
    ++i;
  }
  /* put some zero-bit insert codes */for (; i < 261u; ++i) {
    r[i].type = tcmplxA_InsCopy_Insert;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = 0;
    r[i].copy_bits = 0;
    r[i].insert_first = first_insert;
    r[i].copy_first = 0;
    first_insert += 1u;
  }
  /* put most of the other insert codes */for (; i < 285u; ++i) {
    r[i].type = tcmplxA_InsCopy_Insert;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = bits;
    r[i].copy_bits = 0;
    r[i].insert_first = first_insert;
    r[i].copy_first = 0;
    first_insert += (1u<<bits);
    if ((i%4) == 0) {
      bits += 1u;
    }
  }
  /* put code 285 */{ /* i=285; */
    r[i].type = tcmplxA_InsCopy_Insert;
    r[i].zero_distance_tf = 0;
    r[i].insert_bits = 0;
    r[i].copy_bits = 0;
    r[i].insert_first = first_insert;
    r[i].copy_first = 0;
    /* ++i; */
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
    r[i].type = tcmplxA_InsCopy_InsertCopy;
    r[i].zero_distance_tf = lookup_matrix[j].zero_dist_tf;
    r[i].insert_bits = insert_tabs[insert_code].bits;
    r[i].insert_first = insert_tabs[insert_code].first;
    r[i].copy_bits = copy_tabs[copy_code].bits;
    r[i].copy_first = copy_tabs[copy_code].first;
  }
  return;
}
/* END   insert copy table / static */

/* BEGIN insert copy table / public */
struct tcmplxA_inscopy* tcmplxA_inscopy_new(int t) {
  struct tcmplxA_inscopy* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_inscopy));
  if (out != NULL
  &&  tcmplxA_inscopy_init(out, t) != tcmplxA_Success)
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
/* END   insert copy table / public */
