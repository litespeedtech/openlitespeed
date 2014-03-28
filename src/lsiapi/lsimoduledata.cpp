#include "lsimoduledata.h"
#include "lsiapi.h"

typedef void * void_pointer;

bool LsiModuleData::initData(int count)
{
    assert( m_pData == NULL);
    
    m_pData = new void_pointer[count];
    if ( !m_pData )
        return false;
    
    memset( m_pData, 0, sizeof( void_pointer) * count );
    m_count = count;
    return true;
}
