/****************************************************************************
*    Copyright (C) 2019  LiteSpeed Technologies, Inc.                       *
*    All rights reserved.                                                   *
****************************************************************************/
#ifndef _CGROUP_USE_H
#define _CGROUP_USE_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include "cgroupconn.h"

/**
 * @file cgroupuse.h
 */

/**
 * @class CGroupUse
 * @brief Create an instance of this class after you have done the fork and 
 * but are still root.  It applies the CGroup info by changing the slice the 
 * process runs in.
 * @note This class does not do error reporting or debug logging.
 **/

class CGroupUse 
{
private:
    friend class CGroupConn;
    CGroupConn *m_conn;
    int apply_slice();
    int child_validate(pid_t pid);
    int m_uid;
#define CUSE_UNIT_FORMAT "run-%u.scope"
    
public:
    /**
     * @fn CGroupUse
     * @brief Constructor, call with a pointer to the connection. 
     **/
    CGroupUse(CGroupConn *conn);
    ~CGroupUse();
    
    /**
     * @fn int validate(int uid)
     * @brief Call after the connection has been created but before using the
     * facility.  This tells you whether it will work or not.
     * @note Fork before creating the Connection and Use classes.  The child
     * should call the apply and validate.  The parent should wait for the
     * child's return.
     * @return 0 if success or -1 if it failed.  You cannot call getErrorText()
     * for details.  It works or not.
     * @note Must be called in a child of the main task.
     * @note Must be called AFTER the apply has been done.
     * @note VERY ugly code inside.  The forked child should not be used any
     * more after this call.
     **/
    int validate();
    /**
     * @fn int apply(int uid)
     * @brief Call after the fork running as root.                        
     * @param[in] uid: the UID to run the task as.  Must exist.
     * @return 0 if success or -1 if it failed.  If it failed you can call
     * getErrorText to get the details.
     **/
    int apply(int uid);
 
    LS_NO_COPY_ASSIGN(CGroupUse);
};

#endif
#endif // _CGROUP_USE_H

