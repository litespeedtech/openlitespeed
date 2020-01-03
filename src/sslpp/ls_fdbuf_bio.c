/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

#include <lsdef.h>
#include <lsr/ls_pool.h>
#include <socket/ls_sock.h>
#include <sslpp/ls_fdbuf_bio.h>
#include <openssl/bio.h>

//#ifdef RUN_TEST
#define DEBUGGING
//#endif

#ifdef DEBUGGING

#ifdef TEST_PGM
#define DEBUG_MESSAGE(...)     printf(__VA_ARGS__);
#define INFO_MESSAGE(...)      printf(__VA_ARGS__);
#else
#include <lsr/ls_log.h>
#define DEBUG_MESSAGE(...)     LSR_DBG_H(__VA_ARGS__);
#define INFO_MESSAGE(... )     LSR_INFO(__VA_ARGS__);
#endif

#else

#define DEBUG_MESSAGE(...)
#define INFO_MESSAGE(...)

#endif

const int RBIO_BUF_SIZE = 4096; // Initial size
#if OPENSSL_VERSION_NUMBER < 0x10100000
BIO_METHOD  s_biom_s;
#endif
BIO_METHOD *s_biom = NULL; // Note that this never gets deallocated.
const BIO_METHOD *s_biom_fd_builtin = NULL;

// All from: https://github.com/openssl/openssl/blob/master/crypto/bio/bss_fd.c
static int bio_fd_free(BIO *a);
static int bio_fd_read(BIO *b, char *out, int outl);
static int bio_fd_write(BIO *b, const char *in, int inl);
static int bio_fd_puts(BIO *bp, const char *str);
static int bio_fd_gets(BIO *bp, char *buf, int size);

#ifdef OPENSSL_IS_BORINGSSL
static int BIO_fd_should_retry(int i);
static int BIO_fd_non_fatal_error(int err);
#endif

#if EWOULDBLOCK != EAGAIN
#define SIMPLE_RETRY(err) ((err == EAGAIN) || (err == EWOULDBLOCK))
#else
#define SIMPLE_RETRY(err) (err == EAGAIN)
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10100000
void *BIO_get_data(BIO *a);
#define LS_FDBUF_FROM_BIO(b) (ls_fdbio_data *)BIO_get_data(b)
#else
#define LS_FDBUF_FROM_BIO(b) (ls_fdbio_data *)BIO_get_app_data(b)
#endif

void ls_fdbuf_bio_init(ls_fdbio_data *fdbio)
{
    memset(fdbio, 0, sizeof(*fdbio));
    fdbio->m_rbuf_size = RBIO_BUF_SIZE;
}


int ls_fdbio_alloc_rbuff(ls_fdbio_data *fdbio, int size)
{
    DEBUG_MESSAGE("[FDBIO] alloc read buf %d\n", size);
    assert(fdbio->m_rbuf_used == fdbio->m_rbuf_read);
    if (fdbio->m_rbuf && fdbio->m_rbuf_size >= size)
        return LS_OK;
    void *buf = ls_palloc(size);
    if (buf)
    {
        if (fdbio->m_rbuf)
            ls_pfree(fdbio->m_rbuf);
        fdbio->m_rbuf = (uint8_t *)buf;
        fdbio->m_rbuf_size = size;
        fdbio->m_rbuf_used = fdbio->m_rbuf_read = 0;
        return LS_OK;
    }
    else
    {
        DEBUG_MESSAGE("Insufficient memory allocating rbuff %d bytes\n", size);
        errno = ENOMEM;
        return LS_FAIL;
    }
}


void ls_fdbio_release_rbuff(ls_fdbio_data *fdbio)
{
    if (fdbio->m_rbuf == NULL)
        return;
    assert(fdbio->m_rbuf_used == fdbio->m_rbuf_read);
    DEBUG_MESSAGE("[FDBIO] free rbuf %d\n", (int)fdbio->m_rbuf_size);
    ls_pfree(fdbio->m_rbuf);
    fdbio->m_rbuf = NULL;
}


