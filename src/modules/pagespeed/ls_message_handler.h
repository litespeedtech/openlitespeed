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
#ifndef LSI_MESSAGE_HANDLER_H_
#define LSI_MESSAGE_HANDLER_H_

#include "pagespeed.h"
#include <cstdarg>

#include "third_party/apr/src/include/apr_time.h"

#include "net/instaweb/util/public/basictypes.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/scoped_ptr.h"
#include "net/instaweb/util/public/string.h"
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb
{

    class AbstractMutex;
    class SharedCircularBuffer;
    class Timer;
    class Writer;

// Implementation of a message handler that uses log_error()
// logging to emit messages, with a fallback to GoogleMessageHandler
    class LsiMessageHandler : public GoogleMessageHandler
    {
    public:
        explicit LsiMessageHandler( AbstractMutex* mutex );

        // Installs a signal handler for common crash signals that tries to print
        // out a backtrace.
        static void InstallCrashHandler();

        // When LsiRewriteDriver instantiates the LsiMessageHandlers, the
        // SharedCircularBuffer and ngx_log_t are not available yet. These
        // will later be set in RootInit/Childinit
        // Messages logged before that will be passed on to handler_;
        void set_buffer( SharedCircularBuffer* buff );


        void SetPidString( const int64 pid )
        {
            pid_string_ = StrCat( "[", Integer64ToString( pid ), "]" );
        }
        // Dump contents of SharedCircularBuffer.
        bool Dump( Writer* writer );

    protected:
        virtual void MessageVImpl( MessageType type, const char* msg, va_list args );

        virtual void FileMessageVImpl( MessageType type, const char* filename,
                                       int line, const char* msg, va_list args );

    private:
        lsi_log_level GetLsiLogLevel( MessageType type );
        scoped_ptr<AbstractMutex> mutex_;
        GoogleString pid_string_;
        // handler_ is used as a fallback when we can not use log_errort
        // It's also used when calling Dump on the internal SharedCircularBuffer
        GoogleMessageHandler handler_;
        SharedCircularBuffer* buffer_;

    };

}  // namespace net_instaweb

#endif  // LSI_MESSAGE_HANDLER_H_


