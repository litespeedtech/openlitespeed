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

#ifndef THASH_H
#define THASH_H


/**
  *@author George Wang
  */

#include <lsr/xxhash.h>
#include <lsr/ls_types.h>
#include <util/autostr.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <inttypes.h>
#include <lsr/ls_str.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
typedef union ls_sockaddr_st
{
    struct sockaddr     m_sa;
    struct sockaddr_in  m_ipv4;
    struct sockaddr_in6 m_ipv6;
} ls_sa_t;



template<typename ktype, typename htype>
class HashFunc
{
public:
    uint32_t hash_to_slot(htype h, uint32_t buckets) const
    {   return h % buckets;     }
    htype key_to_hash(ktype k) const
    {   return k;               }
    bool comp_hash(htype h1, htype h2) const
    {   return h1 == h2;    }
    bool comp_key(const ktype &k1, const ktype &k2) const
    {   return k1 == k2;    }
};


template <>
class HashFunc <ls_str_t, uint32_t>
{
public:
    uint32_t hash_to_slot(uint32_t h, uint32_t buckets) const
    {   return h % buckets;     }
    uint32_t key_to_hash(const ls_str_t &k) const
    {   return XXH32(k.ptr, k.len, 0);               }
    bool comp_hash(uint32_t h1, uint32_t h2) const
    {   return h1 == h2;    }
    bool comp_key(const ls_str_t &k1, const ls_str_t &k2) const
    {   return k1.len == k2.len && memcmp(k1.ptr, k2.ptr, k1.len) == 0;   }
};


template<>
class HashFunc <ls_sa_t, uint32_t>
{
public:
    uint32_t hash_to_slot(uint32_t h, uint32_t buckets) const
    {   return h % buckets;     }

    uint32_t hash_ipv6(const struct in6_addr *addr) const
    {
        return *((const uint32_t *)addr) +
              *(((const uint32_t *)addr) + 1) +
              *(((const uint32_t *)addr) + 2) +
              *(((const uint32_t *)addr) + 3);
    }

    uint32_t key_to_hash(const ls_sa_t &k) const
    {   return (k.m_sa.sa_family == AF_INET)
                ? k.m_ipv4.sin_addr.s_addr
                : hash_ipv6(&k.m_ipv6.sin6_addr);
    }

    bool comp_hash(uint32_t h1, uint32_t h2) const
    {   return h1 == h2;    }
    bool comp_key(const ls_sa_t &k1, const ls_sa_t &k2) const
    {   return k1.m_sa.sa_family == k2.m_sa.sa_family
               && (k1.m_sa.sa_family == AF_INET)
                   ? k1.m_ipv4.sin_addr.s_addr == k2.m_ipv4.sin_addr.s_addr
                   : memcmp(&k1.m_ipv6.sin6_addr, &k2.m_ipv6.sin6_addr, sizeof(struct in6_addr)) == 0;
    }
};


template <>
class HashFunc <ls_str_t, uint64_t>
{
public:
    uint32_t hash_to_slot(uint64_t h, uint32_t buckets) const
    {   return h % buckets;     }
    uint32_t key_to_hash(const ls_str_t &k) const
    {   return XXH64(k.ptr, k.len, 0);               }
    bool comp_hash(uint64_t h1, uint64_t h2) const
    {   return h1 == h2;    }
    bool comp_key(const ls_str_t *k1, const ls_str_t *k2) const
    {   return k1->len == k2->len && memcmp(k1->ptr, k2->ptr, k1->len) == 0;   }
};


template <>
class HashFunc <AutoStr2, uint32_t>
{
public:
    uint32_t hash_to_slot(uint32_t h, uint32_t buckets) const
    {   return h % buckets;     }
    uint32_t key_to_hash(const AutoStr2 &k) const
    {   return XXH32(k.c_str(), k.len(), 0);               }
    bool comp_hash(uint32_t h1, uint32_t h2) const
    {   return h1 == h2;    }
    bool comp_key(const AutoStr2 &k1, const AutoStr2 &k2) const
    {   return k1.len() == k2.len() && memcmp(k1.c_str(), k2.c_str(), k1.len()) == 0;   }
};


template <>
class HashFunc <AutoStr2, uint64_t>
{
public:
    uint32_t hash_to_slot(uint64_t h, uint32_t buckets) const
    {   return h % buckets;     }
    uint32_t key_to_hash(const AutoStr2 &k) const
    {   return XXH64(k.c_str(), k.len(), 0);               }
    bool comp_hash(uint64_t h1, uint64_t h2) const
    {   return h1 == h2;    }
    bool comp_key(const AutoStr2 &k1, const AutoStr2 &k2) const
    {   return k1.len() == k2.len() && memcmp(k1.c_str(), k2.c_str(), k1.len()) == 0;   }
};


