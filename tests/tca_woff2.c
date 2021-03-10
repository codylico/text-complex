/**
 * @brief Test program for woff2
 */
#include "testfont.h"
#include "text-complex/access/woff2.h"
#include "text-complex/access/offtable.h"
#include "mmaptwo/mmaptwo.h"
#include "munit/munit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static MunitResult test_woff2_cycle
    (const MunitParameter params[], void* data);
static MunitResult test_woff2_item
  (const MunitParameter params[], void* data);
static MunitResult test_woff2_tag_toi
  (const MunitParameter params[], void* data);
static void* test_woff2_setup
    (const MunitParameter params[], void* user_data);
static void* test_woff2_null_setup
    (const MunitParameter params[], void* user_data);
static void test_woff2_teardown(void* fixture);

static MunitParameterEnum test_woff2_params[] = {
  { NULL, NULL },
};

static MunitTest tests_woff2[] = {
  {"cycle", test_woff2_cycle,
    NULL,NULL,
    MUNIT_TEST_OPTION_SINGLE_ITERATION|MUNIT_TEST_OPTION_TODO,
    test_woff2_params},
  {"item", test_woff2_item,
    test_woff2_setup,test_woff2_teardown,0,NULL},
  {"tag_toi", test_woff2_tag_toi, NULL,NULL,0,NULL},
  {NULL, NULL, NULL,NULL,0,NULL}
};

static MunitSuite const suite_woff2 = {
  "access/woff2/", tests_woff2, NULL, 1, 0
};




MunitResult test_woff2_cycle
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_woff2* ptr[1];
  (void)params;
  (void)data;
  ptr[0] = tcmplxA_woff2_new(NULL,0);
  munit_assert_not_null(ptr[0]);
  tcmplxA_woff2_destroy(ptr[0]);
  return MUNIT_OK;
}

void* test_woff2_null_setup(const MunitParameter params[], void* user_data) {
  return tcmplxA_woff2_new(NULL,0);
}

void* test_woff2_setup(const MunitParameter params[], void* user_data) {
  struct tcmplxAtest_arg *const tfa = (struct tcmplxAtest_arg *)user_data;
  struct mmaptwo_i* m2i;
  struct tcmplxA_woff2* w2;
  if (tfa->font_path != NULL)
    m2i = mmaptwo_open(tfa->font_path, "re", 0,0);
  else
    m2i = tcmplxAtest_gen_maptwo
        (tcmplxAtest_Zero, 256, (unsigned int)munit_rand_uint32());
  w2 = tcmplxA_woff2_new(m2i,0);
  if (w2 == NULL) {
    mmaptwo_close(m2i);
  }
  return w2;
}

void test_woff2_teardown(void* fixture) {
  tcmplxA_woff2_destroy((struct tcmplxA_woff2*)fixture);
  return;
}

MunitResult test_woff2_item
  (const MunitParameter params[], void* data)
{
  struct tcmplxA_woff2* const p = (struct tcmplxA_woff2*)data;
  if (p == NULL)
    return MUNIT_SKIP;
  (void)params;
  /* inspect offset table items */{
    struct tcmplxA_offtable const* const offt = tcmplxA_woff2_get_offsets(p);
    size_t l, i;
    munit_assert_ptr_not_null(offt);
    l = tcmplxA_offtable_size(offt);
    for (i = 0; i < l; ++i) {
      struct tcmplxA_offline const* line = tcmplxA_offtable_at_c(offt, i);
      munit_logf(MUNIT_LOG_INFO,
        "[%" MUNIT_SIZE_MODIFIER "u] = {\"%.4s\", %lu from %lu}",
        i, line->tag, (long unsigned int)line->length,
        (long unsigned int)line->offset
        );
    }
  }
  return MUNIT_OK;
}

MunitResult test_woff2_tag_toi
  (const MunitParameter params[], void* data)
{
  (void)params;
  (void)data;
  unsigned int i = (unsigned int)(munit_rand_uint32()&63);
  if (i == 63) {
    unsigned char const* value = tcmplxA_woff2_tag_fromi(i);
    munit_assert_ptr_null(value);
    unsigned char buf[4] = {1,2,3,4};
    unsigned int j = tcmplxA_woff2_tag_toi(buf);
    munit_assert_uint(j,==,i);
  } else {
    unsigned char const* value = tcmplxA_woff2_tag_fromi(i);
    munit_assert_ptr_not_null(value);
    unsigned char buf[4];
    memcpy(buf,value,sizeof(buf));
    unsigned int j = tcmplxA_woff2_tag_toi(buf);
    munit_assert_uint(j,==,i);
  }
  return MUNIT_OK;
}


int main(int argc, char **argv) {
  struct tcmplxAtest_arg tfa;
  int res;
  tcmplxAtest_arg_init(&tfa);
  res = munit_suite_main_custom
    (&suite_woff2, &tfa, argc, argv, tcmplxAtest_get_args());
  tcmplxAtest_arg_close(&tfa);
  return res;
}
