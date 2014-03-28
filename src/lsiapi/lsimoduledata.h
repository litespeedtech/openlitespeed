#ifndef LSIADDONDATA_H
#define LSIADDONDATA_H


#include <stdlib.h>
#include <assert.h>


class LsiModuleData
{
private:
    LsiModuleData& operator=(const LsiModuleData& other);
    bool operator==(const LsiModuleData& other) const;
    LsiModuleData(const LsiModuleData& other);
    
public:
    LsiModuleData()
        : m_pData( NULL )
        , m_count( 0 )
        {}
    ~LsiModuleData()
    {
        if ( m_pData )
            delete [] m_pData; 
    }
    bool isDataInited() { return m_pData != NULL;   }
    bool initData(int count);
    
    void set( short _data_id, void * data ) 
    {
        if (( _data_id >= 0 )&&( _data_id < m_count )) 
            m_pData[_data_id] = data;
    }
    void * get( short _data_id ) const
    {   
        if (( _data_id >= 0 )&&( _data_id < m_count )) 
            return m_pData[_data_id];
        return NULL;
    }
    void reset()
    {
        for( int i = 0; i < m_count; ++i )
        {
            assert(( m_pData[i] == NULL )&& "Addon Module must release data when session finishes" );
        }
    }
    
protected:
    void ** m_pData;
    int     m_count;
    
};


#endif // LSIADDONDATA_H