template<typename T, typename ktype,
        typename H = HashFunc<ktype, ktype> >
class HentryInt
{
public:
    HentryInt()
        : m_next_hentry(NULL)
        , m_hash(0)
    {}

    ~HentryInt()
    {}

    void set_key(const ktype &key)
    {   m_hash = key;     }

    const ktype &get_key() const
    {   return m_hash;     }

    ktype key_to_hash() const
    {   return m_hash;     }

    ktype get_hash() const
    {   return m_hash;  }

    uint32_t hash_to_slot(uint32_t slots) const
    {   H h; return h.hash_to_slot(m_hash, slots);       }

    bool is_equal(ktype h, ktype key)
    {   return h == m_hash;     }

    T *get_next_hentry() const  {   return m_next_hentry;     }
    void set_next_hentry(T *n)  {   m_next_hentry = n;        }
    T **get_next_hentry_ptr()   {   return &m_next_hentry;    }

private:
    T              *m_next_hentry;
    ktype           m_hash;
};


template<typename T, typename ktype, typename htype = uint32_t,
        typename H = HashFunc<ktype, htype> >
class Hentry
{
public:
    Hentry()
        : m_next_hentry(NULL)
        , m_hash(0)
    {}

    ~Hentry()
    {}

    void set_key(const ktype &key)
    {   m_key = key;     }

    const ktype &get_key() const
    {   return m_key;     }

    ktype &get_key()
    {   return m_key;     }

    htype key_to_hash()
    {   H h; return (m_hash = h.key_to_hash(m_key));     }

    htype get_hash() const
    {   return m_hash;  }

    uint32_t hash_to_slot(uint32_t slots) const
    {   H h; return h.hash_to_slot(m_hash, slots);       }

    bool is_equal(htype hash, const ktype &key)
    {   H h; return hash == m_hash && h.comp_key(key, m_key);     }

    T *get_next_hentry() const  {   return m_next_hentry;     }
    void set_next_hentry(T *n)  {   m_next_hentry = n;        }
    T **get_next_hentry_ptr()   {   return &m_next_hentry;    }

private:
    T              *m_next_hentry;
    ktype           m_key;
    htype           m_hash;
};


#include <stdlib.h>
#include <string.h>
template<typename T, typename ktype, typename htype = uint32_t,
         typename hfunc = HashFunc<ktype, htype> >
class Thash
{
public:

    typedef T* iterator;
    typedef const T* const_iterator;
    typedef int (*for_each_fn)(iterator iter);
    typedef int (*for_each_fn2)(iterator iter, void *pUData);

    explicit Thash(size_t bucket_count)
        : m_buckets(NULL)
        , m_bucket_count(0)
        , m_size(0)
    {
        if (bucket_count < 8)
            bucket_count = 8;
        allocate(bucket_count);
    }

    ~Thash()
    {
        if (m_buckets)
        {
            free(m_buckets);
            m_buckets = NULL;
            m_bucket_count = 0;
        }
    }

    int allocate(int n)
    {
        T **buckets = (T **)malloc(n * sizeof(T *));
        if (!buckets)
            return -1;
        memset(buckets, 0, sizeof(T *) * n);
        m_bucket_count = n;
        m_buckets = buckets;
        return 0;
    }

    uint32_t size() const   {   return m_size;  }

    T *insert(T *entry)
    {
        if (m_size >= m_bucket_count)
            rehash();
        htype hash = entry->key_to_hash();
        uint32_t slot = entry->hash_to_slot(m_bucket_count);
        T* old = m_buckets[slot];
        while(old)
        {
            if (old->is_equal(hash, entry->get_key()))
                return old;
            old = old->get_next_hentry();
        }
        entry->set_next_hentry(m_buckets[slot]);
        m_buckets[slot] = entry;
        ++m_size;
        return entry;
    }

    T *insert_update(T *entry)
    {
        if (m_size >= m_bucket_count)
            rehash();
        htype hash = entry->key_to_hash();
        uint32_t slot = entry->hash_to_slot(m_bucket_count);
        T *old = m_buckets[slot];
        T **prev = &m_buckets[slot];
        while(old)
        {
            if (old->is_equal(hash, entry->get_key()))
            {
                entry->set_next_hentry(old->get_next_hentry());
                break;
            }
            prev = old->get_next_hentry_ptr();
            old = old->get_next_hentry();
        }
        if (!old)
        {
            entry->set_next_hentry(NULL);
            ++m_size;
        }
        *prev = entry;
        return old;
    }

