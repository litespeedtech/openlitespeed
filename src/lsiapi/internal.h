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
#include <lsdef.h>
#include <ls.h>

#ifndef LSAPI_INTERNAL_H
#define LSAPI_INTERNAL_H

struct lsi_session_s;
class ModIndex;
class ModuleConfig;
class LogTracker;
class LsiModule;

typedef struct lsi_module_internal_s
{
    /**
     * @brief Initially set to NULL.  After a module is loaded,
     * it will be set to the module name.
     * @since 1.0
     */
    const char                  *name;

    /**
     * @brief Initially set to 0.  After a module is loaded,
     * it will be set to the module id.
     * @since 1.0
     */
    int32_t                      id;

    /**
     * @brief Initially set to 0.  After a module is loaded,
     * it will be set to the user data id.
     * @since 1.0
     */
    int16_t                      data_id[LSI_MODULE_DATA_COUNT];

    /**
     * @brief Initially set to 0.  After a module is loaded,
     * it will be set to the priorities for each hook level.
     * @since 1.0
     */
    int32_t                      priority[LSI_HKPT_TOTAL_COUNT];

    /**
     * @brief Initially set to NULL.  After a module is loaded,
     * it will be set to the index for each hook level.
     */
    ModIndex                    *hook_index;

    /**
     * @brief Initially set to NULL.  If the module has a handler,
     * it will be set to that handler object.
     */
    LsiModule                   *mod_handler;
} lsi_module_internal_t;

#define MODULE_NAME(x)      (((lsi_module_internal_t *)x->_reserved)->name )
#define MODULE_ID(x)        (((lsi_module_internal_t *)x->_reserved)->id )
#define MODULE_DATA_ID(x)   ((lsi_module_internal_t *)x->_reserved)->data_id
#define MODULE_PRIORITY(x)  ((lsi_module_internal_t *)x->_reserved)->priority
#define MODULE_HOOKINDEX(x) ((lsi_module_internal_t *)x->_reserved)->hook_index
#define MODULE_HANDLER(x)   ((lsi_module_internal_t *)x->_reserved)->mod_handler


//#if sizeof( struct lsi_module_internal_t ) > LSI_MODULE_RESERVED_SIZE
//# error not enough space reserved for internal data in struct lsi_module_t
//#endif

struct lsi_session_s
{
};

typedef struct lsi_session_s lsi_session_t;

class LsiSession : public lsi_session_s
{
public:
    LsiSession() {};
    virtual ~LsiSession() {};
    ModuleConfig *getModuleConfig()    { return m_pModuleConfig; };
    virtual LogTracker *getLogTracker() = 0;

    virtual int hookResumeCallback(int level, lsi_module_t *pModule) { return 0;};


protected:
    ModuleConfig *m_pModuleConfig;


    LS_NO_COPY_ASSIGN(LsiSession);
};

#endif
