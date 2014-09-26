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
#include "httplogsource.h"
#include <http/accesslog.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/stderrlogger.h>
#include <util/autostr.h>
#include "util/configctx.h"
#include <util/xmlnode.h>
#include <util/gpath.h>
#include <log4cxx/appender.h>
//#include <log4cxx/appendermanager.h>

//#include <log4cxx/level.h>
//#include <unistd.h>
#include <limits.h>
#include <stdlib.h>


AutoStr  HttpLogSource::s_sDefaultAccessLogFormat;
int HttpLogSource::initAccessLog( const XmlNode *pRoot,
                                      int setDebugLevel )
{
    ConfigCtx currentCtx( "accesslog" );
    const XmlNode *p0 = pRoot->getChild( "logging", 1 );
    const XmlNode *pNode1 = p0->getChild( "accessLog" );

    if ( pNode1 == NULL )
    {
        if ( setDebugLevel )
        {
            currentCtx.log_errorMissingTag( "accessLog" );
            return -1;
        }
    }
    else
    {
        off_t rollingSize = 1024 * 1024 * 1024;
        int useServer = ConfigCtx::getCurConfigCtx()->getLongValue( pNode1, "useServer", 0, 2, 2 );
        enableAccessLog( useServer != 2 );

        if ( setDebugLevel )
        {
            const char *pFmt = pNode1->getChildValue( "logFormat" );

            if ( pFmt )
            {
                s_sDefaultAccessLogFormat.setStr( pFmt );
            }
        }

        if ( setDebugLevel || useServer == 0 )
        {
            if ( initAccessLog( pNode1, &rollingSize ) != 0 )
            {
                currentCtx.log_error( "failed to set up access log!" );
                return -1;
            }
        }

        const char *pByteLog = pNode1->getChildValue( "bytesLog" );

        if ( pByteLog )
        {
            char buf[MAX_PATH_LEN];

            if ( ConfigCtx::getCurConfigCtx()->getAbsoluteFile( buf, pByteLog ) != 0 )
            {
                currentCtx.log_errorPath( "log file",  pByteLog );
                return -1;
            }

            if ( GPath::isWritable( buf ) == false )
            {
                currentCtx.log_error( "log file is not writable - %s", buf );
                return -1;
            }

            setBytesLogFilePath( buf, rollingSize );
        }
    }

    return 0;
}
int HttpLogSource::initAccessLog( const XmlNode *pNode,
                                      off_t *pRollingSize )
{
    char buf[MAX_PATH_LEN];
    int ret = -1;
    const char *pPipeLogger = pNode->getChildValue( "pipedLogger" );

    if ( pPipeLogger )
        ret = setAccessLogFile( pPipeLogger, 1 );

    if ( ret == -1 )
    {
        ret = ConfigCtx::getCurConfigCtx()->getLogFilePath( buf, pNode );

        if ( ret )
            return ret;

        ret = setAccessLogFile( buf, 0 );
    }

    if ( ret == 0 )
    {
        AccessLog *pLog = getAccessLog();
        const char *pValue = pNode->getChildValue( "keepDays" );

        if ( pValue )
            pLog->getAppender()->setKeepDays( atoi( pValue ) );

        pLog->setLogHeaders( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "logHeaders", 0, 7, 3 ) );

        const char *pFmt = pNode->getChildValue( "logFormat" );

        if ( !pFmt )
            pFmt = s_sDefaultAccessLogFormat.c_str();

        if ( pFmt )
            if ( pLog->setCustomLog( pFmt ) == -1 )
            {
                ConfigCtx::getCurConfigCtx()->log_error( "failed to setup custom log format [%s]", pFmt );
            }

        pValue = pNode->getChildValue( "logReferer" );

        if ( pValue )
            pLog->accessLogReferer( atoi( pValue ) );

        pValue = pNode->getChildValue( "logUserAgent" );

        if ( pValue )
            pLog->accessLogAgent( atoi( pValue ) );

        pValue = pNode->getChildValue( "rollingSize" );

        if ( pValue )
        {
            off_t size = getLongValue( pValue );

            if ( ( size > 0 ) && ( size < 1024 * 1024 ) )
                size = 1024 * 1024;

            if ( pRollingSize )
                *pRollingSize = size;

            pLog->getAppender()->setRollingSize( size );
        }

        pLog->getAppender()->setCompress( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "compressArchive", 0, 1, 0 ) );
    }

    return ret;
}
int HttpLogSource::initErrorLog2( const XmlNode *pNode,
                                      int setDebugLevel )
{
    char buf[MAX_PATH_LEN];
    int ret = ConfigCtx::getCurConfigCtx()->getLogFilePath( buf, pNode );

    if ( ret )
        return ret;

    setErrorLogFile( buf );

    const char *pValue = ConfigCtx::getCurConfigCtx()->getTag( pNode, "logLevel" );

    if ( pValue != NULL )
        setLogLevel( pValue );

    off_t rollSize =
        ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "rollingSize", 0,
                                       INT_MAX, 1024 * 10240 );
    int days = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "keepDays", 0,
               LLONG_MAX, 30 );
    setErrorLogRollingSize( rollSize, days );

    if ( setDebugLevel )
    {
        pValue = pNode->getChildValue( "debugLevel" );

        if ( pValue != NULL )
            HttpLog::setDebugLevel( atoi( pValue ) );

        if ( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "enableStderrLog", 0, 1, 1 ) )
        {
            char *p = strrchr( buf, '/' );

            if ( p )
            {
                strcpy( p + 1, "stderr.log" );
                HttpGlobals::getStdErrLogger()->setLogFileName( buf );
                HttpGlobals::getStdErrLogger()->getAppender()->
                setRollingSize( rollSize );
            }
        }
        else
        {
            HttpGlobals::getStdErrLogger()->setLogFileName( NULL );
        }
    }

    return 0;
}
int HttpLogSource::initErrorLog( const XmlNode *pRoot,
                                     int setDebugLevel )
{
    ConfigCtx currentCtx( "errorLog" );
    int confType = 0;
    const XmlNode *p0 = pRoot->getChild( "logging" );

    if ( !p0 )
    {
        confType = 1;
        p0 = pRoot;
    }

    const XmlNode *pNode1 = NULL;

    if ( confType == 0 )
        pNode1 = p0->getChild( "log" );
    else
        pNode1 = p0->getChild( "errorLog" );

    if ( pNode1 == NULL )
    {
        if ( setDebugLevel )
        {
            currentCtx.log_errorMissingTag( "errorLog" );
            return -1;
        }
    }
    else
    {
        int useServer = ConfigCtx::getCurConfigCtx()->getLongValue( pNode1, "useServer", 0, 1, 1 );

        if ( setDebugLevel || useServer == 0 )
        {
            if ( initErrorLog2( pNode1, setDebugLevel ) != 0 )
            {
                currentCtx.log_error( "failed to set up error log!" );
                return -1;
            }
        }
    }

    return 0;
}