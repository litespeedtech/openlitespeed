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
#include "ls_rewrite_driver_factory.h"

#include <cstdio>

#include "log_message_handler.h"
#include "ls_message_handler.h"
#include "ls_rewrite_options.h"
#include "ls_server_context.h"

#include "net/instaweb/http/public/content_type.h"
#include "net/instaweb/http/public/rate_controller.h"
#include "net/instaweb/http/public/rate_controlling_url_async_fetcher.h"
#include "net/instaweb/http/public/wget_url_fetcher.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_driver_factory.h"
#include "net/instaweb/rewriter/public/server_context.h"
#include "net/instaweb/system/public/in_place_resource_recorder.h"
#include "net/instaweb/system/public/serf_url_async_fetcher.h"
#include "net/instaweb/system/public/system_caches.h"
#include "net/instaweb/system/public/system_rewrite_options.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/null_shared_mem.h"
#include "net/instaweb/util/public/posix_timer.h"
#include "net/instaweb/util/public/property_cache.h"
#include "net/instaweb/util/public/scheduler_thread.h"
#include "net/instaweb/util/public/shared_circular_buffer.h"
#include "net/instaweb/util/public/shared_mem_statistics.h"
#include "net/instaweb/util/public/slow_worker.h"
#include "net/instaweb/util/public/stdio_file_system.h"
#include "net/instaweb/util/public/string.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/thread_system.h"
#include "pagespeed/kernel/thread/pthread_shared_mem.h"

namespace net_instaweb
{
    class FileSystem;
    class Hasher;
    class MessageHandler;
    class Statistics;
    class Timer;
    class UrlAsyncFetcher;
    class UrlFetcher;
    class Writer;

    class SharedCircularBuffer;

    LsiRewriteDriverFactory::LsiRewriteDriverFactory(
        const ProcessContext& process_context,
        SystemThreadSystem* system_thread_system, StringPiece hostname, int port )
        : SystemRewriteDriverFactory( process_context, system_thread_system,
                                      NULL /* default shared memory runtime */, hostname, port ),
        main_conf_( NULL ),
        threads_started_( false ),
        use_per_vhost_statistics_( false ),
        lsi_message_handler_( new LsiMessageHandler( thread_system()->NewMutex() ) ),
        lsi_html_parse_message_handler_(
            new LsiMessageHandler( thread_system()->NewMutex() ) ),
        install_crash_handler_( false ),
        lsi_shared_circular_buffer_( NULL ),
        hostname_( hostname.as_string() ),
        port_( port )
    {
        InitializeDefaultOptions();
        default_options()->set_beacon_url( "/ls_pagespeed_beacon" );
        SystemRewriteOptions* system_options = dynamic_cast<SystemRewriteOptions*>(
                default_options() );
        system_options->set_file_cache_clean_inode_limit( 500000 );
        system_options->set_avoid_renaming_introspective_javascript( true );
        set_message_handler( lsi_message_handler_ );
        set_html_parse_message_handler( lsi_html_parse_message_handler_ );
    }

    LsiRewriteDriverFactory::~LsiRewriteDriverFactory()
    {
        ShutDown();
        lsi_shared_circular_buffer_ = NULL;
        STLDeleteElements( &uninitialized_server_contexts_ );
    }

    Hasher* LsiRewriteDriverFactory::NewHasher()
    {
        return new MD5Hasher;
    }

    UrlAsyncFetcher* LsiRewriteDriverFactory::AllocateFetcher(
        SystemRewriteOptions* config )
    {
        return SystemRewriteDriverFactory::AllocateFetcher( config );
    }

    MessageHandler* LsiRewriteDriverFactory::DefaultHtmlParseMessageHandler()
    {
        return lsi_html_parse_message_handler_;
    }

    MessageHandler* LsiRewriteDriverFactory::DefaultMessageHandler()
    {
        return lsi_message_handler_;
    }

