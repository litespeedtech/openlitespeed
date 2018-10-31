/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef SSLENGINE_H
#define SSLENGINE_H


/**
  *@author George Wang
  */

class SslEngine
{
public:
    SslEngine();
    ~SslEngine();
    static int init(const char *pID);
    static void shutdown();
};

#endif
