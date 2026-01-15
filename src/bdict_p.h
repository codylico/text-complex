/**
 * @file text-complex/access/bdict_p.h
 * @brief built-in dictionary
 * @author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_TextComplexAccess_BDict_pH_
#define hg_TextComplexAccess_BDict_pH_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* BEGIN built-in dictionary / transform */
enum tcmplxA_bdict_cb {
  tcmplxA_BDict_Identity = 0u,
  tcmplxA_BDict_FermentFirst = 1u,
  tcmplxA_BDict_FermentAll = 2u,
  tcmplxA_BDict_OmitFirst1 = 3u,
  tcmplxA_BDict_OmitFirst2 = 4u,
  tcmplxA_BDict_OmitFirst3 = 5u,
  tcmplxA_BDict_OmitFirst4 = 6u,
  tcmplxA_BDict_OmitFirst5 = 7u,
  tcmplxA_BDict_OmitFirst6 = 8u,
  tcmplxA_BDict_OmitFirst7 = 9u,
  tcmplxA_BDict_OmitFirst8 = 10u,
  tcmplxA_BDict_OmitFirst9 = 11u,
  tcmplxA_BDict_OmitLast1 = 12u,
  tcmplxA_BDict_OmitLast2 = 13u,
  tcmplxA_BDict_OmitLast3 = 14u,
  tcmplxA_BDict_OmitLast4 = 15u,
  tcmplxA_BDict_OmitLast5 = 16u,
  tcmplxA_BDict_OmitLast6 = 17u,
  tcmplxA_BDict_OmitLast7 = 18u,
  tcmplxA_BDict_OmitLast8 = 19u,
  tcmplxA_BDict_OmitLast9 = 20u
};

/**
 * @internal
 * @brief Perform a word transform and add to an output buffer.
 * @param[out] dst output buffer
 * @param[in,out] dstlen write position
 * @param src source word
 * @param srclen length of source word
 */
void tcmplxA_bdict_cb_do
  ( unsigned char* dst, unsigned int* dstlen,
    unsigned char const* src, unsigned int srclen, unsigned int k);
/* END   built-in dictionary / transform */

/* BEGIN built-in dictionary / access */
/**
 * @brief Access the 4-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 4-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access04(unsigned int i);

/**
 * @brief Access the 5-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 5-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access05(unsigned int i);

/**
 * @brief Access the 6-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 6-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access06(unsigned int i);

/**
 * @brief Access the 7-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 7-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access07(unsigned int i);

/**
 * @brief Access the 8-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 8-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access08(unsigned int i);

/**
 * @brief Access the 9-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 9-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access09(unsigned int i);

/**
 * @brief Access the 10-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 10-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access10(unsigned int i);

/**
 * @brief Access the 11-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 11-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access11(unsigned int i);

/**
 * @brief Access the 12-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 12-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access12(unsigned int i);

/**
 * @brief Access the 13-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 13-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access13(unsigned int i);

/**
 * @brief Access the 14-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 14-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access14(unsigned int i);

/**
 * @brief Access the 15-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 15-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access15(unsigned int i);

/**
 * @brief Access the 16-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 16-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access16(unsigned int i);

/**
 * @brief Access the 17-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 17-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access17(unsigned int i);

/**
 * @brief Access the 18-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 18-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access18(unsigned int i);

/**
 * @brief Access the 19-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 19-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access19(unsigned int i);

/**
 * @brief Access the 20-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 20-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access20(unsigned int i);

/**
 * @brief Access the 21-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 21-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access21(unsigned int i);

/**
 * @brief Access the 22-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 22-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access22(unsigned int i);

/**
 * @brief Access the 23-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 23-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access23(unsigned int i);

/**
 * @brief Access the 24-length section of the dictionary.
 * @param i word index
 * @return a pointer to the requested word in the 24-length
 *   dictionary section
 */
unsigned char const* tcmplxA_bdict_access24(unsigned int i);
/* END   built-in dictionary / access */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BDict_pH_*/
