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
#include "http/httprespheaders.h"
#include <arpa/inet.h>
#include <http/httpver.h>
#include <socket/gsockaddr.h>

/*******************************************************************************
 *          Something  about kvapir
 * For a regular foramt header, keyOff is the real offset of the key/name
 *                              valOff is the real offset of the value
 * 
 * For a SPDY format header, keyOff is the first position of the section of key/name
 *  Since the key/name section is short length (2 bytes) + key, 
 *      though keyOff + 2 equals to real offset of the key/name
 *  And the same way, valOff + 2 is the real offset of the value
 * 
 * ****************************************************************************/

HttpRespHeaders::HttpRespHeaders()
{
    m_sKVPair = "";
    incKVPairs(20); //init 20 kvpair spaces 
    reset();
}

void HttpRespHeaders::reset(RespHeader::FORMAT format)
{
    m_headerFormat = format;
    m_hasHole = 0;
    m_iHeaderCount = 0;
    m_buf.clear();
    memset(m_KVPairindex, 0xFF, H_HEADER_END);
    memset(m_sKVPair.buf(), 0, m_sKVPair.len());
    m_hLastHeaderKVPairIndex = -1;
    m_pStatusLine = NULL;
}

void HttpRespHeaders::incKVPairs(int num)
{
    int size = m_sKVPair.len() + sizeof(key_value_pair) * num;
    m_sKVPair.resizeBuf(size);
    m_sKVPair.setLen(size);
    
    size = sizeof(key_value_pair) * num;
    char *temp = m_sKVPair.buf() + m_sKVPair.len() - size;
    memset(temp, 0, size);
}

inline key_value_pair * HttpRespHeaders::getKVPair(int index)
{
    key_value_pair *pKVPair = (key_value_pair *) (m_sKVPair.buf() + index * sizeof(key_value_pair));
    return pKVPair;
}

//Replace the value with new value in pKv, in this case must have enough space
void HttpRespHeaders::replaceHeader(key_value_pair *pKv, const char * pVal, unsigned int valLen)
{
    char *pOldVal = getVal(pKv);
    memcpy( pOldVal, pVal, valLen );
    memset( pOldVal + valLen, ' ', pKv->valLen - valLen);
}

inline void appendLowerCase(AutoBuf& buf , const char *str, unsigned int len)
{
    const char *pEnd = str + len;
    while (str < pEnd)
        buf.append( *str ++ | 0x20);  
}

inline void appendNetInt(AutoBuf &buf, int n, int bytesNum)
{
    assert( bytesNum == 2 || bytesNum == 4);
    char *p = buf.end() - 1;
    buf.used(bytesNum);
    while( bytesNum )
    {
        p[bytesNum--] = (char)(n & 0xFF);
        n >>= 8;
    }
}

int HttpRespHeaders::appendHeader(key_value_pair *pKv, const char * pName, unsigned int nameLen, 
                                  const char * pVal, unsigned int valLen, RespHeader::ADD_METHOD method)
{
    /*******************************************************************
    *  About Append in regular header
    * ie. set-cookie: "123" will append set-cookie: "4567"
    * At beginning, key: set-cookie, Val: "123"
    * End, key: set-cookie, Val: "123\r\nset-cookie: 4567 "
    * In this way, we will store set-cookie in one place of kvlist
    ******************************************************************/
    if (method == RespHeader::REPLACE)
        memset(pKv, 0, sizeof(key_value_pair));
    
    //Will insert a "\0" between new value and old value when spdy
    if ( m_buf.available() < (int)(pKv->valLen + nameLen * 2 + valLen + 8 + 2) )
    {
        if ( m_buf.grow( pKv->valLen + nameLen * 2 + valLen + 8 + 2 ) )
            return -1;
    }
    
    pKv->keyOff = m_buf.size();
    pKv->keyLen = nameLen;
    
    if (m_headerFormat == RespHeader::REGULAR)
    {
        m_buf.appendNoCheck(pName, nameLen);
        m_buf.appendNoCheck(": ", 2);
        if (pKv->valLen > 0)
        {
            //include 2 more chars, they ARE \r\n
            m_buf.appendNoCheck(m_buf.begin() + pKv->valOff, pKv->valLen + 2);
            m_buf.appendNoCheck(pName, nameLen);
            m_buf.appendNoCheck(": ", 2);
            pKv->valLen += 2 + nameLen + 2;
        }
        m_buf.appendNoCheck(pVal, valLen);
        m_buf.appendNoCheck("\r\n", 2);
        
        pKv->valLen +=  valLen;
    }
    else
    {
        appendNetInt(m_buf, nameLen,  ((m_headerFormat == RespHeader::SPDY2) ? 2 : 4 ));
        appendLowerCase(m_buf , pName, nameLen);
        
        int tmpValLen = ((pKv->valLen > 0)?(pKv->valLen + 1):(0)) + valLen;
        appendNetInt(m_buf, tmpValLen,  ((m_headerFormat == RespHeader::SPDY2) ? 2 : 4 ));
        
        if (pKv->valLen > 0)
        {
            m_buf.appendNoCheck(m_buf.begin() + pKv->valOff + 
                ((m_headerFormat == RespHeader::SPDY2) ? 2 : 4 ), pKv->valLen);
            m_buf.append('\0');
        }
        m_buf.appendNoCheck(pVal, valLen);
        
        pKv->valLen = tmpValLen;
    }
    pKv->valOff = pKv->keyOff + nameLen + 2 + (( m_headerFormat == RespHeader::SPDY3) ? 2 : 0 );
    return 0;
}


