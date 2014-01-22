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
#ifndef HTTPRESP_H
#define HTTPRESP_H


#include <util/autobuf.h>
#include <http/httpstatusline.h>
#include <util/iovec.h>
#include <http/httprespheaders.h>
#include <http/httpvhost.h>
#define RANGE_HEADER_LEN    22

class HttpReq;

class HttpResp
{
public:
    static char     s_sCommonHeaders[66];
    static char     s_sKeepAliveHeader[25];
    static char     s_sConnCloseHeader[20];
    static char     s_chunked[29];
    static char     s_sGzipEncodingHeader[48];

    static header_st       m_commonHeaders[3];
    static header_st       m_gzipHeaders[2];
    static header_st       m_keepaliveHeader;
    static header_st       m_chunkedHeader;
    static header_st       m_concloseHeader;
    static int             m_commonHeadersCount;
    

private:    
    IOVec           m_iovec;
    HttpRespHeaders         m_respHeaders;
    

    long            m_lEntityLength;
    long            m_lEntityFinished;
    int             m_iHeaderLeft;
    int             m_iHeaderTotalLen;
    short           m_iSSL;
    short           m_iLogAccess;
    
    HttpResp( const HttpResp& rhs ) {}
    void operator=( const HttpResp& rhs ) {}

    
    void addWWWAuthHeader( const HttpReq * pReq);
public:
    explicit HttpResp();
    ~HttpResp();

    void reset(RespHeader::FORMAT format);
//    {
//        m_outputBuf.clear();
//        m_iovec.clear();
//        memset( &m_iGotDate, 0,
//                (char *)((&m_iHeaderLeft) + 1) - (char*)&m_iGotDate );
//    }
    void resetHeaderLen()
    {   m_iHeaderTotalLen = 0; }    

    int appendHeader( const char * pName, int nameLen,
                        const char * pValue, int valLen );
    void addLocationHeader( const HttpReq * pReq);

    void prepareHeaders( const HttpReq * pReq, int rangeHeaderLen = 0 );
    void appendContentLenHeader();

    HttpRespHeaders& getRespHeaders()
    {   return m_respHeaders;  }

    void setContentLen( long len )      {   m_lEntityLength = len;  }
    long getContentLen() const          {   return m_lEntityLength; }

    void written( long len )            {   m_lEntityFinished += len;    }
    long getBodySent() const            {   return m_lEntityFinished; }

    int  replySent() const              {   return m_iHeaderTotalLen > m_iHeaderLeft;   }

    void needLogAccess( short n )       {   m_iLogAccess = n;       }
    short shouldLogAccess() const       {   return m_iLogAccess;    }
    
    bool isChunked() const
    {   return (m_lEntityLength == -1); }

    static void buildCommonHeaders();
    static void updateDateHeader();

    const char * getProtocol() const;
    int   getProtocolLen() const;

    void setSSL( int ssl )
    {   m_iSSL = (ssl != 0 );   }

    void finalizeHeader( int ver, int code, const HttpVHost *vhost );
    
    IOVec& getIov()    {   return m_iovec;  }
    void parseAdd( const char * pBuf, int len )
    {    m_respHeaders.parseAdd(pBuf, len, RespHeader::APPEND );    }
    
    void appendExtra( const char * pBuf, int len )
    {
        m_respHeaders.parseAdd(pBuf, len, RespHeader::APPEND );
        m_iHeaderTotalLen += len; 
        m_iHeaderLeft += len; 
    }

    void addGzipEncodingHeader()
    {
        m_respHeaders.add( HttpResp::m_gzipHeaders, 2);
    }
    
    void appendChunked()
    {
        m_respHeaders.add( &HttpResp::m_chunkedHeader, 1);
    }

    int& getHeaderLeft()            {   return m_iHeaderLeft;   }
    void setHeaderLeft( int len )   {   m_iHeaderLeft = len;    }

    long getTotalLen() const    {   return m_lEntityFinished + m_iHeaderTotalLen;   }
    int  getHeaderSent() const      {   return m_iHeaderTotalLen - m_iHeaderLeft;   }
    int  getHeaderTotal() const     {   return m_iHeaderTotalLen;   }    
    const char * getContentTypeHeader(int &len )  {    return m_respHeaders.getContentTypeHeader(len);  }
       
    int  appendLastMod( long tmMod );
    int addCookie( const char * pName, const char * pVal,
                 const char * path, const char * domain, int expires,
                 int secure, int httponly );  
        
};

#endif
