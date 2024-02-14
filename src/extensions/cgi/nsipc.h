/*
 * Copyright 2002-2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NSIPC_H
#define _NSIPC_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file nsipc.h
 * @brief This module defines the IPC between the clone'd parent and child.
 */

enum nsipc_msgs
{
    NSIPC_PARENT_DONE,
    NSIPC_CHILD_STARTED,
    NSIPC_ERROR,
};

typedef struct nsipc_parent_done_s
{
    enum nsipc_msgs m_type;
} nsipc_parent_done_t;

typedef struct nsipc_child_started_s
{
    enum nsipc_msgs m_type;
    pid_t             m_pid;
} nsipc_child_started_t;

typedef struct nsipc_error_s
{
    enum nsipc_msgs m_type;
    int               m_rc;
} nsipc_error_t;

typedef union nsipc_u
{
    nsipc_parent_done_t   m_done;
    nsipc_child_started_t m_started;
    nsipc_error_t         m_error;
} nsipc_t;

#ifdef __cplusplus
}
#endif

#endif // Linux only

#endif // _NSIPC_H
