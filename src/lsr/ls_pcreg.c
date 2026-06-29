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

#include <lsdef.h>
#include <lsr/ls_hash.h>
#include <lsr/ls_lock.h>
#include <lsr/ls_pcreg.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_str.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

#ifdef USE_THRSAFE_POOL
static ls_atom_spinlock_t s_store_lock = 0;
#endif
static ls_hash_t *s_pcre_store = NULL;


ls_pcre_t *ls_pcre_new()
{
    ls_pcre_t *pThis = (ls_pcre_t *)ls_palloc(sizeof(ls_pcre_t));
    return ls_pcre(pThis);
}


ls_pcre_t *ls_pcre(ls_pcre_t *pThis)
{
    if (pThis == NULL)
        return NULL;
    pThis->regex = NULL;
    pThis->context = NULL;
    pThis->substr = 0;
    pThis->pattern = NULL;
    return pThis;
}


void ls_pcre_d(ls_pcre_t *pThis)
{
    ls_pcre_release(pThis);
    pThis->substr = 0;
    if (pThis->pattern != NULL)
        ls_pfree(pThis->pattern);
}


void ls_pcre_delete(ls_pcre_t *pThis)
{
    ls_pcre_d(pThis);
    ls_pfree(pThis);
}


ls_pcre_t *ls_pcre_load(const char *pRegex, unsigned long iFlags)
{
    ls_hash_iter pNode;
    ls_hash_t *pInnerHash = NULL;
    ls_pcre_t *pcre = NULL;

#ifdef USE_THRSAFE_POOL
    ls_atomic_spin_lock(&s_store_lock);
#endif
    /** Check for:
     * 1. Existance of hash table
     * 2. Existance of inner hash table with given flags.
     * 3. Able to get inner table from node.
     * 4. Existance of pcre structure with given regex.
     * NOTICE: pNode is used for multiple nodes.
     */
    if ((s_pcre_store != NULL)
        && ((pNode = ls_hash_find(s_pcre_store, (void *)iFlags)) != NULL)
        && ((pInnerHash = (ls_hash_t *)ls_hash_getdata(pNode)) != NULL)
        && ((pNode = ls_hash_find(pInnerHash, pRegex)) != NULL))
    {
        pcre = (ls_pcre_t *)ls_hash_getdata(pNode);
        ls_hash_erase(pInnerHash, pNode);
    }
#ifdef USE_THRSAFE_POOL
    ls_atomic_spin_unlock(&s_store_lock);
#endif
    return pcre;
}


int ls_pcre_store(ls_pcre_t *pThis, unsigned long iFlags)
{
    ls_hash_iter pNode;
    ls_hash_t *pInnerHash = NULL;

#ifdef USE_THRSAFE_POOL
    ls_atomic_spin_lock(&s_store_lock);
#endif
    if (s_pcre_store == NULL)
        s_pcre_store = ls_hash_new(50, NULL, NULL, NULL);
    else if ((pNode = ls_hash_find(s_pcre_store,
                                   (void *)iFlags)) != NULL)
        pInnerHash = (ls_hash_t *)ls_hash_getdata(pNode);

    if (pInnerHash == NULL)
    {
        pInnerHash = ls_hash_new(10, ls_hash_hfstring,
                                 ls_hash_cmpstring, NULL);
        ls_hash_insert(s_pcre_store, (void *)iFlags, pInnerHash);
    }
    int ret = (ls_hash_insert(pInnerHash, pThis->pattern, pThis) != NULL);
#ifdef USE_THRSAFE_POOL
    ls_atomic_spin_unlock(&s_store_lock);
#endif
    return ret;
}


ls_pcreres_t *ls_pcre_result(ls_pcreres_t *pThis)
{
    if (pThis == NULL)
        return NULL;
    pThis->pbuf = NULL;
    pThis->matches = 0;
    return pThis;
}


ls_pcresub_t *ls_pcre_sub(ls_pcresub_t *pThis)
{
    if (pThis == NULL)
        return NULL;
    pThis->parsed = NULL;
    pThis->plist = NULL;
    pThis->plistend = NULL;
    return pThis;
}


