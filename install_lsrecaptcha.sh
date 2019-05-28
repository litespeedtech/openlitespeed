#!/bin/sh

if [ -z "${GOROOT}" ]; then
    echo "NOTICE: The GOROOT environment variable MUST be set prior to calling this script."
    echo "The GOROOT environment variable should point to where Go is installed."
    exit 1
fi

OS=`uname -s`

if [ "x$OS" = "xFreeBSD" ]; then
    setenv GOPATH "${PWD}/src/modules/lsrecaptcha/go"
elif [ "x$OS" = "xLinux" ]; then
    export GOPATH="${PWD}/src/modules/lsrecaptcha/go"
else
    echo "unsupported OS ${OS}."
fi

go build lsrecaptcha
cp lsrecaptcha dist/lsrecaptcha/_recaptcha


