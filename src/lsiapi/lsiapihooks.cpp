#include "lsiapihooks.h"
#include "lsiapi.h"
#include <string.h>

IolinkSessionHooks  LsiApiHooks::m_apiIolinkHooks(1);
HttpSessionHooks  LsiApiHooks::m_apiHttpHooks(1);
static LsiApiHooks s_releaseDataHooks[LSI_MODULE_DATA_COUNT];
const LsiApiHooks * LsiApiHooks::getGlobalApiHooks( int index )
{ 
    if (index < LSI_HKPT_L4_COUNT)
        return LsiApiHooks::m_apiIolinkHooks.get(index);
    else
        return LsiApiHooks::m_apiHttpHooks.get(index);
}

LsiApiHooks * LsiApiHooks::getReleaseDataHooks( int index )
{   return &s_releaseDataHooks[index];   }

LsiApiHooks::LsiApiHooks( const LsiApiHooks& other )
    : m_pHooks( NULL )
    , m_iCapacity( 0 )
    , m_iBegin( 0 )
    , m_iEnd( 0 )
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
    if ( flag & LSI_HOOK_FLAG_TRANSFORM )
        m_iFlag |= LSI_HOOK_FLAG_TRANSFORM;
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
        if( ( pHook->_module == pModule ) )
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
        if( ( pHook->_module == pModule ) )
            remove( pHook );
    }
    
    return 0;
}

//need to check Hook count before call this function
int LsiApiHooks::runFunctions(struct lsi_cb_param_t *param) const
{    
    int ret = 0;
    lsi_cb_param_t rec0, rec1;
    int bufLen = param->_param2_len;
    
    rec0._param2 = (void *)param->_param1;
    rec0._param2_len = param->_param1_len;
    rec1._hook_info = param->_hook_info;
    rec1._session = param->_session;
    rec1._param2_len  = 0;
    
    LsiApiHook *hook = begin();
    while(hook != end())
    {
        rec1._cur_hook = (void *)hook;
        rec1._param1 = rec0._param2;
        rec1._param1_len = rec0._param2_len;
        rec1._param2 = param->_param2;
        rec1._param2_len = bufLen;
        ret = hook->_cb(&rec1);

//         if ( D_ENABLED( DL_MORE ))
//             LOG_D(( "[LSIAPIHooks] runkFunctions, module name %s, ret %d", 
//                 hook->_module->_name, ret ));
        
        if (ret != 0)
            break;
    
        rec0 = rec1;
        ++hook;
    }
    
//    *outLen = rec1.outLen;
    return ret;
}

int LsiApiHooks::runFunctions( const LsiSession *session, void *param1, int paramLen1, void *param2, int paramLen2) const
{
    struct lsi_cb_param_t param;
    lsi_hook_info_t info;
    param._session = (void *)session;
    param._cur_hook = (void *)begin();
    info._hooks = this;
    info._termination_fp = NULL;
    param._hook_info = &info;
    param._param1 = param1;
    param._param1_len = paramLen1;
    param._param2 = param2;
    param._param2_len = paramLen2;
    return runFunctions(&param);
}

//need to check Hook count before call this function
int LsiApiHooks::runFunctionsViewData( const LsiSession *session, void *inBuf, int inLen) const
{
    return runFunctions(session, inBuf, inLen, NULL, 0);
}

int LsiApiHooks::runNoParamFunctions( const LsiSession *session) const
{
    return runFunctions(session, NULL, 0, NULL, 0);
}


