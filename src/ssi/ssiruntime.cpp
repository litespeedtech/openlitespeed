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
#include "ssiruntime.h"

#include <http/httpreq.h>

SsiRuntime::SsiRuntime()
    : m_pMainReq(NULL)
    , m_iInSsiEngine(0)
{
}


void SsiStack::setCurrentBlock(const SsiBlock *pBlock)
{
    m_pCurBlock = pBlock;
    m_pCurComponent = pBlock;
}


const SsiComponent *SsiStack::nextComponentOfCurScript()
{
    if (!m_pCurComponent)
        return NULL;
    if (m_pCurBlock == m_pCurComponent)
        m_pCurComponent = m_pCurBlock->getFirstComponent();

    else
        m_pCurComponent = (SsiComponent *)m_pCurComponent->next();
    while (!m_pCurComponent)
    {
        if (!m_pCurBlock)
            break;
        if (m_pCurBlock->isConditional())
        {
            //pop block stack one more level
            m_pCurBlock = m_pCurBlock->getParentBlock();
            if (m_pCurBlock && ! m_pCurBlock->isWrapper())
            {
                //something is wrong
            }

        }
        m_pCurComponent = m_pCurBlock;
        if (m_pCurComponent)
            m_pCurComponent = (SsiComponent *)m_pCurComponent->next();
        if (m_pCurBlock)
            m_pCurBlock = m_pCurBlock->getParentBlock();
    }
    return m_pCurComponent;

}


SsiRuntime::~SsiRuntime()
{
}


int SsiRuntime::initConfig(SsiConfig *pConfig)
{
    m_config.copy(pConfig);
    if (m_config.getTimeFmt()->c_str() == NULL)
        m_config.setTimeFmt("%A, %d-%b-%Y %H:%M:%S %Z", 24);
//     if (m_config.getErrMsg()->c_str() == NULL)
//         m_config.setErrMsg("[an error occurred while processing this directive]",
//                            50);
    if (m_config.getEchoMsg()->c_str() == NULL)
        m_config.setEchoMsg("(none)", 6);
    return 0;
}


int SsiRuntime::execRegex(Pcregex *pReg, const char *pSubj, int len)
{
    if ((!pReg) || (!pSubj) || (len < 0) || (len >= 40960))
        return LS_FAIL;
    m_strRegex.setStr(pSubj, len);
    m_regexResult.setBuf(m_strRegex.c_str());
    return pReg->exec(m_strRegex.c_str(), len, 0, 0, &m_regexResult);
}


int SsiRuntime::setVar(const char *pKey, int keyLen, const char *pValue,
                       int valLen)
{
    m_pMainReq->addEnv(pKey, keyLen, pValue, valLen);
    return 0;
}

