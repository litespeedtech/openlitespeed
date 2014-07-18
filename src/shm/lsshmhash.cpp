/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <http/httplog.h>
#if 0
#include <log4cxx/logger.h>
#endif

#ifdef DEBUG_RUN
using namespace LOG4CXX_NS;
#endif

enum { prime_count = 31    };
static const size_t s_prime_list[prime_count] =
{
  7ul,          13ul,         29ul,  
  53ul,         97ul,         193ul,       389ul,       769ul,
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
  1610612741ul, 3221225473ul, 4294967291ul
};

static int findRange( size_t sz )
{
    int i = 1;
    for( ; i < prime_count - 1; ++i )
    {
        if ( sz <= s_prime_list[i] )
            break;
    }
    return i;
}

static size_t roundUp( size_t sz )
{
    return s_prime_list[findRange(sz)];
}

// Hash for 32bytes session id
LsShm_hkey_t LsShmHash::hash_32id(const void* __s, int len)
{
    register const uint32_t * lp = (const uint32_t *) __s;
    register LsShm_hkey_t __h = 0;
    
    if (len >= 8)
        __h = *lp ^ *(lp+1);
    else
    {
        while (len >= 4)
        {
            __h ^= *lp++;
            len -= 4;
        }
        // ignore the leftover!
        // if (len)
    }
    return __h;
}

LsShm_hkey_t LsShmHash::hash_buf(const void* __s, int len)
{
    register LsShm_hkey_t __h = 0;
    register const uint8_t * p = (const uint8_t *)__s;
    register uint8_t ch = *(const uint8_t*)p++;

    // we will need a better hash key generator for buf key
    while (--len >= 0)
    {
        ch = *p++;
        __h = __h * 31 + (ch );
    }
    return __h;
}

int  LsShmHash::comp_buf( const void * pVal1, const void * pVal2, int len )
{   return memcmp( (const char *)pVal1, (const char *)pVal2, len);  }

LsShm_hkey_t LsShmHash::hash_string(const void* __s, int len)
{
  register LsShm_hkey_t __h = 0;
  register const char * p = (const char *)__s;
  register char ch = *(const char*)p++;
  for ( ; ch ; ch = *((const char*)p++))
    __h = __h * 31 + (ch );

  return __h;
}

int  LsShmHash::comp_string( const void * pVal1, const void * pVal2, int len )
{   return strcmp( (const char *)pVal1, (const char *)pVal2 );  }

LsShm_hkey_t LsShmHash::i_hash_string(const void* __s, int len)
{
    register LsShm_hkey_t __h = 0;
    register const char * p = (const char *)__s;
    register char ch = *(const char*)p++;
    for ( ; ch ; ch = *((const char*)p++))
    {
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        __h = __h * 31 + (ch );
    }
    return __h;
}

int  LsShmHash::i_comp_string( const void * pVal1, const void * pVal2, int len )
{   return strncasecmp( (const char *)pVal1, (const char *)pVal2, strlen((const char *)pVal1) );  }
    
LsShm_hkey_t LsShmHash::hf_ipv6( const void * pKey, int len )
{
    LsShm_hkey_t key;
    if ( sizeof( LsShm_hkey_t ) == 4 )
    {
        key = *((const LsShm_hkey_t *)pKey) +
              *(((const LsShm_hkey_t *)pKey) + 1 ) +
              *(((const LsShm_hkey_t *)pKey) + 2 ) +
              *(((const LsShm_hkey_t *)pKey) + 3 );
    }
    else
    {
        key = *((const LsShm_hkey_t *)pKey) +
              *(((const LsShm_hkey_t *)pKey) + 1 );
    }
    return key;
}
    
int  LsShmHash::cmp_ipv6( const void * pVal1, const void * pVal2, int len )
{
    return memcmp( pVal1, pVal2, 16 );
}

