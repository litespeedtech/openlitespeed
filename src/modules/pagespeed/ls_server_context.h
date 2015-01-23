/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#ifndef LSI_SERVER_CONTEXT_H_
#define LSI_SERVER_CONTEXT_H_

#include "pagespeed.h"

#include "ls_message_handler.h"
#include "net/instaweb/automatic/public/proxy_fetch.h"
#include "net/instaweb/system/public/system_server_context.h"


namespace net_instaweb
{
    class LsiRewriteDriverFactory;
    class LsiRewriteOptions;
    class SystemRequestContext;

    class LsiServerContext : public SystemServerContext
    {
    public:
        LsiServerContext(
            LsiRewriteDriverFactory* factory, StringPiece hostname, int port );
        virtual ~LsiServerContext();

        // We expect to use ProxyFetch with HTML.
        virtual bool ProxiesHtml() const
        {
            return true;
        }

        LsiRewriteOptions* config();
        LsiRewriteDriverFactory* ls_rewrite_driver_factory()
        {
            return ls_factory_;
        }
        
        SystemRequestContext* NewRequestContext( lsi_session_t* session );

        LsiMessageHandler* lsi_message_handler()
        {
            return dynamic_cast<LsiMessageHandler*>( message_handler() );
        }
        
        virtual GoogleString FormatOption( StringPiece option_name, StringPiece args );
        
    private:
        LsiRewriteDriverFactory* ls_factory_;

    };

}  // namespace net_instaweb

#endif  // LSI_SERVER_CONTEXT_H_