void HttpRespHeaders::verifyHeaderLength(HEADERINDEX headerIndex, const char * pName, unsigned int nameLen)
{
#ifndef NDEBUG    
    assert ( headerIndex == getRespHeaderIndex(pName) );
    if ( getFormat() != RespHeader::SPDY3 || *pName != ':' )
    {
        assert ( (int)nameLen == getHeaderStringLen(headerIndex) );
    }
    else
    {
        assert ( (int)nameLen - 1 == getHeaderStringLen(headerIndex) );
    }
#endif
}

//replace_or_append: 0 replace,1 append    
int HttpRespHeaders::add( HEADERINDEX headerIndex, const char * pName, unsigned int nameLen, const char * pVal, unsigned int valLen, RespHeader::ADD_METHOD method )
{
    assert (headerIndex < H_HEADER_END);
    assert ( nameLen > 0 );
    
    int kvOrderNum = -1;
    key_value_pair *pKv = NULL;
    
    //if unkonw, check if it is already in our index list
    if ( headerIndex == H_UNKNOWN )
        headerIndex = getRespHeaderIndex( pName );
    
    if ( headerIndex == H_HEADER_END ) // || headerIndex == H_UNKNOWN )
    {
        int ret = getHeaderKvOrder(pName, nameLen);
        if ( ret != -1)
            kvOrderNum = ret;
        else
            kvOrderNum = m_iHeaderCount;
    }
    else
    {
        verifyHeaderLength(headerIndex,  pName, nameLen);
        if (m_KVPairindex[headerIndex] == 0xFF)
            m_KVPairindex[headerIndex] = m_iHeaderCount;
        kvOrderNum = m_KVPairindex[headerIndex];
    }

    if ( kvOrderNum == m_iHeaderCount )
    {
        //Add a new header not inside before
        if (getFreeSpaceCount() == 0)
            incKVPairs(10);
        ++m_iHeaderCount;
    }
    
    pKv = getKVPair(kvOrderNum);
    
    //enough space for replace, use the same keyoff, and update valoff, add padding, make it is the same leangth as before
    if (method == RespHeader::REPLACE && pKv->keyLen + pKv->valLen >= (int) (nameLen + valLen) )
    {
        assert ( pKv->keyLen == (int)nameLen );
        assert ( strncasecmp(getName(pKv), pName, nameLen) ==0 );
        replaceHeader(pKv, pVal, valLen);
        return 0;
    }
    //Newly add will also be treated as append
    else
    {
        m_hLastHeaderKVPairIndex = kvOrderNum;
        
        //Under append situation, if has existing key and valLen > 0, then makes a hole
        if (pKv->valLen > 0)
        {
            assert(pKv->keyLen > 0);
            m_hasHole = 1;
        }

        return appendHeader(pKv, pName, nameLen, pVal, valLen, method);
    }
}

