/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: reqparser.cpp,v 1.2.2.3 2014/12/14 23:31:54 gwang Exp $
                         -------------------
    begin                : Fri Sep 22 2006
    author               : George Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#include "reqparser.h"

#include <http/httpreq.h>
#include <http/httplog.h>
#include <http/requestvars.h>

#include <util/gpath.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

ReqParser::ReqParser()
    : m_decodeBuf( 8192 )
    , m_multipartBuf( 8192 )
    , m_partParsed( 0 )
    , m_state_kv( 0 )
    , m_last_char( 0 )
    , m_beginIndex( 0 )
    , m_args( 0 )
    , m_maxArgs( 0 )
    , m_pArgs( NULL )
    , m_iCookies( -1 )
    , m_iMaxCookies( 0 )
    , m_pCookies( NULL )

{
    allocArgIndex( 8 );
    allocCookieIndex( 8 );
}

ReqParser::~ReqParser()
{
    if ( m_pArgs )
    {
        free( m_pArgs );
        m_pArgs = NULL;
    }
    if ( m_pCookies )
    {
        free( m_pCookies );
        m_pCookies = NULL;
    }
}


void ReqParser::reset()
{
    m_decodeBuf.clear();
    m_multipartBuf.clear();
    m_state_kv = 0;
    m_beginIndex = 0;
    m_args = 0;
    m_part_boundary = "";
    m_ignore_part = 1;
    m_multipartState = MPS_INIT_BOUNDARY;
    m_iBeginOff = 0;
    m_iCurOff = 0;
    m_iData = MPS_NODATA;
    m_pErrStr = NULL;
    m_partParsed = 0;
    m_qsBegin   = 0;
    m_qsArgs    = 0;
    m_postBegin = 0;
    m_postArgs  = 0;
    m_iCookies = -1;
}


int ReqParser::allocArgIndex( int newMax )
{
    if ( newMax <= m_maxArgs )
        return 0;
    KeyValuePair * pNewBuf = (KeyValuePair* )realloc( m_pArgs,
                    sizeof( KeyValuePair ) * newMax );
    if ( pNewBuf )
    {
        m_pArgs = pNewBuf;
        m_maxArgs = newMax;
    }
    else
        return -1;
    return 0;
}

int ReqParser::allocCookieIndex( int newMax )
{
    if ( newMax <= m_iMaxCookies )
        return 0;
    KeyValuePair * pNewBuf = (KeyValuePair* )realloc( m_pCookies,
                    sizeof( KeyValuePair ) * newMax );
    if ( pNewBuf )
    {
        m_pCookies = pNewBuf;
        m_iMaxCookies = newMax;
    }
    else
        return -1;
    return 0;
}

int ReqParser::appendArgKeyIndex( int begin, int len )
{
    ++m_args;
    if ( m_args >= m_maxArgs )
    {
        int newMax = m_maxArgs << 1;
        if ( allocArgIndex( newMax ) )
            return -1;
    }
    m_pArgs[m_args-1].keyOffset = begin;
    m_pArgs[m_args-1].keyLen    = len;
    m_pArgs[m_args-1].valueOffset   = begin+len+1;
    m_pArgs[m_args-1].valueLen      = 0;
    return 0;
}

int ReqParser::appendArg( int beginIndex, int endIndex, int isValue )
{
    int len = endIndex - beginIndex;

    //if( len > 0 )
    //    len = normalisePath( beginIndex, len );

    if (isValue )
    {
        //if ( m_args > 0)
        {
            m_pArgs[m_args-1].valueOffset   = beginIndex;
            m_pArgs[m_args-1].valueLen      = len;
        }
    }
    else
    {
        return appendArgKeyIndex( beginIndex, len );
    }
    return 0;
}

int ReqParser::normalisePath( int begin, int len )
{
    int n;
    m_decodeBuf.append( '\0' );
    n = GPath::clean( m_decodeBuf.getPointer( begin ), len );
    if ( n < len )
    {
        m_decodeBuf.pop_end( len - n + 1 );    
    }
    else
        m_decodeBuf.pop_end( 1 );
    return n;
}

int ReqParser::appendArgKey( const char * pStr, int len )
{
    if ( m_decodeBuf.size() )
        m_decodeBuf.append( '&' );
    int begin = m_decodeBuf.size();
    m_decodeBuf.append( pStr, len );

    //len = normalisePath( begin, len );

    m_decodeBuf.append( '=' );
    return appendArgKeyIndex( begin, len );

}


