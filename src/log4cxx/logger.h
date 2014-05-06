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
#ifndef LOGGER_H
#define LOGGER_H



#include <stdarg.h>

#include <log4cxx/nsdefs.h>
#include <log4cxx/level.h>
#include <log4cxx/appender.h>
#include <util/autostr.h>
#include <util/duplicable.h>

#define ROOT_LOGGER_NAME "__root"

BEGIN_LOG4CXX_NS

#define __logger_log( level, format ) \
     do { \
        if ( isEnabled( level ) ) { \
            va_list  va; va_start( va, format ); \
            vlog( level, format, va ); \
            va_end( va ); \
        } \
     }while(0)

class Layout;
class Logger : public Duplicable
{
    int         m_iLevel;
    Appender *  m_pAppender;
    int         m_iAdditive;
    Layout *    m_pLayout;
    Logger *    m_pParent;

protected:
    explicit Logger( const char * pName);
    Duplicable * dup( const char * pName );
    
public: 
    ~Logger() {};
    static void init();
    
    static Logger* getRootLogger()
    {   return getLogger( ROOT_LOGGER_NAME ); }
    
    static Logger* getLogger( const char * pName );
    
    void vlog( int level, const char * format, va_list args, int no_linefeed = 0 );

    void log( int level, const char * format, ... )
    {
        __logger_log( level, format );
    }

    void vdebug( const char * format, va_list args )
    {
        vlog( Level::DEBUG, format, args );
    }

    void debug( const char * format, ... )
    {
        __logger_log( Level::DEBUG, format );
    }

    void vtrace( const char * format, va_list args )
    {
        vlog( Level::TRACE, format, args );
    }

    void trace( const char * format, ... )
    {
        __logger_log( Level::TRACE, format );
    }

    void vinfo( const char * format, va_list args )
    {
        vlog( Level::INFO, format, args );
    }

    void info( const char * format, ... )
    {
        __logger_log( Level::INFO, format );
    }

    void vnotice( const char * format, va_list args )
    {
        vlog( Level::NOTICE, format, args );
    }

    void notice( const char * format, ... )
    {
        __logger_log( Level::NOTICE, format );
    }

    void vwarn( const char * format, va_list args )
    {
        vlog( Level::WARN, format, args );
    }

    void warn( const char * format, ... )
    {
        __logger_log( Level::WARN, format );
    }

    void verror( const char * format, va_list args )
    {
        vlog( Level::ERROR, format, args );
    }
    void error( const char * format, ... )
    {
        __logger_log( Level::ERROR, format );
    }

    void vfatal( const char * format, va_list args )
    {
        vlog( Level::FATAL, format, args );
    }
    void fatal( const char * format, ... )
    {
        __logger_log( Level::FATAL, format );
    }

    void valert( const char * format, va_list args )
    {
        vlog( Level::ALERT, format, args );
    }

    void alert( const char * format, ... )
    {
        __logger_log( Level::ALERT, format );
    }

    void vcrit( const char * format, va_list args )
    {
        vlog( Level::CRIT, format, args );
    }

    void crit( const char * format, ... )
    {
        __logger_log( Level::CRIT, format );
    }

    void lograw( const char * pBuf, int len );

    bool isEnabled( int level ) const
    {   return level <= m_iLevel; }


    int getLevel() const
    {   return m_iLevel;  }

    void setLevel( int level )
    {   m_iLevel = level;  }

    void setLevel( const char * pLevel )
    {   setLevel( Level::toInt( pLevel ) ); }
    
    int getAdditivity() const
    {   return m_iAdditive;  }

    void setAdditivity( int additive )
    {   m_iAdditive = additive;   }
        
    Appender * getAppender()
    {   return m_pAppender;  }
    void setAppender( Appender * pAppender )
    {   m_pAppender = pAppender;    }

    const Layout * getLaout() const
    {   return m_pLayout;  }
    void setLayout( Layout * pLayout )
    {   m_pLayout = pLayout;    }

    void setParent( Logger * pParent )    {   m_pParent = pParent;    }
    
};


END_LOG4CXX_NS

#endif