//This will only append value to the last item
int HttpRespHeaders::appendLastVal(const char * pName, int nameLen, const char * pVal, int valLen)
{
    if (m_hLastHeaderKVPairIndex == -1 || valLen <= 0)
        return -1;
    
    assert( m_hLastHeaderKVPairIndex < m_iHeaderCount );
    key_value_pair *pKv = getKVPair(m_hLastHeaderKVPairIndex);
    
    //check if it is the end
    if (m_buf.size() != pKv->valOff + pKv->valLen + 2 + ((m_headerFormat == RespHeader::SPDY3) ? 2 : 0))
        return -1;
    
    //check if the name match
    if (pKv->keyLen != nameLen || strncasecmp(pName, getName(pKv), nameLen) != 0)
        return -1;

    if ( m_buf.available() < valLen)
    {
        if ( m_buf.grow( valLen ) )
            return -1;
    }
    
    //Only update the valLen of the kvpair
    pKv->valLen += valLen;
    if (m_headerFormat == RespHeader::REGULAR)
    {
        m_buf.used( -2 );//move back two char, to replace the \r\n at the end
        m_buf.appendNoCheck(pVal, valLen);
        m_buf.appendNoCheck("\r\n", 2);
    }
    else if (m_headerFormat == RespHeader::SPDY2)
    {
        m_buf.appendNoCheck(pVal, valLen);
        short *valLenRef = (short *)(m_buf.begin() + pKv->valOff);
        *valLenRef = htons(pKv->valLen);
    }
    else
    {
        m_buf.appendNoCheck(pVal, valLen);
        int32_t *valLenRef = (int32_t *)(m_buf.begin() + pKv->valOff);
        *valLenRef = htonl(pKv->valLen);
    }
        
    return 0;
}

int HttpRespHeaders::add( header_st *headerArray, int size, RespHeader::ADD_METHOD method )
{
    int ret = 0;
    for (int i=0; i<size; ++i) 
    {
        if ( 0 != add(headerArray[i].index, headerArray[i].name, headerArray[i].nameLen,
                        headerArray[i].val, headerArray[i].valLen, method))
        {
            ret = -1;
            break;
        }
    }
    
    return ret;
}

void HttpRespHeaders::delAndMove(int kvOrderNum)
{
    if (kvOrderNum == -1)
        return;
    
    key_value_pair *pKvLastPos = getKVPair(m_iHeaderCount- 1);
    if (kvOrderNum != m_iHeaderCount- 1)
    {
        //move the last pos memory to the deleled pos
        key_value_pair *pKv = getKVPair(kvOrderNum);
        memcpy(pKv, pKvLastPos, sizeof(key_value_pair));
    }
    //need to empty this section for later using won't mess up
    memset(pKvLastPos, 0, sizeof(key_value_pair));
    
    for (int i=0; i<H_HEADER_END; ++i)
    {
        if (m_KVPairindex[i] == m_iHeaderCount- 1)
        {
            m_KVPairindex[i] = kvOrderNum;
            break ;
        }
    }
    
    m_hasHole = 1;
    m_iHeaderCount --;
}

//del( const char * pName, int nameLen ) is lower than  del( int headerIndex )
//
int HttpRespHeaders::del( const char * pName, int nameLen )
{
    assert (nameLen > 0);
    size_t idx = getRespHeaderIndex(pName);
    if (idx == H_HEADER_END)
    {
        int kvOrderNum = getHeaderKvOrder(pName, nameLen);
        delAndMove(kvOrderNum);
    }
    else
        del ((HEADERINDEX)idx);
    return 0;
}
 
//del() will make some {0,0,0,0} kvpair in the list and make hole
int HttpRespHeaders::del( HEADERINDEX headerIndex )
{
    if (headerIndex < 0)
        return -1;
    
    if (m_KVPairindex[headerIndex] != 0xFF)
    {
        delAndMove(m_KVPairindex[headerIndex]);
        m_KVPairindex[headerIndex] = 0xFF;
    }
    return 0;
}

void HttpRespHeaders::getHeaders(IOVec *io)
{
     if (m_headerFormat == RespHeader::REGULAR && m_pStatusLine) 
        io->push_front(m_pStatusLine->get(), m_pStatusLine->getLen());
    
    if (m_hasHole == 0)
    {
        if (m_buf.size() > 0)
            io->append(m_buf.begin(), m_buf.size());
    }
    else
    {
        for (int i=0; i<m_iHeaderCount; ++i) 
        {
            key_value_pair *pKv = getKVPair(i);
            assert (pKv->keyLen > 0);
            if (m_headerFormat != RespHeader::SPDY3)
                io->appendCombine(m_buf.begin() + pKv->keyOff, pKv->keyLen + pKv->valLen + 4);
            else
                io->appendCombine(m_buf.begin() + pKv->keyOff, pKv->keyLen + pKv->valLen + 8);
        }
    }
    
    //Only for those normal header (have status code) need add \r\n
    if (m_headerFormat == RespHeader::REGULAR && m_pStatusLine) 
       io->append("\r\n", 2);
}