void ReqParser::resumeDecode( const char * &pStr, const char * pEnd )
{
    char ch;
    if ( m_last_char == 1 )
    {
        if ( pEnd > pStr )
        {
            if ( isxdigit( *pStr ))
                m_last_char = *pStr++;
            else
                m_last_char = 0;
        }
    }
    if ( isxdigit( m_last_char ) )
    {
        if ( pEnd > pStr )
        {
            if( isxdigit( *pStr ))
            {
                ch = ( hexdigit( m_last_char ) << 4) + hexdigit( *pStr++ );
                if ( !ch )
                    ch = ' ';
                m_decodeBuf.append( ch );
            }
            else
                m_decodeBuf.append( m_last_char );
            m_last_char = 0;
        }
    }
}

int ReqParser::parseArgs( const char * pStr, int len, int resume, int last )
{
    const char * pEnd = pStr + len;
    char   ch = 0;
    if ( m_decodeBuf.available() < len )
    {
        m_decodeBuf.grow( len );
    }
    if ( !resume )
        m_beginIndex = m_decodeBuf.size();
    else
    {
        if ( m_last_char )
            resumeDecode( pStr, pEnd );
    }
    while( pStr < pEnd )
    {
        ch = *pStr++;
        switch( ch )
        {
        case '#':
            pStr = pEnd;
            break;
        case '=':
            if ( m_state_kv )
                break;
            //fall through
        case '&':
            appendArg( m_beginIndex, m_decodeBuf.size(), m_state_kv );
            m_state_kv = (ch == '=');
            m_decodeBuf.append( ch );
            m_beginIndex = m_decodeBuf.size();
            continue;
        case '+':
            ch = ' ';
            break;
        case '%':
            m_last_char = 1;
            resumeDecode( pStr, pEnd );
            continue;
        }
        if ( !ch )
            ch = ' ';
        m_decodeBuf.append( ch );
    }
    if ( last )
    {
        if ( isxdigit( m_last_char ) )
            m_decodeBuf.append( ch );
        if ( m_beginIndex != m_decodeBuf.size() )
        {
            if (( !m_state_kv )||( m_args ))
                appendArg( m_beginIndex, m_decodeBuf.size(), m_state_kv );
        }
    }
    return 0;
}

int ReqParser::parseQueryString( const char * pQS, int len )
{
    if ( len > 0 )
    {
        int ret;
        m_last_char = 0;
        m_qsBegin = m_args;
        ret = parseArgs( pQS, len, 0, 1 );
        m_qsArgs = m_args - m_qsBegin;
        return ret;
    }
    return 0;
}

int ReqParser::initMutlipart( const char * pContentType, int len )
{
    if ( !pContentType )
    {
        m_pErrStr = "Missing ContentType";
        return -1;
    }
    m_last_char = 0;
    m_multipartBuf.clear();
    const char * p = strstr( pContentType, "boundary=" );
    if ( p && *(p+9) )
    {
        len = len - ((p+9) - pContentType );
        if ( len > MAX_BOUNDARY_LEN )
        {
            m_pErrStr = "Boundary string is too long";
            m_multipartState = MPS_ERROR;
            return -1;
        }
        m_part_boundary.setStr( p+9, len );
    }
    else
    {
        m_pErrStr = "Boundary string is not available";
        m_multipartState = MPS_ERROR;
        return -1;
    }
    m_iData = MPS_NODATA;
    m_multipartState = MPS_INIT_BOUNDARY;
    return 0;
}

int ReqParser::parseKeyValue( char * &pBegin, char * pLineEnd,
                        char *&pKey, char *&pValue, int &valLen )
{
    char *pValueEnd;
    pValue = (char *)memchr( pBegin, '=', pLineEnd - pBegin );
    if ( !pValue )
    {
        m_pErrStr = "Invalid Content-Disposition value";
        m_multipartState = MPS_ERROR;
        return -1;
    }
    *pValue++ = 0;
    pKey = pBegin;
    if ( *pValue == '"' )
    {
        ++pValue;
        pValueEnd = (char *)memchr( pValue, '"', pLineEnd - pBegin );
        if ( !pValueEnd )
        {
            m_pErrStr = "missing ending '\"' in Content-Disposition value";
            m_multipartState = MPS_ERROR;
            return -1;
        }
        else
            *pValueEnd = 0;

    }
    else
    {
        pValueEnd = strpbrk( pValue, " \t;\r\n" );
        if ( !pValueEnd )
            pValueEnd = pLineEnd;
        else
            *pValueEnd = 0;

    }
    valLen = pValueEnd - pValue;
    pBegin = pValueEnd + 1;
    return 0;
}


