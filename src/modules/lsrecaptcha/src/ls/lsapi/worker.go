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
	"io"
	"net"
	"net/http"
	"sync"
)

type worker struct {
	mutex    sync.Mutex
	conn     io.ReadWriteCloser
	p        packet //KEVIN: I am not sure that this needs to be saved.
	bodyLeft int64
	handler  http.Handler
	resp     *response

	// TODO: fcgi has a mutex and a map of request objects.
}

func newWorker(c net.Conn, handler http.Handler) *worker {
	return &worker{
		conn:    c,
		handler: handler,
		resp:    newResponse(c),
	}
}

func (self *worker) Close() error {
	self.mutex.Lock()
	defer self.mutex.Unlock()
	return self.conn.Close()
}

func (self *worker) Read(p []byte) (int, error) {
	if len(p) == 0 {
		return 0, nil
	} else if self.bodyLeft <= 0 {
		return 0, io.EOF
	}

	off := 0

	if self.p.HasBufferedPayload() {
		bytesRead := self.p.ReadBufferedPayload(p)
		self.bodyLeft -= int64(bytesRead)
		if self.bodyLeft == 0 || bytesRead >= len(p) {
			return bytesRead, nil
		}
		off = bytesRead
	}

	bytesRead, err := self.conn.Read(p[off:])
	self.bodyLeft -= int64(bytesRead)
	return off + bytesRead, err
}

func (self *worker) getRequest() (*http.Request, error) {

	self.p = packet{}
	if err := self.p.read(self.conn); err != nil {
		return nil, err
	}
	var err error
	http_req := &http.Request{}
	if http_req, err = self.p.handleNewRequest(http_req); err != nil {
		return nil, err
	}
	http_req.Body = self
	self.bodyLeft = http_req.ContentLength
	return http_req, nil
}

func (self *worker) cleanUp() {
	workerDone()
	self.Close()
}

func (self *worker) serve() {
	defer self.cleanUp()

	Debugf("In worker serve, worker: %p.\n", self)
	if envAcceptNotify != 0 {
		if err := sendPacketHeader(self.conn, lsapiReqReceived); err != nil {
			return
		}
	}
	for IsRunning() {
		Debugf("is running, wait for request, worker: %p.\n", self)
		// If that is -1, we exit the loop.
		http_req, err := self.getRequest()
		if err != nil {
			Debugf("get request returned worker: %p, err %+v\n", self, err)
			return
		}
		Debugf("serve request, worker: %p\n", self)
		if err := self.serveRequest(http_req); err != nil {
			return
		}

	}
	Debugf("Exit serve, worker: %p\n", self)
}

func (self *worker) serveRequest(http_req *http.Request) error {
	self.handler.ServeHTTP(self.resp, http_req)
	self.resp.finish()

	return nil
}
