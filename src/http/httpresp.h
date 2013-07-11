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

#define RANGE_HEADER_LEN    22

class HttpReq;

class HttpResp
{
private:
    static HttpRespHeaders     s_CommonHeaders[3];
    static char     s_sKeepAliveHeader[25];
    static char     s_sConnCloseHeader[20];
    static char     s_chunked[29];
    static char     s_sGzipEncodingHeader[48];

    IOVec           m_iovec;
    HttpRespHeaders         m_outputBuf;
    

    long            m_lEntityLength;
    long            m_lEntityFinished;
    int             m_iHeaderLeft;
    int             m_iSetCookieOffset;
    int             m_iSetCookieLen;
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

    HttpRespHeaders& getHeaders()
    {   return m_outputBuf;  }

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
    static void updateDateHeader(int index = 0);    //0 means all, and 1, 2 and 3 mean the (index -1) one

    const char * getProtocol() const;
    int   getProtocolLen() const;

    void setSSL( int ssl )
    {   m_iSSL = (ssl != 0 );   }

    void finalizeHeader( int ver, int code);
    
    IOVec& getIov()    {   return m_iovec;  }
    void iovAppend( const char * pBuf, int len );
    
    void appendExtra( const char * pBuf, int len )
    {   //m_iovec.append( pBuf, len );
        m_outputBuf.addNoCheckExptSpdy(pBuf, len );
        m_iHeaderTotalLen += len; m_iHeaderLeft += len; }

    void addGzipEncodingHeader()
    {
        iovAppend( s_sGzipEncodingHeader, 47 );
    }
    
    void appendChunked()
    {
        iovAppend( s_chunked, sizeof( s_chunked )- 1 );
    }

    int& getHeaderLeft()            {   return m_iHeaderLeft;   }
    void setHeaderLeft( int len )   {   m_iHeaderLeft = len;    }

    long getTotalLen() const    {   return m_lEntityFinished + m_iHeaderTotalLen;   }
    int  getHeaderSent() const      {   return m_iHeaderTotalLen - m_iHeaderLeft;   }
    int  getHeaderTotal() const     {   return m_iHeaderTotalLen;   }    
    const char * getContentTypeHeader(int &len )  {    return m_outputBuf.getContentTypeHeader(len);  }
       
    int  appendLastMod( long tmMod );
    int addCookie( const char * pName, const char * pVal,
                 const char * path, const char * domain, int expires,
                 int secure, int httponly );  
        
};

#endif
