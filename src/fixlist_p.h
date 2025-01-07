/**
 * @file text-complex/access/fixlist_p.h
 * @brief Context map for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_FixList_pH_
#define hg_TextComplexAccess_FixList_pH_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct tcmplxA_fixline;

struct tcmplxA_fixlist {
  struct tcmplxA_fixline* p;
  size_t n;
};

/**
 * @brief Initialize a prefix list.
 * @param x the prefix list to initialize
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_fixlist_init(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Resize a prefix list.
 * @param x the list to resize
 * @param sz number of lines in the list
 * @return zero on success, nonzero otherwise
 */
int tcmplxA_fixlist_resize(struct tcmplxA_fixlist* x, size_t sz);
/**
 * @brief Close a prefix list.
 * @param x the prefix list to close
 */
void tcmplxA_fixlist_close(struct tcmplxA_fixlist* x);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_FixList_pH_*/
