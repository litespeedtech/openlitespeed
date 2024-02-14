#ifndef EDSSLSTREAM_H
#define EDSSLSTREAM_H

#include <edio/ediostream.h>
#include <edio/streamstat.h>
#include <log4cxx/logsession.h>
#include <lsr/ls_str.h>
/**
 * @todo write docs
 */

class SslConnection;
class SslContext;
class SslClientSessCache;

class SslEdStream :  public EdStream, public StreamStat, virtual public LogSession
{
public:
    SslEdStream()
        : m_ssl(NULL)
        {}
    virtual ~SslEdStream();

    void setSslAgain();
    int connectSsl();
    int initSsl(SslContext *ctx, ls_str_t *sni, bool enable_h2,
                SslClientSessCache *cache);
    int verifySni(ls_str_t *sni);
    void takeover(EdStream * old, SslConnection *ssl);

    void continueRead() override;
    void suspendRead() override;
    void continueWrite() override;
    void suspendWrite() override;

    int read(char* pBuf, int size) override;
    int readv(iovec* vector, int count) override;

    int write(const char* buf, int len) override;
    int writev(const iovec* iov, int count) override;
    int writev(IOVec& vector) override;
    int writev(IOVec& vector, int total) override;

    void tobeClosed();

    int close() override;

    int flush() override;

    int shutdown() override;

protected:
    int handleEvents(short int event) override;
    int onPeerClose() override;

private:
    int handleEventsSsl(short int event);
    int flushSslWpending();

private:
    SslConnection *m_ssl;
};

#endif // EDSSLSTREAM_H
