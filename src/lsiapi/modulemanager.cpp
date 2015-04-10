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
#include "modulemanager.h"

#include <http/handlertype.h>
#include <http/httplog.h>
#include <lsiapi/internal.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/lsiapilib.h>
#include <lsiapi/moduletimer.h>
#include <main/mainserverconfig.h>
#include <util/xmlnode.h>

#include <dlfcn.h>

#define INITIAL_MODULE_COUNT   10

ModuleConfig ModuleManager::g_moduleConfig;


LsiModule::LsiModule(lsi_module_t *pModule)
{
    setHandlerType(HandlerType::HT_MODULE);
    m_pModule = pModule;
}


int ModuleManager::initModule()
{
    if (LsiapiBridge::initLsiapi() != 0)
        return LS_FAIL;

    memset(m_aModuleDataCount, 0, sizeof(short) * LSI_MODULE_DATA_COUNT);
    clear();
    return 0;
}


int ModuleManager::getModulePath(const char *name, char *path, int max_len)
{
    int n = snprintf(path, max_len - 1, "%s/modules/%s.so",
                     MainServerConfig::getInstance().getServerRoot(),
                     name);
    if (n >= max_len)
    {
        n = max_len - 1;
        path[n] = 0;
    }
    return n;
}


// int ModuleManager::testModuleCount(const XmlNodeList *pList)
// {
//     int count = 0;
//     char sFilePath[512];
//     const char *name;
//     void *sLib;
//     lsi_module_t *pModule;
//     const XmlNode *pModuleNode;
//     XmlNodeList::const_iterator iter;
//     for( iter = pList->begin(); iter != pList->end(); ++iter )
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


int ModuleManager::storeModulePointer(int index, lsi_module_t *pModule)
{
    static int module_count = INITIAL_MODULE_COUNT;
    if (module_count <= index)
    {
        module_count += 5;
        m_pModuleArray = (ModulePointer *)realloc(m_pModuleArray,
                         module_count * sizeof(ModulePointer));
    }
    m_pModuleArray[index] = pModule;
    return 0;
}


ModuleManager::iterator ModuleManager::addModule(const char *name,
        const char *pType, lsi_module_t *pModule)
{
    iterator iter;
    LsiModule *pLmHttpHandler = new LsiModule(pModule);
    MODULE_NAME(pModule) = strdup(name);
    MODULE_ID(pModule) = getModuleCount();
    memset(MODULE_DATA_ID(pModule), 0xFF,
           sizeof(short) * LSI_MODULE_DATA_COUNT); //value is -1 now.
    MODULE_HOOKINDEX(pModule) = new ModIndex();
    MODULE_HANDLER(pModule) = pLmHttpHandler;
    //m_gModuleArray[pModule->_id] = pModule;
    storeModulePointer(MODULE_ID(pModule), pModule);

    iter = insert((char *) MODULE_NAME(pModule), pLmHttpHandler);
    if (D_ENABLED(DL_MORE))
        LOG_D(("%s module [%s] built with API v%hd.%hd has been"
               " registered successfully.",
               pType, MODULE_NAME(pModule),
               (int16_t)(pModule->_signature >> 16),
               (int16_t)pModule->_signature));
    return iter;
}


//extern lsi_module_t * getPrelinkedModule( const char * pModule );
extern int getPrelinkedModuleCount();
extern lsi_module_t *getPrelinkedModuleByIndex(unsigned int index,
        const char **pName);


int ModuleManager::loadPrelinkedModules()
{
    int n = getPrelinkedModuleCount();
    lsi_module_t *pModule;
    int count = 0;
    const char *pName;
    for (int i = 0; i < n; ++i)
    {
        pModule = getPrelinkedModuleByIndex(i, &pName);
        if (pModule)
        {
            if (addModule(pName, "Intenal", pModule) != end())
            {
                ModuleConfig::parsePriority(NULL, MODULE_PRIORITY(pModule));
                ++count;
            }
        }
    }
    return count;
}


lsi_module_t *ModuleManager::loadModule(const char *name)
{
    const char *error;
    lsi_module_t *pModule = NULL;
    void *dlLib = NULL;
    const char *pType = "External";

    iterator iter = find(name);
    if (iter != end())
        return iter.second()->getModule();

    //prelinked checked, now, check file
    char sFilePath[512];
    getModulePath(name, sFilePath, sizeof(sFilePath));
    dlLib = dlopen(sFilePath, RTLD_LAZY);
    if (dlLib)
        pModule = (lsi_module_t *) dlsym(dlLib, name);
    if (!pModule || !dlLib)
        error = dlerror();

    if (pModule)
    {
        if ((pModule->_signature >> 32) != 0x4C53494D)
            error = "Module signature does not match";
        else
        {
            if (addModule(name, pType, pModule) != end())
                return pModule;
            else
                error = "Registration failure.";
        }
    }

    if (dlLib)
        dlclose(dlLib);

    LOG_ERR(("Failed to load module [%s], error: %s", name, error));
    return NULL;
}


