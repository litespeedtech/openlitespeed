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
                        "<html style=\"height:100%%\">\n<head><title>%s</title></head>\n"
                        "<body style=\"color: #444; margin:0;font: normal 14px/20px Arial, Helvetica, sans-serif; height:100%%; background-color: #fff;"
                        "\">\n"
                        "<div style=\"height:auto; min-height:100%%; \">"
                        "     <div style=\"text-align: center; width:800px; margin-left: -400px; position:absolute; top: 30%%; left:50%%;"
                        "\">\n"
                        "        <h1 style=\"margin:0; font-size:150px; line-height:150px; font-weight:bold;\">%c%c%c</h1>\n"
                        "<h2 style=\"margin-top:20px;font-size: 30px;\">%s</h2>\n"                        
                        "<p>%s</p>\n"      
                        "</div></div>"
                        ,
                pStatus, pStatus[1], pStatus[2], pStatus[3], &pStatus[5], message ? message : "" );
            //p += safe_snprintf( p, pEnd - p, "%s", message );
            if (( code >= SC_403 )||( code <= SC_404 ))
                p += snprintf( p, pEnd - p,
                        "<div style=\"color:#f0f0f0; font-size:12px;margin:auto;padding:0px 30px 0px 30px;"
                        "position:relative;clear:both;height:100px;margin-top:-101px;background-color:#474747;"
                        "border-top: 1px solid rgba(0,0,0,0.15);box-shadow: 0 1px 0 rgba(255, 255, 255, 0.3) inset;\">\n"
                        "<br>Proudly powered by  <a style=\"color:#fff;\" href=\"http://www.litespeedtech.com/error-page\">LiteSpeed Web Server</a>"
                        "<p>Please be advised that LiteSpeed Technologies Inc. is not a web hosting"
                        " company and, as such, has no control over content found on this site.</p></div>"
                         );
            
            p += safe_snprintf( p, pEnd -p, "</body></html>\n" );

            m_iBodySize = p - achBuf;
            m_pHeaderBody = (char *)malloc( m_iBodySize + 1 );
            if ( !m_pHeaderBody )
                m_iBodySize =0;
            else
                memcpy( m_pHeaderBody, achBuf, m_iBodySize + 1 );
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

