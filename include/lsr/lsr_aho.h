/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef LSR_AHO_H
#define LSR_AHO_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * @file
 * @todo Change to full keyword match rather than prefix.  Add object value to
 * associate with full keyword like hash.
 */

#define LSR_AHO_MAX_STRING_LEN 8192
#define LSR_AHO_MAX_FIRST_CHARS 256
//#define LSR_AHO_DEBUG

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct lsr_aho_s lsr_aho_t;
typedef struct lsr_aho_state_s lsr_aho_state_t;

/**
 * @typedef lsr_aho_t
 * @brief Aho Corasick Tree Structure.  Contains the root and tree information.
 */
struct lsr_aho_s
{
    lsr_aho_state_t *m_zero_state;
    unsigned int     m_next_state;
    int              m_case_sensitive;
};



/** @lsr_aho_new
 * @brief Creates a new Aho tree object, sets case sensitivity based on parameter.
 * 
 * @param[in] case_sensitive - A case sensitivity flag.
 * 1 for case sensitive, 0 for case insensitive.
 * @return A pointer to the Aho Tree object if successful, NULL if unsuccessful.
 * 
 * @see lsr_aho_delete
 */
lsr_aho_t  *lsr_aho_new( int case_sensitive );

/** @lsr_aho
 * @brief Initializes an Aho tree object that's passed in.
 * 
 * @param[in] pThis - A pointer to an allocated Aho tree object.
 * @param[in] case_sensitive - A case sensitivity flag.
 * 1 for case sensitive, 0 for case insensitive.
 * @return 1 for success, 0 for failure.
 * 
 * @see lsr_aho_d
 */
int         lsr_aho( lsr_aho_t *pThis, int case_sensitive );

/** @lsr_aho_d
 * @brief Destroys the Aho tree, opposite of lsr_aho
 * @details If an Aho Tree was initialized with lsr_aho, it is assumed that the user
 * is responsible for freeing the tree object itself.  This function just does 
 * garbage collection for the internal constructs and will NOT free \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object.
 * @return Void.
 * 
 * @see lsr_aho
 */
void        lsr_aho_d( lsr_aho_t *pThis );

/** @lsr_aho_delete
 * @brief Deletes the Aho tree, opposite of lsr_aho_new
 * @details If an Aho Tree was created with lsr_aho_new, it must be deleted with
 * lsr_aho_delete; otherwise, there will be a memory leak.  This function does
 * garbage collection for the internal constructs and will free \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object.
 * @return Void.
 * 
 * @see lsr_aho_new
 */
void        lsr_aho_delete( lsr_aho_t *pThis );

/** @lsr_aho_add_non_unique_pattern
 * @brief Adds parameter pattern of length \e size to the Aho tree.  Should be called after initialization
 * and before \link #lsr_aho_make_tree make tree.\endlink
 * @details This function will take a look at the pattern and potentially add it to the tree.
 * @note If a PREFIX of the pattern is found, then the pattern will be ignored.  If a PREFIX is NOT
 * found, then the pattern will be added.  This function may be repeated for any number of
 * patterns, but it must be called AFTER INITIALIZATION and BEFORE MAKE TREE.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object.
 * @param[in] pattern - A pointer to the pattern to add to the tree.
 * @param[in] size - The length of the pattern.
 * @return 1 for success, 0 for failure.
 * 
 * @see lsr_aho_new, lsr_aho, lsr_aho_make_tree
 */
int         lsr_aho_add_non_unique_pattern( lsr_aho_t *pThis, const char *pattern, size_t size );

/** @lsr_aho_add_patterns_from_file
 * @brief Adds patterns found in the file to the Aho tree.  Should be called after initialization
 * and before \link #lsr_aho_make_tree make tree.\endlink
 * @details This function will take a look at the patterns and potentially add it to the tree.
 * @note If PREFIX of any pattern is found, then the pattern will be ignored.  If a PREFIX is NOT
 * found, then the pattern will be added.  This function may be repeated for any number of
 * files, but it must be called AFTER INITIALIZATION and BEFORE MAKE TREE.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object to add the patterns to.
 * @param[in] filename - A pointer to the filename of the file containing the patterns.
 * @return 1 for success, 0 for failure.
 * 
 * @see lsr_aho_new, lsr_aho, lsr_aho_make_tree
 */
int         lsr_aho_add_patterns_from_file( lsr_aho_t *pThis, const char *filename );

/** @lsr_aho_make_tree
 * @brief Finishes up the structure of the tree after all the patterns are added.
 * Called after patterns are added and before \link #lsr_aho_optimize_tree optimize tree.\endlink
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object to complete.
 * @return 1 for success, 0 for failure.
 * 
 * @see lsr_aho_add_non_unique_pattern, lsr_aho_add_patterns_from_file, lsr_aho_optimize_tree
 */
int         lsr_aho_make_tree( lsr_aho_t *pThis );

/** @lsr_aho_optimize_tree
 * @brief Optimizes the tree for searches.  Must be called after make tree.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object to optimize.
 * @return 1 for success, 0 for failure.
 * 
 * @see lsr_aho_make_tree
 */
int         lsr_aho_optimize_tree( lsr_aho_t *pThis );

/** @lsr_aho_search
 * @brief Searches for pattern matches starting from a given start state.
 * Must be called after tree optimization.
 * 
 * @param[in] pThis - A pointer to an initialized Aho tree object to search.
 * @param[in] start_state - A pointer to the starting state.  If NULL, will be 
 * considered as the zero state.  First call should start at the zero state.
 * @param[in] string - A pointer to the string to search through.
 * @param[in] size - The length of the string.
 * @param[in] startpos - The start position of the string to start comparing against.
 * @param[out] out_start - A pointer to the start position of the matching pattern in the string.
 * Will be set to -1 if the string is not found.
 * @param[out] out_end - A pointer to the end position of the matching pattern in the string.
 * Will be set to -1 if the string is not found.
 * @param[out] out_last_state - A pointer to the ending state after searching the string.  
 * This can be the final character of the pattern if it matched, or the last one reached
 * when finished with the string.
 * @return Non-zero for match, 0 for no match.
 * 
 * @see lsr_aho_optimize_tree
 */
unsigned int lsr_aho_search( lsr_aho_t *pThis, lsr_aho_state_t *start_state, const char *string, 
                             size_t size, size_t startpos, size_t *out_start, size_t *out_end, 
                             lsr_aho_state_t **out_last_state );

#ifdef __cplusplus
}
#endif

#endif //LSR_AHO_H
