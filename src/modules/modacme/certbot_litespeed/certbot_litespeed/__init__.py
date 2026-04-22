"""Plugin for authenticating using LiteSpeed's mod_acme."""
# Modeled on Varnish plugin, but OLS authenticator is even simpler.
# Because OLS's mod_acme serves simply echos back the request string
# (similar to acme.sh's "stateless" mode [1]), the authenticator
# does not have to reload the server.  It is the administrator's job
# to have configured the server with the correct thumbprint beforehand.
#
# 1. https://github.com/acmesh-official/acme.sh/wiki/Stateless-Mode

import logging

import zope.interface

from acme import challenges

from certbot import interfaces
from certbot.plugins import common

L = logging.getLogger(__name__)

@zope.interface.implementer(interfaces.IAuthenticator)
@zope.interface.provider(interfaces.IPluginFactory)
class Authenticator(common.Plugin):
    """LiteSpeed Authenticator"""

    description = "Manual configuration, authentication via LiteSpeed"

    def prepare(self):  # pylint: disable=missing-docstring,no-self-use
        pass  # pragma: no cover

    def more_info(self):  # pylint: disable=missing-docstring,no-self-use
        return ""

    def get_chall_pref(self, domain):
        # pylint: disable=missing-docstring,no-self-use,unused-argument
        return [challenges.HTTP01]

    def perform(self, achalls):  # pylint: disable=missing-docstring
        responses = []
        for achall in achalls:
            responses.append(self._perform_single(achall))
        return responses

    def _perform_single(self, achall):
        response, _ = achall.response_and_validation()

        if response.simple_verify(
                achall.chall, achall.domain,
                achall.account_key.public_key(), self.config.http01_port):
            L.debug('self-verify of domain %s successful', achall.domain)
            return response

        L.error('self-verify of domain %s failed, authorization abandoned!',
                achall.domain)
        return None

    def cleanup(self, _):   # pylint: disable=no-self-use,missing-docstring
        L.debug('no changes were made, nothing to do')