int ReqParser::multipartParseHeader( char * pBegin, char *pLineEnd )
{
    char  * pName       = NULL;
    int     nameLen     = 0;
    char  * pFileName   = NULL;
    int     fileNameLen = 0;
    int     ret = 0;
    while( pBegin < pLineEnd )
    {
        if ( strncasecmp( "content-disposition:", pBegin, 20 ) == 0 )
        {
            pBegin += 20;
            while( isspace( *pBegin ) )
                ++pBegin;
            if ( strncasecmp( "form-data;", pBegin, 10 ) != 0 )
            {
                m_pErrStr = "missing 'form-data;' string in Content-Disposition value";
                m_multipartState = MPS_ERROR;
                return -1;
            }
            pBegin += 10;
            while( isspace( *pBegin ) )
                ++pBegin;
            while((*pBegin)&&( *pBegin != '\n' )&&( *pBegin != '\r' ))
            {
                char *pKey, *pValue;
                int valLen;
                if ( parseKeyValue( pBegin, pLineEnd, pKey, pValue, valLen) == -1 )
                    break;
                if ( strcasecmp( "name", pKey ) == 0 )
                {
                    if ( valLen == 0 )
                        m_ignore_part = 1;
                    else
                    {
                        pName   = pValue;
                        nameLen = valLen;
                    }
                }
                else if ( strcasecmp( "filename", pKey ) == 0 )
                {
                    m_ignore_part   = 1;
                    pFileName       = pValue;
                    fileNameLen     = valLen;
                }
                while( ( *pBegin == ' ' )||( *pBegin == '\t' )||(*pBegin == ';') )
                    ++pBegin;

            }
        }
        else if ( strncasecmp( "content-type:", pBegin, 13 ) == 0 )
        {
            pBegin += 13;
            while( isspace( *pBegin ) )
                ++pBegin;
            if ( strncasecmp( "multipart/mixed", pBegin, 15 ) == 0 )
                m_ignore_part = 1;
        }
        pBegin = (char *)memchr( pBegin, '\n', pLineEnd - pBegin );
        if ( !pBegin )
            pBegin = pLineEnd;
        else
            ++pBegin;
    }
    if ( !pName )
    {
        m_pErrStr = "missing 'name' value in Content-Disposition value";
        m_multipartState = MPS_ERROR;
        return -1;
    }
    m_iData = MPS_FORM_DATA;
    if ( pFileName )
    {
        ret = appendArgKey( pName, nameLen );
        if ( ret == 0 )
        {
            m_decodeBuf.append( pFileName, fileNameLen );

            fileNameLen = normalisePath( m_pArgs[m_args-1].valueOffset, fileNameLen );
            
            m_pArgs[m_args-1].valueLen = fileNameLen;
        }
    }
    else if ( !m_ignore_part )
    {
        ret = appendArgKey( pName, nameLen );
    }
    return ret;
}

int ReqParser::popProcessedData( char * pBegin )
{
    m_iBeginOff = pBegin - m_multipartBuf.begin();
    m_iCurOff   = m_multipartBuf.size();
    if ( m_iBeginOff > 0 )
    {
        m_multipartBuf.pop_front( m_iBeginOff );
        m_iCurOff -= m_iBeginOff;
        m_iBeginOff = 0;
    }
    return 0;    
}

int ReqParser::checkBoundary( char * &pBegin, char * &pCur )
{
    char * p;
    int len;
    if ( memcmp( pBegin + 2, m_part_boundary.c_str(),
                    m_part_boundary.len()) == 0 )
    {
        if ( !m_ignore_part )
        {
            if ( *(m_decodeBuf.end() - 2) == '\r' )
            {
                m_decodeBuf.pop_end( 2 );
            }
            else
                m_decodeBuf.pop_end( 1 );
            *(m_decodeBuf.end() ) = 0;
            if ( m_args > 0 )
            {
                len = m_decodeBuf.size() - m_pArgs[m_args-1].valueOffset;
                len = normalisePath( m_pArgs[m_args-1].valueOffset, len );
                m_pArgs[m_args-1].valueLen = len;
            }
        }
        p = pBegin + 2 + m_part_boundary.len();
        if ( *p == '-' )
        {
            if ( *(p+1) == '-' )
            {
                m_multipartState = MPS_END;
                return 1;
            }
        }
        else
        {
            if ( *p == '\r' )
                ++p;
            if ( *p == '\n' )
            {
                m_multipartState = MPS_PART_HEADER;
                pCur = pBegin = ++p;
                m_ignore_part = 0;
                return 1;
            }
        }
        m_pErrStr = "Invalid boundary string encountered.";
        m_multipartState = MPS_ERROR;
        return -1;
    }
    if ( m_multipartState == MPS_INIT_BOUNDARY )
    {
        m_pErrStr = "initial BOUNDARY string is missing";
        m_multipartState = MPS_ERROR;
        return -1;
    }
    m_multipartState = MPS_PART_DATA;
    pCur = pBegin;
    return 0;
}


