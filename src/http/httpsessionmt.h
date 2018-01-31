#ifndef HTTPSESSIONMT_H
#define HTTPSESSIONMT_H
#define HSF_MT_NOTIFIED             (1<<1)
#define HSF_MT_CANCEL               (1<<2)
#define HSF_MT_FLUSH                (1<<3)
#define HSF_MT_END_RESP             (1<<4)
#define HSF_MT_RECYCLE              (1<<5)
#define HSF_MT_READING              (1<<6)
#define HSF_MT_WRITING              (1<<7)
#define HSF_MT_INIT_RESP_BUF        (1<<8)
#define HSF_MT_END                  (1<<9)
#define HSF_MT_PARSE_REQ_ARGS       (1<<10)
#define HSF_MT_SND_RSP_HDRS         (1<<11)
#define HSF_MT_SENDFILE             (1<<12)
#define HSF_MT_SET_URI_QS           (1<<13)
#define HSF_MT_RESP_HDR_SENT        (1<<14)
#define HSF_MT_FLUSH_RBDY_LBUF      (1<<15)
#define HSF_MT_FLUSH_RBDY_LBUF_Q    (1<<16)

#define HSF_MT_MASK                 (HSF_MT_FLUSH | HSF_MT_END_RESP \
                                    | HSF_MT_INIT_RESP_BUF)

#endif /* HTTPSESSIONMT_H */
