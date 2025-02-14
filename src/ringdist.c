/**
 * @file text-complex/access/ringdist.c
 * @brief Distance ring buffer
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/ringdist.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>

struct tcmplxA_ringdist {
  tcmplxA_uint32 ring[4];
  unsigned short int i;
  unsigned short int special_size;
  unsigned short int sum_direct : 8;
  unsigned short int direct_one : 8;
  unsigned short int postfix : 4;
  unsigned short int bit_adjust : 4;
  unsigned short int postmask : 8;
};

/**
 * @brief Initialize a distance ring.
 * @param x the distance ring to initialize
 * @param s_tf special flag
 * @param d direct count
 * @param p postfix size
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_ringdist_init
  (struct tcmplxA_ringdist* x, int s_tf, unsigned int d, unsigned int p);
/**
 * @brief Close a distance ring.
 * @param x the distance ring to close
 */
static void tcmplxA_ringdist_close(struct tcmplxA_ringdist* x);
/**
 * @brief Record a back distance in a ring buffer.
 * @param ring the buffer
 * @param i index
 * @param v the value to record
 */
static void tcmplxA_ringdist_record
  (tcmplxA_uint32* ring, unsigned short* i, tcmplxA_uint32 v);
/**
 * @brief Retrieve a back distance from a ring buffer.
 * @param ring the buffer
 * @param i current buffer index
 * @param nlast position of value to retrieve (2 for second last, ...)
 * @return the recorded value
 */
static tcmplxA_uint32 tcmplxA_ringdist_retrieve
  (tcmplxA_uint32* ring, unsigned int i, unsigned int nlast);
/**
 * @brief Compute the number of bits in a number.
 * @param n this number
 * @return a bit count
 */
static unsigned int tcmplxA_ringdist_bitlen(tcmplxA_uint32 n);

/* BEGIN distance ring / static */
int tcmplxA_ringdist_init
  (struct tcmplxA_ringdist* x, int s_tf, unsigned int d, unsigned int p)
{
  static tcmplxA_uint32 const base_ring[4] = { 4u, 11u, 15u, 16u };
  if (d > 120 || p > 3)
    return tcmplxA_ErrParam;
  x->i = 0u;
  memcpy(x->ring, base_ring, 4*sizeof(tcmplxA_uint32));
  x->special_size = s_tf?16u:0u;
  x->sum_direct = d+x->special_size;
  x->direct_one = d+1u;
  x->postfix = p;
  x->bit_adjust = p+1u;
  x->postmask = (1u<<p)-1u;
  return tcmplxA_Success;
}

void tcmplxA_ringdist_close(struct tcmplxA_ringdist* x) {
  return;
}

void tcmplxA_ringdist_record
  (tcmplxA_uint32* ring, unsigned short* i, tcmplxA_uint32 v)
{
  unsigned short int const xi = *i;
  ring[xi] = v;
  (*i) = (xi+1u)%4u;
  return;
}

unsigned int tcmplxA_ringdist_bitlen(tcmplxA_uint32 n) {
  unsigned int out = 0u;
  while (n > 0) {
    n >>= 1;
    out += 1u;
  }
  return out;
}

tcmplxA_uint32 tcmplxA_ringdist_retrieve
  (tcmplxA_uint32* ring, unsigned int i, unsigned int nlast)
{
  return ring[(i+4u-nlast)%4u];
}
/* END   distance ring / static */