int ReqParser::parseMutlipart( const char * pBuf, size_t size, int resume, int last )
{
    if (size > 0 )
    {
        m_multipartBuf.append( pBuf, size );
    }
    int    ret;
    char * pBegin = m_multipartBuf.begin()+m_iBeginOff;
    char * pEnd   = m_multipartBuf.end();
    char * pCur   = m_multipartBuf.begin()+m_iCurOff;
    char * pLineEnd;
    char * p;
    int count = 0;
    char * pOldCur = NULL;
    while( pCur < pEnd )
    {
        if ( pOldCur != pCur )
        {
            count = 0;
            pOldCur = pCur;
        }
        else
        {
            ++count;
            if ( count > 20 )
            {
                if ( pCur + 100 < pEnd )
                    *(pCur + 100) = 0;
                LOG_ERR(( "Parsing Error! resume: %d, last: %d, state: %d, pBegin: %p:%s, pCur: %p:%s, pEnd: %p",
                            resume, last, m_multipartState, pBegin, pBegin, pCur, pCur, pEnd ));
                *((char *)(0x0)) = 0;
            }
        }

        switch( m_multipartState )
        {
        case MPS_END:
            return 0;
        case MPS_INIT_BOUNDARY:
            if ( pEnd - pBegin >= m_part_boundary.len() + 4 )
            {
                ret = checkBoundary( pBegin, pCur );
                if ( ret == -1 )
                    return ret;
            }
            else
            {
                if ( last )
                {
                    //unexpected end of initial boundary string
                    return -1;
                }
                else
                    return popProcessedData( pBegin );
            }
            break;
            
        case MPS_PART_HEADER:
            pLineEnd = (char *)memchr( pCur, '\n', pEnd - pCur );
            if ( !pLineEnd )
            {
                if ( !last )
                {
                    m_iCurOff = m_multipartBuf.size();
                    return popProcessedData( pBegin );
                }
                else
                {
                    //unexpected end of data, part header expected
                    return -1;
                }
            }
            p = pLineEnd - 1;
            if ( *p == '\r' )
                --p;
            if ( *p == '\n' )  //found end of part header
            {
                multipartParseHeader( pBegin, p );
                pBegin = pCur = pLineEnd + 1;
                m_multipartState = MPS_PART_DATA_BOUNDARY;
            }
            else
                pCur = pLineEnd + 1;
            continue;
        case MPS_PART_DATA_BOUNDARY:
            if ( *pBegin == '-' )
            {
                if ( pEnd - pBegin >= m_part_boundary.len() + 4 )
                {
                    ret = checkBoundary( pBegin, pCur );
                    if ( ret == -1 )
                        return ret;
                    if ( ret == 1 )
                        continue;
                }
                else
                    return popProcessedData( pBegin );
            }
            m_multipartState = MPS_PART_DATA;
            //fall through
        case MPS_PART_DATA:
            pCur = pLineEnd = (char *)memchr( pCur, '\n', pEnd - pCur );
            if ( !pCur )
                pCur = pEnd - 1;
            ++pCur;
            if ( !m_ignore_part )
            {
                m_decodeBuf.append( pBegin, pCur - pBegin );
            }
            if ( pLineEnd )
            {
                m_multipartState = MPS_PART_DATA_BOUNDARY;
            }
            pBegin = pCur;
            continue;
        case MPS_ERROR:
            return -1;
        }

    }
    popProcessedData( pBegin );
    return 0;
}


int ReqParser::parsePostBody( const char * pBuf, size_t size,
                            int bodyType, int resume, int last )
{
    int ret;
    if ( bodyType == REQ_BODY_FORM )
    {
        ret = parseArgs( pBuf, size, resume, last );
    }
    else
    {
        ret = parseMutlipart( pBuf, size, resume, last );
    }
    return ret;
}


int ReqParser::parsePostBody( HttpReq * pReq )
{
    if ( pReq->getContentLength() <= 0 )
        return 0;
    VMemBuf * pBodyBuf = pReq->getBodyBuf();
    if ( !pBodyBuf )
        return 0;
    m_state_kv = 0;
    
    char * pBuf;
    size_t size;
    int resume = 0;
    short bodyType = pReq->getBodyType();
    if (bodyType == REQ_BODY_MULTIPART)
    {
        if ( initMutlipart( pReq->getHeader( HttpHeader::H_CONTENT_TYPE ),
                        pReq->getHeaderLen( HttpHeader::H_CONTENT_TYPE ) ) == -1)
        {
            return -1;
        }
    }
    pBodyBuf->rewindReadBuf();
    while(( pBuf = pBodyBuf->getReadBuffer( size ) )&&(size>0))
    {
        parsePostBody( pBuf, size, pReq->getBodyType(), resume, 0 );
        pBodyBuf->readUsed( size );
        resume = 1;
    }
    parsePostBody( pBuf, 0, pReq->getBodyType(), 1, 1 );
    //pBodyBuf->rewindReadBuf();
    m_postArgs = m_args - m_postBegin ;

    return 0;
}

