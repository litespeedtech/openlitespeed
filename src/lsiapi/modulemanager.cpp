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
#include "modulemanager.h"
#include <log4cxx/logger.h>
#include <http/httpglobals.h>
#include <lsiapi/lsiapi.h>
#include <dlfcn.h>
#include <errno.h>
#include <time.h>
#include <util/xmlnode.h>
#include <lsiapi/lsiapihooks.h>
#include <util/datetime.h>

ModuleConfig ModuleManager::m_gModuleConfig;
extern lsi_api_t  gLsiapiFunctions;
const char *ModuleConfig::FilterKeyName[] = {
    "L4_BEGSESSION",
    "L4_ENDSESSION",
    "L4_RECVING",
    "L4_SENDING",
    
    "HTTP_BEGIN",
    "RECV_REQ_HDR",
    "URI_MAP",
    "RECV_REQ_BDY",
    "RECVED_REQ_BDY",
    "RECV_RSP_HDR",
    "RECV_RSP_BDY",
    "RECVED_RSP_BDY",
    "SEND_RSP_HDR",
    "SEND_RSP_BDY",
    "HTTP_END"};


int ModuleManager::initModule()
{
    if ( LsiapiBridge::init_lsiapi() != 0 )
        return -1;

    memset(m_iModuleDataCount, 0, sizeof(short) * LSI_MODULE_DATA_COUNT);    
    clear();
    return 0;
}

char *ModuleManager::getModulePath(const char *name, char *path)
{
    strcpy ( path, HttpGlobals::s_pServerRoot );
    strcat ( path, "/modules/" );
    strcat ( path, name );
    strcat ( path, ".so" );
    return path;
}

// int ModuleManager::testModuleCount(const XmlNodeList *pList)
// {
//     int count = 0;
//     char sFilePath[512];
//     const char *name;
//     void *sLib;
//     lsi_module_t *pModule;
//     const XmlNode *pModuleNode;
//     for( XmlNodeList::const_iterator iter = pList->begin(); iter != pList->end(); ++iter )
//     {
//         pModuleNode = *iter;
//         name = pModuleNode->getChildValue( "name" );
//         getModulePath(pModuleNode->getChildValue( "name" ), sFilePath);
//         sLib = dlopen ( sFilePath, RTLD_LAZY );
//         if ( sLib )
//         {
//             pModule = ( lsi_module* ) dlsym ( sLib, name );
//             if ( dlerror() == NULL && pModule )
//                 ++count;
//             dlclose(sLib); 
//         }
//     }
//     return count;
// }

#define module_count_init_num   10
int ModuleManager::storeModulePointer( int index, lsi_module_t *pModule )
{
    static int module_count = module_count_init_num;
    if ( module_count <= index )
    {
        module_count += 5;
        m_gModuleArray = (ModulePointer *)realloc( m_gModuleArray, module_count * sizeof(ModulePointer));
    }
    m_gModuleArray[index] = pModule;
    
}

int ModuleManager::loadModule( const XmlNode *pModuleNode )
{
    const char * error;
    const char *name = pModuleNode->getChildValue( "name" );
    char sFilePath[512];
    getModulePath(name, sFilePath);

    void *sLib = dlopen ( sFilePath, RTLD_LAZY );
    if ( sLib )
    {
        lsi_module_t *pModule = ( lsi_module_t* ) dlsym ( sLib, name );
        if ( pModule )
        {
            if ( D_ENABLED ( DL_MORE ) )
                LOG_D (( "[%s] main module object located.", name ));
            if( (pModule->_signature >> 32) != 0x4C53494D )
            {
                error = "Module signature does not match";
            }
            else
            {
                LsiModule *pLmHttpHandler = new LsiModule ( pModule );
                pModule->_name = strdup ( name );
                pModule->_id = getModuleCount();
                memset(pModule->_data_id, 0xFF, sizeof(short) * LSI_MODULE_DATA_COUNT); //value is -1 now.
                ModuleConfig::parsePriority(pModuleNode, pModule->_priority);
                //m_gModuleArray[pModule->_id] = pModule;
                storeModulePointer( pModule->_id, pModule );
                
                insert ( ( char * ) pLmHttpHandler->getModule()->_name, pLmHttpHandler );
                if ( D_ENABLED ( DL_MORE ) )
                    LOG_D (( "[%s] built with API v%hd.%hd, registered successfully.", ( char * ) pLmHttpHandler->getModule()->_name,
                        (int16_t)(pModule->_signature >> 16), (int16_t)pModule->_signature ));
                return 0;
            }
        }
        else
            error = dlerror();
        dlclose(sLib); 
    }
    else
        error = dlerror();

    LOG_ERR(( "[%s] load module failed with error: %s", name, error ));

    return -1;
}