ls_pcreres_t *ls_pcreres_new()
{
    ls_pcreres_t *pThis = (ls_pcreres_t *)ls_palloc(sizeof(ls_pcreres_t));
    return ls_pcre_result(pThis);
}


void ls_pcreres_d(ls_pcreres_t *pThis)
{
    memset(pThis, 0, sizeof(ls_pcreres_t));
    //Since I did not allocate the buffer, I will not deallocate the buffer.
    pThis->pbuf = NULL;
}


void ls_pcreres_delete(ls_pcreres_t *pThis)
{
    ls_pcreres_d(pThis);
    ls_pfree(pThis);
}


ls_pcresub_t *ls_pcresub_new()
{
    ls_pcresub_t *pThis = (ls_pcresub_t *)ls_palloc(sizeof(ls_pcresub_t));
    return ls_pcre_sub(pThis);
}


ls_pcresub_t *ls_pcresub_copy(ls_pcresub_t *dest, const ls_pcresub_t *src)
{
    assert(dest && src);
    char *p = ls_pdupstr2(src->parsed, (char *)src->plistend - src->parsed);
    if (p == NULL)
        return NULL;
    dest->parsed = p;
    dest->plist = (ls_pcresubent_t *)(p + ((char *)src->plist - src->parsed));
    dest->plistend = dest->plist + (src->plistend - src->plist);

    return dest;
}


void ls_pcresub_d(ls_pcresub_t *pThis)
{
    ls_pfree(pThis->parsed);
    pThis->plist = NULL;
    pThis->plistend = NULL;
}


void ls_pcresub_delete(ls_pcresub_t *pThis)
{
    ls_pcresub_d(pThis);
    ls_pfree(pThis);
}


int ls_pcreres_getsubstr(const ls_pcreres_t *pThis, int i, char **pValue)
{
    assert(pValue);
    if (i < pThis->matches)
    {
        const int *pParam = &pThis->ovector[ i << 1 ];
        *pValue = (char *)pThis->pbuf + *pParam;
        return *(pParam + 1) - *pParam;
    }
    return 0;
}


#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
static int s_jit_key_inited = 0;
static pthread_key_t s_jit_stack_key;


void ls_pcre_init_jit_stack()
{
    s_jit_key_inited = 1;
    pthread_key_create(&s_jit_stack_key, ls_pcre_release_jit_stack);
}


void ls_pcre_release_jit_stack(void *pValue)
{
    pcre2_jit_stack_free((pcre2_jit_stack *) pValue);
}


pcre2_jit_stack *ls_pcre_get_jit_stack()
{
    pcre2_jit_stack *jit_stack;

    if (s_jit_key_inited == 0)
        ls_pcre_init_jit_stack();
    jit_stack = (pcre2_jit_stack *)pthread_getspecific(s_jit_stack_key);
    if (jit_stack == NULL)
    {
        jit_stack = pcre2_jit_stack_create(32 * 1024, 512 * 1024, NULL);
        pthread_setspecific(s_jit_stack_key, jit_stack);
    }
    return jit_stack;
}
#endif
#endif


/* Per-thread match data reused across exec() calls so the hot path stays
 * allocation-free (PCRE2 needs a heap pcre2_match_data, unlike PCRE1's caller
 * supplied stack ovector).  Grown on demand. */
static pthread_once_t s_md_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t s_md_key;
static int s_md_key_create_failed = 0;

static void ls_pcre_release_match_data(void *pValue)
{
    if (pValue != NULL)
        pcre2_match_data_free((pcre2_match_data *) pValue);
}


static void ls_pcre_init_match_data_key()
{
    if (pthread_key_create(&s_md_key, ls_pcre_release_match_data) != 0)
        s_md_key_create_failed = 1;
}


