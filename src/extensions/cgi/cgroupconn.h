/****************************************************************************
*    Copyright (C) 2019  LiteSpeed Technologies, Inc.                       *
*    All rights reserved.                                                   *
*    LiteSpeed Technologies Proprietary/Confidential.                       *
****************************************************************************/
#ifndef _CGROUP_CONN_H
#define _CGROUP_CONN_H

#include <lsdef.h>

/**
 * @file cgroupconn.h
 */

struct _GDBusConnection;
struct _GDBusProxy;
struct _GError;
struct _GVariantBuilder;
struct _GError;
struct _GDBusInterfaceInfo;
struct _GVariant;

typedef struct _GDBusConnection         GDBusConnection;
typedef struct _GDBusProxy              GDBusProxy;
typedef struct _GError                  GError;
typedef struct _GVariantBuilder         GVariantBuilder;
typedef struct _GError                  GError;
typedef struct _GDBusInterfaceInfo      GDBusInterfaceInfo;
typedef struct _GVariant                GVariant;
typedef char GVariantType;

class CGroupUse;

/**
 * @note The following definitions are taken from glib.  If you import gio/gio.h
 * you'll get these so you'll need to comment them out.  
 **/
typedef uint32_t GQuark;

struct _GError
{
  GQuark      domain;
  int         code;
  char       *message;
};
typedef enum
{
  G_BUS_TYPE_STARTER = -1,
  G_BUS_TYPE_NONE = 0,
  G_BUS_TYPE_SYSTEM  = 1,
  G_BUS_TYPE_SESSION = 2
} GBusType;
#define G_VARIANT_TYPE_ARRAY                ((const GVariantType *) "a*")
    

/**
 * @note End of duplicate gio.h definitions.
 **/

/**
 * @class CGroupConn  
 * @brief Every task (including subtasks) which need to access CGroups needs
 * to create an instance of this class which is passed into the CGroupUse
 * class.  Typically you create the CGroupConn and then immediately use it
 * with CGroupUse in the subtask.
 * @note This class does not do error reporting or debug logging.
 **/

class CGroupConn
{
private:
    friend class CGroupUse;
    /**
     * @enum CGroupErrors 
     * @brief If a method (in either this class or the attached CGroupUse 
     * class) returns -1, you must call set_error with one of these errors 
     * indicating the cause of the error.  Used in getErrorText.
     **/
    enum CGroupErrors {
        CERR_NO_ERROR = 0,
        CERR_GDERR,
        CERR_INSUFFICIENT_MEMORY,
        CERR_BAD_SYSTEMD,
        CERR_SYSTEM_ERROR
    };
    
    _GDBusConnection *m_conn;
    _GDBusProxy      *m_proxy;
    GError           *m_err;
    CGroupErrors      m_err_num;
    /** 
     * @note Stuff for glib that would be statically included if possible
     **/
    void             *m_glib20;
    void             *m_gio20;
    void             *m_gobject20;
    typedef void (*tg_clear_error)(GError       **err);    
    tg_clear_error m_g_clear_error;
    typedef GVariant *(*tg_variant_new)(const char          *format_string,
                                        ...);
    tg_variant_new m_g_variant_new;
    typedef GVariantBuilder *(*tg_variant_builder_new)(const GVariantType *type);
    tg_variant_builder_new m_g_variant_builder_new;
    typedef void (*tg_variant_builder_add_value)(GVariantBuilder      *builder,
                                                 GVariant             *value);
    tg_variant_builder_add_value m_g_variant_builder_add_value;
    typedef void (*tg_variant_builder_add)(GVariantBuilder      *builder,
                                           const GVariantType   *format_string,
                                           ...);
    tg_variant_builder_add m_g_variant_builder_add;
    
    typedef GDBusConnection *(*tg_bus_get_sync)(GBusType            bus_type,
                                                void               *cancellable,
                                                GError            **error);
    tg_bus_get_sync m_g_bus_get_sync;
    typedef GDBusProxy *(*tg_dbus_proxy_new_sync)(GDBusConnection     *connection,
                                                  int /*GDBusProxyFlags*/ flags,
                                                  GDBusInterfaceInfo *info,
                                                  const char         *name,
                                                  const char         *object_path,
                                                  const char         *interface_name,
                                                  void               *cancellable,
                                                  GError             **error);
    tg_dbus_proxy_new_sync m_g_dbus_proxy_new_sync;
    typedef GVariant *(*tg_dbus_proxy_call_sync)(GDBusProxy          *proxy,
                                                 const char          *method_name,
                                                 GVariant            *parameters,
                                                 int/*GDBusCallFlags*/ flags,
                                                 int                 timeout_msec,
                                                 void                *cancellable,
                                                 GError             **error);
    tg_dbus_proxy_call_sync m_g_dbus_proxy_call_sync;
    
    typedef void *(*tg_object_unref)(void * object);
    tg_object_unref m_g_object_unref;
    
    int load_gdb();
    /**
     * @note End of glib static include stuff
     **/
    void set_error(enum CGroupErrors err)
    {   m_err_num = err;   }
    void clear_err();
        
public:
    CGroupConn();
    ~CGroupConn();
    
    /**
     * @fn int create()
     * @brief Call after instantiation to perform the actual creation.  
     * @return 0 if success or -1 if it failed.  If it failed you can call
     * getErrorText to get the details.
     **/
    int create();
    /**
     * @fn char *getErrorText
     * @brief Call if a method returns -1 in either this class or the attached
     * CGroupUse class(s).
     * @return A pointer to a text description of the problem for error 
     * reporting or debugging.  
     **/
    char *getErrorText(); // For any functions returning -1

    /**
     * @fn int getErrorNum
     * @brief Call if a method returns -1 in either this class or the attached
     * CGroupUse class(s).
     * @return A number that might mean something to someone (actually one of 
     * the enumerated types).   
     **/
    int  getErrorNum(); // For any functions returning -1

    LS_NO_COPY_ASSIGN(CGroupConn);
};

#endif // _CGROUP_USE_H

