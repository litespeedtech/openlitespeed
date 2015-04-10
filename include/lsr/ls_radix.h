/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#ifndef LS_RADIX_H
#define LS_RADIX_H



/**
 * @file
 */

// #define LS_RADIX_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ls_radix_node_s ls_radix_node_t;

typedef struct ls_radix_s
{
    ls_radix_node_t *children;
} ls_radix_t;

/** @ls_radix_new
 * @brief Creates a new radix tree.  Allocates from the global pool
 * and initializes the tree.
 *
 * @return A pointer to a new initialized radix tree on success,
 * NULL on failure.
 *
 * @see ls_radix_delete
 */
ls_radix_t *ls_radix_new();

/** @ls_radix
 * @brief Initializes the radix tree.
 *
 * @param[in] pThis - A pointer to an allocated radix tree.
 * @return Void.
 *
 * @see ls_radix_d
 */
void ls_radix(ls_radix_t *pThis);

/** @ls_radix_d
 * @brief Destroys the radix tree.  Does not free the tree structure itself,
 * only the internals.
 * @note This function should be used in conjunction with ls_radix.
 * The user is responsible for freeing the objects held.
 *
 * @param[in] pThis - A pointer to an initialized radix tree.
 * @return Void.
 *
 * @see ls_radix
 */
void ls_radix_d(ls_radix_t *pThis);

/** @ls_radix_delete
 * @brief Deletes the radix tree.  Frees the tree internals and the
 * tree structure itself.
 * @note This function should be used in conjunction with ls_radix_new.
 * The user is responsible for freeing the objects held.
 *
 * @param[in] pThis - A pointer to an initialized radix tree.
 * @return Void.
 *
 * @see ls_radix_new
 */
void ls_radix_delete(ls_radix_t *pThis);

/** @ls_radix_insert
 * @brief Inserts a string that identifies the given object from within the
 * radix tree.
 *
 * @param[in] pThis - A pointer to an initialized radix tree.
 * @param[in] pStr - A pointer to the string of the new node.
 * @param[in] iStrLen - The length of the string.
 * @param[in] pObj - The object associated with the string.
 * @return LS_OK if successful, LS_FAIL if not.
 */
int ls_radix_insert(ls_radix_t *pThis, const char *pStr, int iStrLen,
                    void *pObj);

/** @ls_radix_update
 * @brief Updates the value of the string to the given object.
 * @note This function will \b not insert the string into the tree if the
 * string is not found.
 *
 * @param[in] pThis - A pointer to an initialized radix tree.
 * @param[in] pStr - A pointer to the string to match.
 * @param[in] iStrLen - The length of the string.
 * @param[in] pObj - The new object to update the node to.
 * @return Original object if the string is matched, NULL if not.
 */
void *ls_radix_update(ls_radix_t *pThis, const char *pStr, int iStrLen,
                      void *pObj);

/** @ls_radix_find
 * @brief Finds the string within the given radix tree.
 *
 * @param[in] pThis - A pointer to an initialized radix tree.
 * @param[in] pStr - A pointer to the string to match.
 * @param[in] iStrLen - The length of the string.
 * @return The object associated with the matching string if found,
 * NULL if not.
 */
void *ls_radix_find(ls_radix_t *pThis, const char *pStr, int iStrLen);

#ifdef LS_RADIX_DEBUG
void ls_radix_printtree(ls_radix_t *pThis);
#endif

//TODO: Delete/erase/remove?




#ifdef __cplusplus
}
#endif
#endif //LS_RADIX_H