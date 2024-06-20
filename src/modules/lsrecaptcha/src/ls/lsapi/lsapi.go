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
#include "lsgo.h"
*/
import "C"

import (
	"context"
	"errors"
	"log"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"syscall"
	"time"
)

type DupListener interface {
	net.Listener
	File() (f *os.File, err error)
	SetDeadline(t time.Time) error
}

var wg sync.WaitGroup
var cnt_threads int = 0
var cond_threads = sync.NewCond(&sync.Mutex{})
var l DupListener

type lsapi struct {
	req       http.Request
	resp      *response
	lsapi_req C.lsgo_req_t
}

func Init(appDir, appName string) {
	setDebugHandler(&dDisabled)
	initLsapi()
	initSys(appDir, appName)
}

func ListenAndServe(addr string, handler http.Handler) error {
	initLsapiEnv()

	if envLsapiOn != 0 {
		return lsapiListenAndServe(addr, handler)
	} else {
		return http.ListenAndServe(addr, handler)
	}
}

func GetLogger(w http.ResponseWriter) *log.Logger {
	r := w.(*response)
	if r.logger == nil {
		lsLogger := newLogger(r.conn)
		r.logger = log.New(lsLogger, log.Prefix(), 0)
	}
	return r.logger
}

func GetReqEnv(c context.Context) map[string]string {
	return c.Value(keyLsapiReqEnv).(map[string]string)
}

func initFromAddress(address string) {
	initLsapi()
	dir, file := filepath.Split(address)
	name := strings.TrimSuffix(file, filepath.Ext(file))
	initSys(dir, name)
}

func initLsapi() {
	if os.Getuid() == 0 {
		log.Fatal("LSAPI MUST NOT RUN AS ROOT")
	}

	setRunningState(runStateRunning)
}

func isNetErrorBad(e error) bool {
	switch err := e.(type) {
	case *net.OpError:
		if err.Timeout() || err.Err == syscall.EINTR || err.Err == syscall.EAGAIN {
			return false
		}
	default:
	}
	return true
}

func checkForAutoStart() *os.File {
	var st syscall.Stat_t
	err := syscall.Fstat(0, &st)
	if err != nil {
		log.Println("Failed to stat FD 0, cannot use current address.")
		return nil
	}
	if st.Mode&syscall.S_IFSOCK == 0 {
		log.Println("FD 0 is not a sock, skip.")
		return nil
	}

	f := os.NewFile(0, "listener")
	if f == nil {
		log.Println("Failed to open listener fd.")
	}

	return f
}

func checkForGracefulRestart(network, address string) *os.File {

	log.Printf("Check for recover address/network. %+v %+v\n", network, address)
	if envRecoverAddr == "" && envRecoverNetwork == "" {
		log.Println("No recover address/network found.")
		return nil
	} else if envRecoverAddr != address || envRecoverNetwork != network {
		// fd invalid.
		log.Println("Recover address or network does not match. Do not use recover FD.")
		syscall.Close(5)
		return nil
	}

	// try to recover the socket.
	log.Println("Same address/network, Try to reuse FD.")
	f := os.NewFile(5, "listener")
	if f == nil {
		log.Println("Failed to open listener fd.")
	}

	return f
}

func getDupListenerFromFile(f *os.File) DupListener {

	ln, err := net.FileListener(f)
	if ln == nil {
		log.Printf("Failed to create file listener %v\n", err)
		return nil
	}
	dl, ok := ln.(DupListener)
	if !ok {
		log.Println("Listener created not correct type.")
		ln.Close()
		return nil
	}

	log.Println("Successfully recovered FD.")
	return dl
}

func tryRecoverListener(network, address string) DupListener {

	f := checkForAutoStart()

	if f == nil {
		f = checkForGracefulRestart(network, address)
	}

	if f == nil {
		return nil
	}

	defer f.Close()
	return getDupListenerFromFile(f)
}

func initListener(network, address string) (DupListener, error) {

	newListener := tryRecoverListener(network, address)
	if newListener != nil {
		return newListener, nil
	}
	log.Println("No recovered listener, create new one.")

	ln, err := net.Listen(network, address)
	if err != nil {
		return nil, err
	}
	return ln.(DupListener), nil
}

func getListenerFromAddr(addr string) (DupListener, error) {
	unixCmp := "uds:/"
	lowerCase := strings.ToLower(addr)
	var network string
	var address string
	var initParam string

	if strings.HasPrefix(lowerCase, unixCmp) {
		network = "unix"
		address = addr[len(unixCmp):]
		initParam = address
	} else {
		network = "tcp"
		address = addr
		initParam = ""
	}

	if !IsRunning() {
		initFromAddress(initParam)
	}

	return initListener(network, address)
}

func getListenerFromStream() DupListener {
	if !IsRunning() {
		initFromAddress("")
	}
	f := checkForAutoStart()

	if f == nil {
		return nil
	}

	defer f.Close()

	ln, err := net.FileListener(f)
	if ln == nil {
		log.Printf("Failed to create file listener %v\n", err)
		return nil
	}
	dl, ok := ln.(DupListener)
	if !ok {
		log.Println("Listener created not correct type.")
		ln.Close()
		return nil
	}

	log.Println("Successfully recovered FD.")
	return dl
}

func lsapiListenAndServe(addr string, handler http.Handler) error {
	var last_req_time int64
	if handler == nil {
		handler = http.DefaultServeMux
	}

	if addr == "" {
		l = getListenerFromStream()
		if l == nil {
			return errors.New("No address passed in, FD 0 is not a stream. Nothing to listen to.")
		}
	} else {
		var err error
		l, err = getListenerFromAddr(addr)
		if err != nil {
			return err
		}
	}
	defer l.Close()

	last_req_time = time.Now().Unix();

	for IsRunning() {
		l.SetDeadline(time.Now().Add(time.Second))

		if envPpid != 0 && envPpid != syscall.Getppid() {
			log.Println("parent pid changed, stop.")
			close(chStop)
			break
		}
		if (envPgrpMaxIdle > 0 &&
			time.Now().Unix() - last_req_time > int64(envPgrpMaxIdle)) {
			log.Println("Max idle time for process group reached, stop.")
			close(chStop)
			break
		}

		c, err := l.Accept()
		if err != nil {
			if isNetErrorBad(err) {
				return err
			} else {
				continue
			}
		}
		last_req_time = time.Now().Unix();
		w := newWorker(c, handler)
		Debugf("created worker %p, add one to wait group.\n", w)
		wg.Add(1)
		go w.serve()
		cond_threads.L.Lock()
		cnt_threads++
		Debugf("Now have %d workers.\n", cnt_threads)
		for cnt_threads >= envChildren {
			cond_threads.Wait()
		}
		cond_threads.L.Unlock()
	}
	log.Println("Stop listening, wait for threads to finish.")
	wg.Wait()
	log.Println("All threads done.")
	return nil
}

func workerDone() {
	cond_threads.L.Lock()
	cnt_threads--
	Debugf("workerDone, now have %d workers.\n", cnt_threads)
	cond_threads.L.Unlock()
	cond_threads.Signal()

	wg.Done()
}
