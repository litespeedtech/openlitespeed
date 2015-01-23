/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#ifndef LSSHMHASH_H
#define LSSHMHASH_H

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;
#endif

#include <string.h>
#include <shm/lsshmpool.h>
#include <socket/gsockaddr.h>
class   LsShmPool;
typedef uint32_t LsShm_hkey_t;

/*
 *  HASH element 
 *  
 *  +----------------
 *  | Element info
 *  | hkey
 *  | real key len
 *  | data len
 *  +---------------- 
 *  | real key
 *  +---------------- 
 *  | data ----> where data started
 *  +--------------------
 * 
 *  It is reasonable to support max 32k key string.
 *  Otherwise we should change to uint32_t
 */
typedef int32_t      lsShmHashElemLen_t ;
typedef int32_t      lsShmHashElemOffset_t ;
typedef int32_t      lsShmHashRkeyLen_t ;
typedef int32_t      lsShmHashValueLen_t ;

typedef struct lsShm_hElem_s {
    lsShmHashElemLen_t      x_len;          /* element size     */
    lsShmHashElemOffset_t   x_valueOff;     /* offset to value  */
    LsShm_offset_t          x_next;         /* offset to next in element list */
    LsShm_hkey_t            x_hkey;         /* the key itself */
    lsShmHashRkeyLen_t      x_rkeyLen;
    lsShmHashValueLen_t     x_valueLen;
    uint32_t                x_data[0];       /* Enforce 32bit alignment */

    inline uint8_t *        p_key() const   { return (uint8_t*)x_data; }
    inline uint8_t *        p_value() const { return ((uint8_t*)x_data) + x_valueOff; }

    inline const uint8_t *  first() const   { return p_key(); }
    inline uint8_t *        second() const  { return p_value(); }
    
                                            /* real value len */
    inline int32_t          realValLen() const { return x_len - sizeof(struct lsShm_hElem_s) - x_valueOff; }
} lsShm_hElem_t;

typedef struct {
    LsShm_offset_t  x_offset;   // offset location of hash element
} lsShm_hIdx_t;

typedef struct {
    uint32_t        x_magic;
    LsShm_size_t    x_capacity; 
    LsShm_size_t    x_size; 
    LsShm_size_t    x_full_factor;
    LsShm_size_t    x_grow_factor;
    LsShm_offset_t  x_iTable;
    LsShm_offset_t  x_lockOffset;
    LsShm_offset_t  x_glockOffset;
    uint8_t         x_mode;
    uint8_t         x_unused[3];
} lsShm_hTable_t;

class LsShmHash : public ls_shmhash_s, ls_shmobject_s
{
    uint32_t        m_magic;
public:
    typedef lsShm_hElem_t* iterator;
    typedef const lsShm_hElem_t* const_iterator;

    typedef LsShm_hkey_t (*hash_fn) ( const void *, int len  );
    typedef int   (*val_comp) ( const void * pVal1, const void * pVal2, int len );
    typedef int (*for_each_fn)( iterator iter);
    typedef int (*for_each_fn2)( iterator iter, void *pUData);

    static LsShm_hkey_t hash_string(const void * __s, int len );
    static int  comp_string( const void * pVal1, const void * pVal2, int len );

    static LsShm_hkey_t hash_32id(const void * __s, int len );
    static LsShm_hkey_t hash_buf(const void * __s, int len );
    static int  comp_buf( const void * pVal1, const void * pVal2, int len );

    static LsShm_hkey_t i_hash_string(const void * __s, int len);
    static int  i_comp_string( const void * pVal1, const void * pVal2, int len );

    static int  cmp_ipv6( const void * pVal1, const void * pVal2, int len );
    static LsShm_hkey_t hf_ipv6( const void * pKey, int len );
    
    inline LsShm_status_t status()
    { return m_status; }
    
private:
    inline uint32_t getIndex(uint32_t k, uint32_t n)
    { return k % n ; }

