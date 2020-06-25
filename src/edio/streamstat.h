

#ifndef STREAMSTAT_H
#define STREAMSTAT_H

enum StreamState
{
    SS_DISCONNECTED,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_CLOSING,
    SS_SHUTDOWN,
    SS_RESET
};


#define SS_FLAG_PEER_SHUTDOWN      (1<<0)
#define SS_FLAG_LOCAL_SHUTDOWN     (1<<1)
#define SS_FLAG_WANT_READ          (1<<2)
#define SS_FLAG_WANT_WRITE         (1<<3)
#define SS_FLAG_ABORT              (1<<4)
#define SS_FLAG_PEER_RESET         (1<<5)
#define SS_FLAG_HANDLER_RELEASE    (1<<6)
#define SS_FLAG_PAUSE_WRITE        (1<<7)
#define SS_EVENT_PROCESSING        (1<<8)
#define SS_FLAG_DELAY_FLUSH        (1<<9)
#define SS_FLAG_WRITE_BUFFER       (1<<10)


class StreamStat
{
public:
    StreamStat()
    {   reset();    }
    ~StreamStat()
    {};

    void reset()
    {   LS_ZERO_FILL(m_lBytesRecv, m_iPriority); }


    void setFlag(uint32_t flagbit, int val)
    {   m_iFlag = (val) ? (m_iFlag | flagbit) : (m_iFlag & ~flagbit);       }
    uint32_t getFlag(uint32_t flagbit) const {   return flagbit & m_iFlag;  }
    uint32_t getFlag() const                 {   return m_iFlag;            }


    void  bytesRecv(int n)      {   m_lBytesRecv += n;      }
    void  bytesSent(int n)      {   m_lBytesSent += n;      }

    off_t getBytesRecv() const  {   return m_lBytesRecv;    }
    off_t getBytesSent() const  {   return m_lBytesSent;    }
    off_t getBytesTotal() const {   return m_lBytesRecv + m_lBytesSent; }

    void resetBytesCount()
    {
        m_lBytesRecv = 0;
        m_lBytesSent = 0;
    }

    void setActiveTime(uint32_t lTime)
    {   m_tmLastActive = lTime;              }
    uint32_t getActiveTime() const
    {   return m_tmLastActive;               }

    char  getProtocol() const   {   return m_iProtocol;     }
    void  setProtocol(int p)    {   m_iProtocol = p;        }
    char  getState() const      {   return m_iState;        }
    void  setState(int st)      {   m_iState = st;          }

    short getPriority() const   {   return m_iPriority;     }
    void setPriority(int pri)   {   m_iPriority = pri;      }

    bool isWantRead() const     {   return getFlag(SS_FLAG_WANT_READ);      }
    bool isWantWrite() const    {   return getFlag(SS_FLAG_WANT_WRITE);     }
    bool isReadyToRelease() const {    return getFlag(SS_FLAG_HANDLER_RELEASE);  }

    bool isAborted() const      {   return getFlag(SS_FLAG_ABORT);          }
    void setAbortedFlag()       {   setFlag(SS_FLAG_ABORT, 1);              }

    bool isPauseWrite()  const  {   return getFlag(SS_FLAG_PAUSE_WRITE);    }
    bool isPeerShutdown() const {   return getFlag(SS_FLAG_PEER_SHUTDOWN);  }
    bool isWriteBuffer() const  {   return getFlag(SS_FLAG_WRITE_BUFFER);   }

    void handlerReadyToRelease(){   setFlag(SS_FLAG_HANDLER_RELEASE, 1);    }
private:
    off_t               m_lBytesRecv;
    off_t               m_lBytesSent;
    uint32_t            m_iFlag;
    uint32_t            m_tmLastActive;
    char                m_iState;
    char                m_iProtocol;
    short               m_iPriority;

};

#endif //STREAMSTAT_H

