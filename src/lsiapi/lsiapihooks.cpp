#include "lsiapihooks.h"
#include "lsiapi.h"
#include <http/httplog.h>
#include <http/httpsession.h>
#include <util/logtracker.h>
#include <lsiapi/internal.h>

#include <string.h>

const char * LsiApiHooks::s_pHkptName[LSI_HKPT_TOTAL_COUNT] = 
{
    "L4_BEGINSESSION",
    "L4_ENDSESSION",
    "L4_RECVING",
    "L4_SENDING",
    "HTTP_BEGIN",
    "RECV_REQ_HEADER",
    "URI_MAP",
    "HTTP_AUTH",
    "RECV_REQ_BODY",
    "RCVD_REQ_BODY",
    "RECV_RESP_HEADER",
    "RECV_RESP_BODY",
    "RCVD_RESP_BODY",
    "HANDLER_RESTART",
    "SEND_RESP_HEADER",
    "SEND_RESP_BODY",
    "HTTP_END",
    "MAIN_INITED",
    "MAIN_PREFORK",
    "MAIN_POSTFORK",
    "WORKER_POSTFORK",
    "WORKER_ATEXIT",
    "MAIN_ATEXIT",
};

IolinkSessionHooks * LsiApiHooks::m_pIolinkHooks = NULL;
HttpSessionHooks  * LsiApiHooks::m_pHttpHooks = NULL;
ServerSessionHooks  * LsiApiHooks::m_pServerHooks = NULL;
static LsiApiHooks s_releaseDataHooks[LSI_MODULE_DATA_COUNT];

#define LSI_HOOK_FLAG_NO_INTERRUPT  (1<<7) 



void LsiApiHooks::initGlobalHooks()
{
    m_pIolinkHooks = new IolinkSessionHooks( 1 );
    m_pHttpHooks = new HttpSessionHooks( 1 );
    m_pServerHooks = new ServerSessionHooks( 1 );
    
    m_pIolinkHooks->get( LSI_HKPT_L4_BEGINSESSION )->setFlag( LSI_HOOK_FLAG_NO_INTERRUPT );
    m_pIolinkHooks->get( LSI_HKPT_L4_ENDSESSION )->setFlag( LSI_HOOK_FLAG_NO_INTERRUPT );
    
    m_pHttpHooks->get( LSI_HKPT_HTTP_BEGIN )->setFlag( LSI_HOOK_FLAG_NO_INTERRUPT );
    m_pHttpHooks->get( LSI_HKPT_HTTP_END )->setFlag( LSI_HOOK_FLAG_NO_INTERRUPT );
    m_pHttpHooks->get( LSI_HKPT_HANDLER_RESTART )->setFlag( LSI_HOOK_FLAG_NO_INTERRUPT );
}

const LsiApiHooks * LsiApiHooks::getGlobalApiHooks( int index )
{ 
    if (index < LSI_HKPT_L4_COUNT)
        return LsiApiHooks::m_pIolinkHooks->get(index);
    else if (index <= LSI_HKPT_HTTP_END)
        return LsiApiHooks::m_pHttpHooks->get(index);
    else
        return LsiApiHooks::m_pServerHooks->get(index);
}

LsiApiHooks * LsiApiHooks::getReleaseDataHooks( int index )
{   return &s_releaseDataHooks[index];   }

LsiApiHooks::LsiApiHooks( const LsiApiHooks& other )
    : m_pHooks( NULL )
    , m_iCapacity( 0 )
    , m_iBegin( 0 )
    , m_iEnd( 0 )
    , m_iFlag( 0 )
{
    if( reallocate( other.size() + 4, 2 ) != -1 )
    {
        memcpy( m_pHooks + m_iBegin, other.begin(), (char *)other.end() - (char *)other.begin() );
        m_iEnd = m_iBegin + other.size();
    }
}

int LsiApiHooks::copy( const LsiApiHooks& other )
{
    if ( m_iCapacity < other.size() )
        if( reallocate( other.size() + 4, 2 ) == -1 )
            return -1;
    m_iBegin = ( m_iCapacity - other.size() ) / 2;
    memcpy( m_pHooks + m_iBegin, other.begin(), (char *)other.end() - (char *)other.begin() );
    m_iEnd = m_iBegin + other.size();
    return 0;
}


int LsiApiHooks::reallocate( int capacity, int newBegin )
{
    LsiApiHook * pHooks;
    int size = this->size();
    if( capacity < size )
        capacity = size;
    if( capacity < size + newBegin )
        capacity = size + newBegin;
    pHooks = ( LsiApiHook * )malloc( capacity * sizeof( LsiApiHook ) );
    if( !pHooks )
        return -1;
    if( newBegin < 0 )
        newBegin = ( capacity - size ) / 2;
    if( m_pHooks )
    {
        memcpy( pHooks + newBegin, begin(), size * sizeof( LsiApiHook ) );
        free( m_pHooks );
    }

    m_pHooks = pHooks;
    m_iEnd = newBegin + size;
    m_iBegin = newBegin;
    m_iCapacity = capacity;
    return capacity;

}

