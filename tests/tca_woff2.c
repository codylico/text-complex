/**
 * @brief Test program for woff2
 */
#include "testfont.h"
#include "text-complex/access/woff2.h"
#include "text-complex/access/offtable.h"
#include "mmaptwo/mmaptwo.h"
#include "reedkiln/log.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_woff2_cycle(void* data);
static int test_woff2_item(void* data);
static int test_woff2_tag_toi(void* data);
static void* test_woff2_setup(void* user_data);
static void* test_woff2_null_setup(void* user_data);
static void test_woff2_teardown(void* fixture);


static struct reedkiln_box const box_woff2_null = {
  test_woff2_null_setup, test_woff2_teardown
};
static struct reedkiln_box const box_woff2 = {
  test_woff2_setup, test_woff2_teardown
};

static struct reedkiln_entry const tests_woff2[] = {
  {"access/woff2/cycle", test_woff2_cycle, Reedkiln_TODO },
  {"access/woff2/item", test_woff2_item, 0,&box_woff2 },
  {"access/woff2/tag_toi", test_woff2_tag_toi },
  {NULL}
};





int test_woff2_cycle(void* data)
{
  struct tcmplxA_woff2* ptr[1];
  (void)data;
  ptr[0] = tcmplxA_woff2_new(NULL,0);
  reedkiln_assert(!ptr[0]);
  tcmplxA_woff2_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_woff2_null_setup(void* user_data) {
  return tcmplxA_woff2_new(NULL,0);
}

void* test_woff2_setup(void* user_data) {
  struct tcmplxAtest_arg *const tfa = (struct tcmplxAtest_arg *)user_data;
  struct mmaptwo_i* m2i;
  struct tcmplxA_woff2* w2;
  if (tfa->font_path != NULL)
    m2i = mmaptwo_open(tfa->font_path, "re", 0,0);
  else
    m2i = tcmplxAtest_gen_maptwo(tcmplxAtest_Zero, 256, reedkiln_rand());
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

int test_woff2_item(void* data)
{
  struct tcmplxA_woff2* const p = (struct tcmplxA_woff2*)data;
  if (p == NULL)
    return Reedkiln_IGNORE;
  /* inspect offset table items */{
    struct tcmplxA_offtable const* const offt = tcmplxA_woff2_get_offsets(p);
    size_t l, i;
    reedkiln_assert(offt);
    l = tcmplxA_offtable_size(offt);
    for (i = 0; i < l; ++i) {
      struct tcmplxA_offline const* line = tcmplxA_offtable_at_c(offt, i);
      reedkiln_log_printf("[%i]={\"%.4s\", %li+%li};",
        (int)i, line->tag,
        (long int)line->offset, (long int)line->length
        );
    }
  }
  return Reedkiln_OK;
}

int test_woff2_tag_toi(void* data)
{
  (void)data;
  unsigned int i = (reedkiln_rand()&63);
  if (i == 63) {
    unsigned char const* value = tcmplxA_woff2_tag_fromi(i);
    reedkiln_assert(!value);
    unsigned char buf[4] = {1,2,3,4};
    unsigned int j = tcmplxA_woff2_tag_toi(buf);
    reedkiln_assert(j == i);
  } else {
    unsigned char const* value = tcmplxA_woff2_tag_fromi(i);
    reedkiln_assert(value);
    unsigned char buf[4];
    memcpy(buf,value,sizeof(buf));
    unsigned int j = tcmplxA_woff2_tag_toi(buf);
    reedkiln_assert(j == i);
  }
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  struct tcmplxAtest_arg tfa = {NULL};
  int res;
  tcmplxAtest_arg_init(&tfa);
  res = testfont_do_main(tests_woff2, &tfa, argc, argv);
  tcmplxAtest_arg_close(&tfa);
  return res;
}
