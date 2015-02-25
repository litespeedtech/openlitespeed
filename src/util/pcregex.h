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
#ifndef PCREGEX_H
#define PCREGEX_H


#include <util/autostr.h>
#include <pcre.h>

//#define _USE_PCRE_JIT_


class RegexResult
{
public:
    RegexResult()
        : m_pBuf(NULL)
        , m_matches(0)
    {}
    ~RegexResult() {}

    void setBuf(const char *pBuf)    {   m_pBuf = pBuf;          }
    void setMatches(int matches)      {   m_matches = matches;    }
    int getSubstr(int i, char *&pValue) const;
    int *getVector()                   {   return m_ovector;        }
    int  getMatches() const             {   return m_matches;       }

private:
    const char *m_pBuf;
    int    m_ovector[30];
    int    m_matches;
};


class Pcregex       //pcreapi
{
    pcre         *m_regex;
    pcre_extra   *m_extra;
    int           m_iSubStr;

public:
    Pcregex();
    ~Pcregex();
#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
    static void init_jit_stack();
    static pcre_jit_stack *get_jit_stack();
    static void release_jit_stack(void *pValue);
#endif
#endif
    int  compile(const char *regex, int options, int matchLimit = 0,
                 int recursionLimit = 0);
    int  exec(const char *subject, int length, int startoffset,
              int options, int *ovector, int ovecsize) const
    {
#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
        pcre_jit_stack *stack = get_jit_stack();
        pcre_assign_jit_stack(m_extra, NULL, stack);
#endif
#endif
        return pcre_exec(m_regex, m_extra, subject, length, startoffset,
                         options, ovector, ovecsize);
    }
    int  exec(const char *subject, int length, int startoffset,
              int options, RegexResult *pRes) const
    {
        pRes->setMatches(pcre_exec(m_regex, m_extra, subject, length, startoffset,
                                   options, pRes->getVector(), 30));
        return pRes->getMatches();
    }
    void release();
    int  getSubStrCount() const  {   return m_iSubStr;   }

};



class RegSub
{

    typedef struct
    {
        int m_strBegin;
        int m_strLen;
        int m_param;
    } RegSubEntry;
    AutoStr       m_parsed;
    RegSubEntry *m_pList;
    RegSubEntry *m_pListEnd;

public:
    RegSub();
    RegSub(const RegSub &rhs);
    ~RegSub();
    void release();
    int compile(const char *input);
    int exec(const char *input, const int *ovector, int ovec_num,
             char *output, int &length);
};




#endif
