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
#include <util/pcregex.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
static int s_jit_key_inited = 0;
static pthread_key_t s_jit_stack_key;

void Pcregex::init_jit_stack()
{
    s_jit_key_inited = 1;
    pthread_key_create(&s_jit_stack_key, release_jit_stack);
}

void Pcregex::release_jit_stack(void *pValue)
{
    pcre_jit_stack_free((pcre_jit_stack *) pValue);
}


pcre_jit_stack *Pcregex::get_jit_stack()
{
    pcre_jit_stack *jit_stack;

    if (!s_jit_key_inited)
        init_jit_stack();
    jit_stack = (pcre_jit_stack *)pthread_getspecific(s_jit_stack_key);
    if (!jit_stack)
    {
        jit_stack = (pcre_jit_stack *)pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
        pthread_setspecific(s_jit_stack_key, jit_stack);
    }
    return jit_stack;
}
#endif
#endif

Pcregex::Pcregex()
    : m_regex(NULL)
    , m_extra(NULL)
    , m_iSubStr(0)
{
}
Pcregex::~Pcregex()
{
    release();
}

void Pcregex::release()
{
    if (m_regex)
    {
        if (m_extra)
        {
#if defined( _USE_PCRE_JIT_)&&!defined(__sparc__) && !defined(__sparc64__) && defined( PCRE_CONFIG_JIT )
            pcre_free_study(m_extra);
#else
            pcre_free(m_extra);
#endif

        }
        pcre_free(m_regex);
        m_regex = NULL;
    }
}

int Pcregex::compile(const char *regex, int options, int matchLimit,
                     int recursionLimit)
{
    const char *error;
    int          erroffset;
    if (m_regex)
        release();
    m_regex = pcre_compile(regex, options, &error, &erroffset, NULL);
    if (m_regex == NULL)
        return -1;
    m_extra = pcre_study(m_regex,
#if defined( _USE_PCRE_JIT_)&&!defined(__sparc__) && !defined(__sparc64__) && defined( PCRE_CONFIG_JIT )
                         PCRE_STUDY_JIT_COMPILE,
#else
                         0,
#endif
                         & error);
    if (matchLimit > 0)
    {
        m_extra->match_limit = matchLimit;
        m_extra->flags |= PCRE_EXTRA_MATCH_LIMIT;
    }
    if (recursionLimit > 0)
    {
        m_extra->match_limit_recursion = recursionLimit;
        m_extra->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    }
    pcre_fullinfo(m_regex, m_extra, PCRE_INFO_CAPTURECOUNT, &m_iSubStr);
    ++m_iSubStr;
    return 0;
}

RegSub::RegSub()
    : m_pList(NULL)
    , m_pListEnd(NULL)
{}

RegSub::RegSub(const RegSub &rhs)
{
    m_parsed.setStr(rhs.m_parsed.c_str(),
                    (char *)rhs.m_pListEnd - rhs.m_parsed.c_str());
    m_pList = (RegSubEntry *)(m_parsed.c_str() +
                              ((char *)rhs.m_pList - rhs.m_parsed.c_str()));
    m_pListEnd = m_pList + (rhs.m_pListEnd - rhs.m_pList);
}


RegSub::~RegSub()
{
}


int RegSub::compile(const char *rule)
{
    if (!rule)
        return -1;
    const char *p = rule;
    register char c;
    int entries = 0;
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
    if (m_parsed.resizeBuf(bufSize + entries * sizeof(RegSubEntry)) == NULL)
        return -1;
    m_pList = (RegSubEntry *)(m_parsed.buf() + bufSize);
    memset(m_pList, 0xff, entries * sizeof(RegSubEntry));

    char *pDest = m_parsed.buf();
    p = rule;
    RegSubEntry *pEntry = m_pList;
    pEntry->m_strBegin = 0;
    pEntry->m_strLen = 0;
    while ((c = *p++) != '\0')
    {
        if (c == '&')
            pEntry->m_param = 0;
        else if (c == '$' && isdigit(*p))
            pEntry->m_param = *p++ - '0';
        else
        {
            if (c == '\\' && (*p == '$' || *p == '&'))
                c = *p++;
            *pDest++ = c;
            ++(pEntry->m_strLen);
            continue;
        }
        ++pEntry;
        pEntry->m_strBegin = pDest - m_parsed.buf();
        pEntry->m_strLen = 0;
    }
    *pDest = 0;
    if (pEntry->m_strLen == 0)
        --entries;
    else
        ++pEntry;
    m_pListEnd = pEntry;
    assert(pEntry - m_pList == entries);
    return 0;
}

int RegSub::exec(const char *input, const int *ovector, int ovec_num,
                 char *output, int &length)
{
    RegSubEntry *pEntry = m_pList;
    char *p = output;
    char *pBufEnd = output + length;
    while (pEntry < m_pListEnd)
    {
        if (pEntry->m_strLen > 0)
        {
            if (p + pEntry->m_strLen < pBufEnd)
                memmove(p, m_parsed.c_str() + pEntry->m_strBegin, pEntry->m_strLen);
            p += pEntry->m_strLen;
        }
        if ((pEntry->m_param >= 0) && (pEntry->m_param < ovec_num))
        {
            const int *pParam = ovector + (pEntry->m_param << 1);
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
        *p = 0;
    length = p - output;
    return (p < pBufEnd) ? 0 : -1;
}


int RegexResult::getSubstr(int i, char *&pValue) const
{
    if (i < m_matches)
    {
        const int *pParam = &m_ovector[ i << 1 ];
        pValue = (char *)m_pBuf + *pParam;
        return *(pParam + 1) - *pParam;
    }
    return 0;
}