static int bio_fd_read(BIO *b, char *out, int outl)
{
    int ret = 0;
    int err = 0;
    ls_fdbio_data *fdbio = LS_FDBUF_FROM_BIO(b);
    int fd = BIO_get_fd(b, 0);
    int total = 0;
    DEBUG_MESSAGE("[BIO] bio_fd_read: %p, %d bytes on %d\n", b,
                  outl, fd);
    if ((out == NULL) || (!outl))
    {
        INFO_MESSAGE("[BIO] bio_fd_read NO BUFFER!!\n");
        err = EINVAL;
        return -1;
    }
    if (fdbio->m_is_closed)
    {
        DEBUG_MESSAGE("[BIO] bio_fd_read: CLOSED ON PREVIOUS READ\n");
        errno = 0;
        return 0;
    }
    while (total < outl)
    {
        int buffered = fdbio->m_rbuf_used - fdbio->m_rbuf_read;
        int rd_remaining = outl - total;
        int copy = 0;
        if (buffered)
        {
            copy = (buffered > rd_remaining) ? rd_remaining : buffered;
            DEBUG_MESSAGE("[BIO] bio_fd_read: Use buffered %d of %d\n",
                          copy, rd_remaining);
            memcpy(&out[total], &fdbio->m_rbuf[fdbio->m_rbuf_read], copy);
            fdbio->m_rbuf_read += copy;
            total += copy;
            if (total >= outl)
            {
                DEBUG_MESSAGE("[BIO] bio_fd_read: GOT TOTAL %d\n",
                              total);
                return total;
            }
            fdbio->m_rbuf_used = 0;
            fdbio->m_rbuf_read = 0;
            rd_remaining = outl - total;
        }
        if ((rd_remaining < fdbio->m_rbuf_size)
            && (fdbio->m_rbuf || ls_fdbio_alloc_rbuff(fdbio, fdbio->m_rbuf_size) == LS_OK))
        {
            DEBUG_MESSAGE("[BIO] bio_fd_read: Read into buffer\n");
            ret = ls_read(fd, fdbio->m_rbuf, fdbio->m_rbuf_size);
            if (ret < fdbio->m_rbuf_size)
                fdbio->m_need_read_event = 1;
            buffered = ret;
        }
        else
        {
            DEBUG_MESSAGE("[BIO] bio_fd_read: Read into data\n");
            ret = ls_read(fd, out + total, rd_remaining);
            if (ret < rd_remaining)
                fdbio->m_need_read_event = 1;
            buffered = 0;
        }
        if (ret == 0)
        {
            DEBUG_MESSAGE("[BIO] bio_fd_read: CLOSED\n");
            fdbio->m_is_closed = 1;
            errno = 0;
            return total;
        }
        if (ret < 0) 
        {
            err = errno;
            if (total)
            {
                DEBUG_MESSAGE("[BIO] bio_fd_read: error but I have data\n");
                return total;
            }
            if ((SIMPLE_RETRY(err)) || (BIO_fd_should_retry(ret)))
            {
                DEBUG_MESSAGE("[BIO] bio_fd_read: set retry read"
                              " errno: %d\n", err);
                errno = err;
                BIO_set_retry_read(b);
            }
            return -1;
        }
        if (buffered)
        {
            fdbio->m_rbuf_used = buffered;
            fdbio->m_rbuf_read = 0;
            DEBUG_MESSAGE("[BIO] bio_fd_read: Preserve read: %d\n", ret);
        }
        else
        {
            total += ret;
            DEBUG_MESSAGE("[BIO] bio_fd_read: Read into data: %d\n", ret);
        }
    }
    return total;
}


int ls_fdbio_alloc_wbuff(ls_fdbio_data *fdbio, int size)
{
    DEBUG_MESSAGE("[FDBIO] alloc write buf %d\n", size);
    assert(fdbio->m_wbuf_used == 0);
    if (fdbio->m_wbuf && fdbio->m_wbuf_size >= size)
        return LS_OK;
    void *buf = ls_palloc(size);
    if (buf)
    {
        if (fdbio->m_wbuf)
            ls_pfree(fdbio->m_wbuf);
        fdbio->m_wbuf = (uint8_t *)buf;
        fdbio->m_wbuf_size = size;
        fdbio->m_wbuf_sent = 0;
        return LS_OK;
    }
    return LS_FAIL;
}


