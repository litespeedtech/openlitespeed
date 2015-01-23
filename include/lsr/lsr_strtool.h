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
#ifndef LSR_STRTOOL_H
#define LSR_STRTOOL_H


#include <stddef.h>
#include <ctype.h>
#include <sys/types.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_types.h>


/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @skipLeadingSpace
 * @brief Skips past leading spaces and tabs in a string.
 * 
 * @param[in,out] p - A pointer to an input string pointer,
 *   which upon return contains a pointer to the first non-space character
 *   from the beginning of the input string.
 * @return Void.
 */
lsr_inline void skipLeadingSpace( const char **p )
{
    while ( isspace(**p) )
        ++(*p);
}

/**
 * @skipTrailingSpace
 * @brief Skips trailing spaces and tabs in a string.
 * 
 * @param[in,out] p - A pointer to an input string pointer,
 *   which upon return contains a pointer just past the last non-space character
 *   preceding the input string pointer.
 * @return Void.
 */
lsr_inline void skipTrailingSpace( const char **p )
{
    while ( isspace( (*p)[-1] ) )
        --(*p);
}

/**
 * @hexdigit
 * @brief Gets a binary value for a hexadecimal digit character.
 * 
 * @param[in] ch - A hex digit character.
 * @return The binary value of the hex digit character.
 */
lsr_inline char hexdigit( char ch )
{
    return (((ch) <= '9') ? (ch) - '0' : ((ch) & 7) + 9);
}


/**
 * @typedef lsr_parse_t
 */
typedef struct lsr_parse_s
{
    const char * m_pBegin;
    const char * m_pEnd;
    const char * m_delim;
    const char * m_pStrEnd;
} lsr_parse_t;

/**
 * @lsr_parse
 * @brief Initializes a string parsing object.
 * @details This object manages the parsing of strings specified by the user.
 * 
 * @param[in] pThis - A pointer to an allocated string parsing object.
 * @param[in] pBegin - A pointer to the beginning of the string to parse.
 * @param[in] pEnd - A pointer to the end of the string.
 * @param[in] delim - The field delimiters (cannot be an empty string).
 * @return Void.
 *
 * @see lsr_parse_new, lsr_parse_d
 */
lsr_inline void lsr_parse(
  lsr_parse_t *pThis, const char *pBegin, const char *pEnd, const char *delim )
{
    pThis->m_pBegin = pBegin;
    pThis->m_pEnd = pEnd;
    pThis->m_delim = delim;
    pThis->m_pStrEnd = NULL;
}

/**
 * @lsr_parse_new
 * @brief Creates a new string parsing object.
 * @details The routine allocates and initializes an object
 *   which manages the parsing of strings specified by the user.
 * 
 * @param[in] pBegin - A pointer to the beginning of the string to parse.
 * @param[in] pEnd - A pointer to the end of the string.
 * @param[in] delim - The field delimiters (cannot be an empty string).
 * @return A pointer to an initialized string parsing object.
 *
 * @see lsr_parse, lsr_parse_delete
 */
