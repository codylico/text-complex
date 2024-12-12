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
/**
 * @brief Absolute difference of two unsigned integers.
 * @param a one integer
 * @param b other integer
 * @return a difference
 */
static
unsigned tcmplxA_ctxtspan_absdiff(unsigned a, unsigned b);
/**
 * @brief Select a mode from a score collection.
 * @param score collection to use
 * @return a context mode
 */
static
enum tcmplxA_ctxtmap_mode tcmplxA_ctxtspan_select(struct tcmplxA_ctxtscore const* score);

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

enum tcmplxA_ctxtmap_mode tcmplxA_ctxtspan_select(struct tcmplxA_ctxtscore const* score) {
    unsigned current_score = 0;
    int mode = 0;
    int i;
    for (i = 0; i < tcmplxA_CtxtMap_ModeMax; ++i) {
        if (score->vec[i] > current_score) {
            mode = i;
            current_score = score->vec[i];
        }
    }
    return (enum tcmplxA_ctxtmap_mode)mode;
}

unsigned tcmplxA_ctxtspan_absdiff(unsigned a, unsigned b) {
    return a > b ? a - b : b - a;
}
/* END   context span / static */

/* BEGIN context score / public */
void tcmplxA_ctxtspan_guess(struct tcmplxA_ctxtscore* results,
    void const* buf, size_t buf_len)
{
    struct tcmplxA_ctxtscore out = *results;
    unsigned char const* const p = (unsigned char const*)buf;
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

void tcmplxA_ctxtspan_subdivide(struct tcmplxA_ctxtspan* results,
    void const* buf, size_t buf_len, unsigned margin)
{
    unsigned char const* const char_buf = (unsigned char const*)buf;
    struct tcmplxA_ctxtscore scores[tcmplxA_CtxtSpan_Size] = {{{0}}};
    unsigned char groups[tcmplxA_CtxtSpan_Size] = {0};
    size_t stops[tcmplxA_CtxtSpan_Size] = {0};
    size_t const span_len = (buf_len/tcmplxA_CtxtSpan_Size);
    unsigned int i, bit;
    unsigned char last_group = UCHAR_MAX;
    memset(results, 0, sizeof(struct tcmplxA_ctxtspan));
    for (i = 0; i < tcmplxA_CtxtSpan_Size; ++i) {
        groups[i] = (unsigned char)i;
        results->offsets[i] = span_len*i;
        stops[i] = (i+1>=tcmplxA_CtxtSpan_Size) ? buf_len : span_len*(i+1);
    }
    /* Initial guesses. */
    for (i = 0; i < tcmplxA_CtxtSpan_Size; ++i) {
        size_t const start = results->offsets[i];
        size_t const stop = stops[i];
        tcmplxA_ctxtspan_guess(scores+i, char_buf+start, stop-start);
        results->modes[i] = tcmplxA_ctxtspan_select(scores+i);
    }
    /* Collapse. */
    for (bit = 0; bit < 4; ++bit) {
        unsigned const substep = 1u<<bit;
        unsigned const step = substep<<1;
        for (i = 0; i < tcmplxA_CtxtSpan_Size; i += step) {
            unsigned const inner = substep>>1;
            unsigned const next = i+substep;
            size_t const start = results->offsets[i];
            size_t const stop = stops[i];
            unsigned int j;
            if (groups[i] != groups[i+inner]
            ||  groups[next] != groups[next+inner])
            {
                /* Previous grouping step failed. */
                continue;
            }
            /* See if the contexts are similar enough. */
            if (results->modes[i] != results->modes[next]) {
                int const i_mode = results->modes[i];
                int const next_mode = results->modes[next];
                /*
                * Cross difference based on `X` term of seam objective from:
                * Agarwala et al. "Interactive Digital Photomontage."
                *   ACM Transactions on Graphics (Proceedings of SIGGRAPH 2004), 2004.
                * https://grail.cs.washington.edu/projects/photomontage/
                */
                unsigned const cross_diff =
                    tcmplxA_ctxtspan_absdiff(scores[i].vec[i_mode], scores[i].vec[next_mode])
                    + tcmplxA_ctxtspan_absdiff(scores[next].vec[i_mode], scores[next].vec[next_mode]);
                if (cross_diff > margin)
                    continue;
            }
            /* Group the spans. */
            for (j = i; j < i+step; ++j)
                groups[j] = (unsigned char)i;
            tcmplxA_ctxtspan_guess(scores+i, char_buf+start, stop-start);
            {
                unsigned char const mode = (unsigned char)tcmplxA_ctxtspan_select(scores+i);
                results->modes[i] = mode;
                results->modes[next] = mode;
            }
            stops[i] = stops[next];
            results->offsets[next] = stops[next];
        }
    }
    /* Shift */
    results->count = 0;
    results->total_bytes = buf_len;
    for (i = 0; i < tcmplxA_CtxtSpan_Size; ++i) {
        size_t const current = results->count;
        if (groups[i] == last_group || results->offsets[i] == stops[i])
            continue;
        last_group = groups[i];
        results->offsets[current] = results->offsets[i];
        results->modes[current] = results->modes[i];
        results->count += 1;
    }
    for (i = results->count; i < tcmplxA_CtxtSpan_Size; ++i) {
        results->offsets[i] = buf_len;
        results->modes[i] = tcmplxA_CtxtMap_ModeMax;
    }
    return;
}
/* BEGIN context score / public */
