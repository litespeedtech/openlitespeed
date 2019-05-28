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

import (
	"bytes"
	"io"
	"log"
	"os"
)

type logger struct {
	conn     io.Writer
	bytesbuf *bytes.Buffer
}

const (
	defaultLogFile string = "error_log"
)

var log_handle *os.File

func LogToFile(logfile string) error {
	if logfile == "" {
		logfile = defaultLogFile
	}
	var err error
	log_handle, err = os.OpenFile(logfile, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0644)
	if err != nil {
		return err
	}
	log.SetOutput(log_handle)
	log.Printf("opened %s for logging\n", logfile)
	return nil
}

func closeLogFile() {
	if log_handle != nil {
		log_handle.Close()
		log_handle = nil
	}
}

func newLogger(c io.Writer) logger {
	l := logger{
		conn:     c,
		bytesbuf: &bytes.Buffer{},
	}
	l.bytesbuf.Grow(growIncrement)
	return l
}

func (self logger) reset() {
	self.bytesbuf.Reset()
}

func (self logger) Write(p []byte) (n int, err error) {

	const maxRespWriteLen int = lsapiRespBufSize - sizeofPacketHeader
	pLen := len(p)
	for len(p) != 0 {

		var pktHdr [sizeofPacketHeader]byte
		pktLen := len(p)
		if pktLen > maxRespWriteLen {
			pktLen = maxRespWriteLen
		}
		buildPacketHeader(pktHdr[:], lsapiStderrStream, int32(pktLen+sizeofPacketHeader))
		wl, e := self.bytesbuf.Write(pktHdr[:])
		if err := parseWriteRet(sizeofPacketHeader, wl, e); err != nil {
			return 0, err
		}

		wl, e = self.bytesbuf.Write(p[:pktLen])
		log.Printf("bufbytes contains %s", self.bytesbuf.Bytes())
		if err := parseWriteRet(pktLen, wl, e); err != nil {
			return 0, err
		}

		wl64, e := self.bytesbuf.WriteTo(self.conn)
		if err := parseWriteRet(self.bytesbuf.Len(), int(wl64), e); err != nil {
			return 0, err
		}

		p = p[pktLen:]
		self.bytesbuf.Reset()
	}

	return pLen, nil
}
