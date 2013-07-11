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
#include "httpstatuscode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/ssnprintf.h>


StatusCode::StatusCode( int code, const char * pStatus, 
                    const char * message )
{
    memset( this, 0, sizeof( StatusCode ));
    if ( pStatus )
    {
        m_status = pStatus;
        status_size = strlen( pStatus );
        if ( message )
        {
            char achBuf[2048];
            char * p = achBuf;
            char * pEnd = p + 2048;
            p += safe_snprintf( p, pEnd - p, "<html>\n<head><title>%s</title></head>\n"
                              "<body><h1>%s</h1>\n",
                pStatus, pStatus );
            p += safe_snprintf( p, pEnd - p, "%s", message );
            if (( code >= SC_403 )||( code <= SC_404 ))
                p += safe_snprintf( p, pEnd - p,
                    "<hr />\n"
                    "Powered By <a href='http://www.litespeedtech.com'>LiteSpeed Web Server</a><br />\n"
                    "<font face=\"Verdana, Arial, Helvetica\" size=-1>Lite Speed Technologies is not "
                    "responsible for administration and contents of this web site!</font>" );
            
            p += safe_snprintf( p, pEnd -p, "</body></html>\n" );

            m_iBodySize = p - achBuf;
            m_pHeaderBody = (char *)malloc( m_iBodySize + 160 );
            if ( !m_pHeaderBody )
                m_iBodySize =0;
            else
            {
                int n;
                if ( code >= SC_307 )
                {
                    n = safe_snprintf( m_pHeaderBody, 159,
                        "Cache-Control: private, no-cache, max-age=0\r\n"
                        "Pragma: no-cache\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %d\r\n",
                        m_iBodySize );
                    
                }
                else
                {
                    n = safe_snprintf( m_pHeaderBody, 79,
                        "Content-Type: text/html\r\n"
                        "Content-Length: %d\r\n",
                        m_iBodySize );
                }
                m_iHeaderSize = n;
                memcpy( m_pHeaderBody + n, achBuf, m_iBodySize + 1 );
            }
        }
    }
}

StatusCode::~StatusCode()
{
    if ( m_pHeaderBody )
        free( m_pHeaderBody );
}

int HttpStatusCode::codeToIndex( const char * code )
{
    char ch = *code++;
    if (( ch < '0' ) || ( ch > '5'))
        return -1;
    int offset = ch - '0';
    int index;
    ch = *code++;
    index = 10 * (ch - '0' );
    ch = *code;
    if (( ch <'0' ) ||( ch > '9' ))
        return -1;
    index += ch - '0';
    if (index < s_codeToIndex[offset + 1] - s_codeToIndex[offset] )
        return s_codeToIndex[offset] + index;
    else
        return -1;
}

