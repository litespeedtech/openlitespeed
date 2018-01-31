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

#ifndef MTSESSDATA_H
#define MTSESSDATA_H

#include <http/httpsessionmt.h>
#include <lsr/ls_lock.h>
#include <thread/mtnotifier.h>
#include <util/blockbuf.h>
#include <util/dlinkqueue.h>


#define MTSD_EF_PARSE_REQ_BODY          (1<<0)
#define MTSD_EF_UPLOAD_PASS_BY_PATH     (1<<1)

#define MTSD_RET_FAIL                  -1
#define MTSD_RET_UNKNOWN                0
#define MTSD_RET_OK                     1


class MtLock
{
public:
    explicit MtLock(ls_mutex_t & lock)
        : m_lock(&lock)
    {
        ls_mutex_lock(m_lock);
    }

    ~MtLock()
    {
        ls_mutex_unlock(m_lock);
    }

    const ls_mutex_t * getPtr() const { return m_lock; }

private:
    ls_mutex_t * m_lock;
};


enum MtWriteMode {
    WRITE_THROUGH,
    DBL_BUF
};


// convenient holder of ptrs into current write buffer,
// which may be from session VMemBuf or our own allocation
// we just store pointers, no allocation / deletion here
class MtWriteBuf : public BlockBuf, public DLinkedObj
{
public:
    MtWriteBuf(char * ptr = NULL, size_t size = 0, char * cur = NULL)
        : BlockBuf(ptr, size)
        , m_pCurPos(cur)
    {
    }

    virtual ~MtWriteBuf() {}

    void set(char * ptr, size_t size, char * cur)
    {
        setBlockBuf(ptr, size);
        m_pCurPos = cur;
    }

    char * getCurPos() {    return m_pCurPos;   }
private:
    char * m_pCurPos;
};

// Queue of MtWriteBuf used when in DBL_BUF MtWriteMode - module
// thread appends to current MtLocalBufQ ptr and notifies main thread,
// main thread grabs the current ptr and sets it to NULL (so module
// will allocate new one if it needs it) and then drains the queue,
// calling appendDynBody
class MtLocalBufQ
{
public:
    TDLinkQueue<MtWriteBuf>       m_bufs;
    MtLocalBufQ()
        : m_bufs()
    {
    }

    ~MtLocalBufQ()
    {
    }

    void append(MtWriteBuf & buf)
    {
        MtWriteBuf * n = new MtWriteBuf(buf.getBuf(),
                                        buf.getBufEnd() - buf.getBuf(),
                                        buf.getCurPos());
        m_bufs.append(n);
    }

    MtWriteBuf * pop_front()
    {
        if (!m_bufs.size())
        {
            return NULL;
        }
        MtWriteBuf * r = m_bufs.pop_front();
        return r;
    }

    int size()
    {
        return m_bufs.size();
    }


    LS_NO_COPY_ASSIGN(MtLocalBufQ);
};


class MtSessData
{
// used as container, public access
public:
    MtSessData()
        : m_read()
        , m_write()
        , m_event()
        , m_mtEndNextState(0)
        , m_retReqArgs(MTSD_RET_UNKNOWN)
        , m_iEventFlags(0)
        , m_iTmpFilePerms(0)
        , m_pUploadTmpDir(NULL)
        , m_mtWrite()
    {
        ls_mutex_setup(&m_mutex_writer);
        // ls_spinlock_setup(&m_fastlock);
        // ls_spinlock_setup(&m_reqHeaderLock);
        ls_mutex_setup(&m_respHeaderLock);
        // ls_mutex_setup(&m_slowlock);
        // ls_spinlock_setup(&m_setUriQs_lock);
    }
    ~MtSessData()
    {}

    //TODO (Ron): replace with direct access, container class
    MtNotifier *getReadNotifier()   {   return &m_read;     }
    MtNotifier *getWriteNotifier()  {   return &m_write;    }

    MtNotifier      m_read;
    MtNotifier      m_write;
    MtNotifier      m_event;
    ls_mutex_t      m_mutex_writer;
    // ls_spinlock_t   m_fastlock;
    // ls_spinlock_t   m_reqHeaderLock;
    ls_mutex_t      m_respHeaderLock;
    // ls_mutex_t      m_slowlock;
    int             m_mtEndNextState;

    char            m_retReqArgs;   // m_reqHeaderLock locked.

    // Next three are m_reqHeaderLock locked.
    uint32_t        m_iEventFlags;
    int             m_iTmpFilePerms;
    const char     *m_pUploadTmpDir;

    // set_uri_qs
    // ls_spinlock_t   m_setUriQs_lock;

    struct MtWrite_s
    {
        MtWriteBuf      m_writeBuf;
        MtWriteMode     m_writeMode;
        VMemBuf *       m_curVMemBuf;
        MtLocalBufQ    *m_bufQ;
        ls_mutex_t      m_bufQ_lock;

        MtWrite_s()
            : m_writeBuf()
              , m_writeMode(WRITE_THROUGH)
              //, m_writeMode(DBL_BUF) // FOR TESTING ONLY - dynamic switch in code
              , m_curVMemBuf(NULL)
              , m_bufQ(NULL)
        {
            ls_mutex_setup(&m_bufQ_lock);
        }
    } m_mtWrite;

    LS_NO_COPY_ASSIGN(MtSessData);
};

class MtParamUriQs
{
public:
    int          m_action;
    ls_str_t    *m_uri;
    ls_str_t    *m_qs;

    LS_NO_COPY_ASSIGN(MtParamUriQs);
};


class MtParamParseReqArgs
{
public:
    const char *m_pUploadTmpDir;
    int         m_parseReqBody;
    int         m_uploadPassByPath;
    int         m_tmpFilePerms;
    int         m_ret;

    MtParamParseReqArgs()
        : m_pUploadTmpDir(NULL)
          , m_parseReqBody(0)
          , m_uploadPassByPath(0)
          , m_tmpFilePerms(0)
          , m_ret(0)
    {}

    LS_NO_COPY_ASSIGN(MtParamParseReqArgs);
};


class MtParamSendfile
{
public:
    const char     *m_pFile;
    int             m_fd;
    int64_t         m_start;
    int64_t         m_size;
    char            m_ret;

    MtParamSendfile()
        : m_pFile(NULL)
          , m_fd(0)
          , m_start(0)
          , m_size(0)
          , m_ret(0)
    {}

    LS_NO_COPY_ASSIGN(MtParamSendfile);
};

#ifdef NOTUSED
class MtFlushLocalBufParams
{
public:
    char * m_buf;
    char * m_bufEnd;
    // unused int written;
    MtFlushLocalBufParams(char * buf = NULL, char * bufEnd = NULL)
        : m_buf(buf)
          , m_bufEnd(bufEnd)
    {
    }

    LS_NO_COPY_ASSIGN(MtFlushLocalBufParams);
};
#endif

#endif // MTSESSDATA_H
