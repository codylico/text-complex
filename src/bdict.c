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


static
unsigned int tcmplxA_bdict_wordcounts[25] = {
  0, 0, 0, 0, 1024, 1024, 2048, 2048, 
  1024, 1024, 1024, 1024, 1024, 512, 512, 256,
  128, 128, 256, 128, 128, 64, 64, 32,   
  32
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
/* END   built-in dictionary / public */
