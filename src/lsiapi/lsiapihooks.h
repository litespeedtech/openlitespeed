#ifndef LSIAPIHOOKS_H
#define LSIAPIHOOKS_H

//#include "lsiapi/modulemanager.h"
#include "lsiapi/internal.h"
#include "lsiapi/lsiapi.h"
#include "lsiapi/lsimoduledata.h"
#include <stdlib.h>


class LsiSession;


template< int base, int size >
class SessionHooks;

typedef struct _LsiApiHook
{
    const lsi_module_t *_module;
    lsi_callback_pf     _cb;
    short               _priority;
    short               _flag;
} LsiApiHook;

#define LSI_HKPT_L4_COUNT       (LSI_HKPT_HTTP_BEGIN - LSI_HKPT_L4_BEGINSESSION)
#define LSI_HKPT_HTTP_COUNT     (LSI_HKPT_HTTP_END - LSI_HKPT_HTTP_BEGIN + 1)
#define LSI_HKPT_SERVER_COUNT   (LSI_HKPT_TOTAL_COUNT - LSI_HKPT_L4_COUNT - LSI_HKPT_HTTP_COUNT)

typedef SessionHooks<0, LSI_HKPT_L4_COUNT> IolinkSessionHooks;
typedef SessionHooks<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT> HttpSessionHooks;
typedef SessionHooks<LSI_HKPT_MAIN_INITED, LSI_HKPT_SERVER_COUNT> ServerSessionHooks;

class LsiApiHooks;

typedef struct lsi_hook_info_s
{
    const LsiApiHooks * _hooks;
    void *              _termination_fp;
} lsi_hook_info_t;


class LsiApiHooks
{

    
public:
    explicit LsiApiHooks( int capacity = 4 )
        : m_pHooks( NULL )
        , m_iCapacity( 0 )
        , m_iBegin( 0 )
        , m_iEnd( 0 )
        , m_iFlag( 0 )
    {
        reallocate( capacity );
    }

    ~LsiApiHooks()
    {
        if( m_pHooks )
            free( m_pHooks );
    }

    LsiApiHooks( const LsiApiHooks &other );
    
    short size() const              {   return m_iEnd - m_iBegin;   }
    short capacity() const          {   return m_iCapacity;         }
    
    short getFlag() const           {   return m_iFlag;             }
    void  setFlag( short f )        {   m_iFlag |= f;               }

    short add( const lsi_module_t *pModule, lsi_callback_pf cb, short priority, short flag = 0 );

    LsiApiHook * find( const lsi_module_t *pModule, lsi_callback_pf cb );
    int remove( LsiApiHook *pHook );
    LsiApiHook * find( const lsi_module_t *pModule ) const;
    int remove( const lsi_module_t *pModule );
    
    LsiApiHook* get( int index ) const
    {
        return m_pHooks + index;
    }
    LsiApiHook* begin() const  {   return m_pHooks + m_iBegin; }
    LsiApiHook* end() const    {   return m_pHooks + m_iEnd;   }
    LsiApiHooks* dup() const   
    {   return new LsiApiHooks( *this );    }
    
    int copy( const LsiApiHooks& other );
        
    int runCallback( int level, lsi_cb_param_t *param) const;
    int runCallback( int level,  LsiSession *session, void *param1, int paramLen1, int *flag_out, int flag_in) const
    {
        lsi_hook_info_t info = { this, NULL } ;
        lsi_cb_param_t param =
        {   session, &info, begin(), param1, paramLen1, flag_out, flag_in  };
        return runCallback(level, &param);
    }

    int runCallbackViewData( int level, LsiSession *session, void *inBuf, int inLen) const
    {   return runCallback(level, session, inBuf, inLen, NULL, 0);  }
    
    int runCallbackNoParam( int level, LsiSession *session) const
    {   return runCallback(level, session, NULL, 0, NULL, 0);       }
    

    static IolinkSessionHooks  *m_pIolinkHooks;
    static HttpSessionHooks    *m_pHttpHooks;
    static ServerSessionHooks  *m_pServerHooks;
    static IolinkSessionHooks  *getIolinkHooks()   {   return m_pIolinkHooks;   }
    static HttpSessionHooks    *getHttpHooks()     {   return m_pHttpHooks;     }
    static ServerSessionHooks  *getServerHooks()   {   return m_pServerHooks;     }
    
    static const LsiApiHooks * getGlobalApiHooks( int index );
    static LsiApiHooks * getReleaseDataHooks( int index );
    static inline const char * getHkptName( int index ) 
    {   return s_pHkptName[ index ];    }
    static void initGlobalHooks();
    
    
private:
    
    LsiApiHooks &operator=( const LsiApiHooks &other );
    bool operator==( const LsiApiHooks &other ) const;

    int reallocate( int capacity, int newBegin = -1 );

private:
    LsiApiHook * m_pHooks;
    short        m_iCapacity;
    short        m_iBegin;
    short        m_iEnd;
    short        m_iFlag;

public:
    static const char * s_pHkptName[LSI_HKPT_TOTAL_COUNT];

};

#define SESSION_HOOK_FLAG_RELEASE   1
#define SESSION_HOOK_FLAG_TRIGGERED 2

