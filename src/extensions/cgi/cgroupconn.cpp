/****************************************************************************
*    Copyright (C) 2019  LiteSpeed Technologies, Inc.                       *
*    All rights reserved.                                                   *
*    LiteSpeed Technologies Proprietary/Confidential.                       *
****************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>
#include "cgroupconn.h"

CGroupConn::CGroupConn()
{
    m_conn = NULL; 
    m_proxy = NULL;
    m_err = NULL;
    m_err_num = CERR_NO_ERROR;
    m_glib20 = NULL;
    m_gio20 = NULL;
    m_gobject20 = NULL;
    m_g_clear_error = NULL;
    m_g_variant_new = NULL;
    m_g_variant_builder_new = NULL;
    m_g_variant_builder_add_value = NULL;
    m_g_variant_builder_add = NULL;
    m_g_bus_get_sync = NULL;
    m_g_dbus_proxy_new_sync = NULL;
    m_g_dbus_proxy_call_sync = NULL;
    m_g_object_unref = NULL;
}


int CGroupConn::load_gdb()
{
    char *lib;
    m_glib20 = dlopen(lib = (char *)"libglib-2.0.so", RTLD_LAZY);
    if (!m_glib20)
        m_glib20 = dlopen("libglib-2.0.so.0", RTLD_LAZY);
    if (!m_glib20)
    {
        //printf("Unable to load %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }
    m_g_clear_error = (tg_clear_error)dlsym(m_glib20, lib = (char *)"g_clear_error");
    m_g_variant_new = (tg_variant_new)dlsym(m_glib20, lib = (char *)"g_variant_new");
    m_g_variant_builder_new = (tg_variant_builder_new)dlsym(m_glib20, lib = (char *)"g_variant_builder_new");
    if ((!m_g_clear_error) ||
        (!m_g_variant_new) ||
        (!m_g_variant_builder_new))
    {
        //printf("Unable to load symbol: %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }    
    
    m_gio20 = dlopen(lib = (char *)"libgio-2.0.so", RTLD_LAZY);
    if (!m_gio20)
        m_gio20 = dlopen("libgio-2.0.so.0", RTLD_LAZY);
    if (!m_gio20)
    {
        //printf("Unable to load %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }
    m_g_bus_get_sync = (tg_bus_get_sync)dlsym(m_gio20, lib = (char *)"g_bus_get_sync");
    m_g_dbus_proxy_new_sync = (tg_dbus_proxy_new_sync)dlsym(m_gio20, lib = (char *)"g_dbus_proxy_new_sync");
    m_g_dbus_proxy_call_sync = (tg_dbus_proxy_call_sync)dlsym(m_gio20, lib = (char *)"g_dbus_proxy_call_sync");
    m_g_variant_builder_add_value = (tg_variant_builder_add_value)dlsym(m_gio20, lib = (char *)"g_variant_builder_add_value");
    m_g_variant_builder_add = (tg_variant_builder_add)dlsym(m_gio20, lib = (char *)"g_variant_builder_add");
    
    if ((!m_g_bus_get_sync) ||
        (!m_g_dbus_proxy_new_sync) ||
        (!m_g_dbus_proxy_call_sync) ||
        (!m_g_variant_builder_add_value) ||
        (!m_g_variant_builder_add))
    {
        //printf("Unable to load symbol: %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }    

    m_gobject20 = dlopen(lib = (char *)"libgobject-2.0.so", RTLD_LAZY);
    if (!m_gobject20)
        m_gobject20 = dlopen("libgobject-2.0.so.0", RTLD_LAZY);
    if (!m_gobject20)
    {
        //printf("Unable to load %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }
    m_g_object_unref = (tg_object_unref)dlsym(m_gobject20, lib = (char *)"g_object_unref");
    if ((!m_g_object_unref))
    {
        //printf("Unable to load symbol: %s\n", lib);
        set_error(CERR_SYSTEM_ERROR);
        return -1;
    }   
    //printf("Loaded glib\n");
    return 0;
}


void CGroupConn::clear_err()
{
    if (!m_err)
        return;
    if (m_g_clear_error)
        m_g_clear_error(&m_err);
    m_err_num = CERR_NO_ERROR;
}    
    
int CGroupConn::create()
{
    if (load_gdb() == -1)
        return -1;
    
    clear_err();
    m_conn = m_g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                              NULL, // GCancellable
                              &m_err);
    if (m_err)
    {
        set_error(CERR_GDERR);
        return -1;
    }
    clear_err();
    m_proxy = m_g_dbus_proxy_new_sync(m_conn,
                                      0,                                    /* G_DBUS_PROXY_FLAGS_NONE, */
                                      NULL,                                 /* GDBusInterfaceInfo */
                                      "org.freedesktop.systemd1",           /* name */
                                      "/org/freedesktop/systemd1",          /* object path */
                                      "org.freedesktop.systemd1.Manager",   /* interface */
                                      NULL,                                 /* GCancellable */
                                      &m_err);
    if (m_err)
    {
        set_error(CERR_GDERR);
        return -1;
    }

    return 0;
};


CGroupConn::~CGroupConn()
{
    /* Actually this is kind of unnecessary as these are dynamic pointers, but
     * I never really believe that.  */
    if (m_proxy)
        m_g_object_unref(m_proxy);
    if (m_conn)
        m_g_object_unref(m_conn);
    clear_err();
    if (m_gobject20)
        dlclose(m_gobject20);
    if (m_gio20)
        dlclose(m_gio20);
    if (m_glib20)
        dlclose(m_glib20);
}

   
char *CGroupConn::getErrorText()
{
    enum CGroupErrors errnum = m_err_num;
    switch (errnum) 
    {
        case CERR_GDERR :
            return m_err->message;
        case CERR_INSUFFICIENT_MEMORY:
            return (char *)"Insufficient memory";
        case CERR_BAD_SYSTEMD:
            return (char *)"systemd not at high enough level for cgroups";
        case CERR_SYSTEM_ERROR:
            return (char *)"system error accessing cgroups";
        case CERR_NO_ERROR :
        default:
            return NULL;
    }
    return NULL;
}


int CGroupConn::getErrorNum()
{
    return (int)m_err_num;
}
