
#include "../tcmplx-access/api.h"
#include <stdio.h>

int main(int argc, char **argv) {
  fprintf(stdout, "text-complex version: %s\n", tcmplxA_api_version());
  return 0;
}
