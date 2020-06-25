#ifndef EDSSLSTREAM_H
#define EDSSLSTREAM_H

#include <edio/ediostream.h>
#include <http/hiostream.h>
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

    /**
     * @todo write docs
     *
     * @return TODO
     */
    void continueRead() override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    void suspendRead() override;

    /**
     * @todo write docs
     *
     * @param pBuf TODO
     * @param size TODO
     * @return TODO
     */
    int read(char* pBuf, int size) override;

    /**
     * @todo write docs
     *
     * @param vector TODO
     * @param count TODO
     * @return TODO
     */
    int readv(iovec* vector, int count) override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    void continueWrite() override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    void suspendWrite() override;

    /**
     * @todo write docs
     *
     * @param buf TODO
     * @param len TODO
     * @return TODO
     */
    int write(const char* buf, int len) override;

    /**
     * @todo write docs
     *
     * @param iov TODO
     * @param count TODO
     * @return TODO
     */
    int writev(const iovec* iov, int count) override;

    /**
     * @todo write docs
     *
     * @param vector TODO
     * @return TODO
     */
    int writev(IOVec& vector) override;

    /**
     * @todo write docs
     *
     * @param vector TODO
     * @param total TODO
     * @return TODO
     */
    int writev(IOVec& vector, int total) override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    int close() override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    int flush() override;

    /**
     * @todo write docs
     *
     * @return TODO
     */
    virtual int shutdown();

protected:
    virtual int handleEvents(short int event);
    virtual int onPeerClose();

private:
    int handleEventsSsl(short int event);
    int flushSslWpending();

private:
    SslConnection *m_ssl;
};

#endif // EDSSLSTREAM_H