void LsShmHash::remap()
{
    m_pool->checkRemap();
    if ((m_pShmMap != m_pool->getShmMap()) 
            || (m_capacity != m_table->x_capacity) )
    {
        m_table = (lsShm_hTable_t *)m_pool->offset2ptr( m_offset );
        
#if 0
HttpLog::notice(
                    Logger::getRootLogger()
                    , 
                      "LsShmHash::remap %6d %X %X SIZE %X %X size %d cap %d [%d]"
                    , getpid(), m_pShmMap
                    , m_pool->getShmMap()
                    , m_pool->getShmMap_maxSize()
                    , m_pool->getShmMap_oldMaxSize()
                    , m_table->x_size
                    , m_table->x_capacity
                    , m_capacity
               );
#endif
    
        m_idxStart = (lsShm_hIdx_t *)m_pool->offset2ptr( m_table->x_iTable );
        m_idxEnd = m_idxStart + m_table->x_capacity;
        m_pShmMap = m_pool->getShmMap();
        m_capacity = m_table->x_capacity;
        
        m_pShmLock = m_pool->lockPool()->offset2pLock(m_table->x_lockOffset);
        m_pShmGLock = m_pool->lockPool()->offset2pLock(m_table->x_glockOffset);
    }
    else
    {
#ifdef DEBUG_RUN
HttpLog::notice("LsShmHash::remapXXX NOCHANGE %6d %X %X SIZE %X %X size %d cap %d"
                    , getpid(), m_pShmMap
                    , m_pool->getShmMap()
                    , m_pool->getShmMap_maxSize()
                    , m_pool->getShmMap_oldMaxSize()
                    , m_table->x_size
                    , m_table->x_capacity
               );
#endif
    }
}

LsShmHash::LsShmHash( LsShmPool* pool
                        , const char* name
                        , size_t init_size
                        , LsShmHash::hash_fn hf
                        , LsShmHash::val_comp vc )
    
    : m_magic(LSSHM_HASH_MAGIC)
    , m_table( 0 )
    , m_idxStart( 0 )
    , m_idxEnd( 0 )
    
    , m_pool( pool )
    , m_offset( 0 )
    , m_name(strdup(name))
    
    , m_hf( hf )
    , m_vc( vc )
    , m_objBase(pool->getObjBase())
{
    m_ref = 0;
    m_capacity = 0;
    m_status = LSSHM_NOTREADY;
    m_pShmMap = 0;
    m_pShmLock = 0;
    m_lockEnable = 1;
    
    if ((!m_name) || (strlen(m_name) >= LSSHM_MAXNAMELEN))
        return ;
        
    if ( m_hf )
    {
        assert( m_vc );
        m_insert = insert_p;
        m_update = update_p;
        m_set = set_p;
        m_find = find_p;
        m_get = get_p;
        m_mode = 1;
    }
    else
    {
        m_insert = insert_num;
        m_update = update_num;
        m_set = set_num;
        m_find = find_num;
        m_get = get_num;
        m_mode = 0;
    }
    init_size = roundUp( init_size );

    lsShmReg_t * p_reg = m_pool->findReg(name);
    if (!p_reg)
    {
        if (!(p_reg = m_pool->addReg(name)))
        {
            m_status = LSSHM_BADMAPFILE;
            return;
        }
    }
    m_offset = p_reg->x_value;
    if (! m_offset)
    {
        LsShm_offset_t regOffset = m_pool->ptr2offset(p_reg);
        
        /* Create new HASH Table */
        int remapped = 0;
        
        /* NOTE: system is not up yet... ignore remap here */
        m_offset = m_pool->alloc2( sizeof( lsShm_hTable_t ), remapped );
        if ( !m_offset )
            return;
        LsShm_offset_t iOffset;
        iOffset = m_pool->alloc2( init_size * sizeof( lsShm_hIdx_t ), remapped );
        if (!iOffset)
        {
            m_pool->release2(m_offset, sizeof( lsShm_hTable_t) );
            m_offset = 0;
            return;
        }
        
        /* MAP in the system here... */
        m_pShmMap = m_pool->getShmMap();
        m_table = (lsShm_hTable_t *)m_pool->offset2ptr( m_offset );
        m_table->x_iTable = iOffset;
        
        m_table->x_magic = LSSHM_HASH_MAGIC;
        m_table->x_capacity = init_size;
        m_table->x_size = 0;
        m_table->x_full_factor = 2;
        m_table->x_grow_factor = 2;
        m_table->x_lockOffset = 0;
        
        m_pShmLock = m_pool->lockPool()->allocLock();
        if (!m_pShmLock)
        {
            m_pool->release2(m_offset, sizeof( lsShm_hTable_t));
            m_offset = 0;
            return ;
        }
        m_table->x_lockOffset = m_pool->lockPool()->pLock2offset(m_pShmLock);
        
        m_pShmGLock = m_pool->lockPool()->allocLock();
        if (!m_pShmGLock)
        {
            m_pool->release2(m_offset, sizeof( lsShm_hTable_t));
            m_offset = 0;
            return ;
        }
        m_table->x_glockOffset = m_pool->lockPool()->pLock2offset(m_pShmGLock);
        
        m_idxStart = (lsShm_hIdx_t*)m_pool->offset2ptr( m_table->x_iTable );
        m_idxEnd = m_idxStart + m_table->x_capacity;
        m_capacity = m_table->x_capacity;
        
        /* clear all idx */
        ::memset(m_idxStart, 0, m_table->x_capacity * sizeof(lsShm_hIdx_t) );
        
        lsShmReg_t * xp_reg = (lsShmReg_t *)m_pool->offset2ptr(regOffset);
        
        xp_reg->x_value = m_offset;
        m_table->x_mode = m_mode;
    }
    else
    {
        remap();
        //
        // check the magic and mode
        //
        assert ( (m_magic == m_table->x_magic)
                    && (m_mode == m_table->x_mode) );
        
        if ( (m_magic != m_table->x_magic) || (m_mode != m_table->x_mode) )
            return;
    }
    
    // setup local and global lock
    m_pShmLock = m_pool->lockPool()->offset2pLock(m_table->x_lockOffset);
    m_pShmGLock = m_pool->lockPool()->offset2pLock(m_table->x_glockOffset);
    if (m_offset && (!setupLock()) && (!setupGLock()))
    {
        m_ref = 1;
        m_status = LSSHM_READY;
#ifdef DEBUG_RUN
        HttpLog::notice(
                "LsShmHash::LsShmHash insert %s <%p>"
                , m_name, &m_objBase);
#endif
        m_objBase.insert(m_name, this);
    }
    else
    {
        m_status = LSSHM_ERROR;
    }
}

