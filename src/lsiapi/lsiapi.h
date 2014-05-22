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
#ifndef LSIAPI_H
#define LSIAPI_H
#include <../addon/include/ls.h>
#include <util/hashstringmap.h>
#include <http/httplog.h>
#include "lsiapihooks.h"

#define LSIAPI extern "C"




class LsiModuleData ;

typedef struct gdata_key_s 
{
    char* key_str;
    int key_str_len;
} gdata_key_t;

typedef struct gdata_item_val_s 
{
    gdata_key_t key;          //Need to deep copy the original buffer
    void *value;
    time_t tmCreate;
    time_t tmExpire;        //Create time + TTL = expird time
    time_t tmAccess;        //For not checking file too often
    lsi_release_callback_pf release_cb;
} gdata_item_val_t;

typedef  THash<gdata_item_val_t *> __LsiGDataItemHashT;

typedef struct lsi_gdata_cont_val_s {
    gdata_key_t key;          //Need to deep copy the original buffer
    __LsiGDataItemHashT *container;
    time_t tmCreate;
    int type;
} lsi_gdata_cont_val_t;

typedef  THash<lsi_gdata_cont_val_t *> __LsiGDataContHashT;
//extern __LsiGDataContHashT *gLsiGDataContHashT[];


class LsiapiBridge
{
public:
    LsiapiBridge() {};
    ~LsiapiBridge() {};
    
    static lsi_api_t  gLsiapiFunctions;
    static __LsiGDataContHashT *gLsiGDataContHashT[LSI_CONTAINER_COUNT];  //global data
    
    
    static int init_lsiapi();
    static void uninit_lsiapi();
    static void expire_gdata_check();
    static void releaseModuleData( int level, LsiModuleData * pData );
    static lsi_api_t *   getLsiapiFunctions() { return &LsiapiBridge::gLsiapiFunctions; };

};

#endif // LSIAPI_H