    typedef iterator (*hash_insert)(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    typedef iterator (*hash_update)(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    typedef iterator (*hash_set)(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    typedef iterator (*hash_get)(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen, int * pFlag);
    typedef iterator (*hash_find)  (LsShmHash * pThis, const void * pKey , int keyLen);

private: // private data should start from here
    lsShm_hTable_t *    m_table;
    lsShm_hIdx_t *      m_idxStart;
    lsShm_hIdx_t *      m_idxEnd;

    LsShmPool *         m_pool;
    LsShm_offset_t      m_offset;
    char *              m_name;
    
    hash_fn             m_hf;
    val_comp            m_vc;
    hash_insert         m_insert;
    hash_update         m_update;
    hash_set            m_set;
    hash_find           m_find;
    hash_get            m_get;
    
    int8_t              m_poolOwner;
    int8_t              m_lockEnable;
    int8_t              m_mode;         /* keep track on _num mode 0 = _num, 1 = normal*/
    int8_t              m_notused;      /* future use */
    lsShmMap_t *        m_pShmMap;      /* keep track on map */
    lsi_shmlock_t *     m_pShmLock;     /* local lock for Hash */
    lsi_shmlock_t *     m_pShmGLock;    /* global lock */
    LsShm_status_t      m_status;
    LsShm_size_t        m_capacity;
    
    int       rehash();
    iterator    find2( const void * pKey, int keyLen, LsShm_hkey_t key );
    iterator    insert2( const void * pKey, int keyLen, const void *pValue, int valueLen, LsShm_hkey_t key );
    static iterator insert_num(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen );
    static iterator set_num(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    static iterator update_num(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    static iterator find_num(LsShmHash * pThis, const void * pKey, int keyLen );
    static iterator get_num(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen, int * pFlag);
    
    static iterator insert_p(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen );
    static iterator set_p(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    static iterator update_p(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen);
    static iterator find_p(LsShmHash * pThis, const void * pKey, int keyLen );
    static iterator get_p(LsShmHash * pThis, const void * pKey, int keyLen, const void * pValue, int valueLen, int * pFlag);

    inline void setIterData(iterator iter, const void * pValue) 
    { if (pValue)
        ::memcpy( iter->p_value(), pValue, iter->x_valueLen); }
    inline void setIterKey(iterator iter, const void * pKey)
    { ::memcpy( iter->p_key(), pKey, iter->x_rkeyLen); }
    
    void remap() ;
    static int release_hash_elem( iterator iter, void * pUData);
    
private: // will become private soon
    LsShmHash( LsShmPool * pool
                , const char * name
                , size_t init_size
                , hash_fn hf
                , val_comp vc );
    ~LsShmHash();
    
public:
    inline LsShm_offset_t ptr2offset( const void * ptr) const
    { return m_pool->ptr2offset(ptr); }
    inline void * offset2ptr( LsShm_offset_t  offset) const
    { return m_pool->offset2ptr(offset); }
    
    inline iterator offset2iterator( LsShm_offset_t  offset) const
    { return (iterator)m_pool->offset2ptr(offset); }
    
    inline void * offset2iteratorData( LsShm_offset_t  offset) const
    { return ((iterator)m_pool->offset2ptr(offset))->p_value(); }
    
    inline LsShm_offset_t alloc2(LsShm_size_t size, int & remapped)
    { return m_pool->alloc2(size, remapped); }
    inline void release2(LsShm_offset_t offset, LsShm_size_t size)
    { m_pool->release2(offset, size); }
    
    inline int round4(int x) const
    { return (x + 0x3) & ~0x3; }
    inline void * getIterDataPtr(iterator iter) const
    { return iter->p_value(); }
    
    inline const char * name() const
    { return m_name; }
    
    inline void checkRemap()
    { remap(); }
    
#if 0
private: // will become private soon
    LsShmHash( LsShmPool * pool
                , const char * name
                , size_t init_size
                , hash_fn hf
                , val_comp vc );
    ~LsShmHash();
    
public:
#endif
    
    static LsShmHash * get( size_t init_size
                , hash_fn hf
                , val_comp vc );
    static LsShmHash * get( const char * name
                , size_t init_size
                , hash_fn hf
                , val_comp vc );
    static LsShmHash * get( LsShmPool * pool
                , const char * name
                , size_t init_size
                , hash_fn hf
                , val_comp vc );
    void unget();
    
    void clear();
    
    inline void remove( const void * pKey, int keyLen)
    {
        iterator iter;
        if ( (iter = find_iterator (pKey, keyLen)) )
        {
            erase_iterator(iter);
        }
    }
    
    inline LsShm_offset_t find( const void * pKey, int keyLen, int * retsize)
    { 
        iterator iter;
        if ( (iter = find_iterator(pKey, keyLen)) )
        {
            *retsize = iter->x_valueLen;
            return ptr2offset( iter->p_value() );
        }
        else
        {
            *retsize = 0;
            return 0;
        }
        // return ptr2offset( find_iterator(pKey, keyLen).x_data ) ;
    }
    
    inline LsShm_offset_t get( const void * pKey, int keyLen, int * valueLen, int * pFlag )
    {
        int myValueLen = *valueLen;
        iterator iter;
        
        if ( (iter = get_iterator(pKey, keyLen, NULL, myValueLen, pFlag)) )
        {
            *valueLen = iter->x_valueLen;
            return ptr2offset( iter->p_value() );
        }
        else
        {
            *valueLen = 0;
            return 0;
        }
    }
    
    inline LsShm_offset_t set(const void * pKey, int keyLen, const void * pValue, int valueLen)
    { 
        iterator iter;
        iter = set_iterator(pKey, keyLen, pValue, valueLen);
        return iter ? ptr2offset( iter->p_value() ) : 0;
    }
    inline LsShm_offset_t insert(const void * pKey, int keyLen, const void * pValue, int valueLen)
    {
        iterator iter;
        iter = insert_iterator(pKey, keyLen, pValue, valueLen);
        return iter ? ptr2offset( iter->p_value() ) : 0;
    }
    inline LsShm_offset_t update(const void * pKey, int keyLen, const void * pValue, int valueLen)
    { 
        iterator iter;
        iter = update_iterator(pKey, keyLen, pValue, valueLen);
        return iter ? ptr2offset( iter->p_value() ) : 0;
    }

    void erase_iterator( iterator iter )
    {
        autoLock(); remap();
        erase_iterator_helper( iter );
        autoUnlock();
    }
    
    //
    //  Noted - the iterator should not be saved use ptr2offset(iterator)
    //          to save the offset
    iterator find_iterator( const void * pKey, int keyLen)
    {
        autoLock(); remap(); 
        iterator iter = (*m_find)( this, pKey, keyLen ); 
        autoUnlock(); return iter;
    }

    //
    //  Noted - the iterator should not be saved use ptr2offset(iterator)
    //          to save the offset
    iterator insert_iterator(const void * pKey, int keyLen, const void * pValue, int valueLen)
    {
        autoLock(); remap(); 
        iterator iter = (*m_insert)( this, pKey, keyLen, pValue, valueLen); 
        autoUnlock(); return iter ;
    }

    //
    //  Noted - the iterator should not be saved use ptr2offset(iterator)
    //          to save the offset
    iterator get_iterator(const void * pKey, int keyLen, const void * pValue, int valueLen, int * pFlag)
    {
        autoLock(); remap(); 
        iterator iter = (*m_get)( this, pKey, keyLen, pValue, valueLen, pFlag ); 
        autoUnlock(); return iter ;
    }
    
    //
    //  Noted - the iterator should not be saved use ptr2offset(iterator)
    //          to save the offset
    iterator set_iterator(const void * pKey, int keyLen, const void * pValue, int valueLen)
    {
        autoLock(); remap(); 
        iterator iter = (*m_set)( this, pKey, keyLen, pValue, valueLen ); 
        autoUnlock(); return iter ;
    }

    //
    //  Noted - the iterator should not be saved use ptr2offset(iterator)
    //          to save the offset
    iterator update_iterator(const void * pKey, int keyLen, const void * pValue, int valueLen)
    {
        autoLock(); remap(); 
        iterator iter = (*m_update)( this, pKey, keyLen, pValue, valueLen ); 
        autoUnlock(); return iter ;
    }

    hash_fn get_hash_fn() const     {   return m_hf;    }
    val_comp get_val_comp() const   {   return m_vc;    }

    void set_full_factor( int f )   {   if ( f > 0 )    m_table->x_full_factor = f;  }
    void set_grow_factor( int f )   {   if ( f > 0 )    m_table->x_grow_factor = f;  }

    bool empty() const              {   return m_table->x_size == 0; }
    size_t size() const             {   return m_table->x_size;      }
    size_t capacity() const         {   return m_table->x_capacity;  }
    iterator begin();
    iterator end()                  {   return NULL;    }
    const_iterator begin() const    {   return ((LsShmHash*)this)->begin(); }
    const_iterator end() const      {   return ((LsShmHash*)this)->end();   }

    iterator next( iterator iter );
    const_iterator next( const_iterator iter ) const
    {   return ((LsShmHash *)this)->next((iterator)iter); }
    int for_each( iterator beg, iterator end, for_each_fn fun );
    int for_each2( iterator beg, iterator end, for_each_fn2 fun, void * pUData );
    
    void destroy();
    
    //  @brief stat 
    //  @brief gether statistic on HashTable
    int stat(LsHash_stat_t * pHashStat, for_each_fn2 fun);
    
     // Special section for registries entries 
    inline lsShmReg_t *  getReg(int num)
    { return m_pool->getReg(num); }
    inline lsShmReg_t *  findReg(const char * name)
    { return m_pool->findReg(name); }
    inline lsShmReg_t *  addReg(const char * name)
    { return m_pool->addReg(name); }
    
    //
    //  @brief erase_iterator_helper 
    //  @brief  should only be called after SHM-HASH-LOCK have been aquired.
    //
    void erase_iterator_helper( iterator iter );
    
public:
#if 0
    inline int g_lock () { return lsi_shmlock_lock (m_pShmGLock); }

    inline int g_trylock () { return lsi_shmlock_trylock (m_pShmGLock); }
    
    inline int g_unlock () { return lsi_shmlock_unlock (m_pShmGLock); }
#endif
    void enableManualLock()
    { m_pool->disableLock(); disableLock(); }
    void disableManualLock()
    { m_pool->enableLock(); enableLock(); }
    
    inline int lock ()
    { return m_lockEnable ? 0 : lsi_shmlock_lock(m_pShmLock); }

    inline int trylock ()
    { return m_lockEnable ? 0 : lsi_shmlock_trylock(m_pShmLock); }

    inline int unlock ()
    {   return m_lockEnable ? 0 : lsi_shmlock_unlock(m_pShmLock); }

private:
    inline void enableLock()
    { m_lockEnable = 1; };
    
    inline void disableLock()
    { m_lockEnable = 0; };
    
    inline int autoLock ()
    { return m_lockEnable && lsi_shmlock_lock(m_pShmLock); }

    inline int autoUnlock ()
    {   return m_lockEnable && lsi_shmlock_unlock(m_pShmLock); }

    // stat helper
    int statIdx(iterator iter, for_each_fn2 fun, void * pUData);
    
    int setupLock()
    { return m_lockEnable
            && lsi_shmlock_setup(m_pShmLock); }


    int setupGLock()
    {   return lsi_shmlock_setup( m_pShmGLock );    }

    // house keeping
    int m_ref;
    HashStringMap< lsi_shmobject_t *> & m_objBase;
    inline int up_ref() { return ++m_ref; }
    inline int down_ref() { return --m_ref; }
    
    // disable the bad boys!
    LsShmHash( const LsShmHash & );
    LsShmHash & operator=( const LsShmHash & );

    
#ifdef LSSHM_DEBUG_ENABLE
    // for debug purpose - should debug this later
    friend class debugBase;
#endif
};


//
//  TShmHash - LiteSpeed Shm Hash object
//  SIMON - haven't verified this yet; will do this later.
//
#if 0
template< class T >
class TShmHash
    : public LsShmHash
{
public:
    class iterator
    {
        LsShmHash::iterator m_iter;
    public:
        iterator()
        {}

        iterator( LsShmHash::iterator iter ) : m_iter( iter )
        {}
        iterator( LsShmHash::const_iterator iter )
            : m_iter( (LsShmHash::iterator)iter )
        {}

        iterator( const iterator& rhs ) : m_iter( rhs.m_iter )
        {}

        const void * first() const
        {  return  m_iter->first();   }

        T second() const
        {   return (T)( m_iter->second() );   }

        operator LsShmHash::iterator ()
        {   return m_iter;  }

    };
    typedef iterator const_iterator;

    TShmHash( int initsize, LsShmHash::hash_fn hf, LsShmHash::val_comp cf )
        : LsShmHash( initsize, hf, cf )
        {};
    ~TShmHash() {};

    iterator insert( const void * pKey, const T& val )
    {   return LsShmHash::insert( pKey, (void *)val );  }

    iterator update( const void * pKey, const T& val )
    {   return LsShmHash::update( pKey, (void *)val );  }

    iterator find( const void * pKey )
    {   return LsShmHash::find( pKey );   }

    const_iterator find( const void * pKey ) const
    {   return LsShmHash::find( pKey );   }
    
    iterator begin()
    {   return LsShmHash::begin();        }
    
    static int deleteObj( LsShmHash::iterator iter )
    {
        delete (T)(iter->second());
        return 0;
    }

    void release_objects()
    {
        LsShmHash::for_each( begin(), end(), deleteObj );
        LsShmHash::clear();
    }

};
#endif

#endif // LSSHMHASH_H

