
#include "text-complex/access/bdict.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_bdict_crc
    (const MunitParameter params[], void* data);

/**
 * @brief Resumable CRC32 calculator.
 * @param v data to check
 * @param len length of data
 * @param oldcrc last CRC value; initialize with `0`
 * @return the new CRC value
 */
static
unsigned long int test_bdict_CRC32_resume
    ( unsigned char const* v, unsigned long int len,
      unsigned long int oldcrc);

static MunitTest tests_bdict[] = {
  {"crc", test_bdict_crc,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_bdict = {
  "access/bdict/", tests_bdict, NULL, 1, 0
};


/* NOTE: adapted from RFC 7932, Appendix C */
unsigned long int test_bdict_CRC32_resume
    ( unsigned char const* v, unsigned long int len,
      unsigned long int oldcrc)
{
  unsigned long int const poly = 0xedb88320UL;
  unsigned long int i;
  unsigned long int crc = oldcrc ^ 0xffffffffUL;
  for (i = 0; i < len; ++i) {
    unsigned int k;
    unsigned long int c;
    c = (crc ^ v[i]) & 0xff;
    for (k = 0; k < 8; k++) {
      c = c & 1 ? poly ^ (c >> 1) : c >> 1;
    }
    crc = c ^ (crc >> 8);
  }
  return crc ^ 0xffffffffUL;
}

MunitResult test_bdict_crc
  (const MunitParameter params[], void* data)
{
  unsigned int const max_wordlen = 25;
  unsigned int j;
  unsigned long int crc = 0;
  for (j = 0; j < max_wordlen; ++j) {
    unsigned int const wordcount =
      tcmplxA_bdict_word_count(j);
    unsigned int i;
    for (i = 0; i < wordcount; ++i) {
      unsigned char const* ptr = tcmplxA_bdict_get_word(j,i);
      munit_assert_ptr_not_null(ptr);
      crc = test_bdict_CRC32_resume(ptr, j, crc);
    }
  }
  munit_assert_ulong(crc, ==, 0x5136cb04);
  return MUNIT_OK;
}



int main(int argc, char **argv) {
  return munit_suite_main(&suite_bdict, NULL, argc, argv);
}
