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
#ifndef AUTOBUF_H
#define AUTOBUF_H


#include <string.h>

class AutoBuf
{
    char   *m_pBuf;
    char   *m_pEnd;
    char   *m_pBufEnd;

    AutoBuf(const AutoBuf &rhs) {}
    void operator=(const AutoBuf &rhs) {}

    int     allocate(int size);
    void    deallocate();
public:
    explicit AutoBuf(int size = 1024);
    ~AutoBuf();
    int available() const { return m_pBufEnd - m_pEnd; }

    char *begin() const { return m_pBuf; }
    char *end() const  { return m_pEnd; }

    char *getPointer(int offset) const  { return m_pBuf + offset; }

    void used(int size)   {   m_pEnd += size;     }
    void clear()            {   m_pEnd = m_pBuf;    }

    int capacity() const    {   return m_pBufEnd - m_pBuf; }
    int size()   const      {   return m_pEnd - m_pBuf;   }

    int reserve(int size)  {   return allocate(size);    }
    void resize(int size)  {   m_pEnd = m_pBuf + size;    }

    int grow(int size);
    int appendNoCheck(const char *pBuf, int size)
    {
        memmove(end(), pBuf, size);
        used(size);
        return size;
    }

    void append(char ch)
    {   *m_pEnd++ = ch; }
    int append(const char *pBuf, int size);
    int append(const char *pBuf)
    {   return append(pBuf, strlen(pBuf)); }
    bool empty()  const     {   return (m_pBuf == m_pEnd);  }
    bool full() const       {   return (m_pEnd == m_pBufEnd); }
    int getOffset(const char *p) const
    {   return p - m_pBuf;    }

    int moveTo(char *pBuf, int size);
    int pop_front(int size);
    int pop_end(int size);
    void swap(AutoBuf &rhs);
};

#endif