LsShmHash::~LsShmHash()
{
    if (m_name)
    {
        if (!m_ref)
        {
#ifdef DEBUG_RUN
            HttpLog::notice(
                "LsShmHash::~LsShmHash remove %s <%p>"
                , m_name, &m_objBase);
#endif
            m_objBase.remove(m_name);
        }
        
        free (m_name);
        m_name = NULL;
    }
}

//
//  @brief get - return default hash map
//
LsShmHash* LsShmHash::get( size_t init_size, 
                           LsShmHash::hash_fn hf, 
                           LsShmHash::val_comp vc )
{
    LsShmPool * pool = LsShmPool::get();
    
    if (pool)
    {
        LsShmHash * hash = get(pool, LSSHM_SYSHASH, init_size, hf, vc);
        if (hash)
        {
            hash->m_poolOwner = 1;
            return hash;
        }
        pool->unget();
    }
    return NULL;
}

//
//  @brief get - return the hash map by itself with system pool
//
LsShmHash* LsShmHash::get( const char* name, 
                           size_t init_size, 
                           LsShmHash::hash_fn hf, 
                           LsShmHash::val_comp vc )
{
    if (!name)
        return get(init_size, hf, vc);
    
    LsShm * map = LsShm::get(name, LSSHM_INITSIZE);
    if (map)
    {
        LsShmPool * pool = LsShmPool::get(map, LSSHM_SYSPOOL);
        if (pool)
        {
            LsShmHash * hash = get(pool, name, init_size, hf, vc);
            if (hash)
            {
                hash->m_poolOwner = 1;
                return hash;
            }
            pool->unget();
        }
        map->unget();
    }
    return NULL;
}