void ls_fdbio_release_wbuff(ls_fdbio_data *fdbio)
{
    assert(fdbio->m_wbuf_used == 0);
    if (fdbio->m_wbuf == NULL)
        return;
    DEBUG_MESSAGE("[FDBIO] free wbuf %d\n", (int)fdbio->m_wbuf_size);
    ls_pfree(fdbio->m_wbuf);
    fdbio->m_wbuf_size = 0;
    fdbio->m_wbuf = NULL;
}


static int ls_fdbio_combine_write(ls_fdbio_data *fdbio, int fd, const void *buf, int num)
{
    DEBUG_MESSAGE("[FDBIO] ls_fdbio_combine_write, to write: %d, used: %d, sent: %d\n",
           num, (int)fdbio->m_wbuf_used, (int)fdbio->m_wbuf_sent);
    struct iovec iov[2];
    iov[0].iov_base = fdbio->m_wbuf + fdbio->m_wbuf_sent;
    iov[0].iov_len = fdbio->m_wbuf_used - fdbio->m_wbuf_sent;
    iov[1].iov_base = (void *)buf;
    iov[1].iov_len = num;
    int ret = ls_writev(fd, iov, 2);
    DEBUG_MESSAGE("[FDBIO] ls_writev(%d + %d) ret: %d\n",
                  (int)iov[0].iov_len, (int)iov[1].iov_len, ret);
    if (ret > 0)
    {
        if (ret >= (int)iov[0].iov_len)
        {
            ret -= iov[0].iov_len;
            fdbio->m_wbuf_used = 0;
            fdbio->m_wbuf_sent = 0;
            if (ret == 0)
            {
                ret = -1;
                errno = EWOULDBLOCK;
            }
        }
        else
        {
            fdbio->m_wbuf_sent += ret;
            ret = -1;
            errno = EWOULDBLOCK;
        }
    }
    DEBUG_MESSAGE("[FDBIO] combine_write ret: %d, buffer used: %d, sent: %d\n",
                  ret, fdbio->m_wbuf_used, fdbio->m_wbuf_sent);
    return ret;
}


#define TLS_RECORD_MAX_SIZE 16413
static int ls_fdbio_buff_write(ls_fdbio_data *fdbio, int fd, const void *buf, int num)
{
    DEBUG_MESSAGE("[FDBIO] ls_fdbio_buff_write, to write: %d, used: %d, sent: %d\n",
           num, (int)fdbio->m_wbuf_used, (int)fdbio->m_wbuf_sent);
    if (!fdbio->m_wbuf)
    {
        if (num < 4096)
            if (ls_fdbio_alloc_wbuff(fdbio, 4096) == LS_FAIL)
                return LS_FAIL;
    }

    int avail = fdbio->m_wbuf_size - fdbio->m_wbuf_used;
    if (avail < num)
    {
        int ret;
        if (fdbio->m_wbuf_used > 0)
            ret = ls_fdbio_combine_write(fdbio, fd, buf, num);
        else
            ret = ls_write(fd, buf, num);
        if (ret >= num && fdbio->m_wbuf_size < TLS_RECORD_MAX_SIZE * 4)
        {
            int new_size = fdbio->m_wbuf_size + TLS_RECORD_MAX_SIZE;
            if (new_size < TLS_RECORD_MAX_SIZE * 2)
                new_size = TLS_RECORD_MAX_SIZE * 2;
            if (ls_fdbio_alloc_wbuff(fdbio, new_size) == LS_FAIL)
                return LS_FAIL;
        }
        return ret;
    }
    if (!fdbio->m_wbuf)
        return LS_FAIL;
    memmove(fdbio->m_wbuf + fdbio->m_wbuf_used, buf, num);
    fdbio->m_wbuf_used += num;
    DEBUG_MESSAGE("[FDBIO] lstls_buff_write, to write: %d, finished: %d, used: %d, sent: %d\n",
           num, num, (int)fdbio->m_wbuf_used, (int)fdbio->m_wbuf_sent);
    return num;
}


