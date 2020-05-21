
#include "../tcmplx-access/api.h"
#include <stdio.h>

int main(int argc, char **argv) {
  fprintf(stdout, "text-complex version: %s\n", tcmplxA_api_version());
  /* error code "test" */ {
    int pre_v;
    for (pre_v = -5; pre_v <= 0; ++pre_v) {
      enum tcmplxA_api_error v = (enum tcmplxA_api_error)pre_v;
      char const* v_text = tcmplxA_api_error_toa(v);
      fprintf(stdout, "error value %i: %s\n", v,
        ((v_text!=NULL) ? v_text : "(nil)"));
    }
  }
  return 0;
}
