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
#ifndef FILTERMATCH_H
#define FILTERMATCH_H


#include <util/pcregex.h>

class FilterMatch
{
public:

    explicit FilterMatch(const char * filter);
    // filter is expected to be nul terminated
    ~FilterMatch();

    bool match(const char * val, size_t val_len = 0);
    // we don't know about val. if can't limit matches to val_len
    // will have to make local copy and nul terminate it

    bool isNegated()    { return m_negate; }
private:
    bool        m_negate;
    bool        m_igncase;;
    Pcregex     m_reg;
    int         m_fnmatchFlags;
    const char *m_filter;
    size_t      m_filterLen;
    typedef enum FilterType_ {
        STRCMP,
        FNMATCH,
        REGEX
    } FilterType;
    FilterType  m_type;

    void        init(const char * filter);

    LS_NO_COPY_ASSIGN(FilterMatch);
};
#endif
