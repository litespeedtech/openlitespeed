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
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <lsr/ls_pool.h>
#include <socket/ls_sock.h>
#include <sslpp/ls_fdbuf_bio.h>

#ifdef RUN_TEST
#define DEBUGGING
#endif

#ifdef DEBUGGING

#ifdef TEST_PGM
#define DEBUG_MESSAGE(...)     printf(__VA_ARGS__);
#define INFO_MESSAGE(...)      printf(__VA_ARGS__);
#else
#include <ls.h>
#define DEBUG_MESSAGE(...)     LSI_DBG(NULL, __VA_ARGS__);
#define INFO_MESSAGE(... )     LSI_INF(NULL, __VA_ARGS__);
#endif

#else

#define DEBUG_MESSAGE(...)
#define INFO_MESSAGE(...)

#endif

const int RBIO_BUF_SIZE = 2048; // Initial size
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

#define SIMPLE_RETRY(err) ((err == EAGAIN) || (err = EWOULDBLOCK))

#if OPENSSL_VERSION_NUMBER >= 0x10100000
void *BIO_get_data(BIO *a);
#define LS_FDBUF_FROM_BIO(b) (ls_fdbio_data *)BIO_get_data(b)
#else
#define LS_FDBUF_FROM_BIO(b) (ls_fdbio_data *)BIO_get_app_data(b)
#endif

void ls_fdbuf_bio_init(ls_fdbio_data *fdbio)
{
    memset(fdbio, 0, sizeof(*fdbio));
}




static int bio_fd_free(BIO *a)
{
    ls_fdbio_data *fdbio;
    DEBUG_MESSAGE("[BIO: %d] bio_fd_free: %p\n", getpid(), a);
    if (a == NULL)
        return 0;
    fdbio = LS_FDBUF_FROM_BIO(a);
    if (!fdbio)
        return 1;
    if (fdbio->m_rbioBuf)
    {
        ls_pfree(fdbio->m_rbioBuf);
        fdbio->m_rbioBuf = NULL;
    }
    ls_fdbuf_bio_init(fdbio);
    return 1;
}

static int bio_fd_read(BIO *b, char *out, int outl)
{
    int ret = 0;
    int err = 0;
    ls_fdbio_data *fdbio = LS_FDBUF_FROM_BIO(b);
    int fd = BIO_get_fd(b, 0);
    int total = 0;
    DEBUG_MESSAGE("[BIO: %d] bio_fd_read: %p, %d bytes on %d\n", getpid(), b, 
                  outl, fd);
    if ((out == NULL) || (!outl))
    {
        INFO_MESSAGE("[BIO: %d] bio_fd_read NO BUFFER!!\n", getpid());
        err = ENOMEM;
        return -1;
    }
    if (fdbio->m_rbioClosed)
    {
        DEBUG_MESSAGE("[BIO: %d] bio_fd_read: CLOSED ON PREVIOUS READ\n", getpid());
        errno = 0;
        return 0;
    }
    while (total < outl)
    {
        int buffered = fdbio->m_rbioBuffered - fdbio->m_rbioIndex;
        int rd_remaining = outl - total;
        int copy = 0;
        if (buffered)
        {
            copy = (buffered > rd_remaining) ? rd_remaining : buffered;
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: Use buffered %d of %d\n", 
                          getpid(), copy, rd_remaining);
            memcpy(&out[total], &fdbio->m_rbioBuf[fdbio->m_rbioIndex], copy);
            fdbio->m_rbioIndex += copy;
            total += copy;
            if (total >= outl)
            {
                DEBUG_MESSAGE("[BIO: %d] bio_fd_read: GOT TOTAL %d\n", 
                              getpid(), total);
                return total;
            }
            fdbio->m_rbioBuffered = 0;
            fdbio->m_rbioIndex = 0;
            rd_remaining = outl - total;
        }
        if ((rd_remaining < fdbio->m_rbioBufSz) && (fdbio->m_rbioBuf))
        {
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: Read into buffer\n", 
                          getpid());
            ret = ls_read(fd, fdbio->m_rbioBuf, fdbio->m_rbioBufSz);
            if (ret < fdbio->m_rbioBufSz)
                fdbio->m_rbioWaitEvent = 1;
            buffered = ret;
        }
        else
        {
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: Read into data\n", 
                          getpid());
            ret = ls_read(fd, out + total, rd_remaining);
            if (ret < rd_remaining)
                fdbio->m_rbioWaitEvent = 1;
            buffered = 0;
        }
        if (ret == 0)
        {
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: CLOSED\n", getpid());
            fdbio->m_rbioClosed = 1;
            errno = 0;
            return total;
        }
        if (ret < 0) 
        {
            errno = err;
            if (total)
            {
                DEBUG_MESSAGE("[BIO: %d] bio_fd_read: error but I have data\n", 
                              getpid());
                return total;
            }
            if ((SIMPLE_RETRY(err)) || (BIO_fd_should_retry(ret)))
            {
                DEBUG_MESSAGE("[BIO: %d] bio_fd_read: set retry read"
                              " errno: %d\n", getpid(), err);
                errno = err;
                BIO_set_retry_read(b);
            }
            return -1;
        }
        if (buffered)
        {
            fdbio->m_rbioBuffered = buffered;
            fdbio->m_rbioIndex = 0;
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: Preserve read: %d\n", 
                          getpid(), ret);
        }
        else
        {
            total += ret;
            DEBUG_MESSAGE("[BIO: %d] bio_fd_read: Read into data: %d\n", 
                          getpid(), ret);
        }
    }
    return total;
}


