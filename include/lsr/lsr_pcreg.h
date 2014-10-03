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

#ifndef LSR_PCREG_H
#define LSR_PCREG_H


#include <lsr/lsr_str.h>
#include <lsr/lsr_types.h>
#include <pcre.h>

/**
 * @file
 */

//#define _USE_PCRE_JIT_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsr_pcre_result_s lsr_pcre_result_t;
typedef struct lsr_pcre_s lsr_pcre_t;
typedef struct lsr_pcre_sub_s lsr_pcre_sub_t;
typedef struct lsr_pcre_sub_entry_s lsr_pcre_sub_entry_t;

/**
 * @typedef lsr_pcre_result_t
 * @brief A structure to hold the pcre results in one struct.
 */
struct lsr_pcre_result_s
{
    const char *m_pBuf;
    int         m_ovector[30];
    int         m_matches;
};

/**
 * @typedef lsr_pcre_t
 * @brief The base pcre structure.  Used for pcre compilation and execution.
 */
struct lsr_pcre_s
{
    pcre       *m_regex;
    pcre_extra *m_extra;
    int         m_iSubStr;
};

/**
 * @typedef lsr_pcre_sub_entry_t
 * @brief A substitution entry.
 * result.
 */
struct lsr_pcre_sub_entry_s
{
    int m_strBegin;
    int m_strLen;
    int m_param;  
};

/**
 * @typedef lsr_pcre_sub_t
 * @brief Contains the list of the substitution entries.
 */
struct lsr_pcre_sub_s
{
    char                   *m_parsed;
    lsr_pcre_sub_entry_t   *m_pList;
    lsr_pcre_sub_entry_t   *m_pListEnd;
};

/** @lsr_pcre_result_new
 * @brief Creates a new pcre result object and initializes it.
 * Must be \link #lsr_pcre_result_delete deleted \endlink when finished.
 * 
 * @return The created result object, or NULL if it failed.
 * 
 * @see lsr_pcre_result_delete
 */
lsr_pcre_result_t  *lsr_pcre_result_new();

/** @lsr_pcre_result
 * @brief Initializes a given pcre result object.
 * 
 * @param[in] pThis - A pointer to an allocated pcre result object.
 * @return The initialized result object, or NULL if it failed.
 * 
 * @see lsr_pcre_result_d
 */
lsr_pcre_result_t  *lsr_pcre_result( lsr_pcre_result_t *pThis );

/** @lsr_pcre_result_d
 * @brief Destroys the pcre result object.  DOES NOT FREE THE STRUCT ITSELF!
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @return Void.
 * 
 * @see lsr_pcre_result
 */
void                lsr_pcre_result_d( lsr_pcre_result_t *pThis );

/** @lsr_pcre_result_delete
 * @brief Destroys and deletes the pcre result object.
 * @details The object should have been created with a previous
 *   successful call to lsr_pcre_result_new.
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @return Void.
 * 
 * @see lsr_pcre_result_new
 */
void                lsr_pcre_result_delete( lsr_pcre_result_t *pThis );

/** @lsr_pcre_new
 * @brief Creates a new pcre object and initializes it.
 * Must be \link #lsr_pcre_delete deleted \endlink when finished.
 * 
 * @return The created pcre object, or NULL if it failed.
 * 
 * @see lsr_pcre_delete
 */
lsr_pcre_t         *lsr_pcre_new();

/** @lsr_pcre
 * @brief Initializes a given pcre object.
 * Must be \link #lsr_pcre_d destroyed \endlink when finished.
 * 
 * @param[in] pThis - A pointer to an allocated pcre object.
 * @return The initialized pcre object, or NULL if it failed.
 * 
 * @see lsr_pcre_d
 */
lsr_pcre_t         *lsr_pcre( lsr_pcre_t *pThis );

/** @lsr_pcre_d
 * @brief Destroys the pcre object.  DOES NOT FREE THE STRUCT ITSELF!
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @return Void.
 * 
 * @see lsr_pcre
 */
void                lsr_pcre_d( lsr_pcre_t *pThis );

/** @lsr_pcre_delete
 * @brief Destroys and deletes the pcre object.
 * @details The object should have been created with a previous
 *   successful call to lsr_pcre_new.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @return Void.
 * 
 * @see lsr_pcre_new
 */
void                lsr_pcre_delete( lsr_pcre_t *pThis );

/** @lsr_pcre_sub_new
 * @brief Creates a new pcre sub object and initializes it.
 * Must be \link #lsr_pcre_sub_delete deleted \endlink when finished.
 * 
 * @return The created pcre sub object, or NULL if it failed.
 * 
 * @see lsr_pcre_sub_delete
 */
