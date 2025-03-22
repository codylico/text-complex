
#include <text-complex/access/brcvt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
int do_zsrtostr(FILE *in, FILE* out) {
  unsigned char inbuf[256];
  unsigned char outbuf[256] = {};
  int ec = EXIT_SUCCESS;
  int done = 0;
  struct tcmplxA_brcvt* state = tcmplxA_brcvt_new(1, 32768, 1);
  if (state == NULL)
    return EXIT_FAILURE;
  else while (ec == EXIT_SUCCESS && !done) {
    if (feof(in))
      break;
    else if (ferror(in)) {
      ec = EXIT_FAILURE;
      break;
    } else {
      size_t const in_count = fread(inbuf, sizeof(unsigned char), 256, in);
      unsigned char const* src;
      for (src = inbuf; ec == EXIT_SUCCESS && src<inbuf+in_count; ) {
        size_t retval;
        int const res = tcmplxA_brcvt_zsrtostr
          (state, &retval, outbuf, 256, &src, inbuf+in_count);
        if (res < tcmplxA_Success) {
          ec = EXIT_FAILURE;
          fprintf(stderr, "error code from conversion:\n\t%s\n",
              tcmplxA_api_error_toa(res));
        } else {
          size_t const out_count = fwrite
            (outbuf, sizeof(unsigned char), retval, out);
          if (out_count != retval) {
            ec = EXIT_FAILURE;
            fputs("error from file write\n", stderr);
          } else if (res == tcmplxA_EOF) {
            done = 1;
            break;
          }
        }
      }
    }
  }
  tcmplxA_brcvt_destroy(state);
  return ec;
}

static
int do_strrtozs(FILE *in, FILE* out) {
  unsigned char inbuf[256];
  unsigned char outbuf[256];
  int ec = EXIT_SUCCESS;
  int done = 0;
  struct tcmplxA_brcvt* state = tcmplxA_brcvt_new(1, 32768, 1);
  if (state == NULL)
    return EXIT_FAILURE;
  else while (ec == EXIT_SUCCESS) {
    if (feof(in))
      break;
    else if (ferror(in)) {
      ec = EXIT_FAILURE;
      break;
    } else {
      size_t const in_count = fread(inbuf, sizeof(unsigned char), 256, in);
      unsigned char const* src;
      for (src = inbuf; ec == EXIT_SUCCESS && src<inbuf+in_count; ) {
        size_t retval;
        int const res = tcmplxA_brcvt_strrtozs
          (state, &retval, outbuf, 256, &src, inbuf+in_count);
        if (res < tcmplxA_Success) {
          ec = EXIT_FAILURE;
          fprintf(stderr, "error code from conversion:\n\t%s\n",
              tcmplxA_api_error_toa(res));
        } else {
          size_t const out_count = fwrite
            (outbuf, sizeof(unsigned char), retval, out);
          if (out_count != retval) {
            ec = EXIT_FAILURE;
            fputs("error from file write\n", stderr);
          }
        }
      }
    }
  }
  if (ec != EXIT_SUCCESS)
    return ec;
  while (ec == EXIT_SUCCESS && !done) {
    size_t retval;
    int const res = tcmplxA_brcvt_delimrtozs(state, &retval, outbuf, 256);
    if (res < tcmplxA_Success) {
      ec = EXIT_FAILURE;
      fprintf(stderr, "error code from conclusion:\n\t%s\n",
          tcmplxA_api_error_toa(res));
    } else {
      size_t const out_count = fwrite
        (outbuf, sizeof(unsigned char), retval, out);
      if (out_count != retval) {
        ec = EXIT_FAILURE;
        fputs("error from file write\n", stderr);
      }
      done = (res == tcmplxA_EOF);
    }
  }
  tcmplxA_brcvt_destroy(state);
  return ec;
}

int main(int argc, char**argv) {
  char const* ifilename = NULL;
  char const* ofilename = NULL;
  int tozs = 1;
  /* */{
    int i;
    for (i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-d") == 0) {
        tozs = 0;
      } else if (ifilename == NULL) {
        ifilename = argv[i];
      } else if (ofilename == NULL) {
        ofilename = argv[i];
      }
    }
    if (ifilename == NULL || ofilename == NULL) {
      fputs("usage: tcmplx_access_brcvt [-d] (input) (output)\n", stderr);
      return EXIT_FAILURE;
    }
  }
  /* */{
    FILE* ifile = fopen(ifilename, "rb");
    FILE* ofile = fopen(ofilename, "wb");
    if (ifile != NULL && ofile != NULL) {
      int const x =
        tozs ? do_strrtozs(ifile, ofile) : do_zsrtostr(ifile, ofile);
      fclose(ifile);
      fclose(ofile);
      return x;
    } else {
      if (ifile == NULL)
        fputs("failed to open input file\n", stderr);
      else fclose(ifile);
      if (ofile == NULL)
        fputs("failed to open output file\n", stderr);
      else fclose(ofile);
      return EXIT_FAILURE;
    }
  }
}
