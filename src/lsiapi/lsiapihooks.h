#ifndef LSIAPIHOOKS_H
#define LSIAPIHOOKS_H

//#include "lsiapi/modulemanager.h"
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
#define LSI_HKPT_HTTP_COUNT     (LSI_HKPT_TOTAL_COUNT - LSI_HKPT_L4_COUNT)

//#define  HttpSessionHooks      SessionHooks<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT>
//#define  IolinkSessionHooks    SessionHooks<0, LSI_HKPT_L4_COUNT>

typedef SessionHooks<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT> HttpSessionHooks;
typedef SessionHooks<0, LSI_HKPT_L4_COUNT> IolinkSessionHooks;


class LsiApiHooks
{

    
public:
    explicit LsiApiHooks( int capacity = 4 )
        : m_pHooks( NULL )
        , m_iCapacity( 0 )
        , m_iBegin( 0 )
        , m_iEnd( 0 )
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
        
    int runFunctions(struct lsi_cb_param_t *param) const;
    int runFunctions( const LsiSession *session, void *param1, int paramLen1, void *param2, int paramLen2) const;
    int runFunctionsViewData( const LsiSession *session, void *inBuf, int inLen) const;
    int runNoParamFunctions( const LsiSession *session) const;
    

    static IolinkSessionHooks  m_apiIolinkHooks;
    static HttpSessionHooks    m_apiHttpHooks;
    static IolinkSessionHooks  *getIolinkHooks()   {   return &m_apiIolinkHooks;   }
    static HttpSessionHooks    *getHttpHooks()     {   return &m_apiHttpHooks; }
    static const LsiApiHooks * getGlobalApiHooks( int index );
    static LsiApiHooks * getReleaseDataHooks( int index );
    
    
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


};

#define SESSION_HOOK_FLAG_RELEASE   1
#define SESSION_HOOK_FLAG_TRIGGERED 2

template< int base, int size >
class SessionHooks
{
    LsiApiHooks * m_pHookPoints[size];
    char          m_flag[size];
    char          m_iOwnCopy;
public:
    explicit SessionHooks(int isGlobalHooks)
    {
        if (isGlobalHooks)
        {
            for (int i=0; i<size; ++i)
            {
                m_pHookPoints[i] = new LsiApiHooks;
                m_flag[i] = SESSION_HOOK_FLAG_RELEASE;
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
            if ( m_pHookPoints[i] && ( m_flag[i]& SESSION_HOOK_FLAG_RELEASE ) )
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
        const LsiApiHooks * p = get( hookLevel );
        return (!p || (p->size() == 0 ));
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
        if (m_flag[hookLevel] & SESSION_HOOK_FLAG_RELEASE )
            return m_pHookPoints[hookLevel];
            
        LsiApiHooks ** pHooks; 
        if ( hookLevel >= 0 && hookLevel < size ) //should be 0, 1, 2, 3
        {
            pHooks = &m_pHookPoints[hookLevel];
        }
        else
            return NULL;
        
        *pHooks = (*pHooks)->dup();
        m_flag[hookLevel] |= SESSION_HOOK_FLAG_RELEASE;
        m_iOwnCopy |= SESSION_HOOK_FLAG_RELEASE;
        return *pHooks;
    }

};


typedef struct lsi_hook_info_t
{
    const LsiApiHooks * _hooks;
    void *              _termination_fp;
} lsi_hook_info_t;




#endif // LSIAPIHOOKS_H
