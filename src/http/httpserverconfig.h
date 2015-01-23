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
#ifndef HTTPSERVERCONFIG_H
#define HTTPSERVERCONFIG_H



#include <inttypes.h>
#include <sys/types.h>

class HttpServerConfig 
{
    static HttpServerConfig s_config;
        
    int32_t     m_iMaxURLLen;
    int32_t     m_iMaxHeaderBufLen;
    int64_t     m_iMaxReqBodyLen;
    int64_t     m_iMaxDynRespLen;
    int32_t     m_iMaxDynRespHeaderLen;
    int16_t     m_iMaxKeepAliveRequests;
    int8_t      m_iSmartKeepAlive;
    int8_t      m_iUseSendfile;
    int8_t      m_iFollowSymLink;
    int8_t      m_iGzipCompress;
    int8_t      m_iDynGzipCompress;
    int8_t      m_iCompressLevel;
    
    int32_t     m_iCheckDeniedSymLink;
    
    int32_t     m_iMaxFcgiInstances;
    
    int32_t     m_iMaxTempFileSize;
    int32_t     m_iConnTimeout;
    int32_t     m_iKeepAliveTimeout;
    int32_t     m_iForbiddenBits;
    int32_t     m_iRequiredBits;
    int32_t     m_iScriptForbiddenBits;
    int32_t     m_iDirForbiddenBits;
    int32_t     m_iRestartTimeout;

    HttpServerConfig( const HttpServerConfig& rhs );
public:
    HttpServerConfig();
    ~HttpServerConfig();
    static HttpServerConfig& getInstance()
    {   return s_config;        }
    
    void setConnTimeOut( int32_t timeout )  {   m_iConnTimeout = timeout;   }
    int32_t getConnTimeout() const          {   return m_iConnTimeout;      }

    void setRestartTimeOut( int32_t timeout )  {   m_iRestartTimeout = timeout;   }
    int32_t getRestartTimeout() const          {   return m_iRestartTimeout;      }
    
    void setKeepAliveTimeout( int32_t t )   {   m_iKeepAliveTimeout = t;    }
    int32_t getKeepAliveTimeout() const     {   return m_iKeepAliveTimeout; }
        
    void setMaxKeepAliveRequests( int16_t max )
    {   m_iMaxKeepAliveRequests = max - 1;   }
    int16_t getMaxKeepAliveRequests() const {   return m_iMaxKeepAliveRequests; }

    void setSmartKeepAlive( int8_t val )    {   m_iSmartKeepAlive = val;    }
    int8_t getSmartKeepAlive() const        {   return m_iSmartKeepAlive;   }

    void setUseSendfile( int8_t val )       {   m_iUseSendfile = val;       }
    int8_t getUseSendfile() const           {   return m_iUseSendfile;      }
    
    void setFollowSymLink( int32_t follow ) {   m_iFollowSymLink = follow;  }
    int8_t getFollowSymLink() const         {   return m_iFollowSymLink;    }

    void setGzipCompress( int32_t compress )
    {   m_iGzipCompress = compress;   }
    int8_t  getGzipCompress() const         {   return m_iGzipCompress;     }

    void setDynGzipCompress( int32_t compress )
    {   m_iDynGzipCompress = compress;   }
    int8_t  getDynGzipCompress() const      {   return m_iDynGzipCompress;  }
    
    void setCompressLevel( int32_t compress )
    {   if (compress < 1 )     compress = 1;
        if ( compress > 9 )    compress = 9;
        m_iCompressLevel = compress;
    }
    int8_t  getCompressLevel() const        {   return m_iCompressLevel;    }
    

    void setDebugLevel( int32_t level );


    void setMaxURLLen( int len );
    void setMaxHeaderBufLen( int len );
    void setMaxReqBodyLen( int64_t len );
    void setMaxDynRespLen( int64_t len );
    void setMaxDynRespHeaderLen( int len );
    
    int32_t getMaxURLLen() const            {   return m_iMaxURLLen;        }
    int32_t getMaxHeaderBufLen() const      {   return m_iMaxHeaderBufLen;  }
    int32_t getMaxReqBodyLen() const        {   return m_iMaxReqBodyLen;    }
    int32_t getMaxDynRespLen() const        {   return m_iMaxDynRespLen;    }
    int32_t getMaxDynRespHeaderLen() const  {   return m_iMaxDynRespHeaderLen;  }

    int32_t getMaxFcgiInstances() const     {   return m_iMaxFcgiInstances; }

    void setMaxFcgiInstances( int i )       {   m_iMaxFcgiInstances = i;    }
    void setMaxCgiInstances( int i );

    int32_t getMaxTempFileSize() const      {   return m_iMaxTempFileSize;  }
    void setMaxTempFilesize(int32_t sz)     {   m_iMaxTempFileSize = sz;    }

    int32_t getForbiddenBits() const        {   return m_iForbiddenBits;    }
    void setForbiddenBits( int32_t bits)    {   m_iForbiddenBits = bits;    }
    
    int32_t getRequiredBits() const         {   return m_iRequiredBits;     }
    void setRequiredBits( int32_t bits)     {   m_iRequiredBits = bits;     }

    void    checkDeniedSymLink( int32_t c)  {   m_iCheckDeniedSymLink = c;      }
    int32_t checkDeniedSymLink()            {   return m_iCheckDeniedSymLink;   }
    void    setScriptForbiddenBits( int32_t bit ) { m_iScriptForbiddenBits = bit;   }
    int32_t getScriptForbiddenBits() const  {   return m_iScriptForbiddenBits;  }    
    void    setDirForbiddenBits( int32_t bit )  { m_iDirForbiddenBits = bit;    }
    int32_t getDirForbiddenBits() const     {   return m_iDirForbiddenBits;     }    
};

#endif