void HttpRespHeaders::addStatusLine(int ver, int code)
{
    const StatusLineString& statusLine = HttpStatusLine::getStatusLine( ver, code );
    
    if (m_headerFormat == RespHeader::REGULAR)
        m_pStatusLine = &statusLine;
    else if (m_headerFormat == RespHeader::SPDY2)
    {
        add(HttpRespHeaders::CGI_STATUS, "status", 6, statusLine.get() + 9, 3);
        add(HttpRespHeaders::H_HTTP_VERSION, "version", 7, statusLine.get(), 8);
    }
    else
    {
        add(HttpRespHeaders::CGI_STATUS, ":status", 7, statusLine.get() + 9, 3);
        add(HttpRespHeaders::H_HTTP_VERSION, ":version", 8, statusLine.get(), 8);
    }
}

char *HttpRespHeaders::getContentTypeHeader(int &len)
{
    int index = m_KVPairindex[HttpHeader::H_CONTENT_TYPE];
    if (index == 0xFF)
    {
        len = -1;
        return NULL;
    }
    
    key_value_pair *pKv = getKVPair(index);
    len = pKv->valLen;
    return getVal(pKv);
}

//The below calling will not maintance the kvpaire when regular case
void HttpRespHeaders::parseAdd(const char * pStr, int len, RespHeader::ADD_METHOD method )
{
    //When SPDY format, we need to parse it and add one by one
    const char *pName = NULL;
    int nameLen = 0;
    const char *pVal = NULL;
    int valLen = 0;
    
    const char *pBEnd = pStr + len;
    const char *pMark = NULL;
    const char *pLineEnd= NULL;
    const char* pLineBegin  = pStr;
    
    while((pLineEnd  = ( const char *)memchr(pLineBegin, '\n', pBEnd - pLineBegin )) != NULL)
    {
        pMark = ( const char *)memchr(pLineBegin, ':', pLineEnd - pLineBegin);
        if(pMark != NULL)
        {
            pName = pLineBegin;
            nameLen = pMark - pLineBegin; //Should - 1 to remove the ':' position
            pVal = pMark + 2;
            if (*pVal == ' ')
                ++ pVal;
            valLen = pLineEnd - pVal;
            if (*(pLineEnd - 1) == '\r')
                -- valLen;
            
            //This way, all the value use APPEND as default
            add(H_UNKNOWN, pName, nameLen, pVal, valLen, method);
        }
        
        pLineBegin = pLineEnd + 1;
        if (pBEnd <= pLineBegin + 1)
            break;
    }
}

int HttpRespHeaders::getHeaderKvOrder(const char *pName, unsigned int nameLen)
{
    int index = -1;
    key_value_pair *pKv = NULL;

    for (int i=0; i<m_iHeaderCount; ++i)
    {
        pKv = getKVPair(i);
        if (pKv->keyLen == (int)nameLen &&
            strncasecmp(pName, getName(pKv), nameLen) == 0)
        {
            index = i;
            break;
        }
    }
    
    return index;
}

int HttpRespHeaders::_getHeader(int kvOrderNum, char **pVal, int &valLen)
{
    if (kvOrderNum == -1)
        return -1;
     
    key_value_pair *pKv = getKVPair(kvOrderNum);
    *pVal = getVal(pKv);
    valLen = pKv->valLen;
    return 0;
}

int  HttpRespHeaders::getHeader(HEADERINDEX index, char **pVal, int &valLen)
{
    int kvOrderNum = -1;
    if (m_KVPairindex[index] != 0xFF)
        kvOrderNum = m_KVPairindex[index];
    
    return _getHeader( kvOrderNum, pVal, valLen);
}

int HttpRespHeaders::getHeader(const char *pName, int nameLen, char **pVal, int &valLen)
{
    HEADERINDEX idx = getRespHeaderIndex(pName);
    if (idx != H_HEADER_END)
        return getHeader(idx, pVal, valLen);
       
    int kvOrderNum = getHeaderKvOrder(pName, nameLen);
    return _getHeader( kvOrderNum, pVal, valLen);
}

