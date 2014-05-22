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
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsiapigd.h>
#include <util/vmembuf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "modulemanager.h"
#include "util/ghash.h"
#include <http/httpglobals.h>
#include <http/staticfilecachedata.h>
#include <lsiapi/lsiapi.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <util/ni_fio.h>
#include <util/gpath.h>
#include <util/datetime.h>
#include <stdio.h>

static hash_key_t  lsi_global_data_hash_fn (const void *val)
{
    const gdata_key_t *key = (const gdata_key_t *)val;
    register hash_key_t __h = 0;
    register const char * p = (const char *)key->key_str;
    int count = key->key_str_len;
    while(count-- > 0)
        __h = __h * 37 + *((const char*)p++);

    return __h;
}

static int  lsi_global_data_cmp( const void * pVal1, const void * pVal2 )
{
    const gdata_key_t * pKey1 = (const gdata_key_t *)pVal1;
    const gdata_key_t * pKey2 = (const gdata_key_t *)pVal2; 
    if (!pKey1 || !pKey2 || pKey1->key_str_len != pKey2->key_str_len)
        return -1;
    
    return memcmp( pKey1->key_str, pKey2->key_str, pKey1->key_str_len );
} 

void AllGlobalDataHashTInit()
{
    for(int i=0;i<LSI_CONTAINER_COUNT; ++i )
        LsiapiBridge::gLsiGDataContHashT[i] = new __LsiGDataContHashT(30, lsi_global_data_hash_fn, lsi_global_data_cmp);
}

void releaseGDataContainer(__LsiGDataItemHashT *containerInfo)
{
    __LsiGDataItemHashT::iterator iter;
    for(iter = containerInfo->begin(); iter != containerInfo->end(); iter = containerInfo->next(iter))
    {
        iter.second()->release_cb(iter.second()->value);
        free(iter.second()->key.key_str);
    }
}

void AllGlobalDataHashTUnInit()
{
    __LsiGDataContHashT *pLsiGDataContHashT = NULL;
    lsi_gdata_cont_val_t *containerInfo = NULL;
    __LsiGDataContHashT::iterator iter;
    int i;
    for( i=0;i<LSI_CONTAINER_COUNT; ++i )
    {
        pLsiGDataContHashT = LsiapiBridge::gLsiGDataContHashT[i];
        for(iter = pLsiGDataContHashT->begin(); iter != pLsiGDataContHashT->end(); iter = pLsiGDataContHashT->next(iter))
        {
            containerInfo = iter.second();
            releaseGDataContainer(containerInfo->container);
            free(containerInfo->key.key_str);
            delete containerInfo->container;
            free(containerInfo);
        }
        delete LsiapiBridge::gLsiGDataContHashT[i];
    }
}


/***
 * 
 * 
 * Below is the gdata functions
 * IF NEED TO UPDAT THE HASH FUNCTION, THEN THE BELOW FUNCTION NEED TO BE UPDATED!!!
 * 
 * 
 */
static int buildFileDataLocation( char *pBuf, int len, const gdata_key_t &key, const char *gDataTag, int type )
{
    long tmp = lsi_global_data_hash_fn(&key);
    unsigned char * achHash = (unsigned char *)&tmp;
    int n = snprintf( pBuf, len, "%sgdata/%s%d/%x%x/%x%x/%d.tmp", 
                      HttpGlobals::s_pServerRoot, gDataTag, type,
                      achHash[0], achHash[1], achHash[2], achHash[3], key.key_str_len);
    return n;
}

__LsiGDataItemHashT::iterator get_gdata_iterator(__LsiGDataItemHashT *container, const char *key, int key_len)
{
    gdata_key_t key_st = {(char *)key, key_len};
    __LsiGDataItemHashT::iterator iter = container->find(&key_st);
    return iter;
}