    T *find(const ktype &key) const
    {
        hfunc fun;
        uint32_t h = fun.key_to_hash(key);
        uint32_t slot = fun.hash_to_slot(h, m_bucket_count);
        T *entry = m_buckets[slot];
        while(entry)
        {
            if (entry->is_equal(h, key))
                return entry;
            entry = entry->get_next_hentry();
        }
        return NULL;
    }

    int erase(T *entry)
    {
        if (!entry)
            return -1;
        uint32_t slot = entry->hash_to_slot(m_bucket_count);
        if (entry == m_buckets[slot])
        {
            m_buckets[slot] = entry->get_next_hentry();
            --m_size;
            return 1;
        }
        T *prev = m_buckets[slot];
        while(prev)
        {
            if (prev->get_next_hentry() == entry)
            {
                prev->set_next_hentry(entry->get_next_hentry());
                --m_size;
                return 1;
            }
            prev = prev->get_next_hentry();
        }
        return 0;
    }

    T *remove(const ktype &key)
    {
        hfunc fun;
        uint32_t h = fun.key_to_hash(key);
        uint32_t slot = fun.hash_to_slot(h, m_bucket_count);
        T *entry = m_buckets[slot];
        T **prev = &m_buckets[slot];
        while(entry)
        {
            if (entry->is_equal(h, key))
            {
                *prev = entry->get_next_hentry();
                --m_size;
                return entry;
            }
            prev = entry->get_next_hentry_ptr();
            entry = entry->get_next_hentry();
        }
        return NULL;
    }

    T *begin() const
    {
        uint32_t slot = 0;
        return next(slot);
    }

    T *end() const
    {   return NULL;    }

    T *next(T *iter) const
    {
        if (!iter)
            return NULL;
        if (iter->get_next_hentry())
            return iter->get_next_hentry();
        int slot = iter->hash_to_slot(m_bucket_count) + 1;
        return next(slot);
    }

    int rehash()
    {
        uint32_t new_size = m_bucket_count << 1;
        while(new_size < m_size)
            new_size = new_size << 1;
        T **buckets = (T **)malloc(new_size * sizeof(T *));
        if (!buckets)
            return -1;
        memset(buckets, 0, sizeof(T *) * new_size);
        T *iterNext = begin();

        while (iterNext != end())
        {
            T *iter = iterNext;
            iterNext = next(iter);
            T **slot = buckets + iter->hash_to_slot(new_size);
            iter->set_next_hentry(*slot);
            *slot = iter;
        }
        free(m_buckets);
        m_buckets = buckets;
        m_bucket_count = new_size;
        return 0;
    }


    int for_each(iterator begin, iterator end, for_each_fn fun)
    {
        if (!fun)
        {
            errno = EINVAL;
            return -1;
        }
        if (!begin)
            return 0;
        int n = 0;
        iterator iterNext = begin;
        iterator iter ;
        while (iterNext && iterNext != end)
        {
            iter = iterNext;
            iterNext = next(iterNext);
            if (fun(iter))
                break;
            ++n;
        }
        return n;
    }

    int for_each2(iterator beg, iterator end, for_each_fn2 fun, void *pUData)
    {
        if (!fun)
        {
            errno = EINVAL;
            return -1;
        }
        if (!beg)
            return 0;
        int n = 0;
        iterator iterNext = beg;
        iterator iter ;
        while (iterNext && iterNext != end)
        {
            iter = iterNext;
            iterNext = next(iterNext);
            if (fun(iter, pUData))
                break;
            ++n;
        }
        return n;
    }

    void clear()
    {
        ::memset(m_buckets, 0, sizeof(T *) * m_bucket_count);
        m_size = 0;
    }

    static int deleteObj(iterator iter)
    {
        delete iter;
        return 0;
    }

    void release_objects()
    {
        for_each(begin(), end(), deleteObj);
        clear();
    }

//     T *pop(T *iter)
//     {
//         if (!iter)
//             return NULL;
//         if (iter->getHashNext())
//             return iter->getHashNext();
//         int slot = iter->getHash() / m_bucket_count + 1;
//         return next(slot);
//     }

private:
    T  *next(uint32_t slot) const
    {
        while(slot < m_bucket_count)
        {
            if (m_buckets[slot])
                return m_buckets[slot];
            ++slot;
        }
        return NULL;
    }

private:
    T             **m_buckets;
    uint32_t        m_bucket_count;
    uint32_t        m_size;
};


#endif