void ReqParser::logParsingError( HttpReq * pReq )
{
    LOG_INFO(( pReq->getLogger(), "[%s] Error while parsing request: %s!",
                pReq->getLogId(), getErrorStr() ));
}


/*
int ReqParser::parseArgs( HttpReq * pReq, int scanPost )
{
    if ((scanPost != 2 )&&!( m_partParsed & SEC_LOC_ARGS_GET ))
    {
        m_partParsed |= SEC_LOC_ARGS_GET;
        if (( pReq->getQueryStringLen() > 0 )&&
            ( parseQueryString( pReq->getQueryString(),
                                pReq->getQueryStringLen() ) == -1 ))
        {
            logParsingError(  pReq );
            return -1;
        }
    }
    if ( scanPost && !(m_partParsed & SEC_LOC_ARGS_POST ) )
    {
        m_postBegin = m_args;
        m_partParsed |= SEC_LOC_ARGS_POST;
        if ( pReq->getContentLength() > 0 )
        {
            VMemBuf * pBodyBuf = pReq->getBodyBuf();
            size_t offset = pBodyBuf->getCurROffset();
            int ret = parsePostBody( pReq );
            if ( offset != pBodyBuf->getCurROffset() )
            {
                pBodyBuf->setROffset( offset );
            }
            if ( ret == -1)
            {
                logParsingError( pReq );
                return -1;
            }
        }
    }
    return 0;
}
*/

int ReqParser::parseCookies( const char * pCookies, int len )
{
    if ( m_iCookies != -1 )
        return 0;
    m_iCookies = 0;
    if ( !pCookies || !*pCookies || len == 0 )
        return 0;
    const char * pEnd = pCookies + len ;
    const char * p, *pVal, *pNameEnd, *pValEnd;
    int nameOff, valOff;
    for(; pCookies < pEnd; pCookies = p + 1 )
    {
        p = (const char *)memchr( pCookies, ';', pEnd - pCookies );
        if ( !p )
            p = pEnd;
        pValEnd = p;
        while( isspace( *pCookies ) )
            ++pCookies;
        if ( p == pCookies )
            continue;
        pVal = (const char *)memchr( pCookies, '=', p - pCookies );
        if ( !pVal )
            continue;
        pNameEnd = pVal++;
        while( isspace( *pVal ) )
            ++pVal;
        nameOff = m_decodeBuf.size();
        m_decodeBuf.append( pCookies, pNameEnd - pCookies );
        m_decodeBuf.append( '\0' );
        valOff = m_decodeBuf.size();
        while( isspace( pValEnd[-1] ) )
            --pValEnd;
        if ( pValEnd > pVal )
            m_decodeBuf.append( pVal, pValEnd - pVal );
        else
            pValEnd = pVal;
        m_decodeBuf.append( '\0' );
        
        if ( m_iCookies >= m_iMaxCookies )
        {
            int newMax = m_iMaxCookies << 1;
            if ( allocCookieIndex( newMax ) )
                return -1;
        }
        m_pCookies[m_iCookies].keyOffset = nameOff;
        m_pCookies[m_iCookies].keyLen    = pNameEnd - pCookies;
        m_pCookies[m_iCookies].valueOffset   = valOff;
        m_pCookies[m_iCookies].valueLen      = pValEnd - pVal;
        ++m_iCookies;

    }
    return 0;
}

int ReqParser::testCookies( HttpSession* pSession, SecRule * pRule, AutoBuf * pMatched, int countOnly )
{
    int i;
    const char * p;
    int len;
    int ret, ret1;
    int offset;
    int total = 0; 
    for( i = 0; i < m_iCookies; ++i )
    {
        offset = m_pCookies[i].keyOffset;
        len = m_pCookies[i].keyLen;
        p = m_decodeBuf.begin() + offset;
        ret = pRule->getLocation()->shouldTest( p, len, SEC_LOC_COOKIES );
        if (( ret & 4 )||(ret & countOnly) ) //count_only
        {
            ++total;
        }
        if ( ret & 2 ) //value
        {
            offset = m_pCookies[i].valueOffset;
            len = m_pCookies[i].valueLen;
            p = m_decodeBuf.begin() + offset;
            //LOG_D(( "[SECURITY] Cookie value len: %d ", len ));

            ret1 = pRule->matchString( pSession, p, len, pMatched );
            if ( ret1 )
                return 1;
            
        }
        if ( ret & 1 ) //name
        {
            offset = m_pCookies[i].keyOffset;
            len = m_pCookies[i].keyLen;
            p = m_decodeBuf.begin() + offset;
            ret1 = pRule->matchString( pSession, p, len, pMatched );
            if ( ret1 )
                return 1;
        }
    }
    if (( total )&&( !countOnly ))
    {
        return pRule->matchCount( total, pSession, pMatched );
    }
    return total;
}


