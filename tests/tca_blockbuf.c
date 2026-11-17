/**
 * @brief Test program for block buffer
 */
#include "testfont.h"
#include "text-complex/access/blockbuf.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_blockbuf_cycle(void* data);
static int test_blockbuf_item(void* data);
static int test_blockbuf_gen(void* data);
static int test_blockbuf_noconv(void* data);
static int test_blockbuf_bypass(void* data);
static int test_blockbuf_add(void* data);
static void* test_blockbuf_setup(void* user_data);
static void test_blockbuf_teardown(void* fixture);

static struct reedkiln_box box_blockbuf = {
  &test_blockbuf_setup, &test_blockbuf_teardown
};

static struct reedkiln_entry tests_blockbuf[] = {
  {"access/blockbuf/cycle", test_blockbuf_cycle },
  {"access/blockbuf/item", test_blockbuf_item, NULL, &box_blockbuf },
  {"access/blockbuf/gen", test_blockbuf_gen, NULL, &box_blockbuf },
  {"access/blockbuf/noconv", test_blockbuf_noconv, NULL, &box_blockbuf },
  {"access/blockbuf/bypass", test_blockbuf_bypass, NULL, &box_blockbuf },
  {"access/blockbuf/add", test_blockbuf_add, NULL, &box_blockbuf },
  {NULL}
};




int test_blockbuf_cycle(void* data)
{
  struct tcmplxA_blockbuf* ptr[1];
  unsigned const block_size = testfont_rand_uint_range(1u,128u);
  unsigned const window_size = testfont_rand_uint_range(128u,512u);
  size_t const chain_length = (size_t)testfont_rand_int_range(1u,32u);
  int const bdict_tf = munit_rand_int_range(0,1);
  (void)data;
  ptr[0] = tcmplxA_blockbuf_new
    (block_size, window_size, chain_length, bdict_tf);
  reedkiln_assert(ptr[0]);
  reedkiln_assert(tcmplxA_blockbuf_capacity(ptr[0]) == block_size);
  reedkiln_assert(tcmplxA_blockbuf_extent(ptr[0]) == window_size);
  tcmplxA_blockbuf_destroy(ptr[0]);
  return Reedkiln_OK;
}

void* test_blockbuf_setup(void* user_data) {
  unsigned const block_size = testfont_rand_uint_range(1u,128u);
  unsigned const window_size = testfont_rand_uint_range(128u,512u);
  size_t const chain_length = testfont_rand_size_range(1u,32u);
  int const bdict_tf = testfont_rand_int_range(0,1);
  return tcmplxA_blockbuf_new
    (block_size, window_size, chain_length, bdict_tf);
}

void test_blockbuf_teardown(void* fixture) {
  tcmplxA_blockbuf_destroy((struct tcmplxA_blockbuf*)fixture);
  return;
}

int test_blockbuf_item(void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = testfont_rand_size_range
    (1, tcmplxA_blockbuf_capacity(p));
  if (p == NULL)
    return Reedkiln_SKIP;
  /* add some stuff */{
    unsigned char buf[128];
    int res;
    reedkiln_memrand(buf, count);
    res = tcmplxA_blockbuf_write(p, buf, count);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == count);
  }
  return Reedkiln_OK;
}

int test_blockbuf_gen(void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = tcmplxA_blockbuf_capacity(p);
  unsigned char buf[128];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* build the text */{
    size_t i;
    reedkiln_memrand(buf, count);
    for (i = 0; i < count; ++i) {
      buf[i] = (buf[i]&3u)|80u;
    }
  }
  /* add to input */{
    int res;
    res = tcmplxA_blockbuf_write(p, buf, count);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == count);
  }
  /* */{
    int const res = tcmplxA_blockbuf_gen_block(p);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == 0);
  }
  /* inspect */{
    unsigned char const* const output = tcmplxA_blockbuf_output_data(p);
    tcmplxA_uint32 const len = tcmplxA_blockbuf_output_size(p);
    reedkiln_assert(len > 0u);
    reedkiln_assert(output);
    /* parse the command buffer */{
      unsigned char inflated_buf[128];
      size_t j = 0;
      unsigned i;
      for (i = 0; i < len; ++i) {
        unsigned n;
        unsigned char s80 = output[i]&0x80;
        if (output[i]&0x40) {
          reedkiln_assert(i+2u <= len);
          n = ((output[i]&0x3f)*256)+output[i+1];
          i += 2;
        } else {
          n = output[i]&0x3f;
          i += 1;
        }
        if (s80) {
          unsigned backward;
          unsigned x;
          reedkiln_assert(i+2u <= len);
          reedkiln_assert((output[i]&0xC0) == 0x80)/* short distances */;
          backward = ((output[i]&0x3f)*256) + (output[i+1]) + 1;
          reedkiln_assert(backward <= j);
          for (x = 0; x < n && j < 128; ++x, ++j) {
            inflated_buf[j] = inflated_buf[j-backward];
          }
          i += 1;
        } else {
          unsigned x;
          for (x = 0u; x < n && j < 128; ++x, ++i, ++j) {
            reedkiln_assert(i < len);
            inflated_buf[j] = output[i];
          }
          i -= 1;
        }
      }
      reedkiln_assert(j == count);
      reedkiln_assert(memcmp(inflated_buf, buf, j) == 0);
    }
  }
  /* clear */{
    tcmplxA_blockbuf_clear_output(p);
    reedkiln_assert(tcmplxA_blockbuf_output_size(p) == 0);
  }
  return Reedkiln_OK;
}

