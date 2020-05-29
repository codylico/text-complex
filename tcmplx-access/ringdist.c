/**
 * @file tcmplx-access/ringdist.c
 * @brief Distance ring buffer
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ringdist.h"
#include "api.h"
#include "util.h"
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
  (struct tcmplxA_ringdist* x, unsigned int dcode, unsigned int extra)
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
      out = x->ring[x->i];
      break;
    case 1:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      out = x->ring[(x->i+1u)%4u];
      break;
    case 2:
      out = x->ring[(x->i+2u)%4u];
      break;
    case 3:
      out = x->ring[(x->i+3u)%4u];
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
      x->ring[x->i] = out;
      x->i = (x->i+1u)%4u;
    }
    return out;
  } else if (dcode < x->sum_direct) {
    return (dcode - x->special_size) + 1u;
  } else if (x->special_size)/* RFC 7932 branch */{
    unsigned int const xcode = dcode - x->sum_direct;
    unsigned int const bit_size = tcmplxA_ringdist_bit_count(x, dcode);
    tcmplxA_uint32 const high = (tcmplxA_uint32)(xcode>>(x->postfix));
    tcmplxA_uint32 const low = (tcmplxA_uint32)(xcode&x->postmask);
    tcmplxA_uint32 const offset = ((2u | (high&1)) << bit_size) - 4u;
    /* possible mistake in rfc7932? */
      tcmplxA_uint32 const out =
        ((offset + extra)<<x->postfix) + low + x->direct_one;
    /* record the new flat distance */{
      x->ring[x->i] = out;
      x->i = (x->i+1u)%4u;
    }
    return out;
  } else/* RFC 1951 branch */{
    unsigned int const xcode = dcode - x->sum_direct;
    unsigned int const bit_size = tcmplxA_ringdist_bit_count(x, dcode);
    tcmplxA_uint32 const high = (tcmplxA_uint32)(xcode>>(x->postfix));
    tcmplxA_uint32 const low = (tcmplxA_uint32)(xcode&x->postmask);
    tcmplxA_uint32 const offset = ((2u | (high&1)) << bit_size) - 4u;
    /* to better match rfc1951 */
      tcmplxA_uint32 const out =
        ((offset + low)<<x->postfix) + extra + x->direct_one;
    return out;
  }
}
/* END   distance ring / public */