const char *ReqParser::getArgStr( int location, int &len )
{
    switch( location )
    {
    case SEC_LOC_ARGS_ALL:
        if ( m_args == 0 )
            break;
        else
            len = m_pArgs[m_args - 1].valueOffset +
                  m_pArgs[m_args - 1].valueLen -
                  m_pArgs[0].keyOffset;;
        return m_decodeBuf.begin() + m_pArgs[0].keyOffset;
    case SEC_LOC_ARGS_GET:
        if ( m_qsArgs == 0 )
            break;
        else
            len = m_pArgs[m_qsBegin + m_qsArgs - 1].valueOffset +
                  m_pArgs[m_qsBegin + m_qsArgs - 1].valueLen -
                  m_pArgs[m_qsBegin].keyOffset;
        return m_decodeBuf.begin() + m_pArgs[m_qsBegin].keyOffset;
    case SEC_LOC_ARGS_POST:
        if ( m_postArgs == 0 )
            break;
        else
            len = m_pArgs[m_postBegin + m_postArgs - 1].valueOffset +
                  m_pArgs[m_postBegin + m_postArgs - 1].valueLen -
                  m_pArgs[m_postBegin].keyOffset;
        return m_decodeBuf.begin() + m_pArgs[m_postBegin].keyOffset;
    }
    len = 0;
    return "";
    
}

const char * ReqParser::getArgStr( const char * pName, int nameLen, int &len )
{
    int i;
    for( i = 0; i < m_args; ++i )
    {
        const char * p = m_decodeBuf.begin() + m_pArgs[i].keyOffset;
        if (( m_pArgs[i].keyLen == nameLen )&&
            ( strncasecmp( pName, p, nameLen ) == 0 ))
        {
            len = m_pArgs[i].valueLen;
            return m_decodeBuf.begin() + m_pArgs[i].valueOffset;
        }
    }
    len = 0;
    return NULL;
}

const char * ReqParser::getReqHeader( const char * pName, int nameLen, int &len,
                HttpReq * pReq )
{
    const char * pHeader = RequestVars::getUnknownHeader(
                                pReq, pName, nameLen, len);

    return pHeader;
    
}

const char * ReqParser::getReqVar( HttpSession* pSession, int varId, int &len,
                    char * pBegin, int bufLen )
{
    len = RequestVars::getReqVar( pSession, varId, pBegin, bufLen );
    return pBegin;
    
}

/*
int ReqParser::testArgs( int location, HttpSession* pSession, SecRule * pRule, AutoBuf * pMatched, 
                        int countOnly )
{
    int i, n;
    const char * p;
    int len;
    int ret, ret1;
    int offset;
    switch( location )
    {
    case SEC_LOC_ARGS_ALL:
        i = 0; 
        n = m_args;
        break;
    case SEC_LOC_ARGS_GET:
        if ( m_qsArgs == 0 )
            return 0;
        i = m_qsBegin;
        n = m_qsBegin + m_qsArgs;
        break;
    case SEC_LOC_ARGS_POST:
        if ( m_postArgs == 0 )
            return 0;
        i = m_postBegin;
        n = m_postArgs;
        break;
    default:
        return 0;
    }
    int result = 0;
    int total = 0; 
    for( ; i < n; ++i )
    {
        offset = m_pArgs[i].keyOffset;
        len = m_pArgs[i].keyLen;
        p = m_decodeBuf.begin() + offset;
        ret = pRule->getLocation()->shouldTest( p, len, SEC_LOC_ARGS );
        if (( ret & 4 )||(ret && countOnly)) //count_only
        {
            ++total;
        }
        if ( ret & 2 ) //value
        {
            offset = m_pArgs[i].valueOffset;
            len = m_pArgs[i].valueLen;
            p = m_decodeBuf.begin() + offset;
            ret1 = pRule->matchString( pSession, p, len, pMatched );
            if ( ret1 )
            {
                if ( pRule->isMatchVars() )
                     ++result;
                else
                    return 1;
            } 
        }
        if ( ret & 1 ) //name
        {
            offset = m_pArgs[i].keyOffset;
            len = m_pArgs[i].keyLen;
            p = m_decodeBuf.begin() + offset;
            ret1 = pRule->matchString( pSession, p, len, pMatched );
            if ( ret1 )
            {
                if ( pRule->isMatchVars() )
                     ++result;
                else
                    return 1;
            }
        }
    }
    if (( total )&&( !countOnly ))
    {
        return pRule->matchCount( total, pSession, pMatched );
    }
    if ( countOnly )
        return total;
    else
        return result;
}
*/