//For memory type, file_path is useless
//renew_gdata_TTL will not update teh acc time and teh Acc time should be updated when accessing.
static void renew_gdata_TTL(__LsiGDataItemHashT::iterator iter, int TTL, int type, const char *file_path)
{   
    gdata_item_val_t* pItem = iter.second();
    pItem->tmCreate = DateTime::s_curTime;
    pItem->tmExpire = TTL + pItem->tmCreate;
    
    //if type is file, modify it
    if (type == LSI_CONTAINER_FILE && file_path != NULL)
    {
        struct stat st;
        FILE *fp = fopen(file_path, "r+b");
        if(fp)
        {
            fwrite(&pItem->tmExpire, 1, sizeof(time_t), fp);
            fclose(fp);
            stat(file_path, &st);
            pItem->tmCreate = st.st_mtime;
        }
    }
}

void erase_gdata_element(lsi_gdata_cont_val_t* containerInfo, __LsiGDataItemHashT::iterator iter)
{
    gdata_item_val_t* pItem = iter.second();
    if (containerInfo->type == LSI_CONTAINER_FILE)
    {
        char file_path[LSI_MAX_FILE_PATH_LEN] = {0};
        buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, pItem->key, "i", containerInfo->type);
        unlink(file_path);
    }
    
    if (pItem->release_cb)
        pItem->release_cb(pItem->value);
    free(pItem->key.key_str);
    free(pItem);
    containerInfo->container->erase(iter);
}

static FILE *open_file_to_write_n_check_dir(const char *file_path)
{
    FILE *fp = fopen (file_path, "wb");
    if (!fp)
    {
        GPath::createMissingPath((char *)file_path, 0700);
        fp = fopen (file_path, "wb");
    }
    return fp;
}

//-1 error
static int recover_file_gdata(lsi_gdata_cont_val_t *containerInfo, const char *key, int key_len, const char *file_path, __LsiGDataItemHashT::iterator &iter, lsi_release_callback_pf release_cb, lsi_deserialize_pf deserialize_cb)
{
    //If no deserialize presented, can not recover!
    if (!deserialize_cb)
        return -1;
    
//    gdata_item_val_st *data = NULL;
    struct stat st;
    if (stat(file_path, &st) == -1)
        return -1;
    
    int need_del_file = 0;
    time_t tmCur = DateTime::s_curTime;    
    time_t tmExpire = tmCur;
    int fd = -1;
    
    if (containerInfo->tmCreate > st.st_mtime)
        need_del_file = 1;
    else 
    {
        fd = open(file_path, O_RDONLY);
        if (fd != -1)
        {
            read(fd, &tmExpire, 4);
            if (tmExpire <= tmCur)
            {
                close(fd);
                need_del_file = 1;
            }
        }
        else
            need_del_file = 1;
    }
   
    //if tmExpire < st.st_mtime, it is an error, just clean it
    if (need_del_file || tmExpire < st.st_mtime)
    {
        unlink(file_path);
        return -1;
    }
    
    __LsiGDataItemHashT *pCont = containerInfo->container;
    gdata_item_val_t * pItem = NULL;
    if(iter == pCont->end())
    {
        char *p = (char *)malloc(key_len);
        if (!p)
        {
            close(fd);
            return -1;
        }
    
        pItem = (gdata_item_val_t *)malloc(sizeof(gdata_item_val_t));
        memcpy(p, key, key_len);
        pItem->key.key_str = p;
        pItem->key.key_str_len = key_len;
        iter = pCont->insert(&pItem->key, pItem);
    }
    else
        pItem = iter.second();

    char *buf = NULL;
    off_t length = st.st_size;
    
    buf = (char *) mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED)
    {
        close(fd);
        return -1;
    }
    
    void *val = deserialize_cb(buf + sizeof(time_t), length - sizeof(time_t));
    pItem->value = val;
    pItem->release_cb = release_cb;
    pItem->tmCreate = st.st_mtime;
    pItem->tmExpire = tmExpire;
    pItem->tmAccess = tmCur;
    
    munmap(buf, length);
    close(fd);
    return 0;
}


#define GDATA_FILE_NOCHECK_TIMEOUT      5