int ls_fdbio_flush(ls_fdbio_data *fdbio, int fd)
{
    if (fdbio->m_flag & LS_FDBIO_WBLOCK)
        return 0;
    int pending = fdbio->m_wbuf_used - fdbio->m_wbuf_sent;
    if (pending <= 0)
        return 1;
    int ret = ls_write(fd, fdbio->m_wbuf + fdbio->m_wbuf_sent, pending);
    int err = errno;
    DEBUG_MESSAGE("[FDBIO] flush_ex write(%d, %p, %d) return %d, errno: %d\n",
           fd, fdbio->m_wbuf + fdbio->m_wbuf_sent, pending, ret, err);
    if (ret >= pending)
    {
        fdbio->m_wbuf_used = 0;
        fdbio->m_wbuf_sent = 0;
        DEBUG_MESSAGE("[FDBIO] FLUSHED\n");
        return 1;
    }
    else
    {
        if (ret > 0)
        {
            fdbio->m_wbuf_sent += ret;
        }
        else if (ret == -1 && err != EAGAIN && err != EWOULDBLOCK)
        {
            errno = err;
            return -1;
        }
        DEBUG_MESSAGE("[FDBIO] partial write, mark WBLOCK.\n");
        fdbio->m_flag |= LS_FDBIO_WBLOCK;
    }
    return 0;
}


static int bio_fd_write(BIO *b, const char *in, int inl)
{
    int ret;
    int err = 0;
    int fd = BIO_get_fd(b, 0);
    ls_fdbio_data *fdbio = LS_FDBUF_FROM_BIO(b);

    DEBUG_MESSAGE("[FDBIO] bio_fd_write: %p, %d bytes on %d\n", b, inl, fd);
    if (fdbio->m_flag & LS_FDBIO_WBLOCK)
    {
        DEBUG_MESSAGE("[FDBIO] bio_fd_write, FDBIO_WBLOCK flag is set, set errno to EAGAIN\n");
        BIO_set_retry_write(b);
        errno = EAGAIN;
        return -1;
    }

    if (fdbio->m_flag & LS_FDBIO_BUFFERING)
    {
        ret = ls_fdbio_buff_write(fdbio, fd, in, inl);
    }
    else if (fdbio->m_wbuf_used > 0)
    {
        ret = ls_fdbio_combine_write(fdbio, fd, in, inl);
    }
    else
        ret = ls_write(fd, (void *)in, inl);
    if ( ret > 0)
    {
        BIO_clear_retry_flags(b);
        return ret;
    }
    err = errno;
    if ((ret == -1) && (!(SIMPLE_RETRY(err))) && (!BIO_fd_should_retry(ret)))
    {
        DEBUG_MESSAGE("[FDBIO] bio_fd_write: actual write failed, errno: %d\n",
                      err);
        return ret;
    }            
    BIO_clear_retry_flags(b);
    if (ret < 0) {
        if ((BIO_fd_should_retry(ret)) || (SIMPLE_RETRY(err)))
        {
            DEBUG_MESSAGE("[FDBIO] bio_fd_write: %p, early return, errno: %d,"
                          " ret: %d\n", b, err, ret);
            BIO_set_retry_write(b);
            errno = EAGAIN;
            //return bio_fd_write(b, in, inl);
            return ret;
        }
    }
    
    DEBUG_MESSAGE("[FDBIO] bio_fd_write: %p, returning: %d\n", b, ret);
    errno = err;
    return ret;
}


static int bio_fd_puts(BIO *bp, const char *str)
{
    int n, ret;
    
    DEBUG_MESSAGE("[FDBIO] bio_fd_puts: %p, %s\n", bp, str);

    n = strlen(str);
    ret = bio_fd_write(bp, str, n);
    return ret;
}


static int bio_fd_gets(BIO *bp, char *buf, int size)
{
    int ret = 0;
    char *ptr = buf;
    char *end = buf + size - 1;
    
    DEBUG_MESSAGE("[BIO] bio_fd_gets: %p\n", bp);

    while (ptr < end && bio_fd_read(bp, ptr, 1) > 0) {
        if (*ptr++ == '\n')
           break;
    }

    ptr[0] = '\0';

    if (buf[0] != '\0')
        ret = strlen(buf);
    return ret;
}

#ifdef OPENSSL_IS_BORINGSSL
static int BIO_fd_should_retry(int i)
{
    int err;

    if ((i == 0) || (i == -1)) {
        err = errno;

        return BIO_fd_non_fatal_error(err);
    }
    return 0;
}