static pcre2_match_data *ls_pcre_get_match_data(uint32_t pairs)
{
    pcre2_match_data *md;
    if (pairs < 1)
        pairs = 1;
    if (pthread_once(&s_md_key_once, ls_pcre_init_match_data_key) != 0
        || s_md_key_create_failed)
        return NULL;
    md = (pcre2_match_data *)pthread_getspecific(s_md_key);
    if (md != NULL && pcre2_get_ovector_count(md) >= pairs)
        return md;
    if (md != NULL)
        pcre2_match_data_free(md);
    md = pcre2_match_data_create(pairs, NULL);
    pthread_setspecific(s_md_key, md);
    return md;
}


/* PCRE2 reuses low option bits between compile time and match time, so a
 * compile flag accidentally passed to exec() could be misread as a match flag
 * or rejected with PCRE2_ERROR_BADOPTION.  Restrict exec()/dfaexec() options to
 * each function's match-time set. */
#define LS_PCRE2_MATCH_OPTS  (PCRE2_ANCHORED | PCRE2_ENDANCHORED | \
    PCRE2_NOTBOL | PCRE2_NOTEOL | PCRE2_NOTEMPTY | PCRE2_NOTEMPTY_ATSTART | \
    PCRE2_NO_UTF_CHECK | PCRE2_NO_JIT | PCRE2_PARTIAL_HARD | \
    PCRE2_PARTIAL_SOFT | PCRE2_COPY_MATCHED_SUBJECT | \
    PCRE2_DISABLE_RECURSELOOP_CHECK)
#define LS_PCRE2_DFA_OPTS    (PCRE2_ANCHORED | PCRE2_ENDANCHORED | \
    PCRE2_NOTBOL | PCRE2_NOTEOL | PCRE2_NOTEMPTY | PCRE2_NOTEMPTY_ATSTART | \
    PCRE2_NO_UTF_CHECK | PCRE2_PARTIAL_HARD | PCRE2_PARTIAL_SOFT | \
    PCRE2_COPY_MATCHED_SUBJECT | PCRE2_DFA_RESTART | PCRE2_DFA_SHORTEST)


/* Copy up to 'avail' PCRE2 result/capture pairs into the caller's int vector
 * (unset -> -1) and return the match count clamped to that capacity.  Because
 * the per-thread match data is shared and grown across calls, pcre2 may return
 * more pairs than the caller's vector holds; like pcre1/pcre2 with a vector of
 * exactly 'avail' pairs, that case (and pcre2's "0 == vector too small") is
 * reported as 0 so callers never read pairs that were not copied.  'avail' is
 * already capped to ovecsize/2 (DFA) or ovecsize/3 (captures), but the
 * ovecsize >= 2 guard makes the bound explicit: never touch a vector that
 * cannot hold a single offset pair.  'avail' = pairs the caller's vector holds:
 *     - captures (pcre2_match): ovecsize/3  (pcre1 kept a third as workspace)
 *     - DFA results (pcre2_dfa_match): ovecsize/2  (DFA workspace is separate,
 *       so the capture-workspace third does not apply). */
static int ls_pcre_store_result(pcre2_match_data *md, int rc,
                                int *ovector, int ovecsize, uint32_t avail)
{
    if (ovector != NULL && ovecsize >= 2 && avail >= 1)
    {
        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
        uint32_t pairs = (rc == 0 || (uint32_t)rc > avail) ? avail : (uint32_t)rc;
        uint32_t i;
        for (i = 0; i < pairs; ++i)
        {
            ovector[2 * i]     = (ov[2 * i] == PCRE2_UNSET) ? -1 : (int)ov[2 * i];
            ovector[2 * i + 1] = (ov[2 * i + 1] == PCRE2_UNSET)
                                 ? -1 : (int)ov[2 * i + 1];
        }
    }
    if (rc == 0 || (uint32_t)rc > avail)
        return 0;
    return rc;
}