    FileSystem* LsiRewriteDriverFactory::DefaultFileSystem()
    {
        return new StdioFileSystem();
    }

    Timer* LsiRewriteDriverFactory::DefaultTimer()
    {
        return new PosixTimer;
    }

    NamedLockManager* LsiRewriteDriverFactory::DefaultLockManager()
    {
        CHECK( false );
        return NULL;
    }

    RewriteOptions* LsiRewriteDriverFactory::NewRewriteOptions()
    {
        LsiRewriteOptions* options = new LsiRewriteOptions( thread_system() );
        options->SetRewriteLevel( RewriteOptions::kCoreFilters );
        return options;
    }

    bool LsiRewriteDriverFactory::InitLsiUrlAsyncFetchers()
    {
        return true;
    }


    LsiServerContext* LsiRewriteDriverFactory::MakeLsiServerContext(
        StringPiece hostname, int port )
    {
        LsiServerContext* server_context = new LsiServerContext( this, hostname, port );
        uninitialized_server_contexts_.insert( server_context );
        return server_context;
    }

    ServerContext* LsiRewriteDriverFactory::NewDecodingServerContext()
    {
        ServerContext* sc = new LsiServerContext( this, hostname_, port_ );
        InitStubDecodingServerContext( sc );
        return sc;
    }

    ServerContext* LsiRewriteDriverFactory::NewServerContext()
    {
        LOG( DFATAL ) << "MakeLsiServerContext should be used instead";
        return NULL;
    }

    void LsiRewriteDriverFactory::ShutDownMessageHandlers()
    {
        lsi_message_handler_->set_buffer( NULL );
        lsi_html_parse_message_handler_->set_buffer( NULL );

        for( LsiMessageHandlerSet::iterator p =
                    server_context_message_handlers_.begin();
                p != server_context_message_handlers_.end(); ++p )
        {
            ( *p )->set_buffer( NULL );
        }

        server_context_message_handlers_.clear();
    }

    void LsiRewriteDriverFactory::StartThreads()
    {
        if( threads_started_ )
        {
            return;
        }

        SchedulerThread* thread = new SchedulerThread( thread_system(), scheduler() );
        bool ok = thread->Start();
        CHECK( ok ) << "Unable to start scheduler thread";
        defer_cleanup( thread->MakeDeleter() );
        threads_started_ = true;
    }

    void LsiRewriteDriverFactory::LoggingInit()
    {
        Install_log_message_handler();

        if( install_crash_handler_ )
        {
            LsiMessageHandler::InstallCrashHandler();
        }
    }

    void LsiRewriteDriverFactory::SetCircularBuffer(
        SharedCircularBuffer* buffer )
    {
        lsi_shared_circular_buffer_ = buffer;
        lsi_message_handler_->set_buffer( buffer );
        lsi_html_parse_message_handler_->set_buffer( buffer );
    }

    void LsiRewriteDriverFactory::SetServerContextMessageHandler(
        ServerContext* server_context )
    {
        LsiMessageHandler* handler = new LsiMessageHandler(
            thread_system()->NewMutex() );
        // The lsi_shared_circular_buffer_ will be NULL if MessageBufferSize hasn't
        // been raised from its default of 0.
        handler->set_buffer( lsi_shared_circular_buffer_ );
        server_context_message_handlers_.insert( handler );
        defer_cleanup( new Deleter<LsiMessageHandler> ( handler ) );
        server_context->set_message_handler( handler );
    }

    void LsiRewriteDriverFactory::InitStats( Statistics* statistics )
    {
        // Init standard PSOL stats.
        SystemRewriteDriverFactory::InitStats( statistics );
        RewriteDriverFactory::InitStats( statistics );
        RateController::InitStats( statistics );

        // Init Lsi-specific stats.
        LsiServerContext::InitStats( statistics );
        InPlaceResourceRecorder::InitStats( statistics );
    }

}  // namespace net_instaweb