LsShmHash* LsShmHash::get( LsShmPool* pool, 
                           const char* name,
                           size_t init_size,
                           LsShmHash::hash_fn hf,
                           LsShmHash::val_comp vc )
{
    register LsShmHash * pObj;
    GHash::iterator itor;
    
    if (!pool)
        return get(name, init_size, hf, vc);
    
    if (!name)
        name = LSSHM_SYSHASH;
    
#ifdef DEBUG_RUN
    HttpLog::notice(
            "LsShmHash::get find %s <%p>"
            , name, &pool->getObjBase());
#endif
    itor = pool->getObjBase().find(name);
    if (itor)
    {
#ifdef DEBUG_RUN
        HttpLog::notice(
                "LsShmHash::get find %s <%p> return <%p>"
                , name, &pool->getObjBase(), itor);
#endif
        if ( ( pObj = (LsShmHash*) itor->second() )
                    && (pObj->m_magic == LSSHM_HASH_MAGIC)
                    && (pObj->m_hf == hf) 
                    && (pObj->m_vc == vc) )
        {
            pObj->up_ref();
            return pObj;
        }
        return NULL; // bad the parameter is not matching
    }
    pObj = new LsShmHash(pool, name, init_size, hf, vc);
    if (pObj)
    {
        if (pObj->m_ref)
            return pObj;
        delete pObj;
    }
    return NULL;
}

void LsShmHash::unget()
{
    LsShmPool *p = NULL;
    if ( m_poolOwner )
    {
        m_poolOwner = 0;
        p = m_pool;
    }
    if ( down_ref() == 0 )
    {
        delete this;
    }
    if (p)
        p->unget();
}

//
//  The only way to remove from the Shared Memory
//
void LsShmHash::destroy()
{
    if ( m_offset )
    {
        // all elements
        clear();
        if (m_table->x_iTable)
            m_pool->release2 (m_table->x_iTable, m_table->x_capacity * sizeof( lsShm_hIdx_t ) );
        m_pool->release2 (m_offset, sizeof( lsShm_hTable_t ) );
        
        // remove from regMap
        lsShmReg_t * p_reg;
        p_reg = m_pool->findReg(m_name);
        p_reg->x_value = 0; 
        m_offset = 0;
        m_table = 0;
        m_idxEnd = 0;
        m_idxStart = 0;
    }
}

int LsShmHash::rehash()
{
    
    remap();
    int range = findRange( m_table->x_capacity );
    int newSize = s_prime_list[ range + m_table->x_grow_factor ];
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::rehash %6d %X %X size %d cap %d NEW %d"
            , getpid(), m_pShmMap, m_pool->getShmMap()
            , m_table->x_size
            , m_table->x_capacity
            , newSize
                   );
#endif
    
    int remapped = 0;
    LsShm_offset_t newIdxOffset = m_pool->alloc2( newSize * sizeof( lsShm_hIdx_t ), remapped );
    if (!newIdxOffset)
        return -1;
    
    // if (remapped)
    remap();
    lsShm_hIdx_t * pIdx;
    pIdx = (lsShm_hIdx_t*)m_pool->offset2ptr(newIdxOffset);
    
    ::memset( pIdx, 0, sizeof( lsShm_hIdx_t ) * newSize  );
    iterator iterNext = begin();

    while( iterNext != end() )
    {
        iterator iter = iterNext;
        iterNext = next( iter );
        
        lsShm_hIdx_t * npIdx = pIdx + getIndex ( iter->x_hkey, newSize );
        assert( npIdx->x_offset < m_pool->getShmMap()->x_maxSize );
        iter->x_next = npIdx->x_offset;
        npIdx->x_offset = m_pool->ptr2offset(iter);
    }
    m_pool->release2(m_table->x_iTable, 
                     m_table->x_capacity * sizeof(lsShm_hIdx_t));
    
    m_table->x_iTable =  newIdxOffset;
    m_table->x_capacity = newSize;
    m_capacity = newSize;
    m_idxStart = (lsShm_hIdx_t *)m_pool->offset2ptr( m_table->x_iTable );
    m_idxEnd = m_idxStart + m_table->x_capacity;
    remap();
    return 0;
}

