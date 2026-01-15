/*
 * \file text-complex/access/bdict.c
 * \brief Built-in dictionary
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define TCMPLX_A_WIN32_DLL_INTERNAL
#include "bdict_p.h"
#include "text-complex/access/bdict.h"
#include "text-complex/access/api.h"
#include "text-complex/access/util.h"
#include <string.h>


static
unsigned int tcmplxA_bdict_wordcounts[25] = {
  0, 0, 0, 0, 1024, 1024, 2048, 2048, 
  1024, 1024, 1024, 1024, 1024, 512, 512, 256,
  128, 128, 256, 128, 128, 64, 64, 32,   
  32
};


struct tcmplxA_bdict_affix {
  unsigned char p[8];
  unsigned char len;
};
struct tcmplxA_bdict_formula {
  struct tcmplxA_bdict_affix front;
  struct tcmplxA_bdict_affix back;
  unsigned short cb;
};


/** @internal @brief Transform list. */
static
struct tcmplxA_bdict_formula const tcmplxA_bdict_formulas[121u] = {
/*   0 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_Identity},
/*   1 */ { {{0},0},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*   2 */ { {{0x20},1},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*   3 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst1},
/*   4 */ { {{0},0},
            {{0x20},1},
            tcmplxA_BDict_FermentFirst},
/*   5 */ { {{0},0},
            {{0x20,0x74,0x68,0x65,0x20},5},
            tcmplxA_BDict_Identity},
/*   6 */ { {{0x20},1},
            {{0},0},
            tcmplxA_BDict_Identity},
/*   7 */ { {{0x73,0x20},2},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*   8 */ { {{0},0},
            {{0x20,0x6f,0x66,0x20},4},
            tcmplxA_BDict_Identity},
/*   9 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_FermentFirst},
/*  10 */ { {{0},0},
            {{0x20,0x61,0x6e,0x64,0x20},5},
            tcmplxA_BDict_Identity},
/*  11 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst2},
/*  12 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast1},
/*  13 */ { {{0x2c,0x20},2},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*  14 */ { {{0},0},
            {{0x2c,0x20},2},
            tcmplxA_BDict_Identity},
/*  15 */ { {{0x20},1},
            {{0x20},1},
            tcmplxA_BDict_FermentFirst},
/*  16 */ { {{0},0},
            {{0x20,0x69,0x6e,0x20},4},
            tcmplxA_BDict_Identity},
/*  17 */ { {{0},0},
            {{0x20,0x74,0x6f,0x20},4},
            tcmplxA_BDict_Identity},
/*  18 */ { {{0x65,0x20},2},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*  19 */ { {{0},0},
            {{0x22},1},
            tcmplxA_BDict_Identity},
/*  20 */ { {{0},0},
            {{0x2e},1},
            tcmplxA_BDict_Identity},
/*  21 */ { {{0},0},
            {{0x22,0x3e},2},
            tcmplxA_BDict_Identity},
/*  22 */ { {{0},0},
            {{0xa},1},
            tcmplxA_BDict_Identity},
/*  23 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast3},
/*  24 */ { {{0},0},
            {{0x5d},1},
            tcmplxA_BDict_Identity},
/*  25 */ { {{0},0},
            {{0x20,0x66,0x6f,0x72,0x20},5},
            tcmplxA_BDict_Identity},
/*  26 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst3},
/*  27 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast2},
/*  28 */ { {{0},0},
            {{0x20,0x61,0x20},3},
            tcmplxA_BDict_Identity},
/*  29 */ { {{0},0},
            {{0x20,0x74,0x68,0x61,0x74,0x20},6},
            tcmplxA_BDict_Identity},
/*  30 */ { {{0x20},1},
            {{0},0},
            tcmplxA_BDict_FermentFirst},
/*  31 */ { {{0},0},
            {{0x2e,0x20},2},
            tcmplxA_BDict_Identity},
/*  32 */ { {{0x2e},1},
            {{0},0},
            tcmplxA_BDict_Identity},
/*  33 */ { {{0x20},1},
            {{0x2c,0x20},2},
            tcmplxA_BDict_Identity},
/*  34 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst4},
/*  35 */ { {{0},0},
            {{0x20,0x77,0x69,0x74,0x68,0x20},6},
            tcmplxA_BDict_Identity},
/*  36 */ { {{0},0},
            {{0x27},1},
            tcmplxA_BDict_Identity},
/*  37 */ { {{0},0},
            {{0x20,0x66,0x72,0x6f,0x6d,0x20},6},
            tcmplxA_BDict_Identity},
/*  38 */ { {{0},0},
            {{0x20,0x62,0x79,0x20},4},
            tcmplxA_BDict_Identity},
/*  39 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst5},
/*  40 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst6},
/*  41 */ { {{0x20,0x74,0x68,0x65,0x20},5},
            {{0},0},
            tcmplxA_BDict_Identity},
/*  42 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast4},
/*  43 */ { {{0},0},
            {{0x2e,0x20,0x54,0x68,0x65,0x20},6},
            tcmplxA_BDict_Identity},
/*  44 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_FermentAll},
/*  45 */ { {{0},0},
            {{0x20,0x6f,0x6e,0x20},4},
            tcmplxA_BDict_Identity},
/*  46 */ { {{0},0},
            {{0x20,0x61,0x73,0x20},4},
            tcmplxA_BDict_Identity},
/*  47 */ { {{0},0},
            {{0x20,0x69,0x73,0x20},4},
            tcmplxA_BDict_Identity},
/*  48 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast7},
/*  49 */ { {{0},0},
            {{0x69,0x6e,0x67,0x20},4},
            tcmplxA_BDict_OmitLast1},
/*  50 */ { {{0},0},
            {{0xa,0x9},2},
            tcmplxA_BDict_Identity},
/*  51 */ { {{0},0},
            {{0x3a},1},
            tcmplxA_BDict_Identity},
/*  52 */ { {{0x20},1},
            {{0x2e,0x20},2},
            tcmplxA_BDict_Identity},
/*  53 */ { {{0},0},
            {{0x65,0x64,0x20},3},
            tcmplxA_BDict_Identity},
/*  54 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst9},
/*  55 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitFirst7},
/*  56 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast6},
/*  57 */ { {{0},0},
            {{0x28},1},
            tcmplxA_BDict_Identity},
/*  58 */ { {{0},0},
            {{0x2c,0x20},2},
            tcmplxA_BDict_FermentFirst},
/*  59 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast8},
/*  60 */ { {{0},0},
            {{0x20,0x61,0x74,0x20},4},
            tcmplxA_BDict_Identity},
/*  61 */ { {{0},0},
            {{0x6c,0x79,0x20},3},
            tcmplxA_BDict_Identity},
/*  62 */ { {{0x20,0x74,0x68,0x65,0x20},5},
            {{0x20,0x6f,0x66,0x20},4},
            tcmplxA_BDict_Identity},
/*  63 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast5},
/*  64 */ { {{0},0},
            {{0},0},
            tcmplxA_BDict_OmitLast9},
/*  65 */ { {{0x20},1},
            {{0x2c,0x20},2},
            tcmplxA_BDict_FermentFirst},
/*  66 */ { {{0},0},
            {{0x22},1},
            tcmplxA_BDict_FermentFirst},
/*  67 */ { {{0x2e},1},
            {{0x28},1},
            tcmplxA_BDict_Identity},
/*  68 */ { {{0},0},
            {{0x20},1},
            tcmplxA_BDict_FermentAll},
/*  69 */ { {{0},0},
            {{0x22,0x3e},2},
            tcmplxA_BDict_FermentFirst},
/*  70 */ { {{0},0},
            {{0x3d,0x22},2},
            tcmplxA_BDict_Identity},
/*  71 */ { {{0x20},1},
            {{0x2e},1},
            tcmplxA_BDict_Identity},
/*  72 */ { {{0x2e,0x63,0x6f,0x6d,0x2f},5},
            {{0},0},
            tcmplxA_BDict_Identity},
/*  73 */ { {{0x20,0x74,0x68,0x65,0x20},5},
            {{0x20,0x6f,0x66,0x20,0x74,0x68,0x65,0x20},8},
            tcmplxA_BDict_Identity},
/*  74 */ { {{0},0},
            {{0x27},1},
            tcmplxA_BDict_FermentFirst},
/*  75 */ { {{0},0},
            {{0x2e,0x20,0x54,0x68,0x69,0x73,0x20},7},
            tcmplxA_BDict_Identity},
/*  76 */ { {{0},0},
            {{0x2c},1},
            tcmplxA_BDict_Identity},
/*  77 */ { {{0x2e},1},
            {{0x20},1},
            tcmplxA_BDict_Identity},
/*  78 */ { {{0},0},
            {{0x28},1},
            tcmplxA_BDict_FermentFirst},
/*  79 */ { {{0},0},
            {{0x2e},1},
            tcmplxA_BDict_FermentFirst},
/*  80 */ { {{0},0},
            {{0x20,0x6e,0x6f,0x74,0x20},5},
            tcmplxA_BDict_Identity},
/*  81 */ { {{0x20},1},
            {{0x3d,0x22},2},
            tcmplxA_BDict_Identity},
/*  82 */ { {{0},0},
            {{0x65,0x72,0x20},3},
            tcmplxA_BDict_Identity},
/*  83 */ { {{0x20},1},
            {{0x20},1},
            tcmplxA_BDict_FermentAll},
/*  84 */ { {{0},0},
            {{0x61,0x6c,0x20},3},
            tcmplxA_BDict_Identity},
/*  85 */ { {{0x20},1},
            {{0},0},
            tcmplxA_BDict_FermentAll},
/*  86 */ { {{0},0},
            {{0x3d,0x27},2},
            tcmplxA_BDict_Identity},
/*  87 */ { {{0},0},
            {{0x22},1},
            tcmplxA_BDict_FermentAll},
/*  88 */ { {{0},0},
            {{0x2e,0x20},2},
            tcmplxA_BDict_FermentFirst},
/*  89 */ { {{0x20},1},
            {{0x28},1},
            tcmplxA_BDict_Identity},
/*  90 */ { {{0},0},
            {{0x66,0x75,0x6c,0x20},4},
            tcmplxA_BDict_Identity},
/*  91 */ { {{0x20},1},
            {{0x2e,0x20},2},
            tcmplxA_BDict_FermentFirst},
/*  92 */ { {{0},0},
            {{0x69,0x76,0x65,0x20},4},
            tcmplxA_BDict_Identity},
/*  93 */ { {{0},0},
            {{0x6c,0x65,0x73,0x73,0x20},5},
            tcmplxA_BDict_Identity},
/*  94 */ { {{0},0},
            {{0x27},1},
            tcmplxA_BDict_FermentAll},
/*  95 */ { {{0},0},
            {{0x65,0x73,0x74,0x20},4},
            tcmplxA_BDict_Identity},
/*  96 */ { {{0x20},1},
            {{0x2e},1},
            tcmplxA_BDict_FermentFirst},
/*  97 */ { {{0},0},
            {{0x22,0x3e},2},
            tcmplxA_BDict_FermentAll},
/*  98 */ { {{0x20},1},
            {{0x3d,0x27},2},
            tcmplxA_BDict_Identity},
/*  99 */ { {{0},0},
            {{0x2c},1},
            tcmplxA_BDict_FermentFirst},
/* 100 */ { {{0},0},
            {{0x69,0x7a,0x65,0x20},4},
            tcmplxA_BDict_Identity},
/* 101 */ { {{0},0},
            {{0x2e},1},
            tcmplxA_BDict_FermentAll},
/* 102 */ { {{0xc2,0xa0},2},
            {{0},0},
            tcmplxA_BDict_Identity},
/* 103 */ { {{0x20},1},
            {{0x2c},1},
            tcmplxA_BDict_Identity},
/* 104 */ { {{0},0},
            {{0x3d,0x22},2},
            tcmplxA_BDict_FermentFirst},
/* 105 */ { {{0},0},
            {{0x3d,0x22},2},
            tcmplxA_BDict_FermentAll},
/* 106 */ { {{0},0},
            {{0x6f,0x75,0x73,0x20},4},
            tcmplxA_BDict_Identity},
/* 107 */ { {{0},0},
            {{0x2c,0x20},2},
            tcmplxA_BDict_FermentAll},
/* 108 */ { {{0},0},
            {{0x3d,0x27},2},
            tcmplxA_BDict_FermentFirst},
/* 109 */ { {{0x20},1},
            {{0x2c},1},
            tcmplxA_BDict_FermentFirst},
/* 110 */ { {{0x20},1},
            {{0x3d,0x22},2},
            tcmplxA_BDict_FermentAll},
/* 111 */ { {{0x20},1},
            {{0x2c,0x20},2},
            tcmplxA_BDict_FermentAll},
/* 112 */ { {{0},0},
            {{0x2c},1},
            tcmplxA_BDict_FermentAll},
/* 113 */ { {{0},0},
            {{0x28},1},
            tcmplxA_BDict_FermentAll},
/* 114 */ { {{0},0},
            {{0x2e,0x20},2},
            tcmplxA_BDict_FermentAll},
/* 115 */ { {{0x20},1},
            {{0x2e},1},
            tcmplxA_BDict_FermentAll},
/* 116 */ { {{0},0},
            {{0x3d,0x27},2},
            tcmplxA_BDict_FermentAll},
/* 117 */ { {{0x20},1},
            {{0x2e,0x20},2},
            tcmplxA_BDict_FermentAll},
/* 118 */ { {{0x20},1},
            {{0x3d,0x22},2},
            tcmplxA_BDict_FermentFirst},
/* 119 */ { {{0x20},1},
            {{0x3d,0x27},2},
            tcmplxA_BDict_FermentAll},
/* 120 */ { {{0x20},1},
            {{0x3d,0x27},2},
            tcmplxA_BDict_FermentFirst}
};

/**
 * @brief Return `NULL`.
 * @param i (unused)
 * @return `NULL`
 */
static
unsigned char const* tcmplxA_bdict_null(unsigned int i);

/**
 * @brief Word table access callback.
 * @param i word array index
 * @return the word at the given index, NULL otherwise
 */
typedef unsigned char const* (*tcmplxA_bdict_fn)(unsigned int i);


/** @internal @brief Word table directory. */
static
tcmplxA_bdict_fn const tcmplxA_bdict_dir[25] = {
  tcmplxA_bdict_null,
  tcmplxA_bdict_null,
  tcmplxA_bdict_null,
  tcmplxA_bdict_null,
  tcmplxA_bdict_access04,
  tcmplxA_bdict_access05,
  tcmplxA_bdict_access06,
  tcmplxA_bdict_access07,
  tcmplxA_bdict_access08,
  tcmplxA_bdict_access09,

  tcmplxA_bdict_access10,
  tcmplxA_bdict_access11,
  tcmplxA_bdict_access12,
  tcmplxA_bdict_access13,
  tcmplxA_bdict_access14,
  tcmplxA_bdict_access15,
  tcmplxA_bdict_access16,
  tcmplxA_bdict_access17,
  tcmplxA_bdict_access18,
  tcmplxA_bdict_access19,

  tcmplxA_bdict_access20,
  tcmplxA_bdict_access21,
  tcmplxA_bdict_access22,
  tcmplxA_bdict_access23,
  tcmplxA_bdict_access24
};

/* BEGIN built-in dictionary / static */
unsigned char const* tcmplxA_bdict_null(unsigned int i) {
  return NULL;
}
/* END   built-in dictionary / static */

/* BEGIN built-in dictionary / public */
unsigned int tcmplxA_bdict_word_count(unsigned int j) {
  if (j >= 25u)
    return 0u;
  else return tcmplxA_bdict_wordcounts[j];
}

unsigned char const* tcmplxA_bdict_get_word
  (unsigned int j, unsigned int i)
{
  if (j >= 25u)
    return NULL;
  else return (*tcmplxA_bdict_dir[j])(i);
}

int tcmplxA_bdict_transform
  (unsigned char* buf, unsigned int* len, unsigned int k)
{
  if (k >= 121u)
    return tcmplxA_ErrParam;
  else if (*len > 24u)
    return tcmplxA_ErrMemory;
  else {
    unsigned char out[38];
    unsigned int outlen = 0u;
    /* index the arrays */
    struct tcmplxA_bdict_formula const* const formula =
      tcmplxA_bdict_formulas+k;
    tcmplxA_bdict_cb_do
      ( out, &outlen, formula->front.p, formula->front.len,
        tcmplxA_BDict_Identity);
    tcmplxA_bdict_cb_do(out, &outlen, buf, *len, formula->cb);
    tcmplxA_bdict_cb_do
      ( out, &outlen, formula->back.p, formula->back.len,
        tcmplxA_BDict_Identity);
    memset(out+outlen, 0, 38u-outlen);
    memcpy(buf, out, 38);
    *len = outlen;
    return tcmplxA_Success;
  }
}
/* END   built-in dictionary / public */