void ModuleManager::disableModule(lsi_module_t *pModule)
{
    int i;
    pModule->_handler = NULL;

    for (i = 0; i < LSI_HKPT_TOTAL_COUNT; ++i)
        ((LsiApiHooks *)(LsiApiHooks::getGlobalApiHooks(i)))->remove(pModule);

}


static void checkModuleDef(lsi_module_t *pModule)
{
#define DEF_VERSION_INFO_REPFIX  "version:"
#define DEF_VERSION_INFO_REPFIX_SIZE  (sizeof(DEF_VERSION_INFO_REPFIX) -1)


    char defPath[513] = {0};
    snprintf(defPath, 512, "%s/modules/%s.def",
             MainServerConfig::getInstance().getServerRoot(),
             MODULE_NAME(pModule));
    const char *info = pModule->_info;
    int match = 0;
    size_t infoLen = 0;
    if (info)
    {
        infoLen = strlen(info);
        //We will save and compare only the first 240 bytes
        if (infoLen > 240)
            infoLen = 240;
    }


    FILE *fp = fopen(defPath, "rb");
    if (fp)
    {
        char sLine[256] = {0};
        fgets(sLine, 255, fp);
        size_t len2 = strlen(sLine);
        if (len2 > DEF_VERSION_INFO_REPFIX_SIZE)
        {
            char *pinfo = sLine + DEF_VERSION_INFO_REPFIX_SIZE;
            len2 -= DEF_VERSION_INFO_REPFIX_SIZE + 1;

            // We added a \n at the end of the line, so adjust for it.
            if (len2 >= 1)
                len2 -= 1;

            if (info == NULL)
            {
                if (len2 == 0)
                    match = 1;
            }
            else
            {
                if (infoLen == len2 &&
                    memcmp(info, pinfo, infoLen) == 0)
                    match = 1;
            }
        }
        fclose(fp);
    }

    if (!match)
    {
        fp = fopen(defPath, "wb");
        if (fp)
        {
            AutoStr2 sInfo;
            sInfo.setStr(DEF_VERSION_INFO_REPFIX);
            if (info != NULL)
                sInfo.append(info, infoLen);

            fprintf(fp, "%s\n\n", sInfo.c_str());

            if (pModule->_config_parser)
            {
                const char *p;
                while ((p = *pModule->_config_parser->_config_keys) != NULL)
                {
                    fprintf(fp, "param.%s\n", p);
                    ++pModule->_config_parser->_config_keys;
                }
            }

            if (pModule->_serverhook)
            {
                int count = 0;
                while (pModule->_serverhook[count].cb)
                {
                    int index = pModule->_serverhook[count].index;
                    if (index < 0 || index >= LSI_HKPT_TOTAL_COUNT)
                        LOG_ERR(("[%s] create def file failure,"
                                 " wrong index(%d).\n",
                                 MODULE_NAME(pModule), index));
                    else
                        fprintf(fp, "priority.%s:%d\n",
                                LsiApiHooks::s_pHkptName[index],
                                pModule->_serverhook[count].priority);
                    ++count;
                }
            }
            fclose(fp);
        }
    }
}


int ModuleManager::runModuleInit()
{
    lsi_module_t *pModule;
    for (int i = 0; i < getModuleCount(); ++i)
    {
        pModule = m_pModuleArray[i];
        if (pModule->_init)
        {
            checkModuleDef(pModule);
            int ret = pModule->_init(pModule);
            if (ret != 0)
            {
                disableModule(pModule);
                LOG_ERR(("[%s] initialization failure, disabled",
                         MODULE_NAME(pModule)));
            }
            else
            {
                //add global level hooks here
                if (pModule->_serverhook)
                {
                    int count = 0;
                    while (pModule->_serverhook[count].cb)
                    {
                        add_global_hook(pModule->_serverhook[count].index,
                                        pModule,
                                        pModule->_serverhook[count].cb,
                                        pModule->_serverhook[count].priority,
                                        pModule->_serverhook[count].flag);
                        ++count;
                    }
                }

                LOG_INFO(("[Module: %s %s] has been initialized successfully",
                          MODULE_NAME(pModule),
                          ((pModule->_info) ? pModule->_info : "")));
            }
        }
    }
    return 0;
}