lsr_pcre_sub_t     *lsr_pcre_sub_new();

/** @lsr_pcre_sub
 * @brief Initializes a given pcre sub object.
 * Must be \link #lsr_pcre_sub_d destroyed \endlink when finished.
 * 
 * @param[in] pThis - A pointer to an allocated pcre sub object.
 * @return The initialized sub object, or NULL if it failed.
 * 
 * @see lsr_pcre_sub_d
 */
lsr_pcre_sub_t     *lsr_pcre_sub( lsr_pcre_sub_t *pThis );

/** @lsr_pcre_sub_copy
 * @brief Copies the sub internals from src to dest.
 * 
 * @param[in] dest - A pointer to an initialized pcre sub object.
 * @param[in] src - A pointer to an initialized pcre sub object.
 * @return Dest if successful, NULL if not.
 * 
 * @see lsr_pcre_sub_d
 */
lsr_pcre_sub_t     *lsr_pcre_sub_copy( lsr_pcre_sub_t *dest, const lsr_pcre_sub_t *src );

/** @lsr_pcre_sub_d
 * @brief Destroys the pcre sub object.  DOES NOT FREE THE STRUCT ITSELF!
 * 
 * @param[in] pThis - A pointer to an initialized pcre sub object.
 * @return Void.
 * 
 * @see lsr_pcre_sub
 */
void                lsr_pcre_sub_d( lsr_pcre_sub_t *pThis );

/** @lsr_pcre_sub_delete
 * @brief Destroys and deletes the pcre sub object.
 * @details The object should have been created with a previous
 *   successful call to lsr_pcre_sub_new.
 * 
 * @param[in] pThis - A pointer to an initialized pcre sub object.
 * @return Void.
 * 
 * @see lsr_pcre_sub_new
 */
void                lsr_pcre_sub_delete( lsr_pcre_sub_t *pThis );

/** @lsr_pcre_result_set_buf
 * @brief Sets the result buf to \e pBuf.
 * @note This does not create a deep copy!
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @param[in] pBuf - A pointer to the buffer to set the struct buf to.
 * @return Void.
 */
lsr_inline void  lsr_pcre_result_set_buf( lsr_pcre_result_t *pThis, const char *pBuf )
{   pThis->m_pBuf = pBuf;        }

/** @lsr_pcre_result_set_matches
 * @brief Sets the number of matches to \e matches.
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @param[in] matches - The number of matches to set the object to.
 * @return Void.
 */
lsr_inline void  lsr_pcre_result_set_matches( lsr_pcre_result_t *pThis, int matches )
{   pThis->m_matches = matches;  }

/** @lsr_pcre_result_get_vector
 * @brief Gets the ovector from the result.
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @return a pointer to the ovector.
 */
lsr_inline int  *lsr_pcre_result_get_vector( lsr_pcre_result_t *pThis )
{   return pThis->m_ovector;    }

/** @lsr_pcre_result_get_matches
 * @brief Gets the matches from the result.
 * 
 * @param[in] pThis - A pointer to an initialized result object.
 * @return The matches count. 
 */
lsr_inline int   lsr_pcre_result_get_matches( lsr_pcre_result_t *pThis )
{   return pThis->m_matches;    }

/** @lsr_pcre_result_get_substring
 * @brief Gets the substring from the result.
 * 
 * @param[in] pThis - A pointer to an initialized pcre result object.
 * @param[in] i - The index of the substring to get.
 * @param[out] pValue - A pointer to the resulting substring.
 * @return The length of the substring.
 */
int                 lsr_pcre_result_get_substring( const lsr_pcre_result_t *pThis, int i, char ** pValue );

#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
    void lsr_pcre_init_jit_stack();
    pcre_jit_stack * lsr_pcre_get_jit_stack();
    void lsr_pcre_release_jit_stack( void * pValue );
#endif
#endif

/** @lsr_pcre_compile
 * @brief Compiles the pcre object to get it ready for exec.  Must 
 * \link #lsr_pcre_release release \endlink at the end if successful.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @param[in] regex - The regex to use.
 * @param[in] options - Pcre options.
 * @param[in] matchLimit - A limit on the number of matches.  0 to ignore.
 * @param[in] recursionLimit - A limit on the number of recursions.  0 to ignore.
 * @return 0 on success, -1 on failure.
 * 
 * @see lsr_pcre_release
 */