int ls_pcre_exec(ls_pcre_t *pThis, const char *subject, int length,
                 int startoffset, int options, int *ovector, int ovecsize)
{
    uint32_t avail = (ovecsize >= 3) ? (uint32_t)(ovecsize / 3) : 0;
    pcre2_match_data *md = ls_pcre_get_match_data(avail);
    int rc;
    if (md == NULL)
        return PCRE2_ERROR_NOMATCH;
#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
    if (pThis->context != NULL)
        pcre2_jit_stack_assign(pThis->context, NULL, ls_pcre_get_jit_stack());
#endif
#endif
    rc = pcre2_match(pThis->regex, (PCRE2_SPTR)subject, length, startoffset,
                     (uint32_t)options & LS_PCRE2_MATCH_OPTS, md, pThis->context);
    if (rc < 0)
        return rc;
    return ls_pcre_store_result(md, rc, ovector, ovecsize, avail);
}


int ls_pcre_execresult(ls_pcre_t *pThis, const char *subject, int length,
                       int startoffset, int options, ls_pcreres_t *pRes)
{
    ls_pcreres_setmatches(pRes, ls_pcre_exec(pThis, subject, length,
                          startoffset, options, ls_pcreres_getvector(pRes), 30));
    return ls_pcres_getmatches(pRes);
}


int ls_pcre_dfaexec(ls_pcre_t *pThis, const char *subject, int length,
                    int startoffset, int options, int *ovector, int ovecsize)
{
    int aWorkspace[LSR_PCRE_WORKSPACE_LEN];
    /* DFA returns offset pairs for multiple matches at the same start, and its
     * working store is aWorkspace[] (a separate argument), so the result vector
     * holds ovecsize/2 pairs -- not the ovecsize/3 used for captures. */
    uint32_t avail = (ovecsize >= 2) ? (uint32_t)(ovecsize / 2) : 0;
    pcre2_match_data *md = ls_pcre_get_match_data(avail);
    int rc;
    if (md == NULL)
        return PCRE2_ERROR_NOMATCH;
    rc = pcre2_dfa_match(pThis->regex, (PCRE2_SPTR)subject, length, startoffset,
                         (uint32_t)options & LS_PCRE2_DFA_OPTS, md,
                         pThis->context, aWorkspace, LSR_PCRE_WORKSPACE_LEN);
    if (rc < 0)
        return rc;
    return ls_pcre_store_result(md, rc, ovector, ovecsize, avail);
}


int ls_pcre_dfaexecresult(ls_pcre_t *pThis, const char *subject, int length,
                          int startoffset, int options, ls_pcreres_t *pRes)
{
    ls_pcreres_setmatches(pRes, ls_pcre_dfaexec(pThis, subject, length,
                          startoffset, options, ls_pcreres_getvector(pRes), 30));
    return ls_pcres_getmatches(pRes);
}


int ls_pcre_parseoptions(const char *pOptions, size_t iOptLen,
                         unsigned long *pFlags)
{
    int iOptimizeFlags = 0;
    const char *pEnd = pOptions + iOptLen;
    const char *p = pOptions;

    if (pFlags == NULL)
        return LS_FAIL;
    *pFlags = 0;

    while (p < pEnd)
    {
        switch (*p)
        {
        case 'a':
            *pFlags |= LSRE_ANCHORED;
            break;
        case 'd':
            iOptimizeFlags |= LSR_PCRE_DFA_MODE;
            break;
        case 'i':
            *pFlags |= LSRE_CASELESS;
            break;
        case 'j': //Jit mode automatically used when available.
            break;
        case 'm':
            *pFlags |= LSRE_MULTILINE;
            break;
        case 'o':
            iOptimizeFlags |= LSR_PCRE_CACHE_COMPILED;
            break;
        case 's':
            *pFlags |= LSRE_DOTALL;
            break;
        case 'u':
            *pFlags |= LSRE_UTF8;
            break;
        case 'U':
            *pFlags |= (LSRE_UTF8 | LSRE_NO_UTF8_CHECK) ;
            break;
        case 'x':
            *pFlags |= LSRE_EXTENDED;
            break;
        case 'D':
            *pFlags |= LSRE_DUPNAMES;
            break;
        case 'J':
            *pFlags |= LSRE_JAVASCRIPT_COMPAT;
            break;
        default:
            return LS_FAIL;
        }
        ++p;
    }
    return iOptimizeFlags;
}


