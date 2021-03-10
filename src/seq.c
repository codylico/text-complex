/**
 * @file tcmplx-access/seq.c
 * @brief Adapter providing sequential access to bytes from a mmaptwo
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "text-complex/access/seq.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include "mmaptwo/mmaptwo.h"
#include <limits.h>

struct tcmplxA_seq {
  struct mmaptwo_i *fh;
  struct mmaptwo_page_i *hpage;
  unsigned char* hptr;
  size_t pos;
  size_t off;
  size_t last;
};

/**
 * @brief Initialize a sequential.
 * @param x the sequential to initialize
 * @param xfh File access instance to use
 * @return zero on success, nonzero otherwise
 */
static int tcmplxA_seq_init(struct tcmplxA_seq* x, struct mmaptwo_i *xfh);
/**
 * @brief Close a sequential.
 * @param x the sequential to close
 */
static void tcmplxA_seq_close(struct tcmplxA_seq* x);
/**
 * @brief Reset a sequential's input stream to match the current position.
 * @param x the sequential to reset
 */
static int tcmplxA_seq_reset_sync(struct tcmplxA_seq* x, size_t p);

/* BEGIN sequential / static */
int tcmplxA_seq_init(struct tcmplxA_seq* x, struct mmaptwo_i *xfh) {
  x->fh = xfh;
  x->hpage = NULL;
  x->pos = 0u;
  x->off = 0u;
  x->last = 0u;
  tcmplxA_seq_reset_sync(x, 0u);
  return tcmplxA_Success;
}

void tcmplxA_seq_close(struct tcmplxA_seq* x) {
  if (x->hpage != NULL) {
    mmaptwo_page_close(x->hpage);
  }
  x->hpage = NULL;
  return;
}

int tcmplxA_seq_reset_sync(struct tcmplxA_seq* x, size_t p) {
  int res = 0;
  struct mmaptwo_page_i *n_hpage = NULL;
  size_t n_pos = p;
  size_t n_off = 0u;
  size_t n_last = 0u;
  unsigned char const* n_hptr = NULL;
  do {
    if (x->fh != NULL) {
      size_t const len = mmaptwo_length(x->fh);
      size_t pagestart;
      size_t pagelen;
      if (n_pos > len) {
        res = 0;
      } else if (n_pos == len) {
        /* repair pos */
        n_off = len;
        n_last = len;
        res = 1;
      } else /* compute page bounds */{
        size_t const presize = mmaptwo_get_page_size();
        size_t const pagesize = (presize>256u ? presize : 256u);
        size_t const pagediff = n_pos%pagesize;
        pagestart = n_pos - pagediff;
        pagelen =
          ((len-pagestart<pagesize*2u) ? len-pagestart : pagesize*2u);
        n_hpage = mmaptwo_acquire(x->fh, pagelen, pagestart);
        if (n_hpage != NULL) {
          n_off = pagestart;
          n_last = pagelen + pagestart;
          res = 1;
        }
      }
    } else {
      if (p == 0u) {
        n_pos = 0u;
        res = 1;
      } else res = 0;
    }
  } while (0);
  if (res) {
    if (x->hpage != NULL) {
      mmaptwo_page_close(x->hpage);
    }
    x->hpage = n_hpage;
    x->off = n_off;
    x->last = n_last;
    x->pos = n_pos;
    x->hptr = (unsigned char*)(
        (n_hpage!=NULL) ? mmaptwo_page_get(n_hpage) : NULL
      );
  }
  return res;
}
/* END   sequential / static */

/* BEGIN sequential / public */
struct tcmplxA_seq* tcmplxA_seq_new(struct mmaptwo_i* x) {
  struct tcmplxA_seq* out;
  out = tcmplxA_util_malloc(sizeof(struct tcmplxA_seq));
  if (out != NULL
  &&  tcmplxA_seq_init(out,x) != tcmplxA_Success)
  {
    tcmplxA_util_free(out);
    return NULL;
  }
  return out;
}

void tcmplxA_seq_destroy(struct tcmplxA_seq* x) {
  if (x != NULL) {
    tcmplxA_seq_close(x);
    tcmplxA_util_free(x);
  }
  return;
}

size_t tcmplxA_seq_get_pos(struct tcmplxA_seq const* x) {
  return x->pos;
}

size_t tcmplxA_seq_set_pos(struct tcmplxA_seq* x, size_t i) {
  if (i >= x->off && i < x->last) {
    x->pos = i;
    return x->pos;
  } else {
    int const res = tcmplxA_seq_reset_sync(x, i);
    if (res)
      return x->pos;
    else return ~(size_t)0u;
  }
}

int tcmplxA_seq_get_byte(struct tcmplxA_seq* x) {
  if (x->pos >= x->last) {
    int const res = tcmplxA_seq_reset_sync(x, x->pos);
    if (!res) {
      /* acquisition error */
      return -2;
    }/* else continue; */
  }
  if (x->pos < x->last) {
    int const out = (unsigned char)(x->hptr[x->pos-x->off]) & 255;
    x->pos += 1;
    return out;
  } else /* end of stream, so */return -1;
}

long int tcmplxA_seq_seek(struct tcmplxA_seq* x, long int i, int whence) {
  long int l_out;
  switch (whence) {
  case tcmplxA_SeqSet:
    {
      if (i < 0)
        l_out = -2L;
      else {
        size_t const out = tcmplxA_seq_set_pos(x,(size_t)i);
        if (out == ~(size_t)0u) {
          l_out = -2L;
        } else if (out > (size_t)LONG_MAX) {
          l_out = -1L;
        } else l_out = (long int)out;
      }
    }break;
  case tcmplxA_SeqCur:
  case tcmplxA_SeqEnd:
    {
      static unsigned long int const nonzero =
        (unsigned long int)(~(size_t)0u);
      size_t here = (whence == tcmplxA_SeqCur) ? x->pos : (
          (x->fh != NULL) ? mmaptwo_length(x->fh) : 0u
        );
      if (i == 0) {
        /* do nothing */
      } else if (i < 0) {
        long int i_prime[2];
        /* break apart any LONG_MIN values */{
          static long int const branch = (LONG_MIN/2);
          if (i <= branch) {
            i_prime[0] = i-branch;
            i_prime[1] = branch;
          } else {
            i_prime[0] = 0;
            i_prime[1] = i;
          }
        }
        /* */{
          int j;
          for (j = 0; j < 2; ++j) {
            if (((unsigned long int)-i_prime[j]) > nonzero) {
              /* negative overflow */
              break;
            } else {
              size_t const n_prime = (size_t)-i_prime[j];
              if (here >= n_prime)
                here -= n_prime;
              else break;
            }
          }
          if (j < 2) {
            l_out = -2L;
            break;
          }
        }
      } else if (((unsigned long int)i) > nonzero) {
        /* positive overflow */
        l_out = -2L;
        break;
      } else {
        size_t n_prime = (size_t)i;
        if (n_prime > (~(size_t)0u) - here) {
          l_out = -2L;
          break;
        } else here += n_prime;
      }
      /* perform seek */{
        size_t const out = tcmplxA_seq_set_pos(x,(size_t)here);
        if (out == ~(size_t)0u) {
          l_out = -2L;
        } else if (out > (size_t)LONG_MAX) {
          l_out = -1L;
        } else l_out = (long int)out;
      }
    }break;
  default:
    l_out = -2L;
    break;
  }
  return l_out;
}
/* END   sequential / public */
