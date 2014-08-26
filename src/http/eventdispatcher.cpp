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
#include "eventdispatcher.h"
#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <http/adns.h>
#include <util/datetime.h>
#include <http/httpdefs.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpsignals.h>
#include <http/connlimitctrl.h>
#include <main/httpserver.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <lsiapi/modulemanager.h>

int highPriorityTask();


EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
    release();
}

int EventDispatcher::init( const char * pType )
{
    if ( HttpGlobals::getMultiplexer() )
        return 0;
    HttpGlobals::s_iMultiplexerType = MultiplexerFactory::getType( pType );
    Multiplexer * pMultiplexer =
            MultiplexerFactory::get( HttpGlobals::s_iMultiplexerType );
    if ( pMultiplexer != NULL )
    {
        if ( !pMultiplexer->init( DEFAULT_INIT_POLL_SIZE) )
        {
            HttpGlobals::setMultiplexer( pMultiplexer );
            pMultiplexer->setPriHandler( highPriorityTask );
            return 0;
        }
    }
    return -1;
}

int EventDispatcher::reinit( )
{
    if ( !HttpGlobals::getMultiplexer() )
        return -1;
    MultiplexerFactory::recycle( HttpGlobals::getMultiplexer() );
    Multiplexer * pMultiplexer =
            MultiplexerFactory::get( HttpGlobals::s_iMultiplexerType );
    if ( pMultiplexer != NULL )
    {
        if ( !pMultiplexer->init( DEFAULT_INIT_POLL_SIZE) )
        {
            HttpGlobals::setMultiplexer( pMultiplexer );
            pMultiplexer->setPriHandler( highPriorityTask );
            return 0;
        }
    }
    return -1;
}

void EventDispatcher::release()
{
    MultiplexerFactory::recycle( HttpGlobals::getMultiplexer() );
}

int EventDispatcher::stop()
{
    HttpSignals::setSigStop();
    return 0;
}

static void processTimer()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );

    //FIXME: debug code
    //LOG_D(( "processTimer()" ));
    
    DateTime::s_curTime = tv.tv_sec;
    DateTime::s_curTimeUs = tv.tv_usec;
    HttpGlobals::s_tmPrevToken = HttpGlobals::s_tmToken;
    HttpGlobals::s_tmToken = tv.tv_usec / ( 1000000 / TIMER_PRECISION );
    if ( HttpGlobals::s_tmToken < HttpGlobals::s_tmPrevToken )
        HttpServer::getInstance().onTimer();
    HttpGlobals::getMultiplexer()->timerExecute();
}

int highPriorityTask()
{
    if ( HttpSignals::gotSigAlarm() )
    {
        HttpSignals::resetSigAlarm();
        processTimer();
    }
    HttpGlobals::getConnLimitCtrl()->tryAcceptNewConn();
    return 0;
}

static void startTimer( )
{
    struct itimerval tmv;
    memset( &tmv, 0, sizeof( struct itimerval ) );
    tmv.it_interval.tv_usec = 1000000 / TIMER_PRECISION;
    gettimeofday( &tmv.it_value, NULL );
    tmv.it_value.tv_sec = 0;
    HttpGlobals::s_tmPrevToken = HttpGlobals::s_tmToken = 
            tmv.it_value.tv_usec / tmv.it_interval.tv_usec;
    tmv.it_value.tv_usec = tmv.it_interval.tv_usec -
            tmv.it_value.tv_usec % tmv.it_interval.tv_usec;
    setitimer (ITIMER_REAL, &tmv, NULL);
}

