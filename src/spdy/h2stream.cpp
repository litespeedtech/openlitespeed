/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include "h2stream.h"

#include "h2connection.h"
#include "h2protocol.h"

#include <http/httplog.h>
#include <lsr/ls_strtool.h>
#include <util/datetime.h>
#include <util/iovec.h>

H2Stream::H2Stream()
    : m_uiStreamID(0)
    , m_iWindowOut(H2_FCW_INIT_SIZE)
    , m_iWindowIn(H2_FCW_INIT_SIZE)
    , m_pH2Conn(NULL)
    , m_iContentLen(-1)
    , m_iContentRead(0)
    , m_reqHeaderEnd(0)
{
}


const char *H2Stream::buildLogId()
{
    int len ;
    AutoStr2 &id = getIdBuf();

    len = ls_snprintf(id.buf(), MAX_LOGID_LEN, "%s-%d",
                      m_pH2Conn->getStream()->getLogId(), m_uiStreamID);
    id.setLen(len);
    return id.c_str();
}


int H2Stream::init(uint32_t StreamID, H2Connection *pH2Conn, uint8_t flags,
                   HioHandler *pHandler, Priority_st *pPriority)
{
    HioStream::reset(DateTime::s_curTime);
    pHandler->assignStream(this);
    clearLogId();

    setState(HIOS_CONNECTED);
    setFlag((flags & (H2_CTRL_FLAG_FIN | H2_CTRL_FLAG_UNIDIRECTIONAL)), 1);

    m_bufIn.clear();
    m_uiStreamID  = StreamID;
    m_iWindowOut = pH2Conn->getStreamOutInitWindowSize();
    m_iWindowIn = pH2Conn->getStreamInInitWindowSize();

//We disable Priority right now.
//     if (pPriority)
//     {
//         //FIXME:need an algrithem to compute the priority
//         m_iPriority = pPriority->m_weight;
//     }
//     else
//         m_iPriority = 0;

    m_pH2Conn = pH2Conn;
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::init(), id: %d. ",
               getLogId(), StreamID));
    }
    return 0;
}


int H2Stream::onInitConnected(bool bUpgraded)
{
    if (!bUpgraded)
        getHandler()->onInitConnected();

    if (isWantRead())
        getHandler()->onReadEx();
    if (isWantWrite())
        getHandler()->onWriteEx();
    return 0;
}


H2Stream::~H2Stream()
{
    m_bufIn.clear();
}


int H2Stream::appendReqData(char *pData, int len, uint8_t flags)
{
    if (m_bufIn.append(pData, len) == -1)
        return LS_FAIL;
    m_iContentRead += len;
    if (isFlowCtrl())
        m_iWindowIn -= len;
    //Note: H2_CTRL_FLAG_FIN is directly mapped to HIO_FLAG_PEER_SHUTDOWN
    //      H2_CTRL_FLAG_UNIDIRECTIONAL is directly mapped to HIO_FLAG_LOCAL_SHUTDOWN
    if (flags & (H2_CTRL_FLAG_FIN | H2_CTRL_FLAG_UNIDIRECTIONAL))
    {
        setFlag(flags & (H2_CTRL_FLAG_FIN | H2_CTRL_FLAG_UNIDIRECTIONAL), 1);
        if (m_iContentLen != -1 && m_iContentLen != m_iContentRead)
            return LS_FAIL;
    }
    if (isWantRead())
        getHandler()->onReadEx();
    return len;
}


//***int H2Stream::read( char * buf, int len )***//
// return > 0:  number of bytes of that has been read
// return = 0:  0 byte of data has been read, but there will be more data coming,
//              need to read again
// return = -1: EOF (End of File) There is no more data need to be read, the
//              stream can be removed
int H2Stream::read(char *buf, int len)
{
    int ReadCount;
    if (getState() == HIOS_DISCONNECTED)
        return -1;

    ReadCount = m_bufIn.moveTo(buf, len);
    if (ReadCount == 0)
    {
        if (getFlag(HIO_FLAG_PEER_SHUTDOWN))
        {
            return -1; //EOF (End of File) There is no more data need to be read
        }
    }
    if (ReadCount > 0)
        setActiveTime(DateTime::s_curTime);

    return ReadCount;
}


void H2Stream::continueRead()
{
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::continueRead()",
               getLogId()));
    }
    setFlag(HIO_FLAG_WANT_READ, 1);
    if (m_bufIn.size() > 0)
        getHandler()->onReadEx();
}


void H2Stream:: continueWrite()
{
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::continueWrite()",
               getLogId()));
    }
    setFlag(HIO_FLAG_WANT_WRITE, 1);
    m_pH2Conn->setPendingWrite();

}