static int BIO_fd_non_fatal_error(int err)
{
    switch (err) {

# ifdef EWOULDBLOCK
#  ifdef WSAEWOULDBLOCK
#   if WSAEWOULDBLOCK != EWOULDBLOCK
    case EWOULDBLOCK:
#   endif
#  else
    case EWOULDBLOCK:
#  endif
# endif

# if defined(ENOTCONN) && defined(ENOT)
    case ENOT:
# endif

# ifdef EINTR
    case EINTR:
# endif

# ifdef EAGAIN
#  if EWOULDBLOCK != EAGAIN
    case EAGAIN:
#  endif
# endif

# ifdef EPROTO
    case EPROTO:
# endif

# ifdef EINPROGRESS
    case EINPROGRESS:
# endif

# ifdef EALREADY
    case EALREADY:
# endif
        return 1;
    default:
        break;
    }
    return 0;
}
#endif

static int setup_writes(ls_fdbio_data *fdbio)
{
    if (!fdbio)
        return 0;
    DEBUG_MESSAGE("[BIO] setup_writes\n");
    return 0;
}


BIO *ls_fdbio_create(int fd, ls_fdbio_data *fdbio)
{
    if (!s_biom)
    {
        DEBUG_MESSAGE("[BIO] ls_fdbio_create METHOD\n");
    
        s_biom_fd_builtin = BIO_s_fd();
#if OPENSSL_VERSION_NUMBER >= 0x10100000
        if (!(s_biom = BIO_meth_new(
            BIO_TYPE_FD | BIO_TYPE_SOURCE_SINK | BIO_TYPE_DESCRIPTOR, 
            "Litespeed BIO Method")))
        {
            DEBUG_MESSAGE("[BIO] ERROR creating BIO_METHOD\n");
            return NULL;
        }
#ifdef OPENSSL_IS_BORINGSSL
        memcpy(s_biom, s_biom_fd_builtin, sizeof(BIO_METHOD));
#else
        BIO_meth_set_ctrl(s_biom, BIO_meth_get_ctrl(s_biom_fd_builtin));
        BIO_meth_set_create(s_biom, BIO_meth_get_create(s_biom_fd_builtin));
#endif
        BIO_meth_set_write(s_biom, bio_fd_write);
        BIO_meth_set_read(s_biom, bio_fd_read);
        BIO_meth_set_puts(s_biom, bio_fd_puts);
        BIO_meth_set_gets(s_biom, bio_fd_gets);
        BIO_meth_set_destroy(s_biom, bio_fd_free);
#else
        memcpy(&s_biom_s, s_biom_fd_builtin, sizeof(BIO_METHOD));
        s_biom = &s_biom_s;
        s_biom->bwrite = bio_fd_write;
        s_biom->bread = bio_fd_read;
        s_biom->bputs = bio_fd_puts;
        s_biom->bgets = bio_fd_gets;
        s_biom->destroy = bio_fd_free;
#endif        
    }
    fdbio->m_rbuf_used = 0;
    fdbio->m_rbuf_read = 0;

    BIO *bio = BIO_new(s_biom);
    if (!bio)
    {
        DEBUG_MESSAGE("[BIO: %p] ls_fdbio_create error creating rbio\n",
                       fdbio);
        // everything freed in the destructor
        return NULL;
    }
    BIO_set_fd(bio, fd, 0);
#if OPENSSL_VERSION_NUMBER >= 0x10100000
    BIO_set_data(bio, fdbio);
#else
    BIO_set_app_data(bio, fdbio);
#endif    
    DEBUG_MESSAGE("[BIO] ls_fdbio_create bio: %p\n",
                   bio);
    
    setup_writes(fdbio);
    return bio;
}


static int bio_fd_free(BIO *a)
{
    ls_fdbio_data *fdbio;
    DEBUG_MESSAGE("[BIO] bio_fd_free: %p\n", a);
    if (a == NULL)
        return 0;
    fdbio = LS_FDBUF_FROM_BIO(a);
    if (!fdbio)
        return 1;
    fdbio->m_rbuf_used = fdbio->m_rbuf_read = 0;
    ls_fdbio_release_rbuff(fdbio);
    fdbio->m_wbuf_used = 0;
    ls_fdbio_release_wbuff(fdbio);
    ls_fdbuf_bio_init(fdbio);
    return 1;
}

