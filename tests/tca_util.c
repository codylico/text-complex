
#include "text-complex/access/util.h"
#include "reedkiln/reedkiln.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int test_util_alloc(void* data);

static struct reedkiln_entry const tests_util[] = {
  {"access/util/alloc", test_util_alloc },
  {NULL}
};


int test_util_alloc(void* data)
{
  void* ptr[3];
  ptr[0] = tcmplxA_util_malloc(0u);
  reedkiln_assert(ptr[0] == NULL);
  ptr[0] = tcmplxA_util_malloc(sizeof(unsigned long int));
  reedkiln_assert(ptr[0] != NULL);
  tcmplxA_util_free(ptr[0]);
  return Reedkiln_OK;
}

int main(int argc, char **argv) {
  return reedkiln_main(&tests_util, argc, argv, NULL);
}