lsr_inline lsr_parse_t * lsr_parse_new(
  const char *pBegin, const char *pEnd, const char *delim )
{
    lsr_parse_t * pThis;
    if ( ( pThis = (lsr_parse_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_parse( pThis, pBegin, pEnd, delim );
    }
    return pThis;
}

/**
 * @lsr_parse_d
 * @brief Destroys the contents of a string parsing object.
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return Void.
 *
 * @see lsr_parse
 */
lsr_inline void lsr_parse_d( lsr_parse_t *pThis )
{}

/**
 * @lsr_parse_delete
 * @brief Destroys then deletes a string parsing object.
 * @details The object should have been created with a previous
 *   successful call to lsr_parse_new.
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return Void.
 *
 * @see lsr_parse_new
 */
lsr_inline void lsr_parse_delete(
  lsr_parse_t *pThis )
{   lsr_parse_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_parse_isend
 * @brief Specifies whether or not the string has been fully parsed (is the end).
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return 1 if the end, else 0.
 */
lsr_inline int lsr_parse_isend( const lsr_parse_t *pThis )
{   return pThis->m_pEnd <= pThis->m_pBegin;   }
    
/**
 * @lsr_parse_parse
 * @brief Parses or continues to parse the string
 *   managed by the string parsing object.
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return A pointer to the next token from the string.
 */
const char * lsr_parse_parse( lsr_parse_t *pThis );

/**
 * @lsr_parse_trim_parse
 * @brief Parses or continues to parse the string
 *   managed by the string parsing object,
 *   trimming white space from the beginning and end of each token.
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return A pointer to the next token from the string.
 */
lsr_inline const char * lsr_parse_trim_parse( lsr_parse_t *pThis )
{
    skipLeadingSpace( &pThis->m_pBegin );
    const char * p = lsr_parse_parse( pThis );
    if ( p && ( p != pThis->m_pStrEnd ) )
        skipTrailingSpace( &pThis->m_pStrEnd );
    return p;
}

/**
 * @lsr_parse_getstrend
 * @brief Gets the end of the current string token.
 * 
 * @param[in] pThis - A pointer to an initialized string parsing object.
 * @return A pointer to the end of the current token.
 */
lsr_inline const char * lsr_parse_getstrend( const lsr_parse_t *pThis )
{   return pThis->m_pStrEnd;   }

extern const char lsr_s_hex[17];

/**
 * @lsr_strupper
 * @brief Converts a character string to upper case.
 * 
 * @param[in] pSrc - A pointer to the source string.
 * @param[in] pDest - A pointer to the destination buffer.
 * @return The destination pointer, else NULL on error.
 */
char * lsr_strupper( const char *pSrc, char *pDest );

/**
 * @lsr_strnupper
 * @brief Converts a maximum defined length character string to upper case.
 * @details The destination is null-terminated if and only if
 *   there is sufficient space.
 * 
 * @param[in] pSrc - A pointer to the source string.
 * @param[in] pDest - A pointer to the destination buffer.
 * @param[in,out] pCnt - On input, the maximum number of characters to process;
 *   on return, the actual number of characters.
 * @return The destination pointer, else NULL on error.
 */
char * lsr_strnupper( const char *pSrc, char *pDest, int *pCnt );

/**
 * @lsr_strlower
 * @brief Converts a character string to lower case.
 * 
 * @param[in] pSrc - A pointer to the source string.
 * @param[in] pDest - A pointer to the destination buffer.
 * @return The destination pointer, else NULL on error.
 */
char * lsr_strlower( const char *pSrc, char *pDest );

/**
 * @lsr_strnlower
 * @brief Converts a maximum defined length character string to lower case.
 * @details The destination is null-terminated if and only if
 *   there is sufficient space.
 * 
 * @param[in] pSrc - A pointer to the source string.
 * @param[in] pDest - A pointer to the destination buffer.
 * @param[in,out] pCnt - On input, the maximum number of characters to process;
 *   on return, the actual number of characters.
 * @return The destination pointer, else NULL on error.
 */
char * lsr_strnlower( const char *pSrc, char *pDest, int *pCnt );

/**
 * @lsr_strtrim
 * @brief Trims white space from the beginning and end of a string.
 * 
 * @param[in] p - A pointer to the source string.
 * @return A pointer to the \e trimmed string.
 * @note Trimming is done in place.
 */
char * lsr_strtrim( char *p );

/**
 * @lsr_strtrim2
 * @brief Trims white space from the beginning and end of a string.
 * 
 * @param[in,out] pBegin - On input, a pointer to the source string pointer;
 *   on return, the \e trimmed string pointer is returned.
 * @param[in,out] pEnd - On input, a pointer to the source string end pointer;
 *   on return, the \e trimmed string end pointer is returned.
 * @return 0.
 */
int    lsr_strtrim2( const char **pBegin, const char **pEnd );

/**
 * @lsr_hexencode
 * @brief Converts a character buffer to a corresponding string
 *   of ascii hexadecimal digits.
 * @details It is permissible for the destination pointer to be
 *   the same as the source pointer.
 * 
 * @param[in] pSrc - A pointer to the source buffer.
 * @param[in] len - The number of characters to convert from \e pSrc.
 * @param[in] pDest - A pointer to the destination buffer.
 * @return The length of the new converted string.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient space at the destination.
 *   This should be equal to two times the size of the input,
 *   plus one for the null-termination.
 */
int    lsr_hexencode( const char *pSrc, int len, char *pDest );

/**
 * @lsr_hexdecode
 * @brief Converts a character buffer of ascii hexadecimal digits
 *   to its corresponding characters.
 * @details It is permissible for the destination pointer to be
 *   the same as the source pointer.
 * 
 * @param[in] pSrc - A pointer to the source buffer.
 * @param[in] len - The number of characters to convert from \e pSrc.
 * @param[in] pDest - A pointer to the destination buffer.
 * @return The length of the new converted buffer.
 */
int    lsr_hexdecode( const char *pSrc, int len, char *pDest );

/**
 * @lsr_strmatch
 * @brief Determines whether or not a string matches a specified pattern.
 * @details Pattern matching includes the special wildcard characters:\n
 *   \arg '?' Question Mark - Matches exactly one of any character
 *   \arg '*' Asterisk - Matches zero or more of any characters
 * 
 * @param[in] pSrc - A pointer to the source string.
 * @param[in] pEnd - A pointer to the end of the string;
 *   else NULL to specify the null-termination of the source.
 * @param[in] begin - A string list iterator for tokens in the pattern to match.
 * @param[in] end - The string list end iterator.
 * @param[in] case_sens - A case sensitivity flag.
 * 1 for case sensitive, 0 for case insensitive.
 *
 * @return Zero for a match, else non-zero.
 *
 * @see lsr_parsematchpattern
 */
int    lsr_strmatch( const char *pSrc, const char *pEnd,
         lsr_str_t *const*begin, lsr_str_t *const*end, int case_sens );

/**
 * @lsr_parsematchpattern
 * @brief Separates a pattern string into its component tokens.
 * @details Pattern matching includes the special wildcard characters:\n
 *   \arg '?' Question Mark - Matches exactly one of any character
 *   \arg '*' Asterisk - Matches zero or more of any characters
 * 
 * @param[in] pPattern - A pointer to the pattern string.
 * @return A pointer to a new string list object
 *   containing the pattern matching tokens, else NULL on error.
 * @warning It is the responsibility of the user to ensure the
 *   returned string list object is deleted upon completion.
 *
 * @see lsr_strmatch
 */
lsr_strlist_t * lsr_parsematchpattern( const char *pPattern );

/**
 * @lsr_strnextarg
 * @brief Gets the next token (argument) from a string.
 * 
 * @param[in,out] pStr - A pointer to a string pointer.
 *   On input, it specifies the string to process;
 *   on return, its value may be different if the token was quoted.
 * @param[in] pDelim - A pointer specifying the field delimiters;
 *   if NULL, the default is space, tab, carriage return, and newline.
 * @return A pointer past the end of the current arg,
 *   which is typically the delimiter but possibly a quote character.
 */
const char * lsr_strnextarg( const char **pStr, const char *pDelim );

/**
 * @lsr_getline
 * @brief Gets the next newline in a character buffer.
 * 
 * @param[in] pBegin - A pointer to the beginning of the buffer.
 * @param[in] pEnd - A pointer to the end of the buffer.
 * @return A pointer to the next newline character if it exists,
 *   else \e pEnd.
 */
const char * lsr_getline( const char *pBegin, const char *pEnd );

/** @lsr_get_conf_line
 * @brief Gets a line from a block of configuration.
 * @details A line will have any surrounding delimiters removed from the beginning and the end.
 * 
 * @param[in,out] pParseBegin - A pointer to the \e address of the beginning of the line
 * (may include white space) and will point to the beginning of the next line when the function
 * returns. When there are no more lines, the dereferenced pointer will be greater than or equal to pParseEnd.
 * @param[in] pParseEnd - A pointer to the end of the parse buffer.
 * @param[out] pLineEnd - This will point to the address right after the end of the line.
 * @return The pointer to the beginning of the line, or NULL if there are no more lines.
 */
const char *lsr_get_conf_line( const char **pParseBegin, const char *pParseEnd, const char **pLineEnd );

/**
 * @lsr_parsenextarg
 * @brief Parses a string returning the next argument,
 *   and advancing the buffer pointer.
 * @details The routine correctly handles quotes and white space.
 * 
 * @param[in,out] pRuleStr - On input, a pointer to the source input pointer;
 *   on return, a pointer to the token following the argument just returned.
 * @param[in] pEnd - A pointer to the end of the input buffer.
 * @param[out] pArgBegin - A pointer to the beginning of the returned argument buffer.
 * @param[out] pArgEnd - A pointer to the end of the returned argument buffer.
 * @param[out] pError - Error message on error.
 * @return 0 on success, else -1 on error.
 */
int    lsr_parsenextarg( const char **pRuleStr, const char *pEnd,
         const char **pArgBegin, const char **pArgEnd, const char **pError );

/**
 * @lsr_convertmatchtoreg
 * @brief Converts a string from a regular expression format.
 * 
 * @param[in] pStr - A pointer to the beginning of the string.
 * @param[in] pBufEnd - A pointer to the end of the buffer.
 * @return A pointer to the string converted in place.
 */
char * lsr_convertmatchtoreg( char *pStr, char *pBufEnd );

/**
 * @lsr_findclosebracket
 * @brief Finds the matching closing character from inside a \e bracket.
 * @details The routine correctly handles nesting.
 * 
 * @param[in] pBegin - A pointer to the beginning of the buffer.
 * @param[in] pEnd - A pointer to the end of the buffer.
 * @param[in] chOpen - The character which opens (starts) the \e bracket.
 * @param[in] chClose - The character which closes (ends) the \e bracket.
 * @return A pointer to the matching closing character if it exists,
 *   else \e pEnd.
 */
const char * lsr_findclosebracket( const char *pBegin, const char *pEnd,
         char chOpen, char chClose );

/**
 * @lsr_findcharinbracket
 * @brief Finds a specified character inside a \e bracket.
 * @details The character must be found in the current \e bracket level
 *   before the bracket close.
 *   The routine correctly handles nesting.
 * 
 * @param[in] pBegin - A pointer to the beginning of the buffer.
 * @param[in] pEnd - A pointer to the end of the buffer.
 * @param[in] searched - The character to match.
 * @param[in] chOpen - The character which opens (starts) the \e bracket.
 * @param[in] chClose - The character which closes (ends) the \e bracket.
 * @return A pointer to the matching searched character if it exists
 *   in the proper level, else NULL.
 */
const char * lsr_findcharinbracket( const char *pBegin, const char *pEnd,
         char searched, char chOpen, char chClose );

/**
 * @lsr_str_off_t
 * @brief Converts an offset to an ascii string.
 * 
 * @param[out] pBuf - A pointer to the output buffer.
 * @param[in] len - The length (size) in bytes of the output buffer.
 * @param[in] val - The offset value.
 * @return The length in bytes of the converted returned buffer.
 */
int    lsr_str_off_t( char *pBuf, int len, off_t val );

/**
 * @lsr_unescapequote
 * @brief Removes the backslash escapes for a specified character in a buffer.
 * 
 * @param[in] pBegin - A pointer to the beginning of the buffer.
 * @param[in] pEnd - A pointer to the end of the buffer.
 * @param[in] ch - The character escaped.
 * @return The number of characters \e unescaped.
 */
int    lsr_unescapequote( char *pBegin, char *pEnd, int ch );

/**
 * @lsr_lookupsubstring
 * @brief Finds a string key/value pair within a specified input buffer.
 * @details The input buffer is expected to contain a series of
 *   key/value pairs separated by a separator character.
 *   The routine eliminates spaces unless they are within a quoted token.
 * 
 * @param[in] pInput - A pointer to the beginning of the input buffer.
 * @param[in] pEnd - A pointer to the end of the buffer.
 * @param[in] key - A pointer to the beginning of the key to match.
 * @param[in] keyLen - The length of the key.
 * @param[out] retLen - The length in bytes of the returned value.
 * @param[in] sep - The separator between key/value pairs.
 * @param[in] comp - The comparator which associates key to value.
 * @return A pointer to the value for the matched key upon success,
 *   else NULL if not found.
 */
const char * lsr_lookupsubstring( const char *pInput, const char *pEnd,
         const char *key, int keyLen, int *retLen, char sep, char comp);

/**
 * @lsr_mempbrk
 * @brief Finds a character within a specified input buffer.
 * 
 * @param[in] pInput - A pointer to the beginning of the input buffer.
 * @param[in] iSize - The length of the input buffer.
 * @param[in] accept - A pointer to a list of characters to match.
 * @param[in] acceptLen - The length of the \e accept buffer.
 * @return A pointer to the first occurance of one of the accept characters
 *   in the input buffer,
 *   else NULL if not found.
 */
const char * lsr_mempbrk(
         const char *pInput, int iSize, const char *accept, int acceptLen);

/**
 * @lsr_memspn
 * @brief Scans memory for a given set of bytes.
 * 
 * @param[in] pInput - A pointer to the beginning of the input buffer.
 * @param[in] iSize - The length of the input buffer.
 * @param[in] accept - A pointer to a list of bytes to match.
 * @param[in] acceptLen - The length of the \e accept buffer.
 * @return The number of bytes from the start of the input buffer
 *   which are within the match list.
 *
 * @see lsr_memcspn
 */
size_t lsr_memspn(
         const char *pInput, int iSize, const char *accept, int acceptLen);

/**
 * @lsr_memspn
 * @brief Scans memory for bytes \e not within a given set.
 * 
 * @param[in] pInput - A pointer to the beginning of the input buffer.
 * @param[in] iSize - The length of the input buffer.
 * @param[in] accept - A pointer to a list of bytes to match.
 * @param[in] acceptLen - The length of the \e accept buffer.
 * @return The number of bytes from the start of the input buffer,
 *   none of which are within the match list.
 *
 * @see lsr_memspn
 */
size_t lsr_memcspn(
         const char *pInput, int iSize, const char *accept, int acceptLen);

void   lsr_getmd5( const char *src, int len, unsigned char *dstBin );


#ifdef __cplusplus
}
#endif

#endif  /* LSR_STRTOOL_H */

