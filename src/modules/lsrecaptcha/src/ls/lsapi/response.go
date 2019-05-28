package lsapi

import (
	"bytes"
	"errors"
	"io"
	"log"
	"net/http"
	"strings"
)

const growIncrement int = 4096
const offSize int = sizeofPacketHeader + 2*sizeofInt32 +
	sizeofInt16*lsapiMaxRespHeaders

type response struct {
	conn          io.Writer
	header        http.Header
	headerFlushed bool
	bytesbuf      bytes.Buffer
	streamPktOff  int
	logger        *log.Logger
}

func newResponse(c io.Writer) *response {
	resp := &response{
		conn:          c,
		header:        http.Header{},
		headerFlushed: false,
		streamPktOff:  -1,
	}
	resp.bytesbuf.Grow(growIncrement)
	return resp
}

func (self *response) Header() http.Header {
	return self.header
}

func (self *response) appendHeaders(b *bytes.Buffer, lens []int16) (int, error) {
	cnt := 0
	for headerName, headerVals := range self.header {
		headerName = strings.TrimSpace(headerName)
		for _, headerVal := range headerVals {
			// + 1 for ':'
			headerVal = strings.TrimSpace(headerVal)
			if len(headerName)+len(headerVal)+1 > int(lsapiRespHttpHeaderMax) {
				return -1, errors.New("lsapi: response header length too long.")
			} else if cnt >= int(lsapiMaxRespHeaders) {
				return cnt, nil
			}
			nameAndVal := headerName + ":" + headerVal + "\000"
			l := len(nameAndVal)
			if l > b.Cap()-b.Len() {
				b.Grow(growIncrement)
			}
			lens[cnt] = int16(l)
			b.WriteString(nameAndVal)
			cnt++
		}
	}
	return cnt, nil
}

// Implementation notes on WriteHeader
//
// Headers with multiple values are separated to their own line.
// Headers are also expected to be trimmed of whitespace.
//
// Because of these issues, we cannot know the exact length of the headers,
// nor how many there are.
//
// When building the buffer, enough space is reserved (offSize, defined above)
// at the beginning to ensure that the packet header can fit for up to the max
// number of headers. Then the headers are written first, and the lengths
// stored in hdrLens. Once all the headers are added, the beginning is offset
// accordingly to ensure that the entire packet is contiguous.
//

func (self *response) WriteHeader(statusCode int) {
	if self.headerFlushed {
		log.Println("Warning: Response Headers already flushed.\n")
		return
	}

	self.headerFlushed = true

	// Default content type is required.
	if _, hasType := self.header["Content-Type"]; !hasType {
		self.header.Add("Content-Type", "text/html; charset=utf-8")
	}

	var writeOffset [offSize]byte
	written, err := self.bytesbuf.Write(writeOffset[:])
	if err != nil {
		return
	}
	if written != offSize {
		return
	}
	var hdrLens [lsapiMaxRespHeaders]int16
	curHdrCnt, err := self.appendHeaders(&self.bytesbuf, hdrLens[:])

	if curHdrCnt != lsapiMaxRespHeaders {
		// Did not write all the headers, need to offset the beginning by difference.
		self.bytesbuf.Next((lsapiMaxRespHeaders - curHdrCnt) * sizeofInt16)
		if hdrLens[curHdrCnt] != 0 {
			self.bytesbuf.Truncate(self.bytesbuf.Len() - int(hdrLens[curHdrCnt])) // Unfinished header.
		}
	}
	p := self.bytesbuf.Bytes()
	buildPacketHeader(p, lsapiRespHeader, int32(self.bytesbuf.Len()))
	off := sizeofPacketHeader
	lsapiEndian.PutUint32(p[off:], uint32(curHdrCnt))
	off += sizeofInt32
	lsapiEndian.PutUint32(p[off:], uint32(statusCode))
	off += sizeofInt32

	for i := 0; i < curHdrCnt; i++ {
		lsapiEndian.PutUint16(p[off:], uint16(hdrLens[i]))
		off += sizeofInt16
	}

	// ensure enough space for max resp buf
	if lsapiRespBufSize > self.bytesbuf.Cap()-self.bytesbuf.Len() {
		self.bytesbuf.Grow(lsapiRespBufSize)
	}
}