void ModuleManager::disableModule(lsi_module_t *pModule)
{
    int i;
    pModule->_handler = NULL;

    int base = LsiApiHooks::getIolinkHooks()->getBase();
    int size = LsiApiHooks::getIolinkHooks()->getSize();
    for (i=0; i<size; ++i)
        ((LsiApiHooks *)(LsiApiHooks::getIolinkHooks()->get(i + base)))->remove(pModule);
    
    base = LsiApiHooks::getHttpHooks()->getBase();
    size = LsiApiHooks::getHttpHooks()->getSize();
    for (i=0; i<size; ++i)
        ((LsiApiHooks *)(LsiApiHooks::getHttpHooks()->get(i + base)))->remove(pModule);
}

int ModuleManager::runModuleInit()
{
    lsi_module_t *pModule;
    for (int i=0; i<getModuleCount(); ++i)
    {
        pModule = m_gModuleArray[i];
        if (pModule->_init)
        {
            int ret = pModule->_init();
            if (ret != 0)
            {
                disableModule(pModule);
                LOG_ERR(( "[%s] initialization failure, disabled", pModule->_name ));
            }
            else
            {
                LOG_INFO (( "[%s] [%s] has been initialized successfully", pModule->_name,  ((pModule->_info) ? pModule->_info : "") ));
            }
        }
    }
    return 0;
}

int ModuleManager::loadModules( const XmlNodeList *pModuleNodeList )
{
    m_gModuleArray = (ModulePointer *)malloc(module_count_init_num * sizeof(ModulePointer));
    XmlNodeList::const_iterator iter;
    int count = 0;
    for( iter = pModuleNodeList->begin(); iter != pModuleNodeList->end(); ++iter )
    {
        const XmlNode *pModuleNode = *iter;
        if (loadModule(pModuleNode)  == 0)
            ++count;
    }
    
    
    return count;
}

int ModuleManager::unloadModules()
{
    LsiModule *pLmHttpHandler;
    iterator iter;
    for ( iter = begin(); iter != end(); iter = next ( iter ) )
    {
        pLmHttpHandler = ( LsiModule* ) iter.second();
        free ( pLmHttpHandler->getModule()->_name );
        delete pLmHttpHandler;
        pLmHttpHandler = NULL;
    }
    clear();
    free(m_gModuleArray);
    return 0;
}

short ModuleManager::getModuleDataCount(unsigned int level)
{
    if (level < LSI_MODULE_DATA_COUNT)
        return m_iModuleDataCount[level];
    else
        return -1;//Error, should not access this value
}

void ModuleManager::incModuleDataCount(unsigned int level)
{
    if (level < LSI_MODULE_DATA_COUNT)
        ++ m_iModuleDataCount[level];
}

void ModuleManager::OnTimer10sec()
{
    LsiapiBridge::expire_gd_check();
}

void ModuleManager::OnTimer100msec()
{
    //Check module timer here and trigger timeout ones
    LinkedObj *headPos = HttpGlobals::s_ModuleTimerList.head();
    assert(headPos != NULL);
    ModuleTimer *pNext = NULL;
    while(1)
    {
        pNext = (ModuleTimer *)(headPos->next());
        //TODO: need to compare uSec
        if (pNext && ((pNext->m_tmExpire < DateTime::s_curTime) ||
            ((pNext->m_tmExpire == DateTime::s_curTime) && (pNext->m_tmExpireUs <= DateTime::s_curTimeUs))))
        {
            pNext->m_TimerCb(pNext->m_pTimerCbParam);
            headPos->removeNext();
            delete pNext;
        }
        else
            break;
    }
}

