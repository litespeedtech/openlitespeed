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
#ifndef SSIRUNTIME_H
#define SSIRUNTIME_H

#include <util/pcregex.h>
#include <ssi/ssiconfig.h>

#define SSI_STACK_SIZE 10

class SSIScript;


#define SSI_REQ_CGI 1
#define SSI_REQ_CMD 2

class SSIRuntime{
public:
    SSIRuntime();

    ~SSIRuntime();

    void init()
    {   m_pCurScript = &m_stack[-1];   }

    int  push( SSIScript *pScript )
    {
        if ( m_pCurScript < &m_stack[SSI_STACK_SIZE-1] )
        {
            *(++m_pCurScript) = pScript;
            return 0;
        }
        else
            return -1;
    }

    void pop()
    {   if ( m_pCurScript > &m_stack[-1] )
            -- m_pCurScript;
    }
    int  full()
    {   return m_pCurScript >= &m_stack[SSI_STACK_SIZE];    }

    int  done()
    {   return m_pCurScript == &m_stack[-1];    }

    int initConfig( SSIConfig * pConfig );

    SSIScript * getCurrentScript() const
    {   return *m_pCurScript;   }
    
    SSIConfig * getConfig() 
    {   return &m_config;       }
    
    const RegexResult * getRegexResult() const 
    {   return &m_regexResult;      }

    RegexResult * getRegexResult()
    {   return &m_regexResult;      }

    int execRegex( Pcregex * pReg, const char * pSubj, int len );

    void clearFlag()    {   m_flag = 0;     }
    void requireCGI()   {   m_flag = SSI_REQ_CGI;   }
    void requireCmd()   {   m_flag = SSI_REQ_CMD;   }
    int  isCGIRequired() const {   return m_flag > 0;      }

    void savePathInfo( int off, int len, int redirects )
    {
        m_stkPathInfoOff[ m_pCurScript - m_stack ] = off;
        m_stkPathInfoLen[ m_pCurScript - m_stack ] = len;
        m_stkRedirectIdx[ m_pCurScript - m_stack ] = redirects;
    }

    void restorePathInfo( int &off, int &len, short int &redirects )
    {
        off = m_stkPathInfoOff[ m_pCurScript - m_stack ];
        len = m_stkPathInfoLen[ m_pCurScript - m_stack ];
        redirects = m_stkRedirectIdx[ m_pCurScript - m_stack ];
    }


private:
    SSIScript **    m_pCurScript;
    SSIScript *     m_stack[SSI_STACK_SIZE];
    int             m_stkPathInfoOff[SSI_STACK_SIZE];
    int             m_stkPathInfoLen[SSI_STACK_SIZE];
    short int       m_stkRedirectIdx[SSI_STACK_SIZE];
    SSIConfig       m_config;
    AutoStr2        m_strRegex;
    RegexResult     m_regexResult;
    int             m_flag;

};

#endif
