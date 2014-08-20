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
#include "httprange.h"
#include "httpstatuscode.h"

#include <util/autostr.h>
#include <util/gpointerlist.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/ssnprintf.h>

class ByteRange
{
    long m_lBegin;
    long m_lEnd;
public:
    ByteRange()
        : m_lBegin(0), m_lEnd(0)
        {}

    ByteRange(long b, long e )
        : m_lBegin( b ), m_lEnd( e )
        {}
    long getBegin() const   { return m_lBegin;  }
    long getEnd() const     { return m_lEnd;    }
    void setBegin( long b ) { m_lBegin = b;     }
    void setEnd( long e)    { m_lEnd = e;       }
    int check( int entityLen );
    long getLen() const     { return m_lEnd - m_lBegin + 1;   }
};

void HttpRangeList::clear()
{   release_objects();  }



int ByteRange::check( int entityLen )
{
    if ( entityLen > 0 )
    {
        if ( m_lBegin == -1 )
        {
            long b = entityLen - m_lEnd;
            if ( b < 0 )
                b = 0;
            m_lBegin = b;
            m_lEnd = entityLen - 1;
        }
        if (( m_lEnd == -1 )||( m_lEnd >= entityLen ))
        {
            m_lEnd = entityLen - 1 ;
        }
        if ( m_lEnd < m_lBegin )
            return -1;
    }
    return 0;
}

HttpRange::HttpRange( long entityLen )
    : m_lEntityLen( entityLen )
{
    ::memset( m_boundary, 0, sizeof( m_boundary) );
}


int HttpRange::count() const
{   return m_list.size();   }


int HttpRange::checkAndInsert( ByteRange & range )
{
    if (( range.getBegin() == -1 )&&( range.getEnd() == -1 ))
        return -1;
    if ( m_lEntityLen > 0 )
    {
        if ( range.getBegin() >= m_lEntityLen )
            return 0;
        if ( range.check( m_lEntityLen ) )
            return -1;
    }
    m_list.push_back( new ByteRange( range ) );
    return 0;
}

/*
  Range: bytes=10-30,40-50,-60,61-
  A state machine is used to parse the range request string.
  states are:
    0: start parsing a range unit,
    1: first number in the range unit,
    2: '-' between the two number of the range unit
    3: second number in the range unit,
    4: space after first number
    5: space after second number
  states transitions:

    0 -> 0 : receive white space
    0 -> 1 : receive a digit
    0 -> 2 : receive a '-'
    0 -> end: receive '\0'
    0 -> err: receive char other than the above

    1 -> 1 : receive a digit
    1 -> 2 : receive a '-'
    1 -> 4 : receive a white space
    1 -> err: receive char other than the above

    2 -> 2 : receive white space
    2 -> 3 : receive a digit
    2 -> 0 : receive a ','
    2 -> end: receive '\0'
    2 -> err: receive char other than the above

    3 -> 3 : receive a digit
    3 -> 5 : receive a white space
    3 -> 0 : receive a ','
    3 -> end: receive '\0'
    3 -> err: receive char other than the above

    4 -> 4 : receive white space
    4 -> 2 : receive a '-'
    4 -> err: receive char other than the above

    5 -> 5 : receive white space
    5 -> 0 : receive ','
    5 -> end: receive '\0'
    5 -> err: receive char other than the above
*/

int HttpRange::parse( const char * pRange )
{
    m_list.clear();
    if ( strncasecmp( pRange, "bytes=", 6 ) != 0 )
        return SC_400;
    char ch;
    pRange+= 6;
    while( isspace(( ch = *pRange++ )) )
        ;
    if ( !ch )
        return SC_400;
    int state = 0;
    ByteRange range(-1,-1);
    long lValue = 0;
    while( state != 6 )
    {
        switch( ch )
        {
        case ' ':
        case '\t':
            switch( state )
            {
            case 0: case 2: case 4: case 5:
                break;
            case 1:
                state = 4;
                range.setBegin( lValue );
                lValue = 0;
                break;
            case 3:
                state = 5;
                range.setEnd( lValue );
                lValue = 0;
                break;
            }
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            switch( state )
            {
            case 0: case 2:
                lValue = ch - '0';
                ++state;
                break;
            case 1: case 3:
                lValue = (lValue << 3 ) + (lValue << 1 ) + (ch - '0');
                break;
            case 4: case 5:
                state = 6; //error,
                break;
            }
            break;
        case '-':
            switch( state )
            {
            case 1:
                range.setBegin( lValue );
                lValue = 0;
                //don't break, fall through
            case 0: case 4:
                state = 2;
                break;
            case 2: case 3: case 5:
                state = 6;
                break;
            }
            break;
        case '\r': case '\n':
            ch = 0;
            //fall through
        case ',': case '\0': 
            switch( state )
            {
            case 3:
                range.setEnd( lValue );
                lValue = 0;
                //don't break, fall through
            case 2:  case 5:
                if ( checkAndInsert( range ) == -1 )
                {
                    state = 6;
                    break;
                }
                range.setBegin( -1 );
                range.setEnd( -1 );
                state = 0;
                break;
            case 1: case 4:
                state = 6;
                break;
            case 0:
                if ( ch )
                    state = 6;
                break;
            }
            break;
        default:
            state = 6;
            break;
        }
        if ( !ch )
            break;
        ch = *pRange++;
    }
    if ( state == 6 )
    {
        m_list.clear();
        return SC_400;
    }
    if ( m_list.empty() )
        return SC_416; //range not satisfiable
    return 0;
}

