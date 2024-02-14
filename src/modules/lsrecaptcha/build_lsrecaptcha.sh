#!/bin/sh

if which go ; then
    echo "Go is installed, continue with installation."
else
    echo "Go is not yet installed. Please install Golang and try again."
    exit 1
fi

CUR_PATH=`dirname $0`
cd $CUR_PATH
CUR_PATH=`pwd`

if which setenv ; then
    setenv GOPATH "${CUR_PATH}"
else
    export GOPATH="${CUR_PATH}"
fi
export GO111MODULE=off
export CGO_ENABLED=1

echo "GOPATH set to ${GOPATH}"
go build lsrecaptcha

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    cp lsrecaptcha   ../../../dist/lsrecaptcha/_recaptcha
    exit 0
else
    echo "Build failed."
    exit 1
fi

