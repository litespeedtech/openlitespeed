
#ifndef __LS_SENDMMSG__
#define __LS_SENDMMSG__

#ifdef __linux__
#include <sys/syscall.h>
#include <errno.h>

#ifndef __NR_sendmmsg

#if defined(__i386__)

#define __NR_sendmmsg 345

#elif defined( __x86_64 )||defined( __x86_64__ )

#define __NR_sendmmsg 307

//#elif defined(__arm__)
//#define __NR_sendmmsg                       374
//#elif defined(__arm64__)
//#define __NR_sendmmsg                       269

#endif //defined(__i386__)

struct mmsghdr
  {
    struct msghdr msg_hdr;      /* Actual message header.  */
    unsigned int msg_len;       /* Number of received or sent bytes for the
                                   entry.  */
  };

#endif  //__NR_sendmmsg


static inline int ls_sendmmsg(int __fd, struct mmsghdr *__vmessages,
                     unsigned int __vlen, int __flags)
{
    return (syscall(__NR_sendmmsg, __fd, __vmessages, __vlen, __flags ));
}


static inline bool is_sendmmsg_available()
{
    ls_sendmmsg(-1, NULL, 0, 0);
    return (errno != ENOSYS);
}


#else   //__linux__

#define ls_sendmmsg sendmmsg

static inline bool is_sendmmsg_available()
{
    return false;
}

#endif  //__linux__

#endif  //__LS_SENDMMSG__