func parseWriteRet(expectedWritten, written int, err error) error {
	if err != nil {
		return err
	}
	if written != expectedWritten {
		return errors.New("Failed to write all of buffer")
	}
	return nil
}

func (self *response) flushBuf(finish bool) (int, error) {

	writeSize := self.bytesbuf.Len()

	if self.streamPktOff != -1 {
		b := self.bytesbuf.Bytes()
		pkt := b[self.streamPktOff : self.streamPktOff+sizeofPacketHeader]
		pktLen := int32(self.bytesbuf.Len() - self.streamPktOff)
		if finish {
			pktLen -= int32(sizeofPacketHeader)
		}
		buildPacketHeader(pkt, lsapiRespStream, pktLen)
	} else if !finish { // nothing to write
		return 0, nil
	}
	wl, e := self.bytesbuf.WriteTo(self.conn)
	if err := parseWriteRet(writeSize, int(wl), e); err != nil {
		return -1, err
	}
	self.streamPktOff = -1

	return writeSize, nil
}

func (self *response) reset() {
	self.header = http.Header{}
	self.headerFlushed = false
	self.bytesbuf.Reset()
	self.streamPktOff = -1
	if self.logger != nil {
		self.logger = nil
	}
}

func (self *response) finish() error {
	defer self.reset()
	spaceRemain := self.bytesbuf.Cap() - self.bytesbuf.Len()
	if spaceRemain < sizeofPacketHeader {
		self.flushBuf(false)
		return sendPacketHeader(self.conn, lsapiRespEnd)
	}
	var b [sizeofPacketHeader]byte
	buildPacketHeader(b[:], lsapiRespEnd, int32(sizeofPacketHeader))
	wl, e := self.bytesbuf.Write(b[:])
	if err := parseWriteRet(sizeofPacketHeader, wl, e); err != nil {
		return err
	}
	_, err := self.flushBuf(true)
	return err
}

func (self *response) Write(p []byte) (n int, err error) {
	// If headers not sent yet, send headers first.
	if !self.headerFlushed {
		self.WriteHeader(http.StatusOK)
	}

	const maxRespWriteLen int = lsapiRespBufSize - sizeofPacketHeader
	pLen := len(p)
	for len(p) != 0 {

		if self.bytesbuf.Len() >= lsapiRespBufSize {
			if self.streamPktOff == -1 {
				l := self.bytesbuf.Len()
				wl, e := self.bytesbuf.WriteTo(self.conn)
				if err := parseWriteRet(l, int(wl), e); err != nil {
					return 0, err
				}
			} else {
				_, err := self.flushBuf(false)
				if err != nil {
					return -1, err
				}
				self.streamPktOff = -1
			}
		}
		if self.streamPktOff == -1 {
			var writeOffset [sizeofPacketHeader]byte
			self.streamPktOff = self.bytesbuf.Len()
			wl, e := self.bytesbuf.Write(writeOffset[:])
			if err := parseWriteRet(sizeofPacketHeader, wl, e); err != nil {
				return 0, err
			}
		}

		writeLen := self.bytesbuf.Cap() - self.bytesbuf.Len()
		if writeLen > maxRespWriteLen {
			writeLen = maxRespWriteLen
		}
		if writeLen > len(p) {
			writeLen = len(p)
		}
		wl, e := self.bytesbuf.Write(p[:writeLen])
		if err := parseWriteRet(writeLen, wl, e); err != nil {
			return 0, err
		}
		p = p[writeLen:]
	}

	return pLen, nil
}
