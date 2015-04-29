/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: reqparser.h,v 1.2.2.2 2014/12/14 23:31:54 gwang Exp $
                         -------------------
    begin                : Fri Sep 22 2006
    author               : George Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#ifndef REQPARSER_H
#define REQPARSER_H


/**
  *@author George Wang
  */

#include <util/autobuf.h>
#include <util/autostr.h>


#define MAX_BOUNDARY_LEN  1024

#define SEC_LOC_ARGS            (1<<0)
#define SEC_LOC_ARGS_NAMES      (1<<1)
#define SEC_LOC_HEADERS         (1<<2)
#define SEC_LOC_COOKIES         (1<<3)
#define SEC_LOC_ENVS            (1<<4)
#define SEC_LOC_XML             (1<<5)

#define SEC_LOC_ARGS_GET        (1<<6)
#define SEC_LOC_ARGS_POST       (1<<7)
#define SEC_LOC_ARGS_ALL        (SEC_LOC_ARGS_GET|SEC_LOC_ARGS_POST )
#define SEC_LOC_ARGS_GET_NAMES  (1<<8)
#define SEC_LOC_ARGS_POST_NAMES (1<<9)
#define SEC_LOC_COOKIES_NAMES   (1<<10)

enum
{
    REQ_BODY_UNKNOWN,
    REQ_BODY_FORM,
    REQ_BODY_MULTIPART
};


class HttpSession;
class HttpReq;

struct KeyValuePair
{
    int keyOffset;
    int keyLen;
    int valueOffset;
    int valueLen;
};

enum
{
    MPS_INIT_BOUNDARY,
    MPS_PART_HEADER,
    MPS_PART_DATA_BOUNDARY,
    MPS_PART_DATA,
    MPS_END,
    MPS_ERROR
};

enum
{
    MPS_NODATA,
    MPS_FORM_DATA,
    MPS_FILE_DATA
};

class ReqParser
{
public:
    ReqParser();
    ~ReqParser();

    void reset();
    const char * getErrorStr() const    {   return m_pErrStr;   }
    int parseArgs( HttpReq * pReq, int scanPost );
    const char * getArgStr( int location, int &len );
    const char * getArgStr( const char * pName, int nameLen, int &len );
    const char * getReqHeader( const char * pName, int nameLen, int &len,
                             HttpReq * pReq );
    const char * getReqVar( HttpSession* pSession, int varId, int &len,
                             char * pBegin, int bufLen );
    int getArgCount() const {   return m_args;  }

    static const char * getHeaderString( int iIndex );
                             
 
    int getCookieCount() const  {   return m_iCookies;  }

    static void testQueryString();
    static void testMultipart();
    static void testAll();

    int parseCookies( const char * pCookies, int len );

private:

    int allocArgIndex( int newMax );
    int allocCookieIndex( int newMax );

    int parseArgs( const char * pStr, int len, int resume, int last );
    void resumeDecode( const char * &pStr, const char * pEnd );

    int parseQueryString( const char * pQS, int len );
    int parsePostBody( const char * pBuf, size_t size,
                            int bodyType, int resume, int last );
    int parsePostBody( HttpReq * pReq );
    int popProcessedData( char * pBegin );
    int checkBoundary( char * &pBegin, char * &pCur );
    int parseKeyValue( char * &pBegin, char * pLineEnd,
                       char *&pKey, char *&pValue, int &valLen );

    int initMutlipart( const char * pContentType, int len );
    void logParsingError( HttpReq * pReq );

    int normalisePath( int begin, int len );

    int appendArg( int beginIndex, int endIndex, int isValue );
    int appendArgKeyIndex( int begin, int len );
    int appendArgKey( const char * pStr, int len );
    int multipartParseHeader( char * pBegin, char *pLineEnd );
    int parseMutlipart( const char * pBuf, size_t size, int resume, int last );

    

private:
    AutoBuf         m_decodeBuf;

    AutoBuf         m_multipartBuf;
    AutoStr2        m_part_boundary;
    int             m_ignore_part;
    int             m_multipartState;
    int             m_iBeginOff;
    int             m_iCurOff;
    int             m_iDataOff;
    int             m_iData;


    int             m_partParsed;
    char            m_state_kv;
    char            m_last_char;
    short           m_dummy;
    int             m_beginIndex;
    int             m_args;
    int             m_maxArgs;
    KeyValuePair *  m_pArgs;
    int             m_qsBegin;
    int             m_qsArgs;
    int             m_postBegin;
    int             m_postArgs;
    
    const char *    m_pErrStr;

    int             m_iCookies;
    int             m_iMaxCookies;
    KeyValuePair *  m_pCookies;
};

#endif