HttpRespHeaders::HEADERINDEX HttpRespHeaders::getRespHeaderIndex( const char * pHeader )
{
    register HEADERINDEX idx = H_HEADER_END;

    switch( *pHeader++ | 0x20 )
    {
    case 'a':
        if ( strncasecmp( pHeader, "ccept-ranges", 12 ) == 0 )
            idx = H_ACCEPT_RANGES;
        break;        
    case 'c':
        if ( strncasecmp( pHeader, "onnection", 9 ) == 0 )
            idx = H_CONNECTION;
        else if ( strncasecmp( pHeader, "ontent-type", 11) == 0 )
            idx = H_CONTENT_TYPE;
        else if ( strncasecmp( pHeader, "ontent-length", 13 ) == 0 )
            idx = H_CONTENT_LENGTH;
        else if ( strncasecmp( pHeader, "ontent-encoding", 15 ) == 0 )
            idx = H_CONTENT_ENCODING;
        else if ( strncasecmp( pHeader, "ontent-range", 12 ) == 0 )
            idx = H_CONTENT_RANGE;
        else if ( strncasecmp( pHeader, "ontent-disposition", 18 ) == 0 )
            idx = H_CONTENT_DISPOSITION;
        else if ( strncasecmp( pHeader, "ache-control", 12 ) == 0 )
            idx = H_CACHE_CTRL;
        break;
    case 'd':
        if ( strncasecmp( pHeader, "ate", 3 ) == 0 )
                idx = H_DATE;
        break;
    case 'e':
        if ( strncasecmp( pHeader, "tag", 3 ) == 0 )
                idx = H_ETAG;
        else if ( strncasecmp( pHeader, "xpires", 6 ) == 0 )
                idx = H_EXPIRES;
        break;
    case 'k':
        if ( strncasecmp( pHeader, "eep-alive", 9 ) == 0 )
            idx = H_KEEP_ALIVE;
        break;
    case 'l':
        if ( strncasecmp( pHeader, "ast-modified", 12 ) == 0 )
            idx = H_LAST_MODIFIED;
        else if ( strncasecmp( pHeader, "ocation", 7 ) == 0 )
            idx = H_LOCATION;
        else if ( strncasecmp( pHeader, "itespeed-cache-control", 22 ) == 0 )
            idx = H_LITESPEED_CACHE_CONTROL;
        break;
    case 'p':
        if ( strncasecmp( pHeader, "ragma", 5 ) == 0 )
            idx = H_PRAGMA;
        else if ( strncasecmp( pHeader, "roxy-connection", 15 ) == 0 )
            idx = H_PROXY_CONNECTION;
        break;
    case 's':
        if ( strncasecmp( pHeader, "erver", 5 ) == 0 )
            idx = H_SERVER;
        else if ( strncasecmp( pHeader, "et-cookie", 9 ) == 0 )
            idx = H_SET_COOKIE;
        else if ( strncasecmp( pHeader, "tatus", 5 ) == 0 )
            idx = CGI_STATUS;
        break;
    case 't':
        if ( strncasecmp( pHeader, "ransfer-encoding", 16 ) == 0 )
            idx = H_TRANSFER_ENCODING;
        break;
    case 'v':
        if ( strncasecmp( pHeader, "ary", 3 ) == 0 )
            idx = H_VARY;
        else if ( strncasecmp( pHeader, "ersion", 6 ) == 0 )
            idx = H_HTTP_VERSION;
        break;
    case 'w':
        if ( strncasecmp( pHeader, "ww-authenticate", 15 ) == 0 )
            idx = H_WWW_AUTHENTICATE;
        break;
    case 'x':
        if ( strncasecmp( pHeader, "-powered-by", 11 ) == 0 )
            idx = H_X_POWERED_BY;
        else if ( strncasecmp( pHeader, "-litespeed-location", 19 ) == 0 )
            idx = H_LITESPEED_LOCATION;
        break;
    case ':': //only SPDY 3
        if ( strncasecmp( pHeader, "status", 6 ) == 0 )
            idx = CGI_STATUS;
        else if ( strncasecmp( pHeader, "version", 7 ) == 0 )
            idx = H_HTTP_VERSION;
        break;
        
    default:
        break;
    }
    return idx;    
}