void ModuleManager::inheritIolinkApiHooks(IolinkSessionHooks *apiIolinkHooks, ModuleConfig *moduleConfig)
{
    apiIolinkHooks->inherit(LsiApiHooks::getIolinkHooks());
    if( moduleConfig->isMatchGlobal() != 0)
        return ;
    
    int count = moduleConfig->getCount();
    for (short module_id = 0; module_id < count; ++module_id)
    {
        if ( !moduleConfig->getFilterEnable(module_id) )
        {
             for (int level= apiIolinkHooks->getBase(); level< apiIolinkHooks->getBase() + apiIolinkHooks->getSize(); ++level)
                apiIolinkHooks->getCopy(level)->remove(m_gModuleArray[module_id]);
        }
    }
}

/****
 * There is a int fromGlobal because global level inherit is different.
 * Global level will add all hooks to global hooks, even the moduleConfig is disable the hook.
 * We need to do so is because global level hooks should have all of the hook so that 
 * the other level can inheit from it no matter the hook is enabled or disabled
 * 
 * The findParentMatch() will find if match with real global(all ENABLED), if YES, return 1 and 
 * will inherit from the global hooks
 * If not match real global, will try to match parent, if YES, return 2.
 * But if fromGlobal is 1, the parent is also global, the Moduleconfig is NOT all enabled, so that
 * the inherit need to be treated as not match
 */
void ModuleManager::inheritHttpApiHooks(HttpSessionHooks *apiHttpHooks, ModuleConfig *moduleConfig)
{
    apiHttpHooks->inherit(LsiApiHooks::getHttpHooks());  //inherit from global level
    int count = moduleConfig->getCount();
    for (short module_id = 0; module_id < count; ++module_id)
    {
        if ( !moduleConfig->getFilterEnable(module_id) )
        {
            for (int level= apiHttpHooks->getBase(); level< apiHttpHooks->getBase() + apiHttpHooks->getSize(); ++level)
                apiHttpHooks->getCopy(level)->remove(m_gModuleArray[module_id]);
        }
    }
}
    
void ModuleManager::updateHttpApiHook(HttpSessionHooks *apiHttpHooks, ModuleConfig *moduleConfig, int module_id)
{
    if ( !moduleConfig->getFilterEnable(module_id) )
    {
        for (int level= apiHttpHooks->getBase(); level< apiHttpHooks->getBase() + apiHttpHooks->getSize(); ++level)
            apiHttpHooks->getCopy(level)->remove(m_gModuleArray[module_id]);
    }
}

void ModuleConfig::init(int count)
{  
    initData(count);
    for (int i=0; i<m_count; ++i)
    {
        lsi_module_config_t * pConfig = new lsi_module_config_t;
        memset(pConfig, 0, sizeof(lsi_module_config_t));
        LsiModuleData::set(i, (void *) pConfig);
    }
}

ModuleConfig::~ModuleConfig() 
{
    lsi_module_config_t * config = NULL;
    for (int i=0; i<m_count; ++i)
    {
        config = get(i);
        if (config->own_data_flag == 2 && config->config && config->module->_config_parser->_free_config)
            config->module->_config_parser->_free_config(config->config);
        delete config;
    }
}

void ModuleConfig::setFilterEnable(short _module_id, int v) 
{
    ModuleConfig::setFilterEnable(get(_module_id), v);
}
    
int ModuleConfig::getFilterEnable(short _module_id) 
{
    return ModuleConfig::getFilterEnable(get(_module_id));
}

int ModuleConfig::compare(lsi_module_config_t *config1, lsi_module_config_t *config2)
{
    if (config1->filters_enable == config2->filters_enable &&
        config1->module == config2->module)
        return 0;
    else
        return 1;
}