//Should use gdata_container_val_st instead of __LsiGDataItemHashT
//cahe FILE mtim should always be NEWer or the same (>=) than cache buffer
LSIAPI void * get_gdata(lsi_gdata_cont_val_t *containerInfo, const char *key, int key_len, lsi_release_callback_pf release_cb, int renew_TTL, lsi_deserialize_pf deserialize_cb)
{
    __LsiGDataItemHashT *pCont = containerInfo->container;
    __LsiGDataItemHashT::iterator iter = get_gdata_iterator(pCont, key, key_len);
    gdata_item_val_t* pItem = NULL;
    int TTL = 0;
    time_t  tm = DateTime::s_curTime;
    char file_path[LSI_MAX_FILE_PATH_LEN] = {0};
    
    //item exists
    if(iter != pCont->end())
    {
        pItem = iter.second();
        //container emptied
        if (pItem->tmCreate < containerInfo->tmCreate)
        {
            erase_gdata_element(containerInfo, iter);
            return NULL;
        }

        TTL = pItem->tmExpire - pItem->tmCreate;
        if(containerInfo->type == LSI_CONTAINER_MEMORY)
        {
            if (renew_TTL)
                renew_gdata_TTL(iter, TTL, LSI_CONTAINER_MEMORY, NULL);
        }
        else
        {
            //if last access time is still same as tm
            if (pItem->tmAccess != tm)
            {
                buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, pItem->key, "i", containerInfo->type);
            
                if (renew_TTL)
                    renew_gdata_TTL(iter, TTL, LSI_CONTAINER_FILE, file_path);
                else
                {
                    if (tm > GDATA_FILE_NOCHECK_TIMEOUT + pItem->tmAccess)
                    {
                        struct stat st;
                        if (stat(file_path, &st) == -1)
                        {
                            erase_gdata_element(containerInfo, iter);
                            return NULL;
                        }
                        else if (pItem->tmCreate != st.st_mtime)
                        {
                            pItem->release_cb(pItem->value);
                            if (recover_file_gdata(containerInfo, key, key_len, file_path, iter, release_cb, deserialize_cb) != 0)
                            {
                                erase_gdata_element(containerInfo, iter);
                                return NULL;
                            }
                        }
                    }
                }
            }
        }
    }
    else //not exists
    {
        if(containerInfo->type == LSI_CONTAINER_MEMORY)
            return NULL;
        else
        {
            gdata_key_t key_st = {(char *)key, key_len};
            buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, key_st, "i", containerInfo->type);
            if (recover_file_gdata(containerInfo, key, key_len, file_path, iter, release_cb, deserialize_cb) != 0)
            {
                unlink(file_path);
                return NULL;
            }
            pItem = iter.second();
        }
    }   

    pItem->tmAccess = tm;
    return pItem->value;    
}

LSIAPI int delete_gdata(lsi_gdata_cont_val_t *containerInfo, const char *key, int key_len)
{
    __LsiGDataItemHashT::iterator iter = get_gdata_iterator(containerInfo->container, key, key_len);
    __LsiGDataItemHashT *pCont = containerInfo->container;
    if(iter != pCont->end())
    {
        erase_gdata_element(containerInfo, iter);
    }
    
    return 0;
}

LSIAPI int set_gdata(lsi_gdata_cont_val_t *containerInfo, const char *key, int key_len, void *val, int TTL, lsi_release_callback_pf release_cb, int force_update, lsi_serialize_pf serialize_cb)
{
    gdata_item_val_t *data = NULL;
    char file_path[LSI_MAX_FILE_PATH_LEN] = {0};
    __LsiGDataItemHashT *pCont = containerInfo->container;
    __LsiGDataItemHashT::iterator iter = get_gdata_iterator(pCont, key, key_len);
    
    if(iter != pCont->end())
    {
        if(!force_update)
            return 1;
        else
        {
            data = iter.second();
            //Release the prevoius value with the prevoius callback
            if (data->release_cb)
                data->release_cb(data->value);
        }
    }
    else 
    {
        if (key_len <= 0)
            return -1;
        
        char *p = (char *)malloc(key_len);
        if (!p)
            return -1;
    
        data = (gdata_item_val_t *)malloc(sizeof(gdata_item_val_t));
        memcpy(p, key, key_len);
        data->key.key_str = p;
        data->key.key_str_len = key_len;
        iter = pCont->insert(&data->key, data);
    }

    data->value = val;
    data->release_cb = release_cb;
    data->tmCreate = DateTime::s_curTime;
    if (TTL < 0) 
        TTL = 3600 * 24 * 365;
    data->tmExpire = TTL + data->tmCreate;
    data->tmAccess = data->tmCreate;
    
    if (containerInfo->type == LSI_CONTAINER_FILE)
    {
        buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, data->key, "i", containerInfo->type);
        FILE *fp = open_file_to_write_n_check_dir (file_path);
        if (!fp)
            return -1;
        
        struct stat st;
        int length = 0;
        char *buf = serialize_cb(val, &length);
        fwrite(&data->tmExpire, 1, sizeof(time_t), fp);
        fwrite(buf, 1, length, fp);
        fclose(fp);
        free(buf); //serialize_cb MUST USE alloc, malloc or realloc to get memory, so here use free to release it
        
        stat(file_path, &st);
        iter.second()->tmCreate = st.st_mtime;
    }
    
    
    return 0;
}



