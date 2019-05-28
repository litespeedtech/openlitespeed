Lsrecaptcha
========

Description
--------

The lsrecaptcha Go module is a custom Recaptcha processor.
The Recaptcha validation is still performed by Google. This executable
will check if the response is successful and if so, notify the web server
that the client successfully completed the Recaptcha challenge.

Compilation
--------

If desired, the module can be compiled manually. The only prerequisites
are that Golang is installed and the GOROOT environment variable is set.

After that, run `build_lsrecaptcha.sh`. This should generate a binary
named `_recaptcha`. This binary should be copied to `$SERVER_ROOT/lsrecaptcha/`
and installation is complete.

