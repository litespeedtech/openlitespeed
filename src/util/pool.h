/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: pool.h,v 1.1.1.1 2013/12/22 23:42:51 gwang Exp $
                         -------------------
    begin                : Wed Apr 30 2003
    author               : Gang Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#ifndef POOL_H
#define POOL_H


/**
  *@author Gang Wang
  */

#ifdef _REENTRENT
#include <thread/tmutex.h>
#endif

#include <lsr/lsr_pool.h>

class Pool
{
private:

public:
 
     Pool() {};
    ~Pool() {};
    
    // num must be > 0
    static void * allocate( size_t num )
    {   return lsr_palloc( num );   }
    // p may not be 0
    static void   deallocate( void *p, size_t num )
    {   return lsr_pfree( p );   }
    static void * reallocate( void *p, size_t old_sz, size_t new_sz )
    {   return lsr_prealloc( p, new_sz );   }
    
    static void * allocate2( size_t num )
    {   return lsr_palloc( num );   }
    static void   deallocate2( void *p )
    {   return lsr_pfree( p );   }
    static void * reallocate2( void *p, size_t new_sz )
    {   return lsr_prealloc( p, new_sz );   }
    
    static char * dupstr( const char *p )
    {   return lsr_pdupstr( p );   }
    static char * dupstr( const char *p, int len )
    {   return lsr_pdupstr2( p, len );   }
};

#endif
