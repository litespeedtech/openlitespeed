
#include <../addon/include/ls.h>

#ifndef LSAPI_INTERNAL_H
#define LSAPI_INTERNAL_H

struct lsi_session_s;
class ModuleConfig;
class LogTracker;

typedef struct lsi_module_internal_t
{
    /**
     * @brief Initially set to NULL.  After a module is loaded, 
     * it will be set to the module name.
     * @since 1.0
     */
    const char               *   _name;
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the module id.
     * @since 1.0
     */
    int32_t                      _id;
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the user data id.
     * @since 1.0
     */
    int16_t                      _data_id[LSI_MODULE_DATA_COUNT];
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the priorities for each hook level.
     * @since 1.0
     */
    int32_t                      _priority[LSI_HKPT_TOTAL_COUNT]; 
} lsi_module_internal_t;

#define MODULE_NAME(x)      (((lsi_module_internal_t *)x->_reserved)->_name )
#define MODULE_ID(x)        (((lsi_module_internal_t *)x->_reserved)->_id )
#define MODULE_DATA_ID(x)   ((lsi_module_internal_t *)x->_reserved)->_data_id
#define MODULE_PRIORITY(x)  ((lsi_module_internal_t *)x->_reserved)->_priority


//#if sizeof( struct lsi_module_internal_t ) > LSI_MODULE_RESERVED_SIZE
//# error not enough space reserved for internal data in struct lsi_module_t
//#endif

struct lsi_session_s
{
};

typedef struct lsi_session_s lsi_session_t ;

class LsiSession : public lsi_session_s
{
public:
    LsiSession(){};
    virtual ~LsiSession(){};
    ModuleConfig * getModuleConfig()    { return m_pModuleConfig; };
    virtual LogTracker * getLogTracker() = 0;
    
protected:
    ModuleConfig * m_pModuleConfig;
};


#endif
