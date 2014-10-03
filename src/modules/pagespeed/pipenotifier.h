/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#ifndef PIPENOTIFIER_H
#define PIPENOTIFIER_H

#include <../../edio/eventreactor.h>
class Multiplexer;

typedef  void ( * EVENT_CALLBACK )( void* );

class PipeNotifier :  public EventReactor
{
    int m_fdIn;
    EVENT_CALLBACK m_cb;
    void* session_;

public:
    PipeNotifier() : m_fdIn( -1 ), m_cb( NULL ) {};
    ~PipeNotifier();
    virtual int handleEvents( short int event );
    int initNotifier( Multiplexer* pMultiplexer, void* session );
    void notify();
    int getFdIn()
    {
        return m_fdIn;
    }
    void setCb( EVENT_CALLBACK cb )
    {
        m_cb = cb;
    }
    void uninitNotifier( Multiplexer* pMultiplexer );
};

#endif // PIPENOTIFIER_H

