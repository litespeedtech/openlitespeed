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
#ifndef GSOCKADDR_H
#define GSOCKADDR_H


#include <inttypes.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define DO_NSLOOKUP     1
#define NO_ANY          2
#define ADDR_ONLY       4
#define DO_NSLOOKUP_DIRECT  8
#define DO_HOST_OVERRIDE    16
#define SKIP_ADNS_CACHE     32

typedef union
{
    in6_addr    m_addr6;
    in_addr_t   m_addrs[4];
} IpAddr6;


class AdnsReq;
class GSockAddr
{
private:
    union
    {
        struct sockaddr      *m_pSockAddr;
        struct sockaddr_in   *m_v4;
        struct sockaddr_in6 *m_v6;
        struct sockaddr_un   *m_un;
    };

    uint16_t m_len;
    uint16_t m_flag;

    int allocate(int family);
    void release();

    int doLookup(int family, const char *p, int tag);

public:

    // There is no explicit need to make this private, it is just not used outside of this class.
    void reinit()
    {
        m_pSockAddr = NULL;
        m_len = 0;
        m_flag = 0;
    }

    GSockAddr()
    {
        reinit();
    }
    explicit GSockAddr(int family)
    {
        reinit();
        allocate(family);
    }
    GSockAddr(const in_addr_t addr, const in_port_t port)
    {
        reinit();
        set(addr, port);
    }
    explicit GSockAddr(const struct sockaddr *pAddr)
    {
        reinit();
        allocate(pAddr->sa_family);
        memmove(m_pSockAddr, pAddr, m_len);
    }
    GSockAddr(const GSockAddr &rhs);
    GSockAddr &operator=(const GSockAddr &rhs)
    {   return operator=(*(rhs.m_pSockAddr));    }

    GSockAddr &operator=(const struct sockaddr &rhs);
    GSockAddr &operator=(const in_addr_t addr);
    operator const struct sockaddr *() const   {   return m_pSockAddr;  }
    explicit GSockAddr(const struct sockaddr_in &rhs)
    {
        reinit();
        allocate(AF_INET);
        memmove(m_pSockAddr, &rhs, sizeof(rhs));
    }

    ~GSockAddr()                {   release();          }
    struct sockaddr *get()     {   return m_pSockAddr; }

    int family() const
    {
        if (m_pSockAddr)
            return m_pSockAddr->sa_family;
        return -1;
    }

    int len() const
    {   return m_len;   }

    const struct sockaddr *get() const
    {   return m_pSockAddr; }
    const struct sockaddr_in *getV4() const
    {   return (const struct sockaddr_in *)m_pSockAddr;  }
    const struct sockaddr_in6 *getV6() const
    {   return (const struct sockaddr_in6 *)m_pSockAddr; }
    const char *getUnix() const
    {   return ((sockaddr_un *)m_pSockAddr)->sun_path;  }

    static inline void mappedV6toV4( struct sockaddr * pAddr)
    {
        if ((AF_INET6 == pAddr->sa_family)&&
            (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)pAddr)->sin6_addr)))
        {
            pAddr->sa_family = AF_INET;
            ((struct sockaddr_in *)pAddr)->sin_addr.s_addr = ((IpAddr6 *)&
                ((struct sockaddr_in6 *)pAddr)->sin6_addr)->m_addrs[3];
        }
    }

    void set(const in_addr_t addr, const in_port_t port);

    void set(const in6_addr *addr, const in_port_t port,
             uint32_t flowinfo = 0);
    int set(const char *pURL, int tag);
    int set(int family, const char *pURL, int tag = 0);

    void set(const struct sockaddr *addr)
    {
        if ((!m_pSockAddr) || (m_pSockAddr->sa_family != addr->sa_family))
            allocate(addr->sa_family);
        if (m_pSockAddr)
            memmove(m_pSockAddr, addr, m_len);
    }

    void set(const GSockAddr *addr)
    {   set(addr->get());   }


    int set2(int family, const char *pURL, int tag, char *pDest);

    int asyncSet(int family, const char *pURL, int tag
                 , int (*lookup_pf)(void *arg, const long lParam, void *pParam)
                 , void *ctx, AdnsReq **pReq);

    int setHttpUrl(const char *pHttpUrl, const int len);
    int parseAddr(const char *pString);
    /** return the address in string format. */
    static const char *ntop(const struct sockaddr *pAddr, char *pBuf, int len);
    static uint16_t getPort(const struct sockaddr *pAddr);

    const char *toAddrString(char *pBuf, int len) const
    {   return ntop(m_pSockAddr, pBuf, len);      }
    /** return the address and port in string format. */
    const char *toString(char *pBuf, int len) const;
    const char *toString() const;
    uint16_t getPort() const;
    void setPort(uint16_t port);

    static int compareAddr(const struct sockaddr *pAddr1,
                           const struct sockaddr *pAddr2);
    static int compare(const struct sockaddr *pAddr1,
                       const struct sockaddr *pAddr2);
    static int setIp(struct sockaddr *result, const void *ip, int len);
};

#endif