void ModuleConfig::inherit(const ModuleConfig *parentConfig)
{
    for (int i=0; i<m_count; ++i)
    {
        lsi_module_config_t * config = get(i);
        memcpy(config, parentConfig->get(i), sizeof(lsi_module_config_t));
        config->own_data_flag = 0;  //may inherit the config but this flag should not be set
        config->sparam = NULL;
    }
}

//Return 0, no match. 1, match root
int ModuleConfig::isMatchGlobal()
{
    for (int i=0; i<m_count; ++i)
    {
        if (getFilterEnable(i) == 0)
            return 0;
    }
    return 1;
}

int ModuleConfig::parsePriority(const XmlNode *pModuleNode, int *priority)
{
    const char *pValue = NULL;
    const XmlNode *pPriority = pModuleNode->getChild( "hookpriority" );
    for(int i=0; i<LSI_HKPT_TOTAL_COUNT; ++i)
    {
        if (pPriority && ( pValue= pPriority->getChildValue(FilterKeyName[i])))
            priority[i] = atoi(pValue);
        else
            priority[i] = LSI_MAX_HOOK_PRIORITY + 1;
    }
    return 0;
}

int ModuleConfig::saveConfig(const XmlNode *pNode, lsi_module_t *pModule, lsi_module_config_t * module_config)
{
    int ret = 0;
    const char *pValue = NULL;
    
    assert(module_config->module == pModule);
    pValue = pNode->getChildValue("enabled");
    if (pValue)
        module_config->filters_enable = (int16_t)atoi(pValue);
    else
        module_config->filters_enable = -1;
    
    if (pModule->_config_parser && pModule->_config_parser->_parse_config)
    {
        if( (pValue = pNode->getChildValue("param")) != NULL )
        {
            if (!module_config->sparam)
                module_config->sparam = new AutoStr2;

            if (module_config->sparam)
            {
                module_config->sparam->append("\n", 1);
                module_config->sparam->append(pValue, strlen(pValue));
                module_config->own_data_flag = 1;  //has data, not parsed
            }
        }
    }
    else
    {
        module_config->sparam = NULL;
        module_config->own_data_flag = 0;
    }
    
    return 0;
}


int ModuleConfig::parseConfig(const XmlNode *pNode, lsi_module_t *pModule, ModuleConfig *pModuleConfig)
{
    int ret = 0;
    const char *pValue = NULL;
    
    int module_id = pModule->_id;
    lsi_module_config_t *config = pModuleConfig->get(module_id);
    config->module = pModule;
    int updated = 0;
    
    pValue = pNode->getChildValue("enabled");
    if (pValue)
    {
        ModuleConfig::setFilterEnable(config, atoi(pValue));
    }
    
    pValue = pNode->getChildValue("param");
    if ( pModule->_config_parser && pModule->_config_parser->_parse_config)
    {
        config->config = pModule->_config_parser->_parse_config(pValue, config->config);
        config->own_data_flag = 2;
    }
    else
        config->own_data_flag = 0;
    config->sparam = NULL;
    return 0;
}

int ModuleConfig::parseConfigList(const XmlNodeList *moduleConfigNodeList, ModuleConfig *pModuleConfig)
{
    int ret = 0;
    const char *pValue = NULL;
    XmlNode *pNode = NULL;
    int i;
       
    XmlNodeList::const_iterator iter;
    for( iter = moduleConfigNodeList->begin(); iter != moduleConfigNodeList->end(); ++iter )
    {
        pNode = *iter;
        pValue = pNode->getChildValue("name");
        if (!pValue)
        {
            ret = -1;
            if ( D_ENABLED ( DL_MORE ) )
                LOG_D (( "[LSIAPI] parseConfigList error, no module name" ));
            break;
        }
        
        ModuleManager::iterator moduleIter;
        moduleIter = ModuleManager::getInstance().find(pValue);
        if (moduleIter == ModuleManager::getInstance().end())
            continue;
        
        parseConfig(pNode, moduleIter.second()->getModule(), pModuleConfig);
    }
    return ret;
}
