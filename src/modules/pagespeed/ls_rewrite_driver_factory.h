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
#ifndef LSI_REWRITE_DRIVER_FACTORY_H_
#define LSI_REWRITE_DRIVER_FACTORY_H_


#include <set>

#include "net/instaweb/system/public/system_rewrite_driver_factory.h"
#include "net/instaweb/util/public/md5_hasher.h"
#include "net/instaweb/util/public/scoped_ptr.h"
#include "net/instaweb/system/public/serf_url_async_fetcher.h"


// TODO(oschaaf): We should reparent ApacheRewriteDriverFactory and
// LsiRewriteDriverFactory to a new class OriginRewriteDriverFactory and factor
// out as much as possible.

namespace net_instaweb
{
    class LsiMessageHandler;
    class LsiRewriteOptions;
    class LsiServerContext;
    class BlockingFetcher;
    class SharedCircularBuffer;
    class SharedMemRefererStatistics;
    class SlowWorker;
    class Statistics;
    class SystemThreadSystem;

    class LsiRewriteDriverFactory : public SystemRewriteDriverFactory
    {
    public:
        explicit LsiRewriteDriverFactory(
            const ProcessContext& process_context,
            SystemThreadSystem* system_thread_system, StringPiece hostname, int port );
        virtual ~LsiRewriteDriverFactory();
        virtual Hasher* NewHasher();
        virtual UrlAsyncFetcher* AllocateFetcher( SystemRewriteOptions* config );
        virtual MessageHandler* DefaultHtmlParseMessageHandler();
        virtual MessageHandler* DefaultMessageHandler();
        virtual FileSystem* DefaultFileSystem();
        virtual Timer* DefaultTimer();
        virtual NamedLockManager* DefaultLockManager();
        virtual RewriteOptions* NewRewriteOptions();
        virtual ServerContext* NewDecodingServerContext();
        bool InitLsiUrlAsyncFetchers();

        static void InitStats( Statistics* statistics );
        LsiServerContext* MakeLsiServerContext( StringPiece hostname, int port );
        virtual ServerContext* NewServerContext();

        void StartThreads();

        void SetServerContextMessageHandler( ServerContext* server_context );

        LsiMessageHandler* lsi_message_handler()
        {
            return lsi_message_handler_;
        }

        virtual void NonStaticInitStats( Statistics* statistics )
        {
            InitStats( statistics );
        }

        void set_main_conf( LsiRewriteOptions* main_conf )
        {
            main_conf_ = main_conf;
        }

        void LoggingInit();

        virtual void ShutDownMessageHandlers();

        virtual void SetCircularBuffer( SharedCircularBuffer* buffer );

    private:
        Timer* timer_;

        LsiRewriteOptions* main_conf_;

        bool threads_started_;
        LsiMessageHandler* lsi_message_handler_;
        LsiMessageHandler* lsi_html_parse_message_handler_;

        typedef std::set<LsiMessageHandler*> LsiMessageHandlerSet;
        LsiMessageHandlerSet server_context_message_handlers_;

        SharedCircularBuffer* lsi_shared_circular_buffer_;

        GoogleString hostname_;
        int port_;
    };

}  // namespace net_instaweb

#endif  // LSI_REWRITE_DRIVER_FACTORY_H_

