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
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"
)

type logger struct {
	conn     io.Writer
	bytesbuf *bytes.Buffer
}

type fileLogger struct {
	lock     sync.Mutex
	filename string
	handle   *os.File
}

type debugHandler interface {
	printf(format string, v ...interface{})
	print(s string)
}
type debugEnabled struct{}
type debugDisabled struct{}

const (
	defaultLogFile string = "error_log"
	logRotateSize  int64  = 1000000
)

var log_handle *fileLogger

var dEnabled debugEnabled
var dDisabled debugDisabled

var dHandler debugHandler

func LogToFile(logfile string) error {
	if logfile == "" {
		logfile = defaultLogFile
	}
	var err error
	log_handle, err = createFileLogger(logfile)
	if err != nil {
		return err
	}
	log.SetOutput(log_handle)
	log.Printf("opened %s for logging\n", logfile)
	return nil
}

func closeLogFile() {
	if log_handle != nil {
		log_handle.close()
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

func createFileLogger(filename string) (*fileLogger, error) {
	w := &fileLogger{filename: filename}
	err := w.rotate()
	if err != nil {
		log.Printf("Log file not accessible %s\n", w.filename)
		w.filename = "/tmp/go_" + filepath.Base(w.filename) + "." + strconv.Itoa(os.Getpid())
		log.Printf("Try to write to %s\n", w.filename)
		err = w.rotate()
		if err != nil {
			return nil, err
		}
	}
	return w, nil
}

func (w *fileLogger) close() error {
	w.lock.Lock()
	defer w.lock.Unlock()
	err := w.handle.Close()
	w.handle = nil
	return err
}

// Write satisfies the io.Writer interface.
func (w *fileLogger) Write(output []byte) (int, error) {
	w.lock.Lock()
	defer w.lock.Unlock()
	fi, err := w.handle.Stat()
	if err != nil {
		return 0, err
	} else if fi.Size() > logRotateSize {
		err = w.rotate()
		if err != nil {
			w.handle = nil
			log.SetOutput(os.Stderr)
			log.Println("Failed to rotate log, revert to default.")
			return 0, err
		}
	}
	return w.handle.Write(output)
}

// Perform the actual act of rotating and reopening file.
func (w *fileLogger) rotate() (err error) {
	w.lock.Lock()
	defer w.lock.Unlock()

	// Close existing file if open
	if w.handle != nil {
		err = w.handle.Close()
		w.handle = nil
		if err != nil {
			return err
		}
	}
	// Rename dest file if it already exists
	_, err = os.Stat(w.filename)
	if err == nil {
		newName := w.filename + time.Now().Format(".2006_01_02")
		matches, err := filepath.Glob(newName + "*")
		if err != nil {
			return err
		}
		if len(matches) != 0 {
			newName += fmt.Sprintf(".%02d", len(matches))
		}
		//2019_07_11.03
		err = os.Rename(w.filename, newName)
		if err != nil {
			return err
		}
	}

	// Create a file.
	w.handle, err = os.Create(w.filename)
	if err != nil {
		return err
	}
	return nil
}

func Debugf(format string, v ...interface{}) {
	dHandler.printf(format, v)
}

func Debug(s string) {
	dHandler.print(s)
}

func toggleDebug() {
	if getDebugHandler() == &dDisabled {
		log.Println("Enable Debug Logging!")
		setDebugHandler(&dEnabled)
	} else {
		log.Println("Disable Debug Logging!")
		setDebugHandler(&dDisabled)
	}
}

func setDebugHandler(d debugHandler) {
	dHandler = d
}

func getDebugHandler() debugHandler {
	return dHandler
}

func (debugEnabled) printf(format string, v ...interface{}) {
	log.Printf("[DEBUG] "+format, v)
}

func (debugEnabled) print(s string) {
	log.Println("[DEBUG] " + s)
}

func (debugDisabled) printf(format string, v ...interface{}) {
	// Debug disabled, do not log.
}

func (debugDisabled) print(s string) {
	// Debug disabled, do not log.
}
