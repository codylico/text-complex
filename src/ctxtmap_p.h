/**
 * @file text-complex/access/ctxtmap.c
 * @brief Context map for compressed streams
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_CtxtMap_pH_
#define hg_TextComplexAccess_CtxtMap_pH_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
* @brief Query the Brotli Lookup Table #2.
* @param i byte value index
* @return a lookup table value
*/
unsigned char tcmplxA_ctxtmap_getlut2(int i);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_CtxtMap_pH_*/