int test_blockbuf_noconv(void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = tcmplxA_blockbuf_capacity(p);
  unsigned char buf[128];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* build the text */{
    size_t i;
    reedkiln_memrand(buf, count);
    for (i = 0; i < count; ++i) {
      buf[i] = (buf[i]&3u)|80u;
    }
  }
  /* add to input */{
    int res;
    res = tcmplxA_blockbuf_write(p, buf, count);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == count);
  }
  /* */{
    int const res = tcmplxA_blockbuf_noconv_block(p);
    reedkiln_assert(res == tcmplxA_Success);
    tcmplxA_blockbuf_clear_input(p);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == 0);
  }
  /* inspect */{
    unsigned char const* const output = tcmplxA_blockbuf_output_data(p);
    tcmplxA_uint32 const len = tcmplxA_blockbuf_output_size(p);
    reedkiln_assert(len > 0u);
    reedkiln_assert(output);
    reedkiln_assert(memcmp(output, buf, len) == 0);
  }
  return Reedkiln_OK;
}

int test_blockbuf_bypass(void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  size_t const count = tcmplxA_blockbuf_capacity(p);
  unsigned char buf[3];
  if (p == NULL || count < 3)
    return Reedkiln_SKIP;
  /* build the text */{
    size_t i;
    reedkiln_memrand(buf, 3);
    for (i = 0; i < 3; ++i) {
      buf[i] = (buf[i]&3u)|80u;
    }
  }
  /* add to slide ring */{
    size_t const bypass_count = tcmplxA_blockbuf_bypass(p, buf, 3);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == 0u);
    reedkiln_assert(bypass_count == 3);
  }
  /* add to input */{
    int res;
    res = tcmplxA_blockbuf_write(p, buf, 3);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == 3u);
  }
  /* */{
    int const res = tcmplxA_blockbuf_gen_block(p);
    reedkiln_assert(res == tcmplxA_Success);
    reedkiln_assert(tcmplxA_blockbuf_input_size(p) == 0);
  }
  /* inspect */{
    unsigned char const copy_cmd[] = {131u, 128u, 2u};
    unsigned char const* const output = tcmplxA_blockbuf_output_data(p);
    tcmplxA_uint32 const len = tcmplxA_blockbuf_output_size(p);
    reedkiln_assert(len == 3u);
    reedkiln_assert(output);
    reedkiln_assert(memcmp(output, copy_cmd, 3) == 0);
  }
  return Reedkiln_OK;
}

int test_blockbuf_add(void* data)
{
  struct tcmplxA_blockbuf* const p = (struct tcmplxA_blockbuf*)data;
  int const add_count = munit_rand_int_range(1,64);
  unsigned char buf[64];
  if (p == NULL)
    return Reedkiln_SKIP;
  /* fill the buffer */{
    reedkiln_memrand(buf, add_count);
  }
  /* add to slide ring */{
    size_t const bypass_count = tcmplxA_blockbuf_bypass(p, buf, add_count);
    reedkiln_assert(bypass_count == add_count);
  }
  /* check the stored bytes */{
    int j;
    for (j = 0; j < add_count; ++j) {
      int const i = add_count-j-1;
      reedkiln_assert(tcmplxA_blockbuf_peek(p, i) == buf[j]);
    }
  }
  return Reedkiln_OK;
}



int main(int argc, char **argv) {
  return reedkiln_main(&tests_blockbuf, argc, argv, NULL);
}