//Content-Range: 
int HttpRange::getContentRangeString( int n, char * pBuf, int len ) const
{
    char * pBegin = pBuf;
    int ret;
    ret = safe_snprintf( pBuf, len, "%s", "bytes " );
    pBuf += ret;
    len -= ret;
    ret = 0;
    if ( len <= 0 )
        return -1;
    if (( n < 0 )||(n >= (int)m_list.size() ))
    {
        if ( m_lEntityLen == -1 )
            return -1;
        else
            ret = safe_snprintf( pBuf, len, "*/%ld", m_lEntityLen );
    }
    else
    {
        int ret1 = safe_snprintf( pBuf, len, "%ld-%ld/", m_list[ n ]->getBegin(),
                    m_list[ n ]->getEnd() );
        pBuf += ret1;
        len -= ret1;
        if ( len <= 0 )
            return -1;
        if ( m_lEntityLen >= 0 )
        {
            ret = safe_snprintf( pBuf, len, "%ld", m_lEntityLen );
        }
        else
        {
            if ( len > 4 )
            {
                *pBuf++ = '*';
                *pBuf = 0;
            }
            else
                return -1;
        }
    }
    len -= ret;
    pBuf += ret;
    return ( len > 0 ) ? pBuf - pBegin : -1;
}

int HttpRange::getContentOffset( int n, long& begin, long& end ) const
{
    if ((n < 0 )||( n >= (int)m_list.size() ))
        return -1;
    if ( m_list[ n ]->check( m_lEntityLen ) == 0 )
    {
        begin = m_list[ n ]->getBegin();
        end = m_list[ n ]->getEnd() + 1;
        return 0;
    }
    else
        return -1;
}

void HttpRange::clear()
{
    m_list.clear();
}

void HttpRange::makeBoundaryString()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    safe_snprintf( m_boundary, sizeof( m_boundary ) - 1, "%08x%08x",
            (int)now.tv_usec^getpid(), (int)(now.tv_usec^now.tv_sec) );
    *( m_boundary + sizeof( m_boundary ) - 1 ) = 0;
}

void HttpRange::beginMultipart()
{
    makeBoundaryString();
    m_iCurRange = -1;
}

int HttpRange::getPartHeader( int n, const char * pMimeType, char* buf, int size ) const
{
    assert(( n >= 0 )&&( n <= (int)m_list.size() ));
    int ret;
    if ( n < (int)m_list.size() )
        ret = safe_snprintf( buf, size,
                        "\r\n--%s\r\n"
                        "Content-type: %s\r\n"
                        "Content-range: bytes %ld-%ld/%ld\r\n"
                        "\r\n", m_boundary, pMimeType,
                        m_list[ n ]->getBegin(),
                        m_list[ n ]->getEnd(),
                        m_lEntityLen );
    else
        ret = safe_snprintf( buf, size, "\r\n--%s--\r\n", m_boundary );
    return ( ret == size )? -1 : ret;
}

static off_t getDigits(off_t n)
{
    if (n < 10)
        return 1;
    else
        return (off_t)log10((double)n) + 1;
}

long HttpRange::getPartLen( int n, int iMimeTypeLen ) const
{
    long len = 4 + 16 + 2
         + 14 + iMimeTypeLen + 2
         + 21 + getDigits( m_list[ n ]->getBegin() ) + 1
         + getDigits( m_list[ n ]->getEnd() ) + 1
         + 2 + 2
         + m_list[ n ]->getLen();
    if ( m_lEntityLen == -1 )
        ++len;
    else
        len += getDigits( m_lEntityLen );
    return len;
}

long HttpRange::getMultipartBodyLen( const AutoStr2* pMimeType ) const
{
    assert( pMimeType );
    if ( m_lEntityLen == -1 )
        return -1;
    int typeLen = pMimeType->len();
    int size = m_list.size();
    long total = 0;
    for( int i = 0; i < size; ++i )
    {
        total += getPartLen( i, typeLen );
    }
    return total + 24;
}

bool HttpRange::more() const
{
    return m_iCurRange < (int)m_list.size();
}

int HttpRange::getContentOffset( long& begin, long& end ) const
{
    return getContentOffset( m_iCurRange, begin, end );
}

