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
#include "linklist.h"
#include <stdlib.h>
#include <string.h>

void _linklist_init( struct _link_list *list, _linkobj_free freecb )
{
    struct _link_obj *obj = (struct _link_obj *)malloc(sizeof(struct _link_obj));
    list->head = obj;
    list->head->data = NULL;
    list->head->next = NULL;
    list->head->prev = NULL;
    list->size = 0;
    list->freecb = freecb;  
}

int _linklist_push(struct _link_list *list, struct _link_obj *obj)
{
    struct _link_obj * p = list->head;
    while (p->next)
        p = p->next;
    
    obj->prev = p;
    obj->next = NULL;
    p->next = obj;
    ++list->size;
    return 0;
}
    
int _linklist_insert(struct _link_list *list, struct _link_obj *obj, struct _link_obj *objToInsert)
{
    if ( obj == list->head)
        return -1;
    
    if (obj == NULL)
        return _linklist_push( list, objToInsert);
        
    struct _link_obj * prev = obj->prev;
    prev->next = objToInsert;
    objToInsert->prev = prev;
    objToInsert->next = obj;
    obj->prev = objToInsert;
    ++list->size;
    return 0;
}

int _linklist_pushData(struct _link_list *list, void *data)
{
    struct _link_obj *obj = (struct _link_obj *)malloc(sizeof(struct _link_obj));
    if (obj == NULL)
        return -1;
    
    obj->data = data;
    return _linklist_push(list, obj);
}

int _linklist_insertData(struct _link_list *list, struct _link_obj *obj, void *data)
{
    struct _link_obj *objToInsert = (struct _link_obj *)malloc(sizeof(struct _link_obj));
    if (objToInsert == NULL)
        return -1;   
    
    objToInsert->data = data;
    return _linklist_insert(list, obj, objToInsert);    
}

void _linklist_del(struct _link_list *list, struct _link_obj *obj)
{
    if (list->size == 0 || obj == list->head)
        return ;
    
    struct _link_obj *prev = obj->prev;
    prev->next = obj->next;
    if (obj->next)
        obj->next->prev = prev;
    
    if (list->freecb)
        list->freecb(obj->data);
    free(obj);
    
    --list->size;
}

void _linklist_delAllNext(struct _link_list *list, struct _link_obj *obj)
{
    if (obj->next)
    {
        _linklist_delAllNext(list, obj->next);
        if (list->freecb)
            list->freecb(obj->next->data);
        free(obj->next);
    }
}

//once released, need to re-init before using
void _linklist_release(struct _link_list *list)
{
    if (list->size == 0)
        return ;
    
    list->size = 0;
    list->freecb = NULL;
    _linklist_delAllNext(list, list->head);
    free(list->head);
}

// int main()
// {
//     return 0;
// }