int LsShmHash::release_hash_elem( LsShmHash::iterator iter, void* pUData )
{
    LsShmHash * pThis = (LsShmHash *)pUData;
    pThis->m_pool->release2( pThis->m_pool->ptr2offset( iter ), iter->x_len );
    return 0;
}

void LsShmHash::clear()
{
    int n = for_each2( begin(), end(), release_hash_elem, this );
    assert( n == (int)m_table->x_size );
    
    ::memset( m_idxStart, 0, sizeof( lsShm_hIdx_t) * m_table->x_capacity );
    m_table->x_size = 0;
}

//
// @brief erase - remove iter from the SHM pool.
// @brief will destroy the link to itself if any!
//
void LsShmHash::erase_iterator_helper( iterator iter )
{
    if ( !iter )
        return;

    LsShm_offset_t iterOffset = m_pool->ptr2offset(iter);
    lsShm_hIdx_t * pIdx = m_idxStart + getIndex( iter->x_hkey, m_table->x_capacity );
    LsShm_offset_t offset = pIdx->x_offset;
    register lsShm_hElem_t * pElem;
    
#ifdef DEBUG_RUN
    if (!offset)
    {
        HttpLog::notice("LsShmHash::erase_iterator_helper %6d %X %X size %d cap %d"
            , getpid(), m_pShmMap, m_pool->getShmMap()
            , m_table->x_size
            , m_table->x_capacity
        );
        sleep(10);
    }
#endif
    
    if ( offset == iterOffset )
        pIdx->x_offset = iter->x_next;
    else
    {
        do {
            pElem = (lsShm_hElem_t *) m_pool->offset2ptr(offset);
            if (pElem->x_next == iterOffset)
            {
                pElem->x_next = iter->x_next;
                break;
            }
            // next offset...
            offset = pElem->x_next;
        } while (offset);
    }
    
    m_pool->release2(iterOffset, iter->x_len);
    m_table->x_size--;
}

LsShmHash::iterator LsShmHash::find_num( LsShmHash *pThis, 
                                         const void * pKey, int keyLen )
{
    lsShm_hIdx_t * pIdx = pThis->m_idxStart + 
            pThis->getIndex( (LsShm_hkey_t)(long)pKey, pThis->m_table->x_capacity );
    LsShm_offset_t offset = pIdx->x_offset;
    register lsShm_hElem_t * pElem;
    
    while ( offset )
    {
        pElem = (lsShm_hElem_t*)pThis->m_pool->offset2ptr(offset);
        // check to see if the key is the same 
        if ( (pElem->x_hkey == (LsShm_hkey_t)(long)pKey)
                && ((*(LsShm_hkey_t *)pElem->p_key()) == (LsShm_hkey_t)(long) pKey ))
            return pElem;
        offset = pElem->x_next;
    }
    return pThis->end();
}

LsShmHash::iterator LsShmHash::find2( const void * pKey, int keyLen, LsShm_hkey_t key )
{
    lsShm_hIdx_t * pIdx = m_idxStart + getIndex( key, m_table->x_capacity );
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::find %6d %X %X size %d cap %d <%p> %d"
            , getpid(), m_pShmMap, m_pool->getShmMap()
            , m_table->x_size
            , m_table->x_capacity
            , pIdx
            , getIndex(key, m_table->x_capacity)
                   );
#endif
    LsShm_offset_t offset = pIdx->x_offset;
    register lsShm_hElem_t * pElem;
    
    while ( offset )
    {
        pElem = (lsShm_hElem_t*)m_pool->offset2ptr( offset );
        if ( (pElem->x_hkey == key) 
                && (pElem->x_rkeyLen == keyLen)
                && (!(*m_vc)( pKey, pElem->p_key(), keyLen )) )
        {
            return pElem;
        }
        
        offset = pElem->x_next ;
        remap();
    }
    return end();
}

