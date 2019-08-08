//****************************************************************************
//   Open LiteSpeed is an open source HTTP server.                           *
//   Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
//                                                                           *
//   This program is free software: you can redistribute it and/or modify    *
//   it under the terms of the GNU General Public License as published by    *
//   the Free Software Foundation, either version 3 of the License, or       *
//   (at your option) any later version.                                     *
//                                                                           *
//   This program is distributed in the hope that it will be useful,         *
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
//   GNU General Public License for more details.                            *
//                                                                           *
//   You should have received a copy of the GNU General Public License       *
//   along with this program. If not, see http://www.gnu.org/licenses/.      *
//****************************************************************************

package lsapi

/*
#cgo CFLAGS: -g3 -O0
#cgo freebsd CFLAGS: -Wno-unused-value
#cgo linux CFLAGS: -Wno-unused-result
#cgo linux LDFLAGS: -ldl
#include "lsapidef.h"
#include "lsgo.h"
*/
import "C"

import (
	"encoding/binary"
	"errors"
	"io"
	"os"
	"strconv"
	"syscall"
)

type packetType uint8

type lsapiCtxKey int

const (
	lsapiBeginRequest  packetType = packetType(C.LSAPI_BEGIN_REQUEST)
	lsapiAbortRequest  packetType = packetType(C.LSAPI_ABORT_REQUEST)
	lsapiRespHeader    packetType = packetType(C.LSAPI_RESP_HEADER)
	lsapiRespStream    packetType = packetType(C.LSAPI_RESP_STREAM)
	lsapiRespEnd       packetType = packetType(C.LSAPI_RESP_END)
	lsapiStderrStream  packetType = packetType(C.LSAPI_STDERR_STREAM)
	lsapiReqReceived   packetType = packetType(C.LSAPI_REQ_RECEIVED)
	lsapiConnClose     packetType = packetType(C.LSAPI_CONN_CLOSE)
	lsapiInternalError packetType = packetType(C.LSAPI_INTERNAL_ERROR)

	lsapiEndianBit         byte = byte(C.LSAPI_ENDIAN_BIT)
	lsapiEndianLittle      byte = byte(C.LSAPI_ENDIAN_LITTLE)
	lsapiEndianBig         byte = byte(C.LSAPI_ENDIAN_BIG)
	lsapiMaxPacketLen      int  = int(C.LSAPI_MAX_DATA_PACKET_LEN)
	lsapiMaxRespHeaders    int  = int(C.LSAPI_MAX_RESP_HEADERS)
	lsapiRespHttpHeaderMax int  = int(C.LSAPI_RESP_HTTP_HEADER_MAX)
	lsapiRespBufSize       int  = 8192

	sizeofInt32         int = 4
	sizeofInt16         int = 2
	sizeofPacketHeader  int = 8
	sizeofRequestHeader int = 9 * sizeofInt32

	lsapiRecoverNetwork string = "LSAPI_RECOVER_NETWORK"
	lsapiRecoverAddr    string = "LSAPI_RECOVER_ADDR"
)

const (
	keyLsapiReqEnv lsapiCtxKey = iota
)

var httpHeaders = [...]string{
	"Accept", "Accept-Charset",
	"Accept-Encoding",
	"Accept-Language", "Authorization",
	"Connection", "Content-Type",
	"Content-Length", "Cookie", "Cookie2",
	"Host", "Pragma",
	"Referer", "User-Agent",
	"Cache-Control",
	"If-Modified-Since", "If-Match",
	"If-None-Match",
	"If-Range",
	"If-Unmodified-Since",
	"Keep-Alive",
	"Range",
	"X-Forwarded-For",
	"Via",
	"Transfer-Encoding",
}

var lsapiEndian binary.ByteOrder

var envLsapiOn int
var envMaxReqs int
var envAcceptNotify int
var envSlowReqMSecs int
var envMaxIdle int
var envKeepListen int
var envChildren int
var envExtraChildren int
var envMaxIdleChildren int
var envMaxProcessTime int
var envPpid int

var envRecoverNetwork string
var envRecoverAddr string

func setEndian() {
	if lsapiEndianBig == byte(C.LSAPI_ENDIAN) {
		lsapiEndian = binary.BigEndian
	} else {
		lsapiEndian = binary.LittleEndian
	}
}

// name and default
func envStrToInt(n string, d int) int {
	e, exist := os.LookupEnv(n)
	if !exist {
		return d
	}
	if e == "" {
		return d
	}
	ret, _ := strconv.Atoi(e)
	return ret
}

func initLsapiEnv() {

	envPpid = syscall.Getppid()

	if envLsapiOn = envStrToInt("LSAPI_ON", 1); envLsapiOn == 0 {
		// Not lsapi, no need to continue.
		return
	}

	setEndian()

	if noCheckPpid := envStrToInt("LSAPI_PPID_NO_CHECK", 0); noCheckPpid == 1 {
		envPpid = 0
	}

	envMaxReqs = envStrToInt("LSAPI_MAX_REQS", 10000)
	envAcceptNotify = envStrToInt("LSAPI_ACCEPT_NOTIFY", 0)
	envSlowReqMSecs = envStrToInt("LSAPI_SLOW_REQ_MSECS", 0)
	envMaxIdle = envStrToInt("LSAPI_MAX_IDLE", 300)
	envKeepListen = envStrToInt("LSAPI_KEEP_LISTEN", 0)
	envChildren = envStrToInt("LSAPI_CHILDREN", 35)
	envExtraChildren = envStrToInt("LSAPI_EXTRA_CHILDREN", 0)
	envMaxIdleChildren = envStrToInt("LSAPI_MAX_IDLE_CHILDREN", 1)
	envMaxProcessTime = envStrToInt("LSAPI_MAX_PROCESS_TIME", 3600)

	var exist bool
	envRecoverNetwork, exist = os.LookupEnv(lsapiRecoverNetwork)
	if !exist {
		envRecoverNetwork = ""
	}

	envRecoverAddr, exist = os.LookupEnv(lsapiRecoverAddr)
	if !exist {
		envRecoverAddr = ""
	}
}

func buildPacketHeader(b []byte, t packetType, l int32) {
	b[0] = 'L'
	b[1] = 'S'
	b[2] = byte(t)
	if lsapiEndian == binary.BigEndian {
		b[3] = lsapiEndianBig
	} else {
		b[3] = lsapiEndianLittle
	}
	lsapiEndian.PutUint32(b[4:], uint32(l))
}

func sendPacketHeader(w io.Writer, t packetType) error {
	var h [sizeofPacketHeader]byte
	buildPacketHeader(h[:], t, int32(sizeofPacketHeader))
	cnt, err := w.Write(h[:])
	if err != nil {
		return err
	}
	if cnt != sizeofPacketHeader {
		return errors.New("lsapi: failed to send whole packet.")
	}
	return nil
}