void H2Stream::onTimer()
{
    getHandler()->onTimerEx();
}


NtwkIOLink *H2Stream::getNtwkIoLink()
{
    return m_pH2Conn->getNtwkIoLink();
}


int H2Stream::shutdown()
{
    if (getState() == HIOS_SHUTDOWN)
        return 0;

    setState(HIOS_SHUTDOWN);

    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::sendFin()",
               getLogId()));
    }
    m_pH2Conn->sendFinFrame(m_uiStreamID);
    return 0;
}


int H2Stream::close()
{
    if (getState() == HIOS_DISCONNECTED)
        return 0;
    if (getHandler())
        getHandler()->onCloseEx();
    shutdown();
    setFlag(HIO_FLAG_WANT_WRITE, 0);
    setState(HIOS_DISCONNECTED);
    //if (getHandler())
    //{
    //    getHandler()->recycle();
    //    setHandler( NULL );
    //}
    //m_pH2Conn->recycleStream( m_uiStreamID );
    return 0;
}


int H2Stream::flush()
{
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::flush()", getLogId()));
    }
    return LS_DONE;
}


int H2Stream::getDataFrameSize(int wanted)
{
    if ((m_pH2Conn->isOutBufFull()) ||
        (0 >= m_iWindowOut))
    {
        setFlag(HIO_FLAG_BUFF_FULL | HIO_FLAG_WANT_WRITE, 1);
        m_pH2Conn->setPendingWrite();
        return 0;
    }

    if (wanted > m_iWindowOut)
        wanted = m_iWindowOut;
    if (wanted > m_pH2Conn->getCurDataOutWindow())
        wanted = m_pH2Conn->getCurDataOutWindow();
    if (wanted > m_pH2Conn->getPeerMaxFrameSize())
        wanted = m_pH2Conn->getPeerMaxFrameSize();
    return wanted;
}


int H2Stream::writev(IOVec &vector, int total)
{
    int size;
    int ret;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    if (getFlag(HIO_FLAG_BUFF_FULL))
        return 0;
    size = getDataFrameSize(total);
    if (size <= 0)
        return 0;
    if (size < total)
    {
        //adjust vector
        IOVec iov(vector);
        total = iov.shrinkTo(size, 0);
        ret = sendData(&iov, size);
    }
    else
        ret = sendData(&vector, size);
    if (ret == -1)
        return LS_FAIL;
    return size;

}


int H2Stream::writev(const struct iovec *vec, int count)
{
    IOVec iov(vec, count);
    return writev(iov.get(), iov.bytes());
}


int H2Stream::write(const char *buf, int len)
{
    IOVec iov;
    int allowed;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    allowed = getDataFrameSize(len);
    if (allowed <= 0)
        return 0;

    iov.append(buf, allowed);
    if (sendData(&iov, allowed) == -1)
        return LS_FAIL;
    return allowed;
}


int H2Stream::onWrite()
{
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::onWrite()",
               getLogId()));
    }
    if (m_pH2Conn->isOutBufFull())
        return 0;
    if (m_iWindowOut <= 0)
        return 0;
    setFlag(HIO_FLAG_BUFF_FULL, 0);

    if (isWantWrite())
        getHandler()->onWriteEx();
    if (isWantWrite())
        m_pH2Conn->setPendingWrite();
    return 0;
}


int H2Stream::sendData(IOVec *pIov, int total)
{
    int ret;
    ret = m_pH2Conn->sendDataFrame(m_uiStreamID, 0, pIov, total);
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] H2Stream::sendData(), total: %d, ret: %d",
               getLogId(), total, ret));
    }
    if (ret == -1)
    {
        setFlag(HIO_FLAG_ABORT, 1);
        return LS_FAIL;
    }

    setActiveTime(DateTime::s_curTime);
    bytesSent(total);
    if (isFlowCtrl())
    {
        m_iWindowOut -= total;
        if (m_iWindowOut <= 0)
            setFlag(HIO_FLAG_BUFF_FULL, 1);
    }
    return total;
}


int H2Stream::sendRespHeaders(HttpRespHeaders *pHeaders)
{
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;

    m_pH2Conn->add2PriorityQue(this);
    return m_pH2Conn->sendRespHeaders(pHeaders, m_uiStreamID);
}


int H2Stream::adjWindowOut(int32_t n)
{
    if (isFlowCtrl())
    {
        m_iWindowOut += n;
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(), "[%s] stream WINDOW_UPDATE: %d, window size: %d ",
                   getLogId(), n, m_iWindowOut));
        }
        if (m_iWindowOut < 0)
        {
            //window overflow
            return LS_FAIL;
        }
        else if (isWantWrite())
            continueWrite();
    }
    return 0;
}