/* BEGIN distance ring / public */
struct tcmplxA_ringdist* tcmplxA_ringdist_new
    (int special_tf, unsigned int direct, unsigned int postfix)
{
  struct tcmplxA_ringdist* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_ringdist));
  if (out != NULL
  &&  tcmplxA_ringdist_init(out,special_tf,direct,postfix) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_ringdist_destroy(struct tcmplxA_ringdist* x) {
  if (x != NULL) {
    tcmplxA_ringdist_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

unsigned int tcmplxA_ringdist_bit_count
  (struct tcmplxA_ringdist const* x, unsigned int dcode)
{
  if (dcode < x->sum_direct) {
    return 0u;
  } else return 1u + ((dcode - x->sum_direct)>>(x->bit_adjust));
}

tcmplxA_uint32 tcmplxA_ringdist_decode
  (struct tcmplxA_ringdist* x, unsigned int dcode, tcmplxA_uint32 extra)
{
  if (dcode < x->special_size) {
    tcmplxA_uint32 out;
    /* identify ring entry */switch (dcode) {
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      out = tcmplxA_ringdist_retrieve(x->ring, x->i, 1u);
      break;
    case 1:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      out = tcmplxA_ringdist_retrieve(x->ring, x->i, 2u);
      break;
    case 2:
      out = tcmplxA_ringdist_retrieve(x->ring, x->i, 3u);
      break;
    case 3:
      out = tcmplxA_ringdist_retrieve(x->ring, x->i, 4u);
      break;
    }
    /* adjust */switch (dcode) {
    case 4:
    case 10:
      if (out > 1u) {
        out -= 1u;
      } else {
        /* ae = tcmplxA_ErrRingDistUnderflow; */
        return 0u;
      } break;
    case 5:
    case 11:
      if (out < 0xFFffFFfe) {
        out += 1u;
      } else {
        /* ae = tcmplxA_ErrRingDistOverflow; */
        return 0u;
      } break;
    case 6:
    case 12:
      if (out > 2u) {
        out -= 2u;
      } else {
        /* ae = tcmplxA_ErrRingDistUnderflow; */
        return 0u;
      } break;
    case 7:
    case 13:
      if (out < 0xFFffFFfd) {
        out += 2u;
      } else {
        /* ae = tcmplxA_ErrRingDistOverflow; */
        return 0u;
      } break;
    case 8:
    case 14:
      if (out > 3u) {
        out -= 3u;
      } else {
        /* ae = tcmplxA_ErrRingDistUnderflow; */
        return 0u;
      } break;
    case 9:
    case 15:
      if (out < 0xFFffFFfc) {
        out += 3u;
      } else {
        /* ae = tcmplxA_ErrRingDistOverflow; */
        return 0u;
      } break;
    }
    if (dcode != 0u) {
      tcmplxA_ringdist_record(x->ring, &x->i, out);
    }
    return out;
  } else if (dcode < x->sum_direct) {
    tcmplxA_uint32 const out = (dcode - x->special_size) + 1u;
    tcmplxA_ringdist_record(x->ring, &x->i, out);
    return out;
  } else {
    unsigned int const xcode = dcode - x->sum_direct;
    unsigned int const bit_size = tcmplxA_ringdist_bit_count(x, dcode);
    tcmplxA_uint32 const high = (tcmplxA_uint32)(xcode>>(x->postfix));
    tcmplxA_uint32 const low = (tcmplxA_uint32)(xcode&x->postmask);
    tcmplxA_uint32 const offset = ((2u | (high&1)) << bit_size) - 4u;
    tcmplxA_uint32 const out =
      ((offset + extra)<<x->postfix) + low + x->direct_one;
    /* record the new flat distance */{
      tcmplxA_ringdist_record(x->ring, &x->i, out);
    }
    return out;
  }
}

unsigned int tcmplxA_ringdist_encode
  (struct tcmplxA_ringdist* x, unsigned int back_dist, tcmplxA_uint32 *extra)
{
  if (back_dist == 0u) {
    /* zero not allowed */
    /* ae = tcmplxA_ErrRingDistUnderflow; */
    return ~0u;
  } else if (back_dist >= 0xFFffFFfc) {
    /* overflow will result in calculation */
    /* ae = tcmplxA_ErrRingDistOverflow; */
    return ~0u;
  }
  /* check special cache */if (x->special_size) {
    unsigned int j;
    for (j = 0u; j < 4u; ++j) {
      if (back_dist == x->ring[j]) {
        unsigned int const last = (x->i+3u)%4u/* == (i minus 1) mod 4 */;
        *extra = 0u;
        if (last != j) {
          tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
        }
        return (last+4u-j)%4u/* == (last minus j) mod 4 */;
      }
    }
    /* check proximity to last cache item */{
      tcmplxA_uint32 const last = tcmplxA_ringdist_retrieve(x->ring, x->i, 1u);
      tcmplxA_uint32 const last_min = ((last > 3u) ? last-3u : 1u);
      tcmplxA_uint32 const last_max =
        ((last < 0xFFffFFfd) ? last+3u : 0xFFffFFff);
      if (back_dist >= last_min && back_dist < last) {
        *extra = 0u;
        tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
        switch (last-back_dist) {
        case 1u: return 4u;
        case 2u: return 6u;
        case 3u: return 8u;
        }
      } else if (back_dist > last && back_dist <= last_max) {
        *extra = 0u;
        tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
        switch (back_dist-last) {
        case 1u: return 5u;
        case 2u: return 7u;
        case 3u: return 9u;
        }
      }
    }
    /* check proximity to second cache item */{
      tcmplxA_uint32 const second =
        tcmplxA_ringdist_retrieve(x->ring, x->i, 2u);
      tcmplxA_uint32 const second_min = ((second > 3u) ? second-3u : 1u);
      tcmplxA_uint32 const second_max =
        ((second < 0xFFffFFfd) ? second+3u : 0xFFffFFff);
      if (back_dist >= second_min && back_dist < second) {
        *extra = 0u;
        tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
        switch (second-back_dist) {
        case 1u: return 10u;
        case 2u: return 12u;
        case 3u: return 14u;
        }
      } else if (back_dist > second && back_dist <= second_max) {
        *extra = 0u;
        tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
        switch (back_dist-second) {
        case 1u: return 11u;
        case 2u: return 13u;
        case 3u: return 15u;
        }
      }
    }
  }
  /* check direct distances */
  if (back_dist < x->direct_one) {
    *extra = 0u;
    tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
    return (back_dist-1u)+(x->special_size);
  } else {
    tcmplxA_uint32 const sd = back_dist - x->direct_one;
    unsigned int const lowcode = (sd & x->postmask);
    tcmplxA_uint32 const ox = ((sd-lowcode)>>x->postfix)+4u;
    unsigned int const ndistbits = tcmplxA_ringdist_bitlen(ox)-2u;
    tcmplxA_uint32 const out_extra =
      (tcmplxA_uint32)(ox&((1ul<<ndistbits)-1u));
    unsigned int const midcode_x = ((ox>>ndistbits)&1u)<<x->postfix;
    unsigned int const dcode =
        (((ndistbits-1u)<<(x->bit_adjust)) | midcode_x | lowcode)
      + x->sum_direct;
    /* record the new flat distance */{
      tcmplxA_ringdist_record(x->ring, &x->i, back_dist);
    }
    *extra = out_extra;
    return dcode;
  }
}

void tcmplxA_ringdist_copy
  (struct tcmplxA_ringdist* dst, struct tcmplxA_ringdist const* src)
{
  memmove(dst->ring, src->ring, sizeof(dst->ring));
  dst->i = src->i;
  dst->special_size = src->special_size;
  dst->sum_direct = src->sum_direct;
  dst->direct_one = src->direct_one;
  dst->postfix = src->postfix;
  dst->bit_adjust = src->bit_adjust;
  dst->postmask = src->postmask;
  return;
}

unsigned int tcmplxA_ringdist_get_direct(struct tcmplxA_ringdist const* x) {
  return x->direct_one - 1u;
}

unsigned int tcmplxA_ringdist_get_postfix(struct tcmplxA_ringdist const* x) {
  return x->postfix;
}
/* END   distance ring / public */
