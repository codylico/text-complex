/**
 * @file text-complex/access/brmeta.h
 * @brief Brotli metadata storage
 */
#ifndef hg_TextComplexAccess_BrMeta_H_
#define hg_TextComplexAccess_BrMeta_H_

#include "api.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Brotli metadata storage.
 */
struct tcmplxA_brmeta;

/* BEGIN brmeta */
/**
 * @brief Allocate and initialize a metadata store.
 * @param cap initial capacity (or zero for fully empty)
 * @return a metadata store on success, `NULL` otherwise
 */
struct tcmplxA_brmeta* tcmplxA_brmeta_new(size_t cap);

/**
 * @brief Emplace a metadata line at the end of a storage.
 * @param b storage to update
 * @param n length of metadata line in bytes
 * @return Success on success, nonzero otherwise
 */
int tcmplxA_brmeta_emplace(struct tcmplxA_brmeta* b, size_t n);

/**
 * @brief Query the number of lines in a storage.
 * @param b storage to inspect
 * @return a line count
 */
size_t tcmplxA_brmeta_size(struct tcmplxA_brmeta const* b);

/**
 * @brief Query the length of a line in a storage.
 * @param b storage to inspect
 * @param i array index of item to inspect
 * @return a byte count
 */
size_t tcmplxA_brmeta_itemsize(struct tcmplxA_brmeta const* b, size_t i);

/**
 * @brief Access the data of a line in a storage.
 * @param b storage to inspect
 * @param i array index of item to inspect
 * @return a pointer to the start of the byte array on success,
 *   `NULL` on out-of-bounds
 */
unsigned char const* tcmplxA_brmeta_itemdata_c
    (struct tcmplxA_brmeta const* b, size_t i);

/**
 * @brief Access the data of a line in a storage.
 * @param b storage to inspect
 * @param i array index of item to inspect
 * @return a pointer to the start of the byte array on success,
 *   `NULL` on out-of-bounds
 */
unsigned char* tcmplxA_brmeta_itemdata
    (struct tcmplxA_brmeta const* b, size_t i);

/**
 * @brief Destroy a metadata store.
 * @param p (optional) a metadata store to destroy
 */
void tcmplxA_brmeta_destroy(struct tcmplxA_brmeta* p);
/* END   brmeta */

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_TextComplexAccess_BrMeta_H_*/
