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
            char achBuf[4096];
            char * p = achBuf;
            char * pEnd = p + 4096;
            p += safe_snprintf( p, pEnd - p, 
                        "<!DOCTYPE html>\n"
                        "<html>\n<head><title>%s</title></head>\n"
                        
                        "<body style=\"color: #555; margin:0;font: normal 14px/20px Arial, Helvetica, sans-serif; background-color: #474747;\">\n"
                        "<div style=\"background-color:#fff;\"><div style=\"background-color:rgba(0,0,0,0.1);padding:100px 15px 0 15px;\">\n"
                        "    <div style=\"min-width:200px;padding:20px 20px 80px 20px;margin:0 30px;text-align: center;"
                        "-webkit-box-shadow: 0 -30px 30px -4px #999;-moz-box-shadow: 0 -30px 30px -4px #999;box-shadow: 0 -30px 30px -4px #999;\">\n"
                        "        <h1 style=\"font-size:200px;font-weight:bold;\">%c%c%c</h1>\n"
                        "<h2 style=\"margin: 20px 0 0 0;font-size: 48px;  line-height: 48px;\">%s</h2>\n"                        
                        "<div style=\"margin: 20px auto 0 auto;\">\n"
                        "<p>If you wish to report this error page, please contact the website owner.</p>\n"
                        "<p>If you are the website owner: It is possible you have reached this page because the server has been misconfigured.\n" 
                        "If you were not expecting to see this page, please <strong>contact your hosting provider</strong>. \n"
                        "If you do not know who your hosting provider is, you may be able to look it up by Googling \"whois\""
                        "and your domain name. This will tell you who your IP is registered to.</p>"       
                        "</div></div></div></div>"
                        ,
                pStatus, pStatus[1], pStatus[2], pStatus[3], &pStatus[5] );
            //p += safe_snprintf( p, pEnd - p, "%s", message );
            if (( code >= SC_403 )||( code <= SC_404 ))
                p += snprintf( p, pEnd - p,
                        "<div style=\"color:#f0f0f0; font-size:12px;margin:auto;padding:30px;"
                        "border-top: 1px solid rgba(0,0,0,0.15);box-shadow: 0 1px 0 rgba(255, 255, 255, 0.1) inset;\">\n"
                        "<p>Note: Although this site is running LiteSpeed Web Server, it almost certainly"
                        " has no other connection to LiteSpeed Technologies Inc. Please do not send email"
                        " about this site or its contents to LiteSpeed Technologies Inc.</p>\n"
                        "<p><span style=\"font-style: oblique;\">About LiteSpeed Web Server:</span>"
                        "<br>LiteSpeed Web Server is a high-performance Apache drop-in replacement."
                        " Web hosts use LiteSpeed Web Server to improve performance and stability and"
                        " lower operating costs. If you would like to learn more about how LiteSpeed"
                        " Web Server helps web hosts provide a better Internet experience, please visit"
                        " <a style=\"color:#fff;\" href=\"http://www.litespeedtech.com/error-page\">here</a>.</p>"
                        "<p>Please be advised that LiteSpeed Technologies Inc. is not a web hosting"
                        " company and, as such, has no control over content found on this site.</p></div>"
                        );
            
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