int ModuleManager::loadModules(const XmlNodeList *pModuleNodeList)
{
    m_pModuleArray = (ModulePointer *)malloc(INITIAL_MODULE_COUNT * sizeof(
                         ModulePointer));
    XmlNodeList::const_iterator iter;
    int count = loadPrelinkedModules();
    if (!pModuleNodeList)
        return count;

    for (iter = pModuleNodeList->begin(); iter != pModuleNodeList->end();
         ++iter)
    {
        const XmlNode *pModuleNode = *iter;
        lsi_module_t *pModule;
        const char *name = pModuleNode->getChildValue("name", 1);
        if (!name)
            continue;
        if ((pModule = loadModule(name)) != NULL)
        {
            ModuleConfig::parsePriority(pModuleNode, MODULE_PRIORITY(pModule));
            ++count;
        }
    }

    return count;
}


int ModuleManager::unloadModules()
{
    LsiModule *pLmHttpHandler;
    iterator iter;
    for (iter = begin(); iter != end(); iter = next(iter))
    {
        pLmHttpHandler = (LsiModule *) iter.second();
        free((void *)MODULE_NAME(pLmHttpHandler->getModule()));
        delete pLmHttpHandler;
        pLmHttpHandler = NULL;
    }
    clear();
    free(m_pModuleArray);
    return 0;
}


short ModuleManager::getModuleDataCount(unsigned int level)
{
    if (level < LSI_MODULE_DATA_COUNT)
        return m_aModuleDataCount[level];
    else
        return LS_FAIL;//Error, should not access this value
}


void ModuleManager::incModuleDataCount(unsigned int level)
{
    if (level < LSI_MODULE_DATA_COUNT)
        ++ m_aModuleDataCount[level];
}


void ModuleManager::updateDebugLevel()
{
    LsiapiBridge::getLsiapiFunctions()->_debugLevel = HttpLog::getDebugLevel();
}


void ModuleManager::OnTimer10sec()
{
    LsiapiBridge::checkExpiredGData();
}


void ModuleManager::OnTimer100msec()
{
    ModTimerList::getInstance().checkExpired();
}


void ModuleManager::applyConfigToIolinkRt(IolinkSessionHooks
        *pSessionHooks, ModuleConfig *moduleConfig)
{
    int count = moduleConfig->getCount();
    for (short module_id = 0; module_id < count; ++module_id)
    {
        if (!moduleConfig->getFilterEnable(module_id))
            pSessionHooks->setModuleEnable(m_pModuleArray[module_id], 0);
    }
}


void ModuleManager::applyConfigToServerRt(ServerSessionHooks
        *pSessionHooks, ModuleConfig *moduleConfig)
{
    int count = moduleConfig->getCount();
    for (short module_id = 0; module_id < count; ++module_id)
    {
        if (!moduleConfig->getFilterEnable(module_id))
            pSessionHooks->setModuleEnable(m_pModuleArray[module_id], 0);
    }
}


void ModuleManager::applyConfigToHttpRt(HttpSessionHooks *pSessionHooks,
                                        ModuleConfig *moduleConfig)
{
    int count = moduleConfig->getCount();
    for (short module_id = 0; module_id < count; ++module_id)
    {
        if (!moduleConfig->getFilterEnable(module_id))
            pSessionHooks->setModuleEnable(m_pModuleArray[module_id], 0);
    }
}


void ModuleManager::updateHttpApiHook(HttpSessionHooks *pRtHooks,
                                      ModuleConfig *moduleConfig, int module_id)
{
    if (!moduleConfig->getFilterEnable(module_id))
        pRtHooks->setModuleEnable(m_pModuleArray[module_id], 0);
}


void ModuleConfig::init(int count)
{
    initData(count);
    for (int i = 0; i < m_iCount; ++i)
    {
        lsi_module_config_t *pConfig = new lsi_module_config_t;
        memset(pConfig, 0, sizeof(lsi_module_config_t));
        LsiModuleData::set(i, (void *) pConfig);
    }
}