LsShmHash::iterator LsShmHash::insert2( const void * pKey, int keyLen,
                                        const void *pValue, int valueLen,
                                        LsShm_hkey_t key )
{
    register int elementSize;
    elementSize = sizeof(lsShm_hElem_t) + round4(keyLen) + round4(valueLen);
    int remapped = 0;
    LsShm_offset_t offset = m_pool->alloc2( elementSize, remapped );
    if (!offset)
        return end();
    
    remap();
    if ( m_table->x_size * m_table->x_full_factor > m_table->x_capacity )
    {
        if (rehash())
        {
            if ( m_table->x_size == m_table->x_capacity )
                return end();
        }
    }
    lsShm_hElem_t * pNew = (lsShm_hElem_t *)m_pool->offset2ptr(offset);
    
    pNew->x_len = elementSize;
    /* pNew->x_next = 0; */
    pNew->x_hkey = key;
    pNew->x_rkeyLen = keyLen;
    pNew->x_valueOff = round4(keyLen); /* padding */
    pNew->x_valueLen = valueLen;
    
    setIterKey(pNew, pKey);
    setIterData(pNew, pValue);
    
    lsShm_hIdx_t * pIdx = m_idxStart + getIndex( key, m_table->x_capacity );
    pNew->x_next = pIdx->x_offset;
    pIdx->x_offset = offset;
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::insert %6d %X %X size %d cap %d <%p> %d"
            , getpid(), m_pShmMap, m_pool->getShmMap()
            , m_table->x_size
            , m_table->x_capacity
            , pIdx
            , getIndex(key, m_table->x_capacity)
                   );
#endif
    m_table->x_size++;
    
    return pNew;
}

LsShmHash::iterator LsShmHash::insert_num( LsShmHash * pThis,
                                           const void * pKey, int keyLen,
                                           const void * pValue, int valueLen )
{
    iterator iter = find_num( pThis, pKey, sizeof(LsShm_hkey_t) );
    if ( iter != pThis->end())
        return pThis->end(); // problem! already there!
    
    return pThis->insert2( &pKey, sizeof(LsShm_hkey_t), 
                           pValue, valueLen,
                           (LsShm_hkey_t)(long)pKey );
}

LsShmHash::iterator LsShmHash::get_num(LsShmHash * pThis
                                            , const void * pKey, int keyLen
                                            , const void * pValue, int valueLen
                                            , int * pFlag
                                      )
{
    LSSHM_CHECKSIZE(valueLen);
    
    iterator iter = find_num( pThis, pKey, sizeof(LsShm_hkey_t) );
    if ( iter != pThis->end() )
    {
        *pFlag = LSSHM_FLAG_NONE;
        return iter;
    }
    
    iter = pThis->insert2( &pKey, sizeof(LsShm_hkey_t),
                           pValue, valueLen,
                           (LsShm_hkey_t)(long)pKey );
    if ( iter )
    {
        if (*pFlag & LSSHM_FLAG_INIT)
        {
            /* initialize the memory */
            ::memset(iter->p_value(), 0, valueLen);
        }
        *pFlag = LSSHM_FLAG_CREATED;
    } else {
        // dont need to set pFlag... 
        ;
    }
    return iter;
}

LsShmHash::iterator LsShmHash::set_num(LsShmHash * pThis,
                                          const void * pKey, int keyLen,
                                          const void * pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    iterator iter = find_num( pThis, pKey, sizeof(LsShm_hkey_t) );
    if ( iter != pThis->end() )
    {
        // if (iter->x_valueLen >= valueLen)
        if (iter->realValLen() >= valueLen)
        {
            iter->x_valueLen = valueLen;
            pThis->setIterData(  iter, pValue );
            return iter;
        }
        else
        {
            pThis->erase_iterator_helper(iter);
        }
    }
    return pThis->insert2( &pKey, sizeof(LsShm_hkey_t), 
                           pValue, valueLen,
                           (LsShm_hkey_t)(long)pKey );
}