int                 lsr_pcre_compile( lsr_pcre_t *pThis, const char * regex, 
                        int options, int matchLimit, int recursionLimit );

/** @lsr_pcre_exec
 * @brief Executes the regex matching.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @param[in] subject - The subject string to search.
 * @param[in] length - The length of the subject string.
 * @param[in] startoffset - The offset of the subject to start at.
 * @param[in] options - The options to provide to pcre.
 * @param[out] ovector - The output vector for the result.
 * @param[in] ovecsize - The number of elements in the vector (multiple of 3).
 * @return The number of successful matches + 1.
 */
lsr_inline int   lsr_pcre_exec( lsr_pcre_t *pThis, const char *subject, int length, 
                    int startoffset, int options, int *ovector, int ovecsize )
{
#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
    pcre_jit_stack * stack = lsr_pcre_get_jit_stack();
    pcre_assign_jit_stack( pThis->m_extra, NULL, stack );
#endif
#endif
    return pcre_exec( pThis->m_regex, pThis->m_extra, subject, length, startoffset,
                    options, ovector, ovecsize );
}

/** @lsr_pcre_exec_result
 * @brief Executes the regex matching, storing the result in a pcre result object.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @param[in] subject - The subject string to search.
 * @param[in] length - The length of the subject string.
 * @param[in] startoffset - The offset of the subject to start at.
 * @param[in] options - The options to provide to pcre.
 * @param[out] pRes - A pointer to an initialized output result object.
 * @return The number of successful matches + 1.
 */
lsr_inline int  lsr_pcre_exec_result( lsr_pcre_t *pThis, const char *subject, int length, 
                int startoffset, int options, lsr_pcre_result_t *pRes )
{
    lsr_pcre_result_set_matches( pRes, pcre_exec( pThis->m_regex, pThis->m_extra, subject,
                    length, startoffset, options, lsr_pcre_result_get_vector( pRes ), 30 ) );
    return lsr_pcre_result_get_matches( pRes );
}
    
/** @lsr_pcre_release
 * @brief Releases anything that was initialized by the pcre compile/exec calls.
 * MUST BE CALLED IF COMPILE/EXEC ARE CALLED.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object.
 * @return Void.
 */
void                lsr_pcre_release( lsr_pcre_t *pThis );

/** @lsr_pcre_get_substr_count
 * @brief Gets the number of substrings in the pcre.
 * 
 * @param[in] pThis - A pointer to an initialized pcre object
 * @return The substring count.
 */
lsr_inline int   lsr_pcre_get_substr_count( lsr_pcre_t *pThis )
{   return pThis->m_iSubStr;   }

/** @lsr_pcre_sub_release
 * @brief Releases anything that was initialized by the pcre sub compile/exec calls.
 * MUST BE CALLED IF SUB COMPILE/EXEC ARE CALLED.
 * 
 * @param[in] pThis - A pointer to an initialized pcre sub object.
 * @return Void.
 */
void    lsr_pcre_sub_release( lsr_pcre_sub_t *pThis );

/** @lsr_pcre_sub_compile
 * @brief Sets up the pcre sub object to get it ready for substitution.
 * Must \link #lsr_pcre_sub_release release \endlink after
 * \link #lsr_pcre_sub_exec executing. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized sub object.
 * @param[in] rule - The rule to apply to the substrings.
 * @return 0 on success, -1 on failure.
 */
int     lsr_pcre_sub_compile( lsr_pcre_sub_t *pThis, const char *rule );

/** @lsr_pcre_sub_exec
 * @brief Applies the rule from \link #lsr_pcre_sub_compile sub compile \endlink
 * to the result.  The output buffer must be allocated before calling this function.
 * @note Please notice that ovector and ovec_num are \e input parameters.  They should
 * be the output from a call to \link #lsr_pcre_exec pcre exec \endlink that was called
 * prior to calling this function.
 * 
 * @param[in] pThis - A pointer to an initialized sub object.
 * @param[in] input - The original subject string.
 * @param[in] ovector - The result from pcre exec.
 * @param[in] ovec_num - The number of matches as returned by exec.
 * @param[out] output - The allocated output buffer for the result of the substitution.
 * @param[out] length - The length of the output.
 * @return 0 on success, -1 on failure.
 * 
 * @see lsr_pcre_exec
 */
int     lsr_pcre_sub_exec( lsr_pcre_sub_t *pThis, const char * input, 
                    const int *ovector, int ovec_num, char * output, int *length );

#ifdef __cplusplus
}
#endif
#endif //LSR_PCREG_H