static int bio_fd_write(BIO *b, const char *in, int inl)
{
    int ret;
    int err = 0;
    int fd = BIO_get_fd(b, 0);

    DEBUG_MESSAGE("[BIO: %d] bio_fd_write: %p, %d bytes on %d\n", getpid(), b, 
                  inl, fd);
    ret = ls_write(fd, (void *)in, inl);
    if ( ret > 0)
    {
        BIO_clear_retry_flags(b);
        return ret;
    }
    err = errno;
    if ((ret == -1) && (!(SIMPLE_RETRY(err))) && (!BIO_fd_should_retry(ret)))
    {
        DEBUG_MESSAGE("[BIO: %d] bio_fd_write: actual write failed, errno: %d\n",
                      getpid(), err);
        return ret;
    }            
    BIO_clear_retry_flags(b);
    if (ret < 0) {
        if ((BIO_fd_should_retry(ret)) || (SIMPLE_RETRY(err)))
        {
            DEBUG_MESSAGE("[BIO: %d] bio_fd_write: %p, early return, errno: %d,"
                          " ret: %d\n", getpid(), b, err, ret);
            BIO_set_retry_write(b);
            errno = EAGAIN;
            //return bio_fd_write(b, in, inl);
            return ret;
        }
    }
    
    DEBUG_MESSAGE("[BIO: %d] bio_fd_write: %p, returning: %d\n", getpid(), b, 
                  ret);
    errno = err;
    return ret;
}


static int bio_fd_puts(BIO *bp, const char *str)
{
    int n, ret;
    
    DEBUG_MESSAGE("[BIO: %d] bio_fd_puts: %p, %s\n", getpid(), bp, str);

    n = strlen(str);
    ret = bio_fd_write(bp, str, n);
    return ret;
}


static int bio_fd_gets(BIO *bp, char *buf, int size)
{
    int ret = 0;
    char *ptr = buf;
    char *end = buf + size - 1;
    
    DEBUG_MESSAGE("[BIO: %d] bio_fd_gets: %p\n", getpid(), bp);

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
    DEBUG_MESSAGE("[BIO: %d] setup_writes\n", getpid());
    return 0;
}


BIO *ls_fdbio_create(int fd, ls_fdbio_data *fdbio)
{
    if (!s_biom)
    {
        DEBUG_MESSAGE("[BIO: %d] ls_fdbio_create METHOD\n", getpid());
    
        s_biom_fd_builtin = BIO_s_fd();
#if OPENSSL_VERSION_NUMBER >= 0x10100000
        if (!(s_biom = BIO_meth_new(
            BIO_TYPE_FD | BIO_TYPE_SOURCE_SINK | BIO_TYPE_DESCRIPTOR, 
            "Litespeed BIO Method")))
        {
            DEBUG_MESSAGE("[BIO: %d] ERROR creating BIO_METHOD\n", 
                          getpid());
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
    if (!fdbio->m_rbioBufSz)
        fdbio->m_rbioBufSz = RBIO_BUF_SIZE;
    
    if (!(fdbio->m_rbioBuf = (char *)ls_palloc(fdbio->m_rbioBufSz)))
    {
        DEBUG_MESSAGE("Insufficient memory allocating %d bytes\n",
                      fdbio->m_rbioBufSz);
        errno = ENOMEM;
        return NULL;
    }
    fdbio->m_rbioBuffered = 0;
    fdbio->m_rbioIndex = 0;

    BIO *bio = BIO_new(s_biom);
    if (!bio)
    {
        DEBUG_MESSAGE("[BIO: %d:%p] ls_fdbio_create error creating rbio\n",
                      getpid(), fdbio);
        // everything freed in the destructor
        return NULL;
    }
    BIO_set_fd(bio, fd, 0);
#if OPENSSL_VERSION_NUMBER >= 0x10100000
    BIO_set_data(bio, fdbio);
#else
    BIO_set_app_data(bio, fdbio);
#endif    
    DEBUG_MESSAGE("[BIO: %d] ls_fdbio_create bio: %p\n",
                  getpid(), bio);
    
    setup_writes(fdbio);
    return bio;
}



