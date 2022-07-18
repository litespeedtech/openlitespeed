/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifndef ACCESSLOG_H
#define ACCESSLOG_H


#include <lsdef.h>
#include <log4cxx/nsdefs.h>
#include <util/autobuf.h>

#include <sys/types.h>

#define MAX_BUFFERED_LEN    16384
#define MAX_LOG_LINE_LEN    8192
#define LOG_BUF_SIZE        (MAX_BUFFERED_LEN + MAX_LOG_LINE_LEN)

#define LOG_REFERER     1
#define LOG_USERAGENT   2
#define LOG_VHOST       4


BEGIN_LOG4CXX_NS
class Appender;
class AppenderManager;
END_LOG4CXX_NS

class HttpSession;
class CustomFormat;
class AccessLog
{
    LOG4CXX_NS::Appender         *m_pAppender;
    LOG4CXX_NS::AppenderManager *m_pManager;
    CustomFormat                 *m_pCustomFormat;

    short   m_iAsync;
    short   m_iPipedLog;
    int     m_iAccessLogHeader;
    AutoBuf m_buf;

    int appendStr(const char *pStr, int len);
    void appendStrNoQuote(int escape, const char *pSrc, int srcLen);
    void customLog(HttpSession *pSession, CustomFormat *pLogFmt, bool doFlush = true);

public:
    explicit AccessLog(const char *pPath);
    AccessLog();
    ~AccessLog();
    int init(const char *pName, int pipe);

    char *appendReqVar(HttpSession *pSession, int id);
    void log(HttpSession *pSession);
    void log(const char *pVHostName, int len, HttpSession *pSession);
    void flush();

    void accessLogReferer(int referer);
    void accessLogAgent(int agent);
    void setLogHeaders(int flag)          {   m_iAccessLogHeader = flag;  }
    int getAccessLogHeader()                {   return m_iAccessLogHeader; }

    void setAsyncAccessLog(short async)   {   m_iAsync = async;           }
    short asyncAccessLog()  const           {   return m_iAsync;           }

    void setPipedLog(short pipe)          {   m_iPipedLog = pipe;         }
    short isPipedLog()  const               {   return m_iPipedLog;        }

    LOG4CXX_NS::Appender *getAppender() const      {   return m_pAppender; }
    void setAppender(LOG4CXX_NS::Appender *p)    {   m_pAppender = p;    }

    char getCompress() const;
    const char *getLogPath() const;
    int  reopenExist();
    int appendEscape(const char *data, int len);

    void closeNonPiped();
    void setRollingSize(off_t size);
    int  setCustomLog(const char *pFmt);
    int  getLogString(HttpSession *pSession, const char *log_pattern, char *pBuf, int bufLen);

    CustomFormat *parseLogFormat(const char *pFmt);

    LS_NO_COPY_ASSIGN(AccessLog);
};

#endif