LsShmHash::iterator LsShmHash::update_num(LsShmHash * pThis,
                                          const void * pKey, int keyLen,
                                          const void * pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    iterator iter = find_num( pThis, pKey, sizeof(LsShm_hkey_t) );
    if ( iter != pThis->end() )
    {
        // if (iter->x_valueLen >= valueLen)
        if (iter->realValLen() >= valueLen)
        {
            iter->x_valueLen = valueLen;
            pThis->setIterData(  iter, pValue );
            return iter;
        }
        else
        {
            pThis->erase_iterator_helper(iter);
            return pThis->insert2( &pKey, sizeof(LsShm_hkey_t), 
                           pValue, valueLen,
                           (LsShm_hkey_t)(long)pKey );
        }
    }
    return pThis->end();
}


LsShmHash::iterator LsShmHash::insert_p( LsShmHash * pThis, 
                                        const void * pKey, int keyLen,
                                        const void * pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShm_hkey_t key = (*pThis->m_hf)( pKey, keyLen);
    iterator iter = pThis->find2( pKey, keyLen, key );
    if ( iter )
        return pThis->end();
    return pThis->insert2( pKey, keyLen, pValue, valueLen, key );
}

LsShmHash::iterator LsShmHash::set_p(LsShmHash * pThis, 
                                        const void * pKey, int keyLen,
                                        const void * pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShm_hkey_t key = (*pThis->m_hf)( pKey, keyLen);
    iterator iter = pThis->find2( pKey, keyLen, key );
    if ( iter )
    {
        // if (iter->x_valueLen >= valueLen) {
        if (iter->realValLen() >= valueLen) {
            iter->x_valueLen = valueLen;
            pThis->setIterData(iter, pValue );
            return iter;
        }
        else
        {
            /* remove the iter and install new one */
            pThis->erase_iterator_helper(iter);
        }
    }
    return pThis->insert2( pKey, keyLen, pValue, valueLen, key );
}

LsShmHash::iterator LsShmHash::get_p(LsShmHash * pThis
                                        , const void * pKey
                                        , int keyLen
                                        , const void * pValue
                                        , int valueLen
                                        , int * pFlag
                                    )
{
    LSSHM_CHECKSIZE(valueLen);
    
    LsShm_hkey_t key = (*pThis->m_hf)( pKey, keyLen);
    iterator iter = pThis->find2( pKey, keyLen, key );
    
    if ( iter )
    {
        *pFlag = LSSHM_FLAG_NONE;
        return iter;
    }
    
    iter = pThis->insert2( pKey, keyLen, pValue, valueLen, key );
    if ( iter )
    {
        if (*pFlag & LSSHM_FLAG_INIT)
        {
            /* initialize the memory */
            ::memset(iter->p_value(), 0, valueLen);
        }
        *pFlag = LSSHM_FLAG_CREATED;
    } else {
        // dont need to set pFlag... 
        ;
    }
    
    return iter;
}

LsShmHash::iterator LsShmHash::update_p(LsShmHash * pThis, 
                                        const void * pKey, int keyLen,
                                        const void * pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShm_hkey_t key = (*pThis->m_hf)( pKey, keyLen);
    iterator iter = pThis->find2( pKey, keyLen, key );
    if ( iter )
    {
        // if (iter->x_valueLen >= valueLen) {
        if (iter->realValLen() >= valueLen) {
            iter->x_valueLen = valueLen;
            pThis->setIterData(iter, pValue );
            return iter;
        }
        else
        {
            /* remove the iter and install new one */
            pThis->erase_iterator_helper(iter);
            return pThis->insert2( pKey, keyLen, pValue, valueLen, key );
        }
    }
    return pThis->end();
}

LsShmHash::iterator LsShmHash::find_p(LsShmHash * pThis, 
                                      const void * pKey, int keyLen )
{
    return pThis->find2( pKey, keyLen, (*pThis->m_hf)(pKey, keyLen ) );
}

