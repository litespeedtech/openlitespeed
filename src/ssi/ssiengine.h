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
#ifndef SSIENGINE_H
#define SSIENGINE_H


#include <http/httphandler.h>

class HttpConnection;
class ExprToken;
class Expression;
class SSIScript;
class SSIComponent;
class SSIRuntime;
class SubstItem;
class SSI_If;

class SSIEngine : public HttpHandler
{
public:
    SSIEngine();

    ~SSIEngine();

    virtual const char * getName() const;


    static int startExecute( HttpConnection * pConn, 
                             SSIScript * pScript );
    static int resumeExecute( HttpConnection * pConn );

    static int appendLocation( HttpConnection * pConn, const char * pLocation, int len );

    static void printError( HttpConnection * pConn, char * pError );

private:
    static int updateSSIConfig( HttpConnection * pConn, SSIComponent * pComponent, 
                            SSIRuntime * pRuntime );

    static int processEcho( HttpConnection * pConn, SSIComponent * pComponent);

    static int processExec( HttpConnection * pConn, SSIComponent * pComponent );

    static int processFileAttr( HttpConnection * pConn, SSIComponent * pComponent );

    static int processInclude( HttpConnection * pConn, SSIComponent * pComponent );

    static int processPrintEnv( HttpConnection * pConn );

    static int processSet( HttpConnection * pConn, SSIComponent * pComponent );

    static int processSubReq( HttpConnection * pConn, SubstItem *pItem );

    static int executeComponent( HttpConnection * pConn, SSIComponent * pComponent );

    static int endExecute( HttpConnection * pConn );

    static int evalOperator( HttpConnection * pConn, ExprToken * &pTok );
    static int evalExpr( HttpConnection * pConn, SubstItem *pItem );

    static int processIf( HttpConnection * pConn, SSI_If * pComponent );


};

#endif