LSIAPI lsi_gdata_cont_val_t *get_gdata_container(int type, const char *key, int key_len)
{
    if ( (type != LSI_CONTAINER_MEMORY  && type != LSI_CONTAINER_FILE) || key_len <= 0 || key == NULL)
        return NULL;
    
    __LsiGDataContHashT *pLsiGDataContHashT = LsiapiBridge::gLsiGDataContHashT[type];
    lsi_gdata_cont_val_t *containerInfo = NULL;
    char file_path[LSI_MAX_FILE_PATH_LEN] = {0};
        
    //find if exist, otherwise create it
    gdata_key_t key_st = {(char *)key, key_len};
    __LsiGDataContHashT::iterator iter = pLsiGDataContHashT->find(&key_st);
    if (iter != pLsiGDataContHashT->end())
        containerInfo = iter.second();
    else
    {
        char *p = (char *)malloc(key_len);
        if (!p)
            return NULL;
        
        __LsiGDataItemHashT *pContainer = new __LsiGDataItemHashT(30, lsi_global_data_hash_fn, lsi_global_data_cmp);
        memcpy(p, key, key_len);
        
        time_t tm = DateTime::s_curTime;
        containerInfo = (lsi_gdata_cont_val_t *)malloc(sizeof(lsi_gdata_cont_val_t));
        containerInfo->key.key_str = p;
        containerInfo->key.key_str_len = key_len;
        containerInfo->container = pContainer;
        containerInfo->tmCreate = tm;
        containerInfo->type = type;
        pLsiGDataContHashT->insert(&containerInfo->key, containerInfo);
        
        //If cache file exist, use the ceate time recorded
        buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, containerInfo->key, "c", type );
        
        FILE *fp = fopen (file_path, "rb");
        if (fp)
        {
            fread((char *)&tm, 1, 4, fp);
            fclose(fp);
            containerInfo->tmCreate = tm;
        }
        else
        {
            fp = open_file_to_write_n_check_dir (file_path);
            if (fp)
            {
                fwrite((char *)&tm, 1, 4, fp);
                fclose(fp);
            }            
        }
    }
    return containerInfo;
}

//empty will re-set the create time, and all items create time older than the container will be pruged when call purge_gdata_container
LSIAPI int empty_gdata_container(lsi_gdata_cont_val_t *containerInfo)
{
    containerInfo->tmCreate = DateTime::s_curTime;
    char file_path[LSI_MAX_FILE_PATH_LEN] = {0};
    time_t tm = DateTime::s_curTime;
    
    buildFileDataLocation( file_path, LSI_MAX_FILE_PATH_LEN, containerInfo->key, "c", containerInfo->type );
    FILE *fp = open_file_to_write_n_check_dir (file_path);
    if (!fp)
        return -1;
    fwrite((char *)&tm, 1, 4, fp);
    fclose(fp);
    return 0;
}

//purge will delete the item and also will remove the file if exist
LSIAPI int purge_gdata_container(lsi_gdata_cont_val_t *containerInfo)
{
    __LsiGDataItemHashT *pCont = containerInfo->container;
    for(__LsiGDataItemHashT::iterator iter = pCont->begin(); iter != pCont->end(); iter = pCont->next(iter))
    {
        erase_gdata_element(containerInfo, iter);
    }
    return 0;
}




