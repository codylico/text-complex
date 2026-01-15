
#include "text-complex/access/bdict.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_bdict_crc
    (const MunitParameter params[], void* data);
static MunitResult test_bdict_transform_isolate
    (const MunitParameter params[], void* data);
static MunitResult test_bdict_transform_ferment
    (const MunitParameter params[], void* data);
static MunitResult test_bdict_transform_affix
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
/**
 * @brief Compare a fermented character with an unfermented character.
 * @param a the fermented character
 * @param b the unfermented character
 * @return nonzero if "equal", zero otherwise
 */
static
int test_bdict_fermentcmp(int a, int b);
/**
 * @brief The Ferment transform step function.
 * @param word the word to transform
 * @param word_len length of word
 * @param pos active position
 * @return an advance distance
 */
static
int test_bdict_Ferment(unsigned char* word, int word_len, int pos);
/**
 * @brief The FermentAll transform.
 * @param word the word to transform
 * @param word_len length of word
 */
static
void test_bdict_FermentAll(unsigned char* word, int word_len);
static
unsigned int test_bdict_format
  ( unsigned char* out, char const* format,
    unsigned char* word, unsigned int word_len);

static MunitTest tests_bdict[] = {
  {"crc", test_bdict_crc,
    NULL,NULL,MUNIT_TEST_OPTION_SINGLE_ITERATION,NULL},
  {"transform/isolate", test_bdict_transform_isolate,
    NULL,NULL,0,NULL},
  {"transform/ferment", test_bdict_transform_ferment,
    NULL,NULL,0,NULL},
  {"transform/affix", test_bdict_transform_affix,
    NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_bdict = {
  "access/bdict/", tests_bdict, NULL, 1, 0
};





int test_bdict_fermentcmp(int a, int b) {
  if (a >= 0x61 && a <= 0x7a)
    return (a-0x20) == b;
  else return a==b;
}

/* NOTE: adapted from RFC 7932, section 8 */
int test_bdict_Ferment(unsigned char* word, int word_len, int pos) {
   if (word[pos] < 192) {
      if (word[pos] >= 97 && word[pos] <= 122) {
         word[pos] = word[pos] ^ 32;
      }
      return 1;
   } else if (word[pos] < 224) {
      if (pos + 1 < word_len) {
         word[pos + 1] = word[pos + 1] ^ 32;
      }
      return 2;
   } else {
      if (pos + 2 < word_len) {
         word[pos + 2] = word[pos + 2] ^ 5;
      }
      return 3;
   }
}

/* NOTE: adapted from RFC 7932, section 8 */
void test_bdict_FermentAll(unsigned char* word, int word_len) {
   int i = 0;
   while (i < word_len) {
      i += test_bdict_Ferment(word, word_len, i);
   }
}

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

unsigned int test_bdict_format
  ( unsigned char* out, char const* format,
    unsigned char* word, unsigned int word_len)
{
  unsigned int len = 38;
  unsigned int i;
  char const* p;
  for (p = format, i = 0; i < len && *p != '\0'; ++i, ++p) {
    switch (*p) {
      case ' ': out[i] = 0x20; break;
      case '%':
        if (*(p+1) != '\0') {
          char const q = *(++p);
          switch (q) {
            case '+':
            case '-':
              /* insert the word */{
                int dist;
                char* endptr;
                dist = (int)strtol(p, &endptr, 10);
                p = (*endptr == '\0' ? endptr-1 : endptr);
                if (*p == ',') {
                  /* TODO ferment first */;
                } else if (*p == '/') {
                  /* TODO ferment all */;
                } else {
                  /* leave alone */;
                }
                if (dist < 0) { /* cut off the end */
                  int j;
                  int dist_sum = dist+(int)word_len;
                  for (j = 0; j < dist_sum && i < len; ++j, ++i)
                    out[i] = word[j];
                } else { /* cut off the front */
                  unsigned int j;
                  for (j = dist; j < word_len && i < len; ++j, ++i)
                    out[i] = word[j];
                }
                --i;
              }break;
            case 'n': out[i] = 0x0a; break;
            case 't': out[i] = 0x09; break;
            case 'C': out[i] = 0xc2; break;
            case 'A': out[i] = 0xa0; break;
            default: out[i] = q; break;
          }
        }break;
      case '"': out[i] = 0x22; break;
      case '\'': out[i] = 0x27; break;
      case '(': out[i] = 0x28; break;
      case ',': out[i] = 0x2c; break;
      case '.': out[i] = 0x2e; break;
      case '/': out[i] = 0x2f; break;
      case ':': out[i] = 0x3a; break;
      case '=': out[i] = 0x3d; break;
      case '>': out[i] = 0x3e; break;
      case 'T': out[i] = 0x54; break;
      case ']': out[i] = 0x5d; break;
      case 'a': out[i] = 0x61; break;
      case 'b': out[i] = 0x62; break;
      case 'c': out[i] = 0x63; break;
      case 'd': out[i] = 0x64; break;
      case 'e': out[i] = 0x65; break;
      case 'f': out[i] = 0x66; break;
      case 'g': out[i] = 0x67; break;
      case 'h': out[i] = 0x68; break;
      case 'i': out[i] = 0x69; break;
      case 'l': out[i] = 0x6c; break;
      case 'm': out[i] = 0x6d; break;
      case 'n': out[i] = 0x6e; break;
      case 'o': out[i] = 0x6f; break;
      case 'r': out[i] = 0x72; break;
      case 's': out[i] = 0x73; break;
      case 't': out[i] = 0x74; break;
      case 'u': out[i] = 0x75; break;
      case 'v': out[i] = 0x76; break;
      case 'w': out[i] = 0x77; break;
      case 'y': out[i] = 0x79; break;
      case 'z': out[i] = 0x7a; break;
      default: out[i] = *p; break;
    }
  }
  return i;
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

MunitResult test_bdict_transform_isolate
  (const MunitParameter params[], void* data)
{
  static
  unsigned int const isolate_indices[21] =
    { 0, 9, 44,
      3, 11, 26, 34, 39, 40, 55, -1, 54,
      12, 27, 23, 42, 63, 56, 48, 59, 64 };
  unsigned int const transform_subindex = munit_rand_int_range(0,20);
  unsigned char a_word[38], b_word[38];
  unsigned int a_word_len, b_word_len;
  /* compose a word */{
    unsigned char buf[24];
    unsigned int const wordlen = munit_rand_int_range(4,24);
    unsigned int i;
    munit_rand_memory(sizeof(buf), (munit_uint8_t*)buf);
    for (i = 0u; i < wordlen; ++i) {
      unsigned char const x = buf[i];
      if ((x&96u) == 0u)
        buf[i] = (x&127u)|64u;
      else buf[i] = x&127u;
    }
    memcpy(a_word, buf, sizeof(unsigned char)*wordlen);
    a_word_len = wordlen;
  }
  memcpy(b_word, a_word, a_word_len);
  b_word_len = a_word_len;
  /* transform it */{
    int const ae = tcmplxA_bdict_transform
      (b_word, &b_word_len, isolate_indices[transform_subindex]);
    if (isolate_indices[transform_subindex] >= 121) {
      munit_assert_int(ae, ==, tcmplxA_ErrParam);
    } else switch (transform_subindex) {
    case 0: /* identity */
      munit_assert_uint(b_word_len,==,a_word_len);
      munit_assert_memory_equal(a_word_len,a_word,b_word);
      break;
    case 1: /* ferment first */
      munit_assert_uint(b_word_len,==,a_word_len);
      munit_assert_memory_equal(a_word_len-1,a_word+1,b_word+1);
      munit_assert_true(test_bdict_fermentcmp(a_word[0], b_word[0]));
      break;
    case 2: /* ferment all */
      {
        unsigned int i;
        munit_assert_uint(b_word_len,==,a_word_len);
        for (i = 0u; i < a_word_len; ++i) {
          munit_assert_true(test_bdict_fermentcmp(a_word[i], b_word[i]));
        }
      }break;
    default:
      if (transform_subindex <= 11) {
        /* omit first (N) */
        unsigned int n = transform_subindex-2;
        unsigned int const expect_len = a_word_len>n?a_word_len-n:0;
        munit_assert_uint(b_word_len,==,expect_len);
        munit_assert_memory_equal(expect_len,a_word+n,b_word);
      } else {
        /* omit last (N) */
        unsigned int n = transform_subindex-11;
        unsigned int const expect_len = a_word_len>n?a_word_len-n:0;
        munit_assert_uint(b_word_len,==,expect_len);
        munit_assert_memory_equal(expect_len,a_word,b_word);
      }break;
    }
  }
  return MUNIT_OK;
}

MunitResult test_bdict_transform_ferment
  (const MunitParameter params[], void* data)
{
  unsigned char a_word[38], b_word[38];
  unsigned int a_word_len, b_word_len;
  /* compose a word */{
    unsigned char buf[24];
    unsigned int const wordlen = munit_rand_int_range(4,24);
    unsigned int i;
    munit_rand_memory(sizeof(buf), (munit_uint8_t*)buf);
    memcpy(a_word, buf, sizeof(unsigned char)*wordlen);
    a_word_len = wordlen;
  }
  memcpy(b_word, a_word, a_word_len);
  b_word_len = a_word_len;
  /* transform it */{
    int const ae = tcmplxA_bdict_transform(b_word, &b_word_len, 44);
    munit_assert_int(ae,==,tcmplxA_Success);
    test_bdict_FermentAll(a_word, a_word_len);
    munit_assert_uint(b_word_len,==,a_word_len);
    munit_assert_memory_equal(a_word_len,a_word,b_word);
  }
  return MUNIT_OK;
}

MunitResult test_bdict_transform_affix
  (const MunitParameter params[], void* data)
{
  static char const* fmts[] = {
    /*   0 */ "%+0;", "%+0; ", " %+0; ", "%+1;",
    /*   4 */ "%+0, ", "%+0; the ", " %+0;", "s %+0; ",
    /*   8 */ "%+0; of ", "%+0,", "%+0; and ", "%+2;",
    /*  12 */ "%-1;", ", %+0; ", "%+0;, ", " %+0, ",
    /*  16 */ "%+0; in ", "%+0; to ", "e %+0; ", "%+0;\"",
    /*  20 */ "%+0;.", "%+0;\">", "%+0;%n", "%-3;",
    /*  24 */ "%+0;]", "%+0; for ", "%+3;", "%-2;",
    /*  28 */ "%+0; a ", "%+0; that ", " %+0,", "%+0;. ",

    /*  32 */ ".%+0;", " %+0;, ", "%+4;", "%+0; with ",
    /*  36 */ "%+0;'", "%+0; from ", "%+0; by ", "%+5;",
    /*  40 */ "%+6;", " the %+0;", "%-4;", "%+0;. The ",
    /*  44 */ "%+0/", "%+0; on ", "%+0; as ", "%+0; is ",
    /*  48 */ "%-7;", "%-1;ing ", "%+0;%n%t", "%+0;:",
    /*  52 */ " %+0;. ", "%+0;ed ", "%+9;", "%+7;",
    /*  56 */ "%-6;", "%+0;(", "%+0,, ", "%-8;",
    /*  60 */ "%+0; at ", "%+0;ly ", " the %+0; of ", "%-5;",

    /*  64 */ "%-9;", " %+0,, ", "%+0,\"", ".%+0;(",
    /*  68 */ "%+0/ ", "%+0,\">", "%+0;=\"", " %+0;.",
    /*  72 */ ".com/%+0;", " the %+0; of the ", "%+0,'", "%+0;. This ",
    /*  76 */ "%+0;,", ".%+0; ", "%+0,(", "%+0;.",
    /*  80 */ "%+0; not ", " %+0;=\"", "%+0;er ", " %+0/ ",
    /*  84 */ "%+0;al ", " %+0/", "%+0;='", "%+0/\"",
    /*  88 */ "%+0,. ", " %+0;(", "%+0;ful ", " %+0,. ",
    /*  92 */ "%+0;ive ", "%+0;less ", "%+0/'", "%+0;est ",

    /*  96 */ " %+0,.", "%+0/\">", " %+0;='", "%+0;,",
    /* 100 */ "%+0;ize ", "%+0/.", "%C%A%+0;", " %+0;,",
    /* 104 */ "%+0,=\"", "%+0/=\"", "%+0;ous ", "%+0/, ",
    /* 108 */ "%+0,='", " %+0,,", " %+0/=\"", " %+0/, ",
    /* 112 */ "%+0/,", "%+0/(", "%+0/. ", " %+0/.",
    /* 116 */ "%+0/='", " %+0/. ", " %+0,=\"", " %+0/='",
    /* 120 */ " %+0,='"
  };
  unsigned int const transform_index =
    munit_rand_int_range(0,sizeof(fmts)/sizeof(char const*)-1);
  unsigned char a_word[38], b_word[38], c_word[38];
  unsigned int a_word_len, b_word_len, c_word_len;
  /* compose a word */{
    unsigned char buf[24];
    unsigned int const wordlen = munit_rand_int_range(4,24);
    unsigned int i;
    munit_rand_memory(sizeof(buf), (munit_uint8_t*)buf);
    for (i = 0u; i < wordlen; ++i) {
      buf[i] = buf[i]&63u;
    }
    memcpy(a_word, buf, sizeof(unsigned char)*wordlen);
    memset(a_word+wordlen, 0, (38-wordlen)*sizeof(unsigned char));
    a_word_len = wordlen;
  }
  memcpy(b_word, a_word, a_word_len);
  b_word_len = a_word_len;
  /* transform it */{
    int const ae = tcmplxA_bdict_transform
      (b_word, &b_word_len, transform_index);
    munit_assert_int(ae,==,tcmplxA_Success);
    c_word_len = test_bdict_format
      (c_word, fmts[transform_index], a_word, a_word_len);
    munit_assert_uint(b_word_len,==,c_word_len);
    munit_assert_memory_equal(c_word_len,c_word,b_word);
  }
  return MUNIT_OK;
};


int main(int argc, char **argv) {
  return munit_suite_main(&suite_bdict, NULL, argc, argv);
}