int ls_pcre_compile(ls_pcre_t *pThis, const char *regex, int options,
                    int matchLimit, int recursionLimit)
{
    int        errcode;
    PCRE2_SIZE erroffset;
    uint32_t   captureCount = 0;
    int        wantContext = (matchLimit > 0 || recursionLimit > 0);
    if (pThis->regex != NULL)
        ls_pcre_release(pThis);
    pThis->regex = pcre2_compile((PCRE2_SPTR)regex, PCRE2_ZERO_TERMINATED,
                                 (uint32_t)options,
                                 &errcode, &erroffset, NULL);
    if (pThis->regex == NULL)
        return LS_FAIL;
    pThis->pattern = ls_pdupstr(regex);
#if defined( _USE_PCRE_JIT_)&&!defined(__sparc__) && !defined(__sparc64__)
    pcre2_jit_compile(pThis->regex, PCRE2_JIT_COMPLETE);
    wantContext = 1;
#endif
    if (wantContext)
    {
        pThis->context = pcre2_match_context_create(NULL);
        if (pThis->context != NULL)
        {
            if (matchLimit > 0)
                pcre2_set_match_limit(pThis->context, matchLimit);
            if (recursionLimit > 0)
                pcre2_set_depth_limit(pThis->context, recursionLimit);
        }
    }
    pcre2_pattern_info(pThis->regex, PCRE2_INFO_CAPTURECOUNT, &captureCount);
    pThis->substr = (int)captureCount + 1;
    return LS_OK;
}


void ls_pcre_release(ls_pcre_t *pThis)
{
    if (pThis->regex != NULL)
    {
        if (pThis->context != NULL)
        {
            pcre2_match_context_free(pThis->context);
            pThis->context = NULL;
        }
        pcre2_code_free(pThis->regex);
        pThis->regex = NULL;
    }
}


int ls_pcre_getnamedsubcnt(ls_pcre_t *pThis)
{
    uint32_t iCount;
    if (pcre2_pattern_info(pThis->regex, PCRE2_INFO_NAMECOUNT, &iCount) != 0)
        return LS_FAIL;
    return (int)iCount;
}


static int ls_pcre_map_name(unsigned char *pEntry, char **pName)
{
    assert(pName);
    unsigned char iMostSig = pEntry[0], //Most significant byte
                  iLeastSig = pEntry[1]; //Least significant byte
    *pName = (char *)&pEntry[2];
    return ((iMostSig << 8) | iLeastSig);   //Combine bytes for number.
}


int ls_pcre_getnamedsubs(const ls_pcre_t *pThis, const ls_pcreres_t *pRes,
                         ls_strpair_t *pSubPats, int iCount)
{
    int i, iSubLen;
    uint32_t iEntryLen;
    PCRE2_SPTR pNames;
    char *pName, *pSubStr = NULL;

    if (pcre2_pattern_info(
            pThis->regex, PCRE2_INFO_NAMEENTRYSIZE, &iEntryLen) != 0)
        return LS_FAIL;
    if (pcre2_pattern_info(
            pThis->regex, PCRE2_INFO_NAMETABLE, &pNames) != 0)
        return LS_FAIL;

    for (i = 0; i < iCount; ++i)
    {
        unsigned char *pCurEntry = (unsigned char *)pNames + (i * iEntryLen);
        iSubLen = ls_pcreres_getsubstr(pRes,
                                       ls_pcre_map_name(pCurEntry, &pName),
                                       &pSubStr);
        ls_str_set(&pSubPats[i].key, pName, strlen(pName));
        ls_str_set(&pSubPats[i].val, pSubStr, iSubLen);
    }

    return i;
}


void ls_pcresub_release(ls_pcresub_t *pThis)
{
    if (pThis->parsed != NULL)
        ls_pfree(pThis->parsed);
    pThis->parsed = NULL;
    pThis->plist = NULL;
    pThis->plistend = NULL;
}


