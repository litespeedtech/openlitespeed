#ifndef LSIAPIHOOKS_H
#define LSIAPIHOOKS_H

//#include "lsiapi/modulemanager.h"
#include "lsiapi/internal.h"
#include "lsiapi/lsiapi.h"
#include "lsiapi/lsimoduledata.h"
#include <stdlib.h>


class LsiSession;


template< int base, int size >
class HookChainList;

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

typedef HookChainList<0, LSI_HKPT_L4_COUNT> IolinkHookChainList;
typedef HookChainList<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT> HttpHookChainList;
typedef HookChainList<LSI_HKPT_MAIN_INITED, LSI_HKPT_SERVER_COUNT> ServerHookChainList;
typedef SessionHooks<0, LSI_HKPT_L4_COUNT>   IolinkSessionHooks;
typedef SessionHooks<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT>   HttpSessionHooks;
typedef SessionHooks<LSI_HKPT_MAIN_INITED, LSI_HKPT_SERVER_COUNT>   ServerSessionHooks;

class LsiApiHooks;

typedef struct lsi_hook_info_s
{
    const LsiApiHooks *     _hooks;
    int8_t *                _enable_array;
    POINTER_termination_fp  _termination_fp;
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
    
    short getGlobalFlag() const           {   return m_iFlag;             }
    void  setGlobalFlag( short f )        {   m_iFlag |= f;               }

    short add( const lsi_module_t *pModule, lsi_callback_pf cb, short priority, short flag = 0 );

    LsiApiHook * find( const lsi_module_t *pModule, lsi_callback_pf cb );
    int remove( LsiApiHook *pHook );
    LsiApiHook * find( const lsi_module_t *pModule ) const;
    LsiApiHook * find( const lsi_module_t *pModule, int *index ) const;
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
    int runCallback( int level, int8_t *pEnableArray, LsiSession *session, void *param1, int paramLen1, int *flag_out, int flag_in) const
    {
        lsi_hook_info_t info = { this, pEnableArray, NULL } ;
        lsi_cb_param_t param =
        {   session, &info, begin(), param1, paramLen1, flag_out, flag_in  };
        return runCallback(level, &param);
    }

    int runCallbackViewData( int level, int8_t *pEnableArray,LsiSession *session, void *inBuf, int inLen) const
    {   return runCallback(level, pEnableArray, session, inBuf, inLen, NULL, 0);  }
    
    int runCallbackNoParam( int level, int8_t *pEnableArray,LsiSession *session) const
    {   return runCallback(level, pEnableArray, session, NULL, 0, NULL, 0);       }
    
    
    static int runForwardCb( lsi_cb_param_t * param );
    static int runBackwardCb( lsi_cb_param_t * param );
    

    static IolinkHookChainList  *m_pIolinkHooks;
    static HttpHookChainList    *m_pHttpHooks;
    static ServerHookChainList  *m_pServerHooks;
    static IolinkHookChainList  *getIolinkHooks()   {   return m_pIolinkHooks;   }
    static HttpHookChainList    *getHttpHooks()     {   return m_pHttpHooks;     }
    static ServerHookChainList  *getServerHooks()   {   return m_pServerHooks;     }
    
    static ServerSessionHooks *m_pServerSessionHooks;
    static ServerSessionHooks *getServerSessionHooks() {   return m_pServerSessionHooks;    }
    
    
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

template< int base, int size >
class HookChainList
{
    LsiApiHooks * m_pHookPoints[size];

public:
    explicit HookChainList()
    {
        for (int i=0; i<size; ++i)
        {
            m_pHookPoints[i] = new LsiApiHooks;
        }
    }
    ~HookChainList()
    {
        release();
    }
    
    void release()
    {
        for (int i = 0; i < size; ++i)
        {
            if ( m_pHookPoints[i]  )
                delete m_pHookPoints[i];
        }
    }
    
    int getBase()   { return base;  }
    int getSize()   { return size;  }
    
    const LsiApiHooks * get( int hookLevel ) const
    {   return m_pHookPoints[ hookLevel - base ];  }

    LsiApiHooks * get( int hookLevel )
    {   return m_pHookPoints[ hookLevel - base ];  }
    
    void set( int hookLevel, LsiApiHooks * hooks )
    {   m_pHookPoints[hookLevel - base] = hooks;   }

};

template< int base, int size >
class SessionHooks
{
    enum
    {
        UNINIT = 0,
        INITED,
        HASOWN,
    };
    

    
private:
    int8_t *m_pEnableArray[size];
    HookChainList<base, size> *m_pHookChainList;
    short   m_iFlag[size];
    short   m_iStatus;
    
    
    void inheritFromParent(SessionHooks<base, size> *parentSessionHooks)
    {
        assert(m_iStatus != UNINIT);
        if ( m_iStatus == HASOWN)
            return ;
        
        for( int i=0; i<size; ++i )
        {
            int level = base + i;
            int level_size = m_pHookChainList->get(level)->size();
            memcpy(m_pEnableArray[i], parentSessionHooks->getEnableArray(level), level_size * sizeof(int8_t));
        }
        memcpy(m_iFlag, parentSessionHooks->m_iFlag, size * sizeof(short));
    }
    
    void updateFlag(int level)
    {
        int index = level - base;
        m_iFlag[index] = 0;
        int level_size = m_pHookChainList->get(level)->size();
        LsiApiHook *pHook = m_pHookChainList->get(level)->begin();
        int8_t *pEnableArray = m_pEnableArray[index];
        
        for( int j=0; j<level_size; ++j)
        {
            //For the disabled hook, just ignor it
            if (pEnableArray[j])
            {
                m_iFlag[index] |= pHook->_flag | LSI_HOOK_FLAG_ENABLED;
            }
            ++pHook;
        }
    }
    
    void updateFlag()
    {
        assert(m_pHookChainList);
        for( int i=0; i<size; ++i ) 
        {
            updateFlag(base + i);
        }
    }
    
    void inheritFromGlobal()
    {
        assert(m_iStatus != UNINIT);
        if ( m_iStatus == HASOWN)
            return ;
        
        
        for( int i=0; i<size; ++i ) 
        {
            int level_size = m_pHookChainList->get(base + i)->size();
            LsiApiHook *pHook = m_pHookChainList->get(base + i)->begin();
            int8_t *pEnableArray = m_pEnableArray[i];
            for( int j=0; j<level_size; ++j)
            {
                pEnableArray[j] = ( (pHook->_flag & LSI_HOOK_FLAG_ENABLED) ? 1 : 0);
                ++pHook;
            }
        }
        updateFlag();
    }
    
    //int and set the disable array from the global(in the LsiApiHook flag)
    int initSessionHooks(HookChainList<base, size> *gHookChainList)
    {
        if(m_pHookChainList || m_iStatus != UNINIT)
            return 1;
            
        if (!gHookChainList)
            return -1;
        
        m_pHookChainList = gHookChainList;
        for( int i=0; i<size; ++i) 
        {
            //int level = base + i;
            int level_size = m_pHookChainList->get(base + i)->size();
            m_pEnableArray[i] = new int8_t[level_size];
        }
        m_iStatus = INITED;

        return 0;
    }
    
    
public:
    //with the globalHooks for determine the base and size
    SessionHooks() : m_pHookChainList(0), m_iStatus(UNINIT)  {}

    ~SessionHooks() 
    {
        if(m_pHookChainList)
        {
            for( int i=0; i<size; ++i) 
                delete []m_pEnableArray[i];
        }
    }

    void disableAll()
    {
        if (m_iStatus > UNINIT)
        {
            for( int i=0; i<size; ++i ) 
            {
                int level_size = m_pHookChainList->get(base + i)->size();
                memset(m_pEnableArray[i], 0, level_size);
                m_iFlag[i] = 0;
            }
            m_iStatus = INITED; 
        }
    }
    
    void inherit(HookChainList<base, size> *gHookChainList, SessionHooks<base, size> *parentRt, int isGlobal)
    {
        switch(m_iStatus)
        {
        case UNINIT:
            initSessionHooks(gHookChainList);
            //No break, follow with the next
        case INITED:
            if ( parentRt && !parentRt->isAllDisabled())
                inheritFromParent(parentRt);
            else if (isGlobal)
                inheritFromGlobal();
            else
                disableAll();
            break;
        case HASOWN:
        default:
            break;
        }
    }
    
    

    int8_t *getEnableArray(int level)  {   return m_pEnableArray[level - base]; }
    
    int setEnable(int level, const lsi_module_t *pModule, int enable)
    {
        if (m_iStatus < INITED)
            return -1;
        
        assert(m_pHookChainList);
        int level_index = -1;
        LsiApiHook * pHook = m_pHookChainList->get(level)->find(pModule, &level_index);
        if( pHook == NULL)
            return -1;
        
        assert(level_index >= 0);
        int8_t * pEnableArray = m_pEnableArray[level - base];        
        pEnableArray[level_index] = (enable ? 1 : 0);
        
        if(enable)
            m_iFlag[level - base] |= (pHook->_flag | LSI_HOOK_FLAG_ENABLED);
        else
        {
            updateFlag(level);
        }
        m_iStatus = HASOWN;
        return 0;
    }
    
    void setEnableOfModule(const lsi_module_t *pModule, int enable)
    {
        assert(m_pHookChainList);
        for( int level = base; level < base + size; ++level )
            setEnable(level, pModule, enable);
    }
    
    
    void reset()
    {
        if (m_iStatus >= INITED) 
            m_iStatus = INITED; 
    };
    
    int getBase()   { return base;  }
    int getSize()   { return size;  }
    
    int isNotInited() const   {   return ((m_pHookChainList == NULL) || (m_iStatus == UNINIT)); }
    
    int isAllDisabled() const   {   return isNotInited(); }
    
    
    short getFlag( int hookLevel ) { return m_iFlag[hookLevel - base]; }
//    void  setFlag( int hookLevel, short f )        {   m_iFlag[hookLevel - base] |= f;               }
    
    
   int isDisabled(int hookLevel) const  
    {
        if (isAllDisabled())
            return 1;
        return ( (m_iFlag[hookLevel - base] & LSI_HOOK_FLAG_ENABLED) == 0);
    }
    
    int isEnabled( int hookLevel ) const {  return !isDisabled(hookLevel); }
        
    
    int runCallback( int level, lsi_cb_param_t *param) const
    {   return m_pHookChainList->get(level)->runCallback( level, m_pEnableArray[level - base], param );   }
    
    int runCallback( int level, LsiSession *session, void *param1, int paramLen1, int *param2, int paramLen2) const
    {   return m_pHookChainList->get(level)->runCallback( level, m_pEnableArray[level - base], session, param1, paramLen1, param2, paramLen2 );       }
    
    int runCallbackViewData( int level, LsiSession *session, void *inBuf, int inLen) const
    {   return m_pHookChainList->get(level)->runCallbackViewData( level, m_pEnableArray[level - base], session, inBuf, inLen );   }
    
    int runCallbackNoParam( int level, LsiSession *session) const
    {   return m_pHookChainList->get(level)->runCallbackNoParam( level, m_pEnableArray[level - base], session );      }
    
};




#endif // LSIAPIHOOKS_H
