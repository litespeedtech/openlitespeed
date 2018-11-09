/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef SSLERROR_H
#define SSLERROR_H


/**
  *@author George Wang
  */


class SslError
{
    int  m_iError;
    char m_achMsg[4096];
public:
    SslError() throw();
    SslError(int err) throw();
    SslError(const char *pErr) throw();
    ~SslError() throw();
    const char *what() const throw()
    {   return m_achMsg;  }
    //int get() const     {   return m_iError;    }
};

#endif
