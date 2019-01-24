/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include "sslengine.h"
#include <openssl/engine.h>

#include <string.h>

SslEngine::SslEngine()
{
}
SslEngine::~SslEngine()
{
}

int SslEngine::init(const char *pID)
{
#ifndef ENGINE_R_OPERATION_NOT_SUPPORTED
    /* Load all bundled ENGINEs into memory and make them visible */
    if (!pID)
        return 0;
    if ((strcasecmp(pID, "null") == 0) ||
        (strcasecmp(pID, "builtin") == 0))
        return 0;
    ENGINE_load_builtin_engines();
    if ((strcasecmp(pID, "auto") == 0) ||
        (strcasecmp(pID, "all") == 0))
    {
        ENGINE_register_all_complete();
        return 0;
    }
    ENGINE *e;
    int ret = 0;
    e = ENGINE_by_id(pID);
    if (!e)
        return -1;
    if (strcmp(pID, "chil") == 0)
        ENGINE_ctrl_cmd_string(e, "FORK_CHECK", "1", 0);
    if (!ENGINE_set_default(e, ENGINE_METHOD_ALL))
        ret = -1;
    /* Release the structural reference from ENGINE_by_id() */
    ENGINE_free(e);
    return ret;
#else
    return 0;
#endif
}

void SslEngine::shutdown()
{
#ifndef ENGINE_R_OPERATION_NOT_SUPPORTED
    ENGINE_cleanup();
#endif
}