//For same cb and same priority in same module, it will fail to add, but return 0 
short LsiApiHooks::add( const lsi_module_t *pModule, lsi_callback_pf cb, short priority, short flag )
{
    LsiApiHook * pHook;
    if( size() == m_iCapacity )
    {
        if( reallocate( m_iCapacity + 4 ) == -1 )
            return -1;

    }
    for( pHook = begin(); pHook < end(); ++pHook )
    {
        if( pHook->_priority > priority )
            break;
        else if( pHook->_priority == priority && pHook->_module == pModule && pHook->_cb == cb )
            return 0;  //already added
    }
    
    if( ( pHook != begin() ) || ( m_iBegin == 0 ) )
    {
        //cannot insert front
        if( ( pHook != end() ) || ( m_iEnd == m_iCapacity ) )
        {
            //cannot append directly
            if( m_iEnd < m_iCapacity )
            {
                //moving entries toward the end
                memmove( pHook + 1, pHook , (char *)end() - (char *)pHook );
                ++m_iEnd;
            }
            else
            {
                //moving entries before the insert point
                memmove( begin() - 1, begin(), (char *)pHook - (char *)begin() );
                --m_iBegin;
                --pHook;
            }
        }
        else
        {
            ++m_iEnd;
        }
    }
    else
    {
        --pHook;
        --m_iBegin;
    }
    pHook->_module = pModule;
    pHook->_cb = cb;
    pHook->_priority = priority;
    pHook->_flag = flag;
    m_iFlag |= ( flag & ( LSI_HOOK_FLAG_TRANSFORM 
                        | LSI_HOOK_FLAG_DECOMPRESS_REQUIRED 
                        | LSI_HOOK_FLAG_PROCESS_STATIC ) );
    return pHook - begin();
}

LsiApiHook * LsiApiHooks::find( const lsi_module_t *pModule, lsi_callback_pf cb )
{
    LsiApiHook * pHook;
    for( pHook = begin(); pHook < end(); ++pHook )
    {
        if( ( pHook->_module == pModule ) && ( pHook->_cb == cb ) )
            return pHook;
    }
    return NULL;
}

int LsiApiHooks::remove( LsiApiHook *pHook )
{
    if( ( pHook < begin() ) || ( pHook >= end() ) )
        return -1;
    if( pHook == begin() )
        ++m_iBegin;
    else
    {
        if( pHook != end() - 1 )
            memmove( pHook, pHook + 1, (char *)end() - (char *)pHook );
        --m_iEnd;
    }
    return 0;
}

//COMMENT: if one module add more hooks at this level, the return is the first one!!!!
LsiApiHook * LsiApiHooks::find( const lsi_module_t *pModule ) const
{
    LsiApiHook * pHook;
    for( pHook = begin(); pHook < end(); ++pHook )
    {
        if( pHook->_module == pModule )
            return pHook;
    }
    return NULL;
    
}

//COMMENT: if one module add more hooks at this level, the return of 
//LsiApiHooks::find( const lsi_module_t *pModule ) is the first one!!!!
//But the below function will remove all the hooks
int LsiApiHooks::remove( const lsi_module_t *pModule )
{
    LsiApiHook * pHook;
    for( pHook = begin(); pHook < end(); ++pHook )
    {
        if( pHook->_module == pModule )
            remove( pHook );
    }
    
    return 0;
}

//need to check Hook count before call this function
int LsiApiHooks::runCallback(int level, lsi_cb_param_t *param) const
{    
    int ret = 0;
    lsi_cb_param_t rec1;
        
    LsiApiHook *hook = begin();
    LsiApiHook *hookEnd = end();  //FIXME: I found end() will change since m_iEnd will change
    while(hook != hookEnd)
    {
        rec1 = *param;
        rec1._cur_hook = (void *)hook;

        if ( D_ENABLED( DL_MORE ))
        {
            if (param->_session) 
            {
                LogTracker * pTracker = ((LsiSession *)param->_session)->getLogTracker();
                LOG_D(( pTracker->getLogger(), "[%s] [%s] run Hook function for [Module:%s]", 
                    pTracker->getLogId(), s_pHkptName[ level ], MODULE_NAME( hook->_module ) ));
            } 
            else
            {
                LOG_D(( NULL, "[ServerHook: %s] run Hook function for [Module:%s]", 
                    s_pHkptName[ level ], MODULE_NAME( hook->_module ) ));
            }
        }
        
        ret = hook->_cb(&rec1);

        if ( D_ENABLED( DL_MORE ))
        {
            if (param->_session) 
            {
                LogTracker * pTracker = ((LsiSession *)param->_session)->getLogTracker();
                LOG_D(( pTracker->getLogger(), "[%s] [%s] [Module:%s] ret %d", 
                    pTracker->getLogId(), s_pHkptName[ level ], MODULE_NAME( hook->_module ), ret ));
            }
            else
            {
                LOG_D(( NULL, "[ServerHook: %s] [Module:%s] ret %d", 
                    s_pHkptName[ level ], MODULE_NAME( hook->_module ), ret ));
            }
        }
        if ((ret != 0)&&!(m_iFlag & LSI_HOOK_FLAG_NO_INTERRUPT) )
            break;
    
        ++hook;
    }
    
    return ret;
}