ModuleConfig::~ModuleConfig()
{
    lsi_module_config_t *config = NULL;
    for (int i = 0; i < m_iCount; ++i)
    {
        config = get(i);
        if ((config->data_flag == LSI_CONFDATA_PARSED)
            && (config->config != NULL)
            && (config->module->_config_parser->_free_config))
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


int ModuleConfig::compare(lsi_module_config_t *config1,
                          lsi_module_config_t *config2)
{
    if (config1->filters_enable == config2->filters_enable &&
        config1->module == config2->module)
        return 0;
    else
        return 1;
}


void ModuleConfig::inherit(const ModuleConfig *parentConfig)
{
    for (int i = 0; i < m_iCount; ++i)
    {
        lsi_module_config_t *config = get(i);
        memcpy(config, parentConfig->get(i), sizeof(lsi_module_config_t));
        config->data_flag = LSI_CONFDATA_NONE;
        config->sparam = NULL;
    }
}


//Return 0, no match. 1, match root
int ModuleConfig::isMatchGlobal()
{
    for (int i = 0; i < m_iCount; ++i)
    {
        if (getFilterEnable(i) == 0)
            return 0;
    }
    return 1;
}


int ModuleConfig::parsePriority(const XmlNode *pModuleNode, int *priority)
{
    const char *pHkptName;
    const char *pValue = NULL;
    for (int i = 0; i < LSI_HKPT_TOTAL_COUNT; ++i)
    {
        pHkptName = LsiApiHooks::s_pHkptName[i];
        if ((pModuleNode)
            && (pValue = pModuleNode->getChildValue(pHkptName)))
            priority[i] = atoi(pValue);
        else
            priority[i] = LSI_MAX_HOOK_PRIORITY + 1;
    }
    return 0;
}


int ModuleConfig::saveConfig(const XmlNode *pNode, lsi_module_t *pModule,
                             lsi_module_config_t *module_config)
{
    const char *pValue = NULL;

    assert(module_config->module == pModule);
    pValue = pNode->getChildValue("enabled");
    if (pValue)
        module_config->filters_enable = (int16_t)atoi(pValue);
    else
        module_config->filters_enable = -1;

    if (pModule->_config_parser && pModule->_config_parser->_parse_config)
    {
        if ((pValue = pNode->getChildValue("param")) != NULL)
        {
            if (!module_config->sparam)
                module_config->sparam = new AutoStr2;

            if (module_config->sparam)
            {
                module_config->sparam->append("\n", 1);
                module_config->sparam->append(pValue, strlen(pValue));
                module_config->data_flag = LSI_CONFDATA_OWN;
            }
        }
    }
    else
    {
        module_config->sparam = NULL;
        module_config->data_flag = LSI_CONFDATA_NONE;
    }

    return 0;
}


// int ModuleConfig::parseOutsideModuleParam(const XmlNode *pNode)
// {
//     XmlNodeList::const_iterator iter;
//     const XmlNodeList *pUnknList = parentNode->getChildren("unknownkeywords");
//     if (!pUnknList )
//         return 0;
//
//     for( iter = pUnknList->begin(); iter != pUnknList->end(); ++iter )
//     {
//         config->config = pModule->_config_parser->_parse_config((*iter)->getValue(), config->config);
//
//
//     ModuleConfig *pConfig = new ModuleConfig;
//         pConfig->init(ModuleManager::getInstance().getModuleCount());
//         pConfig->inherit(ModuleManager::getGlobalModuleConfig());
//         ModuleConfig::parseConfigList(pModuleList, pConfig);
//         pRootContext->setModuleConfig(pConfig, 1);
//
// }


int ModuleConfig::parseConfig(const XmlNode *pNode, lsi_module_t *pModule,
                              ModuleConfig *pModuleConfig, int level, const char *name)
{
    const char *pValue = NULL;
    int iValueLen = 0;

    int module_id = MODULE_ID(pModule);
    lsi_module_config_t *config = pModuleConfig->get(module_id);
    config->module = pModule;

    pValue = pNode->getChildValue("enabled");
    iValueLen = pNode->getChildValueLen("enabled");
    if (pValue)
        ModuleConfig::setFilterEnable(config, atoi(pValue));

    pValue = pNode->getChildValue("param");
    iValueLen = pNode->getChildValueLen("param");
    if (pModule->_config_parser && pModule->_config_parser->_parse_config)
    {
        if (pModule->_config_parser->_config_keys)
        {
            ;
            //TODO: check the param one by one if it is in array
//             const char *p;
//             while (p = *pModule->_config_parser->_config_keys)
//             {
//                 LOG_INFO(( "%s", p));
//                 ++pModule->_config_parser->_config_keys;
//             }

        }
        config->config = pModule->_config_parser->_parse_config(pValue,
                         iValueLen, config->config, level, name);
        config->data_flag = LSI_CONFDATA_PARSED;
    }
    else
        config->data_flag = LSI_CONFDATA_NONE;
    config->sparam = NULL;
    return 0;
}


int ModuleConfig::parseConfigList(const XmlNodeList *moduleConfigNodeList,
                                  ModuleConfig *pModuleConfig, int level, const char *name)
{
    if (!moduleConfigNodeList)
        return 0;

    int ret = 0;
    const char *pValue = NULL;
    XmlNode *pNode = NULL;

    XmlNodeList::const_iterator iter;
    for (iter = moduleConfigNodeList->begin();
         iter != moduleConfigNodeList->end(); ++iter)
    {
        pNode = *iter;
        pValue = pNode->getChildValue("name", 1);
        if (!pValue)
        {
            ret = -1;
            if (D_ENABLED(DL_MORE))
                LOG_D(("[LSIAPI] parseConfigList error, no module name"));
            break;
        }

        ModuleManager::iterator moduleIter;
        moduleIter = ModuleManager::getInstance().find(pValue);
        if (moduleIter == ModuleManager::getInstance().end())
            continue;

        parseConfig(pNode, moduleIter.second()->getModule(), pModuleConfig,
                    level, name);
    }
    return ret;
}