template< int base, int size >
class SessionHooks
{
    LsiApiHooks * m_pHookPoints[size];
    char          m_flagRelease[size];
    char          m_iOwnCopy;
public:
    explicit SessionHooks(int isGlobalHooks)
    {
        if (isGlobalHooks)
        {
            for (int i=0; i<size; ++i)
            {
                m_pHookPoints[i] = new LsiApiHooks;
                m_flagRelease[i] = SESSION_HOOK_FLAG_RELEASE;
                m_iOwnCopy = SESSION_HOOK_FLAG_RELEASE;
            }
        }
        else
        {
            memset( m_pHookPoints, 0, (&m_iOwnCopy + 1 ) - (char *)&m_pHookPoints ); 
        }
    }
    ~SessionHooks()
    {
        if ( (m_iOwnCopy & SESSION_HOOK_FLAG_RELEASE) )
            release();
    }
    
    void release()
    {
        for (int i = 0; i < size; ++i)
        {
            if ( m_pHookPoints[i] && ( m_flagRelease[i]& SESSION_HOOK_FLAG_RELEASE ) )
                delete m_pHookPoints[i];
        }
    }
        
    char hasOwnCopy() const 
    {   return m_iOwnCopy & SESSION_HOOK_FLAG_RELEASE;    }
    
    void reset()
    {
        if ( (m_iOwnCopy & SESSION_HOOK_FLAG_RELEASE) )
            release();
        memset( m_pHookPoints, 0, (&m_iOwnCopy + 1 ) - (char *)&m_pHookPoints ); 
        
    }
    
    int getBase()   { return base;  }
    int getSize()   { return size;  }
    
    const LsiApiHooks * get( int hookLevel ) const
    {   return m_pHookPoints[ hookLevel - base ];  }

    LsiApiHooks * get( int hookLevel )
    {   return m_pHookPoints[ hookLevel - base ];  }
    
    void set( int hookLevel, LsiApiHooks * hooks )
    {   m_pHookPoints[hookLevel - base] = hooks;   }
    
    void triggered()
    {   m_iOwnCopy |= SESSION_HOOK_FLAG_TRIGGERED;    }

    void clearTriggered()
    {   m_iOwnCopy &= ~SESSION_HOOK_FLAG_TRIGGERED;   }
    
    char isTriggered() const
    {   return m_iOwnCopy & SESSION_HOOK_FLAG_TRIGGERED;    }
    
    int isDisabled( int hookLevel ) const
    {   
        return !isEnabled( hookLevel );
    }
    
    int isEnabled( int hookLevel ) const
    {   
        const LsiApiHooks * p = get( hookLevel );
        return (p && (p->size() > 0 ));
    }
    
    void inherit( const SessionHooks<base, size> * parent )
    {
        if (parent)
            memcpy(m_pHookPoints, parent->m_pHookPoints, sizeof(m_pHookPoints));
    }

    void inherit( int hookLevel, const LsiApiHooks * pHooks )
    {
        hookLevel -= base;
        m_pHookPoints[hookLevel] = (LsiApiHooks *)pHooks;
    }

    LsiApiHooks * getCopy( int hookLevel )
    {
        hookLevel -= base;
        if (m_flagRelease[hookLevel] & SESSION_HOOK_FLAG_RELEASE )
            return m_pHookPoints[hookLevel];
            
        LsiApiHooks ** pHooks; 
        if ( hookLevel >= 0 && hookLevel < size ) //should be 0, 1, 2, 3
        {
            pHooks = &m_pHookPoints[hookLevel];
        }
        else
            return NULL;
        if ( !*pHooks )
            *pHooks = new LsiApiHooks();
        else
            *pHooks = (*pHooks)->dup();
        m_flagRelease[hookLevel] |= SESSION_HOOK_FLAG_RELEASE;
        m_iOwnCopy |= SESSION_HOOK_FLAG_RELEASE;
        return *pHooks;
    }
    
    short getFlag( int hookLevel )
    {   hookLevel -= base;
        return m_pHookPoints[hookLevel]? m_pHookPoints[hookLevel]->getFlag(): 0;  
    }
    
    void restartSessionHooks( int * levels, int nLevels, const SessionHooks<base, size> * parent )
    {
        for (int i = 0; i < nLevels; ++i)
        {
            int l = levels[i] - base;
            if (( m_flagRelease[ l ]& SESSION_HOOK_FLAG_RELEASE ) )
            {
                delete m_pHookPoints[ l ];
                m_flagRelease[ l ] &= ~SESSION_HOOK_FLAG_RELEASE;
            }
            m_pHookPoints[ l ] = (LsiApiHooks *)parent->get( levels[i] );
        }
        
    }
    
    int runCallback( int level, lsi_cb_param_t *param) const
    {   return get( level )->runCallback( level, param );   }
    
    int runCallback( int level, LsiSession *session, void *param1, int paramLen1, int *param2, int paramLen2) const
    {   return get( level )->runCallback( level, session, param1, paramLen1, param2, paramLen2 );       }
    
    int runCallbackViewData( int level, LsiSession *session, void *inBuf, int inLen) const
    {   return get( level )->runCallbackViewData( level, session, inBuf, inLen );   }
    
    int runCallbackNoParam( int level, LsiSession *session) const
    {   return get( level )->runCallbackNoParam( level, session );      }
    

};






#endif // LSIAPIHOOKS_H
