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

#include "l4handler.h"
#include <socket/gsockaddr.h>
#include <http/httplog.h>

L4Handler::L4Handler() 
{
    m_pL4conn = new L4conn(this);
    m_buf = new LoopBuf(MAX_OUTGOING_BUF_ZISE);
    m_iState = 0;
}
    
void L4Handler::recycle()
{
    delete m_buf;
    delete m_pL4conn;
    delete this;
}

int L4Handler::onReadEx()
{
    bool empty = m_pL4conn->getBuf()->empty();
    int space;
    
    while ( (space = m_pL4conn->getBuf()->contiguous()) > 0)
    {
        int n = getStream()->read(m_pL4conn->getBuf()->end(), space);
        if ( D_ENABLED( DL_LESS ) )
        {
            LOG_D ((getLogger(), "[%s] L4Handler: read [%d]", getLogId(), n ));
        }
        
        if (n > 0) 
            m_pL4conn->getBuf()->used(n);
        else if ( n == 0 )
            break;
        else // if (n < 0)
        {
            closeBothConnection();
            return -1;
        }
    }
    
    if ( !m_pL4conn->getBuf()->empty() )
    {
        m_pL4conn->doWrite();
        if ( !m_pL4conn->getBuf()->empty() && empty)
            m_pL4conn->continueWrite();
  
        if (m_pL4conn->getBuf()->available() <= 0 )
        {
            suspendRead();
            if ( D_ENABLED( DL_LESS ) )
                LOG_D ((getLogger(), "[%s] L4Handler: suspendRead", getLogId() ));
        }
    }
    
    return 0;
}

int L4Handler::onWriteEx()
{
    bool full = ((getBuf()->available() == 0) ? true : false);
    int length;
    
    while ((length = getBuf()->blockSize()) > 0 )
    {
        int n = getStream()->write(getBuf()->begin(), length);
        if ( D_ENABLED( DL_LESS ) )
                LOG_D ((getLogger(), "[%s] L4Handler: write [%d of %d]", getLogId(), n, length ));
        
        if (n > 0)
            getBuf()->pop_front(n);
        else if ( n == 0 )
            break;
        else // if (n < 0)
        {
            closeBothConnection();
            return -1;
        }
    }
    
    if (getBuf()->available() != 0)
    {
        if (full)
            m_pL4conn->continueRead();
    
        if ( getBuf()->empty() )
        {
            suspendWrite();
            if ( D_ENABLED( DL_LESS ) )
            {
                LOG_D(( getLogger(), "[%s] [L4conn] m_pL4conn->continueRead",
                    getLogId() ));
            }
        }
    }
        
    return 0;
}

int L4Handler::init(HttpReq &req, const GSockAddr *pGSockAddr, const char *pIP, int iIpLen)
{
    int ret = m_pL4conn->init(pGSockAddr);
    if (ret != 0)
        return ret;
    
    int hasSlashR = 1; //"\r\n"" or "\n"
    LoopBuf *pBuff = m_pL4conn->getBuf();
    pBuff->append(req.getOrgReqLine(), req.getHttpHeaderLen());
    char *pBuffEnd = pBuff->end();
    assert(pBuffEnd[-1] == '\n');
    if (pBuffEnd[-2] == 'n')
        hasSlashR = 0;
    else
    {
        assert(pBuffEnd[-2] == '\r');
    }
    
    pBuff->used( -1 * hasSlashR - 1);
    pBuff->append("X-Forwarded-For", 15);
    pBuff->append(": ", 2);
    pBuff->append(pIP, iIpLen);
    if (hasSlashR)
        pBuff->append("\r\n\r\n", 4);
    else
        pBuff->append("\n\n", 2);
    
    continueRead();
    if ( D_ENABLED( DL_LESS ) )
    {
        LOG_D ((getLogger(), "[%s] L4Handler: init web socket, reqheader [%s], len [%d]",
                getLogId(), req.getOrgReqLine(), req.getHttpHeaderLen() ));
    }
    return 0;
}

void L4Handler::doWrite() 
{ 
    onWriteEx();        
}

void L4Handler::closeBothConnection()
{
    if (m_iState != -1)
    {
        m_pL4conn->close();
        getStream()->close();
        m_iState = -1;
    }
}