LsShmHash::iterator LsShmHash::next( iterator iter )
{
    if ( !iter )
        return end();
    if ( iter-> x_next )
        return (iterator)m_pool->offset2ptr( iter->x_next );
    
    lsShm_hIdx_t * p ;
    p = m_idxStart + getIndex( iter->x_hkey, m_table->x_capacity ) + 1;
    while ( p < m_idxEnd )
    {
        if ( p->x_offset )
        {
#ifdef DEBUG_RUN
            iterator xiter = (iterator)m_pool->offset2ptr(p->x_offset);
            if (xiter)
            {
                if ( (xiter->x_rkeyLen == 0)
                    || (xiter->x_hkey == 0)
                    || (xiter->x_len == 0) )
                {
                    
HttpLog::notice("LsShmHash::next PROBLEM %6d %X %X SIZE %X SLEEPING"
                    , getpid(), m_pShmMap
                    , m_pool->getShmMap()
                    , m_pool->getShmMap_maxSize());
                   sleep(10); 
                }
            }
#endif
            return (iterator)m_pool->offset2ptr(p->x_offset);
        }
        p++;
    }
    return end();
}

int LsShmHash::for_each( iterator beg, iterator end,
                    for_each_fn fun )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter ) )
            break;
        ++n;
    }
    return n;
}

int LsShmHash::for_each2( iterator beg, iterator end,
                    for_each_fn2 fun, void * pUData )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter, pUData ) )
            break;
        ++n;
    }
    return n;
}

//
//  @brief statIdx - helper function which return num of elements in this link
//
int LsShmHash::statIdx( iterator iter, for_each_fn2 fun, void * pUdata)
{
    LsHash_stat_t * pHashStat = (LsHash_stat_t*)pUdata;
#define NUM_SAMPLE  0x20
typedef struct statKey_s {
        LsShm_hkey_t    key;
        int             numDup;
} statKey_t;
    statKey_t keyTable[NUM_SAMPLE];
    register statKey_t * p_keyTable;
    int numKey = 0;
    int curDup = 0; // keep track the number of dup!
    
    register int numInIdx = 0;
    while ( iter )
    {
        register int i;
        p_keyTable = keyTable;
        for (i = 0; i < numKey; i++, p_keyTable++)
        {
            if (p_keyTable->key == iter->x_hkey)
            {
                p_keyTable->numDup++; 
                curDup++;
                break;
            }
        }
        if ((i == numKey) && (i < NUM_SAMPLE) )
        {
            p_keyTable->key = iter->x_hkey;
            p_keyTable->numDup = 0;
            numKey++;
        }
            
        numInIdx++;
        fun(iter, pUdata);
        iter = (iterator)m_pool->offset2ptr( iter->x_next );
    }
    
    pHashStat->numDup += curDup;
    return numInIdx;
}

//
//  @brief stat - populate the statistic of the hash table
//  @brief return num of elements searched.
//  @brief populate the pHashStat.
//
int LsShmHash::stat( LsHash_stat_t* pHashStat, for_each_fn2 fun )
{
    if (!pHashStat)
    {
        errno = EINVAL;
        return -1;
    }
    ::memset(pHashStat, 0, sizeof(LsHash_stat_t));
    
    // search each idx
    register lsShm_hIdx_t * p ;
    p = m_idxStart;
    while ( p < m_idxEnd )
    {
        pHashStat->numIdx++;
        if ( p->x_offset )
        {
            pHashStat->numIdxOccupied++;
            register int num;
            if ( (num = statIdx( (iterator)m_pool->offset2ptr(p->x_offset)
                                    , fun
                                    , (void *)pHashStat)
                 ) ) 
            {
                pHashStat->num += num;
                if (num > pHashStat->maxLink)
                    pHashStat->maxLink = num;
                
                // top 10 listing
                int topidx;
                if (num <= 5)
                {
                    topidx = num - 1;
                } else if (num <= 10) {
                    topidx = 5;
                } else if (num <= 20) {
                    topidx = 6;
                } else if (num <= 50) {
                    topidx = 7;
                } else if (num <= 100) {
                    topidx = 8;
                } else {
                    topidx = 9;
                }
                pHashStat->top[topidx]++;
            }
        }
        ++p;
    }
    return pHashStat->num;
}

LsShmHash::iterator LsShmHash::begin()
{
    if ( ! m_table->x_size )
        return end();
    
    register lsShm_hIdx_t * p ;
    
    p = m_idxStart;
    while( p < m_idxEnd )
    {
        if ( p->x_offset )
            return (iterator)m_pool->offset2ptr(p->x_offset);
        ++p;
    }
    return NULL;
}