void ReqParser::testQueryString()
{
    ReqParser parser;
    char achBufTest[] = "action=upload&./action1=%26+%2Ba%25&action2==%21%40%23%24%25%5E%26*%28%29/./";
    char achDecoded[] = "action=upload&action1=& +a%&action2==!@#$%^&*()/";
    parser.reset();
    parser.parseQueryString(achBufTest, sizeof( achBufTest ) - 1 );
    int res = memcmp( parser.m_decodeBuf.begin(), achDecoded, sizeof( achDecoded ) - 1 );
    assert( res == 0 );
    assert( parser.m_args == 3 );

    assert( parser.m_pArgs[0].keyOffset == 0 );
    assert( parser.m_pArgs[0].keyLen == 6 );
    assert( parser.m_pArgs[0].valueOffset == 7 );
    assert( parser.m_pArgs[0].valueLen == 6 );

    assert( parser.m_pArgs[1].keyOffset == 14 );
    assert( parser.m_pArgs[1].keyLen == 7 );
    assert( parser.m_pArgs[1].valueOffset == 22 );
    assert( parser.m_pArgs[1].valueLen == 5 );

    assert( parser.m_pArgs[2].keyOffset == 28 );
    assert( parser.m_pArgs[2].keyLen == 7 );
    assert( parser.m_pArgs[2].valueOffset == 36 );
    assert( parser.m_pArgs[2].valueLen == 12 );
    
    parser.reset();
    
    parser.parseArgs( "", 0, 0, 0);
    char * p = achBufTest;
    while( *p )
    {
        parser.parseArgs( p, 1, 1, 0 );
        ++p;
    }
    parser.parseArgs("", 0, 1, 1 );
    res = memcmp( parser.m_decodeBuf.begin(), achDecoded, sizeof( achDecoded ) - 1 );
    assert( res == 0 );

    assert( parser.m_args == 3 );

    assert( parser.m_pArgs[0].keyOffset == 0 );
    assert( parser.m_pArgs[0].keyLen == 6 );
    assert( parser.m_pArgs[0].valueOffset == 7 );
    assert( parser.m_pArgs[0].valueLen == 6 );

    assert( parser.m_pArgs[1].keyOffset == 14 );
    assert( parser.m_pArgs[1].keyLen == 7 );
    assert( parser.m_pArgs[1].valueOffset == 22 );
    assert( parser.m_pArgs[1].valueLen == 5 );

    assert( parser.m_pArgs[2].keyOffset == 28 );
    assert( parser.m_pArgs[2].keyLen == 7 );
    assert( parser.m_pArgs[2].valueOffset == 36 );
    assert( parser.m_pArgs[2].valueLen == 12 );
}

