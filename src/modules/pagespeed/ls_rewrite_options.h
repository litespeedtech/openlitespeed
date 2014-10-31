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
#ifndef LSI_REWRITE_OPTIONS_H_
#define LSI_REWRITE_OPTIONS_H_

#include "pagespeed.h"

#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/system/public/system_rewrite_options.h"

namespace net_instaweb
{
    class LsiRewriteDriverFactory;

    class LsiRewriteOptions : public SystemRewriteOptions
    {
    public:
        // See rewrite_options::Initialize and ::Terminate
        static void Initialize();
        static void Terminate();

        LsiRewriteOptions( const StringPiece& description,
                           ThreadSystem* thread_system );
        explicit LsiRewriteOptions( ThreadSystem* thread_system );
        virtual ~LsiRewriteOptions() { }

        // args is an array of n_args StringPieces together representing a directive.
        // For example:
        //   ["RewriteLevel", "PassThrough"]
        // or
        //   ["EnableFilters", "combine_css,extend_cache,rewrite_images"]
        // or
        //   ["ShardDomain", "example.com", "s1.example.com,s2.example.com"]
        // Apply the directive, returning LSI_CONF_OK on success or an error message
        // on failure.
        //
        // pool is a memory pool for allocating error strings.
        const char* ParseAndSetOptions(
            StringPiece* args, int n_args, MessageHandler* handler,
            LsiRewriteDriverFactory* driver_factory, OptionScope scope );

        // Make an identical copy of these options and return it.
        virtual LsiRewriteOptions* Clone() const;

        // Returns a suitably down cast version of 'instance' if it is an instance
        // of this class, NULL if not.
        static const LsiRewriteOptions* DynamicCast( const RewriteOptions* instance );
        static LsiRewriteOptions* DynamicCast( RewriteOptions* instance );

        const GoogleString& statistics_path() const
        {
            return statistics_path_.value();
        }
        const GoogleString& global_statistics_path() const
        {
            return global_statistics_path_.value();
        }
        const GoogleString& console_path() const
        {
            return console_path_.value();
        }
        const GoogleString& messages_path() const
        {
            return messages_path_.value();
        }
        const GoogleString& admin_path() const
        {
            return admin_path_.value();
        }
        const GoogleString& global_admin_path() const
        {
            return global_admin_path_.value();
        }

    private:
        // Helper methods for ParseAndSetOptions().  Each can:
        //  - return kOptionNameUnknown and not set msg:
        //    - directive not handled; continue on with other possible
        //      interpretations.
        //  - return kOptionOk and not set msg:
        //    - directive handled, all's well.
        //  - return kOptionValueInvalid and set msg:
        //    - directive handled with an error; return the error to the user.
        //
        // msg will be shown to the user on kOptionValueInvalid.  While it would be
        // nice to always use msg and never use the MessageHandler, some option
        // parsing code in RewriteOptions expects to write to a MessageHandler.  If
        // that happens we put a summary on msg so the user sees something, and the
        // detailed message goes to their log via handler.
        OptionSettingResult ParseAndSetOptions0(
            StringPiece directive, GoogleString* msg, MessageHandler* handler );

        virtual OptionSettingResult ParseAndSetOptionFromName1(
            StringPiece name, StringPiece arg,
            GoogleString* msg, MessageHandler* handler );

        // We may want to override 2- and 3-argument versions as well in the future,
        // but they are not needed yet.

        // Keeps the properties added by this subclass.  These are merged into
        // RewriteOptions::all_properties_ during Initialize().
        //
        // RewriteOptions uses static initialization to reduce memory usage and
        // construction time.  All LsiRewriteOptions instances will have the same
        // Properties, so we can build the list when we initialize the first one.
        static Properties* lsi_properties_;
        static void AddProperties();
        void Init();

        // Add an option to lsi_properties_
        template<class OptionClass>
        static void add_lsi_option( typename OptionClass::ValueType default_value,
                                    OptionClass LsiRewriteOptions::*offset,
                                    const char* id,
                                    StringPiece option_name,
                                    OptionScope scope,
                                    const char* help,
                                    bool safe_to_print )
        {
            AddProperty( default_value, offset, id, option_name, scope, help,
                        safe_to_print, lsi_properties_ );
        }

        Option<GoogleString> statistics_path_;
        Option<GoogleString> global_statistics_path_;
        Option<GoogleString> console_path_;
        Option<GoogleString> messages_path_;
        Option<GoogleString> admin_path_;
        Option<GoogleString> global_admin_path_;

        // Helper for ParseAndSetOptions.  Returns whether the two directives equal,
        // ignoring case.
        bool IsDirective( StringPiece config_directive, StringPiece compare_directive );

        // Returns a given option's scope.
        RewriteOptions::OptionScope GetOptionScope( StringPiece option_name );

    };

}  // namespace net_instaweb

#endif  // LSI_REWRITE_OPTIONS_H_

