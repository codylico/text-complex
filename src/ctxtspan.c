/**
 * @file text-complex/access/ctxtspan.c
 * @brief Context span heuristic for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "ctxtmap_p.h"
#include "text-complex/access/ctxtspan.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>
#include <limits.h>


enum tcmplxA_ctxtspan_consts {
    tcmplxA_CtxtSpan_History = 4,
    tcmplxA_CtxtSpan_Last = tcmplxA_CtxtSpan_History-1,
    tcmplxA_CtxtSpan_UtfPoint = 6,
    tcmplxA_CtxtSpan_Ceiling = 6
};


/**
 * @brief Count the number of nonzero bits in an integer.
 * @param x integer to inspect
 * @return a bit count
 */
static
int tcmplxA_ctxtspan_popcount(unsigned x);
/**
 * @brief Add and subtract numbers from a score ceiling.
 * @param sub subtractor
 * @param add addend
 * @return a sum
 */
static
unsigned tcmplxA_ctxtspan_subscore(unsigned sub, unsigned add);

/* BEGIN context span / static */
int tcmplxA_ctxtspan_popcount(unsigned x) {
#if (defined __GNUC__)
    return __builtin_popcount(x);
#else
    int out = 0;
    int i;
    for (i = 0; i < (CHAR_BIT *sizeof(unsigned)); ++i, x >>= 1) {
        out += (x&1);
    }
    return out;
#endif //__GNUC__
}

unsigned tcmplxA_ctxtspan_subscore(unsigned sub, unsigned add) {
    return add > sub ? tcmplxA_CtxtSpan_Ceiling : tcmplxA_CtxtSpan_Ceiling+add-sub;
}
/* END   context span / static */

/* BEGIN context score / public */
void tcmplxA_ctxtspan_guess(struct tcmplxA_ctxtscore* results,
    void const* buf, size_t buf_len)
{
    struct tcmplxA_ctxtscore out = *results;
    unsigned char const* p = (unsigned char const*)buf;
    unsigned char last = 0;
    unsigned char utf8count = 0;
    size_t i;
    for (i = 0; i < buf_len; last = p[i], ++i) {
        unsigned char const current = p[i];
        /* LSB6 & MSB6 */{
            unsigned char const lsb_tmp = last ^ current;
            out.vec[tcmplxA_CtxtMap_LSB6] += tcmplxA_ctxtspan_subscore(
                tcmplxA_ctxtspan_popcount(lsb_tmp&0xC0)*3,
                tcmplxA_ctxtspan_popcount(lsb_tmp&0x3F));
            out.vec[tcmplxA_CtxtMap_MSB6] += tcmplxA_ctxtspan_subscore(
                tcmplxA_ctxtspan_popcount(lsb_tmp&0x03)*3,
                tcmplxA_ctxtspan_popcount(lsb_tmp&0xFC));
        }
        /* Signed */{
            unsigned char const last_lut2 = tcmplxA_ctxtmap_getlut2(last);
            unsigned char const lut2 = tcmplxA_ctxtmap_getlut2(current);
            out.vec[tcmplxA_CtxtMap_Signed] += (7 -
                (lut2>last_lut2 ? lut2-last_lut2 : last_lut2-lut2));
        }
        /* UTF8 */{
            if (utf8count > 0) {
                unsigned const ok = ((current&0xC0)==0x80);
                utf8count = ok ? utf8count-1 : 0;
                out.vec[tcmplxA_CtxtMap_UTF8] += ok*tcmplxA_CtxtSpan_UtfPoint;
            } else if (current >= 0xF0) {
                utf8count = 3;
                out.vec[tcmplxA_CtxtMap_UTF8] += tcmplxA_CtxtSpan_UtfPoint;
            } else if (current >= 0xE0) {
                utf8count = 2;
                out.vec[tcmplxA_CtxtMap_UTF8] += tcmplxA_CtxtSpan_UtfPoint;
            } else if (current >= 0xC0) {
                utf8count = 1;
                out.vec[tcmplxA_CtxtMap_UTF8] += tcmplxA_CtxtSpan_UtfPoint;
            } else {
                out.vec[tcmplxA_CtxtMap_UTF8] += (current<0x80)*tcmplxA_CtxtSpan_UtfPoint;
            }
        }
    }
    *results = out;
    return;
}
/* BEGIN context score / public */