void ReqParser::testMultipart()
{
    char achBufTest[] =
        "-----------------------------17146369151957747793424238335\r\n"
        "Content-Disposition: form-data; name=\"action\"\n"
        "\n"
        "upload\n"
        "-----------------------------17146369151957747793424238335\n"
        "Content-Disposition: form-data; name=\"./action1\"\n"
        "\n"
        "& +a%\n"
        "-----------------------------17146369151957747793424238335\r\n"
        "Content-Disposition: form-data; name=\"action2\"\r\n"
        "\r\n"
        "--@#$%^&*()\r\n"
        "-----------------------------17146369151957747793424238335\r\n"
        "Content-Disposition: form-data; name=\"action3\"\r\n"
        "\r\n"
        "line1\nline2\r\n/./bin/\n"
        "-----------------------------17146369151957747793424238335\r\n"
        "Content-Disposition: form-data; name=\"userfile\"; filename=\"abcd\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n"
        "\r\n"
        "-----------------------------1714636915195774779342423833\r"
        "\r\n"
        "\r\n"
        "\r\n"
        "-----------------------------17146369151957747793424238335\r\n"
        "Content-Disposition: form-data; name=\"./userfile2\"; filename=\"/./file2\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n"
        "\r\n"
        "-----------------------------17146369151957747793424238335--\r\n";
    const char * pContentType =
        "multipart/form-data; boundary=---------------------------17146369151957747793424238335";
    char achDecoded[] = "action=upload&action1=& +a%&action2=--@#$%^&*()&"
                        "action3=line1\nline2\r\n/bin/&userfile=abcd&userfile2=/file2";
    int res;
    ReqParser parser;

    parser.reset();
    parser.initMutlipart( pContentType, 86 );

    parser.parseMutlipart( achBufTest, sizeof( achBufTest ) -1, 0, 1 );
    res = memcmp( parser.m_decodeBuf.begin(), achDecoded, sizeof( achDecoded ) - 1 );
    assert( res == 0 );
    assert( parser.m_args == 6 );

    assert( parser.m_pArgs[0].keyOffset == 0 );
    assert( parser.m_pArgs[0].keyLen == 6 );
    assert( parser.m_pArgs[0].valueOffset == 7 );
    assert( parser.m_pArgs[0].valueLen == 6 );

    assert( parser.m_pArgs[1].keyOffset == 14 );
    assert( parser.m_pArgs[1].keyLen == 7 );
    assert( parser.m_pArgs[1].valueOffset == 22 );
    assert( parser.m_pArgs[1].valueLen == 5 );

    assert( parser.m_pArgs[2].keyOffset == 28 );
    assert( parser.m_pArgs[2].keyLen == 7 );
    assert( parser.m_pArgs[2].valueOffset == 36 );
    assert( parser.m_pArgs[2].valueLen == 11 );

    assert( parser.m_pArgs[3].keyOffset == 48 );
    assert( parser.m_pArgs[3].keyLen == 7 );
    assert( parser.m_pArgs[3].valueOffset == 56 );
    assert( parser.m_pArgs[3].valueLen == 18 );

    assert( parser.m_pArgs[4].keyOffset == 75 );
    assert( parser.m_pArgs[4].keyLen == 8 );
    assert( parser.m_pArgs[4].valueOffset == 84 );
    assert( parser.m_pArgs[4].valueLen == 4 );

    assert( parser.m_pArgs[5].keyOffset == 89 );
    assert( parser.m_pArgs[5].keyLen == 9 );
    assert( parser.m_pArgs[5].valueOffset == 99 );
    assert( parser.m_pArgs[5].valueLen == 6 );

    parser.reset();
    parser.initMutlipart( pContentType, 86 );

    parser.parseMutlipart( "", 0, 0, 0);
    char * p = achBufTest;
    char * pEnd = &achBufTest[0] + sizeof( achBufTest ) - 1;
    while( *p )
    {
        res = pEnd - p;
        if ( res > 4 )
            res = 4;
        parser.parseMutlipart( p, res, 1, 0 );
        p += res;
    }
    parser.parseMutlipart("", 0, 1, 1 );
    res = memcmp( parser.m_decodeBuf.begin(), achDecoded, sizeof( achDecoded ) - 1 );
    assert( res == 0 );
    assert( parser.m_args == 6 );

    assert( parser.m_pArgs[0].keyOffset == 0 );
    assert( parser.m_pArgs[0].keyLen == 6 );
    assert( parser.m_pArgs[0].valueOffset == 7 );
    assert( parser.m_pArgs[0].valueLen == 6 );

    assert( parser.m_pArgs[1].keyOffset == 14 );
    assert( parser.m_pArgs[1].keyLen == 7 );
    assert( parser.m_pArgs[1].valueOffset == 22 );
    assert( parser.m_pArgs[1].valueLen == 5 );

    assert( parser.m_pArgs[2].keyOffset == 28 );
    assert( parser.m_pArgs[2].keyLen == 7 );
    assert( parser.m_pArgs[2].valueOffset == 36 );
    assert( parser.m_pArgs[2].valueLen == 11 );

    assert( parser.m_pArgs[3].keyOffset == 48 );
    assert( parser.m_pArgs[3].keyLen == 7 );
    assert( parser.m_pArgs[3].valueOffset == 56 );
    assert( parser.m_pArgs[3].valueLen == 18 );

    assert( parser.m_pArgs[4].keyOffset == 75 );
    assert( parser.m_pArgs[4].keyLen == 8 );
    assert( parser.m_pArgs[4].valueOffset == 84 );
    assert( parser.m_pArgs[4].valueLen == 4 );

    assert( parser.m_pArgs[5].keyOffset == 89 );
    assert( parser.m_pArgs[5].keyLen == 9 );
    assert( parser.m_pArgs[5].valueOffset == 99 );
    assert( parser.m_pArgs[5].valueLen == 6 );
    
}

void ReqParser::testAll()
{
    testQueryString();
    testMultipart();
}


