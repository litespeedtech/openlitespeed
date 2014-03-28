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
#ifndef LSIAPI_LINKLIST_H
#define LSIAPI_LINKLIST_H

#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _link_obj;
struct _link_list;

typedef void (* _linkobj_free)( void *data);


typedef struct _link_obj
{
    void    *data;
    struct _link_obj *next;
    struct _link_obj *prev;
} link_obj;

typedef struct _link_list
{
    struct _link_obj *head;
    _linkobj_free freecb;
    int size;
} link_list;


void _linklist_init( struct _link_list *list, _linkobj_free freecb );

static inline int _linklist_getSize( struct _link_list *list )         {   return list->size;   }
static inline int _linklist_isEmpty( struct _link_list *list )         {   return (list->size == 0); }
static inline struct _link_obj * _linklist_begin( struct _link_list *list )  {  return ((list->head) ?  list->head->next : NULL); }

static inline int _linklist_isEnd( struct _link_list *list, struct _link_obj *obj )     {   return (obj == NULL); }



int _linklist_push( struct _link_list *list, struct _link_obj *obj );
int _linklist_insert( struct _link_list *list, struct _link_obj *obj, struct _link_obj *objToInsert );

int _linklist_pushData( struct _link_list *list, void *data );
int _linklist_insertData( struct _link_list *list, struct _link_obj *obj, void *data );


void _linklist_del( struct _link_list *list, struct _link_obj *obj );
void _linklist_delAllNext( struct _link_list *list, struct _link_obj *obj );
void _linklist_release( struct _link_list *list );


#ifdef __cplusplus
}
#endif

#endif // LSIAPI_LINKLIST_H