int ls_pcresub_compile(ls_pcresub_t *pThis, const char *rule)
{
    if (rule == NULL)
        return LS_FAIL;
    const char     *p = rule;
    char   c;
    int             entries = 0;
    while ((c = *p++) != '\0')
    {
        if (c == '&')
            ++entries;
        else if (c == '$' && isdigit(*p))
        {
            ++p;
            ++entries;
        }
        else if (c == '\\' && (*p == '$' || *p == '&'))
            ++p;
    }
    ++entries;
    int bufSize = strlen(rule) + 1;
    bufSize = ((bufSize + 7) >> 3) << 3;
    if ((pThis->parsed = ls_prealloc(
                             pThis->parsed, bufSize + entries * sizeof(ls_pcresubent_t))) == NULL)
        return LS_FAIL;
    pThis->plist = (ls_pcresubent_t *)(pThis->parsed + bufSize);
    memset(pThis->plist, 0xff, entries * sizeof(ls_pcresubent_t));

    char *pDest = pThis->parsed;
    p = rule;
    ls_pcresubent_t *pEntry = pThis->plist;
    pEntry->strbegin = 0;
    pEntry->strlen = 0;
    while ((c = *p++) != '\0')
    {
        if (c == '&')
            pEntry->param = 0;
        else if (c == '$' && isdigit(*p))
            pEntry->param = *p++ - '0';
        else
        {
            if (c == '\\' && (*p == '$' || *p == '&'))
                c = *p++;
            *pDest++ = c;
            ++(pEntry->strlen);
            continue;
        }
        ++pEntry;
        pEntry->strbegin = pDest - pThis->parsed;
        pEntry->strlen = 0;
    }
    *pDest = 0;
    if (pEntry->strlen == 0)
        --entries;
    else
        ++pEntry;
    pThis->plistend = pEntry;
    assert(pEntry - pThis->plist == entries);
    return LS_OK;
}


int ls_pcresub_exec(ls_pcresub_t *pThis, const char *input,
                    const int *ovector, int ovec_num, char *output, int *length)
{

    ls_pcresubent_t *pEntry = pThis->plist;
    char *p = output;
    assert(length);
    char *pBufEnd = output + *length;
    while (pEntry < pThis->plistend)
    {
        if (pEntry->strlen > 0)
        {
            if (p + pEntry->strlen < pBufEnd)
                memmove(p, pThis->parsed + pEntry->strbegin, pEntry->strlen);
            p += pEntry->strlen;
        }
        if ((pEntry->param >= 0) && (pEntry->param < ovec_num))
        {
            const int *pParam = ovector + (pEntry->param << 1);
            int len = *(pParam + 1) - *pParam;
            if (len > 0)
            {
                if (p + len < pBufEnd)
                    memmove(p, input + *pParam , len);
                p += len;
            }
        }
        ++pEntry;
    }
    if (p < pBufEnd)
        *p = '\0';
    *length = p - output;
    return (p < pBufEnd) ? LS_OK : LS_FAIL;
}


int ls_pcresub_getlen(ls_pcresub_t *pThis, const char *input,
                      const int *ovector, int ovec_num)
{

    ls_pcresubent_t *pEntry = pThis->plist;
    int iLen = 0;
    while (pEntry < pThis->plistend)
    {
        if (pEntry->strlen > 0)
        {
            if (pEntry->strlen > INT_MAX - iLen)
                return LS_FAIL;
            iLen += pEntry->strlen;
        }
        if ((pEntry->param >= 0) && (pEntry->param < ovec_num))
        {
            const int *pParam = ovector + (pEntry->param << 1);
            int len = *(pParam + 1) - *pParam;
            if (len > 0)
            {
                if (len > INT_MAX - iLen)
                    return LS_FAIL;
                iLen += len;
            }
        }
        ++pEntry;
    }
    if (iLen == INT_MAX)
        return LS_FAIL;
    ++iLen;
    return iLen;
}

