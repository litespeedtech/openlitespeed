/****************************************************************************
*    Copyright (C) 2019  LiteSpeed Technologies, Inc.                       *
*    All rights reserved.                                                   *
*    LiteSpeed Technologies Proprietary/Confidential.                       *
****************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include "cgroupuse.h"

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
CGroupUse::CGroupUse(CGroupConn *conn)
    : m_uid(0)
{
    CGroupUse::m_conn = conn;
}


CGroupUse::~CGroupUse()
{
}


int CGroupUse::apply_slice()
{
    // The functions below are supposed to be NULL safe.  That's why I test at
    // the end for NULL.
    GVariantBuilder *properties = m_conn->m_g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    GVariantBuilder *pids_array = m_conn->m_g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    GVariant *pvarpid = m_conn->m_g_variant_new("u", (unsigned int)getpid());
    m_conn->m_g_variant_builder_add_value(pids_array, pvarpid);
    GVariant *pvarparr = m_conn->m_g_variant_new("au", pids_array);
    m_conn->m_g_variant_builder_add(properties, "(sv)", "PIDs", pvarparr);
    char slice[256];
    snprintf(slice, sizeof(slice), "user-%u.slice", (unsigned int)m_uid);
    GVariant *pvarslice = m_conn->m_g_variant_new("s", slice);
    m_conn->m_g_variant_builder_add(properties, "(sv)", "Slice", pvarslice);
    char *fn = (char *)"StartTransientUnit";
    char unit[256];
    snprintf(unit, sizeof(unit), CUSE_UNIT_FORMAT, (unsigned int)getpid());
    GVariant *parms = m_conn->m_g_variant_new("(ssa(sv)a(sa(sv)))",
                                              unit,
                                              "fail",
                                              properties,
                                              NULL);
    if ((!properties) || (!pids_array) || (!pvarpid) || (!pvarparr) ||
        (!pvarslice) || (!parms))
    {
        // Note: These are supposed to be smart and deallocate when no longer
        // referenced.
        m_conn->set_error(CGroupConn::CERR_INSUFFICIENT_MEMORY);
        return -1;
    }
    m_conn->clear_err();
    m_conn->m_g_dbus_proxy_call_sync(m_conn->m_proxy,
                                     fn,
                                     parms,
                                     0,//G_DBUS_CALL_FLAGS_NONE,
                                     -1,   // Default timeout
                                     NULL, // GCancellable
                                     &m_conn->m_err);// userdata
    if (m_conn->m_err)
    {
        m_conn->set_error(CGroupConn::CERR_GDERR);
        return -1;
    }
    return 0;
}


int CGroupUse::apply(int uid)
{
    int rc;
    m_uid = uid;
    rc = apply_slice();
    return rc;
}


int CGroupUse::child_validate(pid_t pid)
{
    char proc_file[128];
    FILE *fd;

    snprintf(proc_file, sizeof(proc_file) - 1, "/proc/%d/cgroup", pid);
    fd = fopen(proc_file, (char *)"r");
    if (!fd)
        return (int)CGroupConn::CERR_SYSTEM_ERROR;
#define CUSE_LINE_LEN   256
    char line[CUSE_LINE_LEN];
    char line_hoped[CUSE_LINE_LEN];
    int  found = 0;
    int  compare_len;
    compare_len = snprintf(line_hoped, sizeof(line_hoped),
                           "1:name=systemd:/user.slice/user-%u.slice/", m_uid);
    while ((!found) && (fgets(line, sizeof(line) - 1, fd)))
    {
        if (!(strncmp(line, line_hoped, compare_len)))
            found = 1;
    }
    fclose(fd);
    if (found)
        return 0;
    return -1;
}


int CGroupUse::validate()
{
    m_conn->clear_err();
    struct passwd *pw = getpwuid(m_uid);
    if (pw)
    {
        setgid(pw->pw_gid);
        initgroups(pw->pw_name, pw->pw_gid);
    }
    seteuid(m_uid);
    setuid(m_uid);
    int pid = fork();
    if (pid == -1)
    {
        m_conn->set_error(CGroupConn::CERR_SYSTEM_ERROR);
        return -1;
    }
    if (pid == 0)
    {
        // child!
        exit(execl("/bin/sleep", "/bin/sleep", "60", NULL));
    }
    // Parent!
    int rc = child_validate(pid);
    kill(pid, 9);
    int session;
    waitpid(pid, &session, 0);
    return rc;
}

#endif
