/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include "../addon/include/ls.h"
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsimoduledata.h>

#include <util/hashstringmap.h>
#include <http/handlertype.h>
#include <http/httphandler.h>
#include <util/tsingleton.h>
#include <stdint.h>

class XmlNodeList;
class XmlNode;
class ModuleHandler;
class ModuleConfig;
class HttpContext;

template< int base, int size >
class SessionHooks;


typedef lsi_module_t * ModulePointer;

class LsiModule : public HttpHandler
{
    friend class ModuleHandler;
public:
    const char * getName() const    {   return "module";    }
    explicit LsiModule(lsi_module_t * pModule) {   setHandlerType(HandlerType::HT_MODULE); m_pModule = pModule;  }
    lsi_module_t * getModule() const  {   return m_pModule;   }
private:
    lsi_module_t * m_pModule;
};

class ModuleManager : public HashStringMap<LsiModule *>, public TSingleton<ModuleManager>
{
    friend class TSingleton<ModuleManager>;
private:
    ModuleManager(){ m_gModuleArray = NULL;};
    iterator addModule( const char * name, const char * pType, lsi_module_t * pModule );
    int getModulePath(const char *name, char *path, int max_len);
    lsi_module_t * loadModule( const char *name );
    void disableModule(lsi_module_t *pModule);
    int storeModulePointer( int index, lsi_module_t *module );
    int loadPrelinkedModules();
    
public:
    ~ModuleManager()
    {}

    int initModule();
    
    int runModuleInit();
    
    int loadModules( const XmlNodeList *pModuleNodeList );
    int unloadModules();

    int getModuleCount()  {   return size();  }
    short getModuleDataCount(unsigned int level);
    void incModuleDataCount(unsigned int level);

    void OnTimer100msec();
    void OnTimer10sec();
    
    void inheritIolinkApiHooks(IolinkSessionHooks *apiIolinkHooks, ModuleConfig *moduleConfig);
    void inheritHttpApiHooks(HttpSessionHooks *apiHttpHooks, ModuleConfig *moduleConfig);
    void updateHttpApiHook(HttpSessionHooks *apiHttpHooks, ModuleConfig *moduleConfig, int module_id);
    
    static ModuleConfig *getGlobalModuleConfig() { return &m_gModuleConfig; }
    static void updateDebugLevel();    
    
private:
    ModulePointer *m_gModuleArray;  //It is pointers stored base on the _module_id order
    short  m_iModuleDataCount[LSI_MODULE_DATA_COUNT];
    static ModuleConfig m_gModuleConfig;
    
};


/***
 * Inheriting of the lsi_module_config_t
 * Global to listener and global to VHost do not inherit, 
 * only between the contexts, may inherit
 * if only copy pointer of the struct, not set the BIT in the context.
 * Otherwise, set the BIT and own_data_flag set to 0 and config and sparam to NULL
 * 
 */
typedef struct lsi_module_config_t 
{
    lsi_module_t *module;
    int16_t     own_data_flag;  //0: no own data, may use the inherited config, no sparam, 1: own, not parsed, 2: own, parsed
    int16_t     filters_enable; //Comment: use bits, now bits count is LSI_HKPT_TOTAL_COUNT 
    void *      config;  //Should be a struct of all parameters, if config_own_flag set, this need to be released
    AutoStr2*   sparam;
} lsi_module_config_t; 


class ModuleConfig : public LsiModuleData
{
public:
    ModuleConfig() {};
    ~ModuleConfig();

    void copy( short _module_id, lsi_module_config_t * module_config ) 
    {  
        lsi_module_config_t *config = get(_module_id);
        memcpy(config, module_config, sizeof(lsi_module_config_t));
        //config->config_own_flag = 0;     //should I have this void *config, in case lsi_module_config_t * module_config will be released outside
    }
    
    lsi_module_config_t * get( short _module_id ) const
    {   return (lsi_module_config_t *)LsiModuleData::get(_module_id);    }
    
    
public:
    int getCount()  { return m_count;   }
    
    void init(int count);
    void inherit(const ModuleConfig *parentConfig);
    
    int isMatchGlobal();
    
    void setFilterEnable(short _module_id, int v);
    int getFilterEnable(short _module_id);
    
    
public:    
    static void setFilterEnable(lsi_module_config_t * module_config, int v) { module_config->filters_enable = ((v) ? 1 : 0);  }
    static int  getFilterEnable(lsi_module_config_t * module_config)   { return module_config->filters_enable; }
    static int  compare(lsi_module_config_t *config1, lsi_module_config_t *config2);
    
    static int  parsePriority(const XmlNode *pModuleNode, int *priority);
    static int  parseConfig(const XmlNode *pModuleNode, lsi_module_t *pModule, ModuleConfig *pModuleConfig, int level, const char *name);
    static int  saveConfig(const XmlNode *pModuleUrlfilterNode, lsi_module_t *pModule, lsi_module_config_t * module_config);
    static int  parseConfigList(const XmlNodeList *moduleConfigNodeList, ModuleConfig *pModuleConfig, int level, const char *name);
    
};


#endif // MODULEMANAGER_H
