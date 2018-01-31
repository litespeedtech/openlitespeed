/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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

#include <lsdef.h>
#include <util/pcregex.h>
#include <ssi/ssiconfig.h>
#include <ssi/ssiscript.h>

#include <lsr/ls_str.h>

#define SSI_STACK_SIZE 10

class HttpReq;


#define SSI_REQ_CGI 1
#define SSI_REQ_CMD 2


class SsiStack
{
public:
    explicit SsiStack(SsiStack *pPrevious, int depth)
        : m_pCurBlock(NULL)
        , m_pCurComponent(NULL)
        , m_pScript(NULL)
        , m_flag(0)
        , m_iDepth(depth)
        , m_pPrevious(pPrevious)
    {}

    const SsiScript *getScript() const
    {   return m_pScript;       }

    const SsiComponent *getCurrentComponent() const
    {   return m_pCurComponent;    }
    const SsiBlock *getCurrentBlock() const
    {   return m_pCurBlock;    }

    void  setScript(const SsiScript *p)
    {
        m_pScript = p;
        m_pCurBlock = p->getMainBlock();
        m_pCurComponent = m_pCurBlock->getFirstComponent();
    }

    void  setCurrentBlock(const SsiBlock *pBlock);
    const SsiComponent *nextComponentOfCurScript();

    void clearFlag()    {   m_flag = 0;     }
    void requireCGI()   {   m_flag = SSI_REQ_CGI;   }
    void requireCmd()   {   m_flag = SSI_REQ_CMD;   }
    int  isCGIRequired() const {   return m_flag > 0;      }

    SsiStack *getPrevious() const   {   return m_pPrevious; }

    char getDepth() const           {   return m_iDepth;    }

private:
    const SsiBlock       *m_pCurBlock;
    const SsiComponent   *m_pCurComponent;
    const SsiScript      *m_pScript;
    short                 m_flag;
    char                  m_iDepth;
    SsiStack             *m_pPrevious;
};


class SsiRuntime
{
public:
    SsiRuntime();

    ~SsiRuntime();


    int initConfig(SsiConfig *pConfig);

    SsiConfig *getConfig()
    {   return &m_config;       }

    const RegexResult *getRegexResult() const
    {   return &m_regexResult;      }

    RegexResult *getRegexResult()
    {   return &m_regexResult;      }

    int execRegex(Pcregex *pReg, const char *pSubj, int len);

    void setMainReq(HttpReq *pReq)  {   m_pMainReq = pReq;  }
    HttpReq *getMainReq() const     {   return m_pMainReq;  }
    int setVar(const char *pKey, int keyLen, const char *pValue, int valLen);

    int  isInSsiEngine() const      {   return m_iInSsiEngine;  }
    void incInSsiEngine()           {   ++m_iInSsiEngine;       }
    void decInSsiEngine()           {   --m_iInSsiEngine;       }

private:
    SsiConfig       m_config;
    AutoStr2        m_strRegex;
    RegexResult     m_regexResult;
    HttpReq        *m_pMainReq;
    int             m_iInSsiEngine;



    LS_NO_COPY_ASSIGN(SsiRuntime);
};

#endif
