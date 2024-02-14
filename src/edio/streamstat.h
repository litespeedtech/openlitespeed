

#ifndef STREAMSTAT_H
#define STREAMSTAT_H

enum stream_state
{
    SS_DISCONNECTED,
    SS_ADNS_LOOKUP,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_CLOSING,
    SS_SHUTDOWN,
    SS_RESET,
    SS_ABORT,
    SS_ERROR
};

enum stream_protocol
{
    HIOS_PROTO_HTTP  = 0,
    HIOS_PROTO_SPDY2 = 1,
    HIOS_PROTO_SPDY3 = 2,
    HIOS_PROTO_SPDY31 = 3,
    HIOS_PROTO_HTTP2 = 4,
    HIOS_PROTO_QUIC = 5,
    HIOS_PROTO_HTTP3 = 6,
    HIOS_PROTO_MAX,
    SS_PROTO_L4,
    SS_PROTO_L4SSL,
};

enum stream_flag
{
    SS_FLAG_NONE               = 0,
    SS_FLAG_PEER_SHUTDOWN      = (1<<0),
    SS_FLAG_LOCAL_SHUTDOWN     = (1<<1),
    SS_FLAG_WANT_READ          = (1<<2),
    SS_FLAG_WANT_WRITE         = (1<<3),
    SS_FLAG_ABORT              = (1<<4),
    SS_FLAG_PEER_RESET         = (1<<5),
    SS_FLAG_HANDLER_RELEASE    = (1<<6),
    SS_FLAG_PAUSE_WRITE        = (1<<7),
    SS_EVENT_PROCESSING        = (1<<8),
    SS_FLAG_DELAY_FLUSH        = (1<<9),
    SS_FLAG_WRITE_BUFFER       = (1<<10),
    SS_FLAG_WANT_EVENT_PROCESS = (1<<11),
    SS_FLAG_FROM_LOCAL         = (1<<12),
    SS_FLAG_PUSH_CAPABLE       = (1<<13),
    SS_FLAG_INIT_SESS          = (1<<14),
    SS_FLAG_IS_PUSH            = (1<<15),
    SS_FLAG_PASS_THROUGH       = (1<<16),
    SS_FLAG_SENDFILE           = (1<<17),
    SS_FLAG_FLOWCTRL           = (1<<18),
    SS_FLAG_PRI_SET            = (1<<19),
    SS_FLAG_ALTSVC_SENT        = (1<<20),
    SS_FLAG_PASS_SETCOOKIE     = (1<<21),
    SS_FLAG_RESP_HEADER_SENT   = (1<<22),
    SS_FLAG_BLACK_HOLE         = (1<<23),
    SS_FLAG_READ_EOS           = (1<<24),
};

inline enum stream_flag operator|(enum stream_flag a, enum stream_flag b)
{
    return static_cast<enum stream_flag>(static_cast<int>(a) | static_cast<int>(b));
}
inline enum stream_flag operator~(enum stream_flag a)
{
    return static_cast<enum stream_flag>(~static_cast<int>(a));
}

class StreamStat
{
public:
    StreamStat()
    {   reset();    }
    ~StreamStat()
    {};

    void reset()
    {   LS_ZERO_FILL(m_lBytesRecv, m_iPriority); }


    void setFlag(enum stream_flag flagbit, int val)
    {   m_iFlag = (val) ? (enum stream_flag)(m_iFlag | flagbit)
                        : (enum stream_flag)(m_iFlag & ~flagbit);       }
    uint32_t getFlag(enum stream_flag flagbit) const
    {   return flagbit & m_iFlag;  }
    enum stream_flag getFlag() const    {   return m_iFlag;         }


    void  bytesRecv(int n)              {   m_lBytesRecv += n;      }
    void  bytesSent(int n)              {   m_lBytesSent += n;      }

    uint64_t getBytesRecv() const       {   return m_lBytesRecv;    }
    uint64_t getBytesSent() const       {   return m_lBytesSent;    }
    uint64_t getBytesTotal() const {   return m_lBytesRecv + m_lBytesSent; }

    void resetBytesCount()
    {
        m_lBytesRecv = 0;
        m_lBytesSent = 0;
    }

    void setActiveTime(uint32_t lTime)
    {   m_tmLastActive = lTime;              }
    uint32_t getActiveTime() const
    {   return m_tmLastActive;               }

    enum stream_protocol  getProtocol() const   {   return m_protocol;     }
    void  setProtocol(enum stream_protocol p)   {   m_protocol = p;        }

    enum stream_state getState() const      {   return m_state;        }
    void setState(enum stream_state st)     {   m_state = st;          }

    short getPriority() const   {   return m_iPriority;     }
    void setPriority(int pri)   {   m_iPriority = pri;      }

    bool isWantRead() const     {   return getFlag(SS_FLAG_WANT_READ);      }
    bool isWantWrite() const    {   return getFlag(SS_FLAG_WANT_WRITE);     }
    bool isReadyToRelease() const {    return getFlag(SS_FLAG_HANDLER_RELEASE);  }

    bool isAborted() const      {   return getFlag(SS_FLAG_ABORT);          }
    void setAbortedFlag()       {   setFlag(SS_FLAG_ABORT, 1);              }

    bool isPauseWrite()  const  {   return getFlag(SS_FLAG_PAUSE_WRITE);    }
    bool isPeerShutdown() const {   return getFlag(SS_FLAG_PEER_SHUTDOWN);  }
    bool isEos() const          {   return getFlag(SS_FLAG_READ_EOS);       }
    bool isWriteBuffer() const  {   return getFlag(SS_FLAG_WRITE_BUFFER);   }

    void handlerReadyToRelease(){   setFlag(SS_FLAG_HANDLER_RELEASE, 1);    }
private:
    uint64_t                m_lBytesRecv;
    uint64_t                m_lBytesSent;
    uint32_t                m_tmLastActive;
    enum stream_flag        m_iFlag;
    enum stream_state       m_state:8;
    enum stream_protocol    m_protocol:8;
    short                   m_iPriority;

};

#endif //STREAMSTAT_H