/*
#define MLTPLX_TIMEOUT 1000
int EventDispatcher::run()
{
    register int ret;
    register int sigEvent;
    startTimer();
    while( true )
    {
        ret = HttpGlobals::getMultiplexer()->waitAndProcessEvents(
                    MLTPLX_TIMEOUT );
        if (( ret == -1 )&& errno )
        {
            if (!((errno == EINTR )||(errno == EAGAIN)))
            {
                LOG_ERR(( "Unexpected error inside event loop: %s", strerror( errno ) ));
                return 1;
            }
        }
        if ( (sigEvent = HttpSignals::gotEvent()) )
        {
            HttpSignals::resetEvents();
            if ( sigEvent & HS_ALARM )
            {
                processTimer();
                HttpGlobals::getConnLimitCtrl()->checkWaterMark();

#ifdef  USE_CARES
                if ( HttpGlobals::s_dnsLookup )
                    Adns::process();
#endif
            }
            if ( sigEvent & HS_USR2 )
            {
                HttpLog::toggleDebugLog();
            }
            if ( sigEvent & HS_STOP )
                break;
        }
    }
    return 0;
}
*/

static inline void processTimerNew()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );

    //FIXME: debug code
    //int n = tv.tv_usec / ( 1000000 / TIMER_PRECISION );
    
    DateTime::s_curTime = tv.tv_sec;
    DateTime::s_curTimeUs = tv.tv_usec;
    HttpGlobals::s_tmPrevToken = HttpGlobals::s_tmToken;
    HttpGlobals::s_tmToken = tv.tv_usec / ( 1000000 / TIMER_PRECISION );
    if ( HttpGlobals::s_tmToken != HttpGlobals::s_tmPrevToken )
    {   
        if ( HttpGlobals::s_tmToken < HttpGlobals::s_tmPrevToken )
        {
            if ( getppid() == 1 )
            {   
                HttpSignals::setSigStop();
            }
            HttpServer::getInstance().onTimer();
        }
        HttpGlobals::getMultiplexer()->timerExecute();
        HttpGlobals::getConnLimitCtrl()->checkWaterMark();
        //LOG_D(( "processTimer()" ));
    }
    
    ModuleManager::getInstance().OnTimer100msec();
}


#define MLTPLX_TIMEOUT 100
int EventDispatcher::run()
{
    register int ret;
    register int sigEvent;
    while( true )
    {
        ret = HttpGlobals::getMultiplexer()->waitAndProcessEvents(
                    MLTPLX_TIMEOUT );
        if (( ret == -1 )&& errno )
        {
            if (!((errno == EINTR )||(errno == EAGAIN)))
            {
                LOG_ERR(( "Unexpected error inside event loop: %s", strerror( errno ) ));
                return 1;
            }
        }
        processTimerNew();
        if ( (sigEvent = HttpSignals::gotEvent()) )
        {
            HttpSignals::resetEvents();
            if ( sigEvent & HS_USR2 )
            {
                HttpLog::toggleDebugLog();
                ModuleManager::updateDebugLevel();

            }
            if ( sigEvent & HS_CHILD )
            {
                HttpServer::cleanPid();   
            }
            if ( sigEvent & HS_STOP )
                break;
        }
    }
    return 0;
}


int EventDispatcher::linger( int timeout )
{
    register int ret;
    long endTime = time(NULL) + timeout;
    HttpGlobals::getMultiplexer()->setPriHandler( NULL );
    startTimer();
    while(( time( NULL ) < endTime )&&
          ( HttpGlobals::getConnLimitCtrl()->getMaxConns() >
            HttpGlobals::getConnLimitCtrl()->availConn() ))
    {
        ret = HttpGlobals::getMultiplexer()->waitAndProcessEvents(
                    MLTPLX_TIMEOUT );
        if ( ret == -1 )
        {
            if (!((errno == EINTR )||(errno == EAGAIN)))
            {
                LOG_ERR(( "Unexpected error inside event loop: %s", strerror( errno ) ));
                return 1;
            }
        }
        if ( HttpSignals::gotSigAlarm() )
        {
            HttpSignals::resetEvents();
            processTimer();
        }
        if ( HttpSignals::gotSigChild() )
        {
            HttpServer::cleanPid();
        }

    }
    return 0;
}


