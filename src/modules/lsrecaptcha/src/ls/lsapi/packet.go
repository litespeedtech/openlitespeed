package lsapi

/*
#cgo CFLAGS: -g3 -O0
#cgo freebsd CFLAGS: -Wno-unused-value
#cgo linux CFLAGS: -Wno-unused-result
#cgo linux LDFLAGS: -ldl
#include "lsgo.h"
#include "lsapidef.h"
*/
import "C"

import (
	"bytes"
	"context"
	"encoding/binary"
	"errors"
	"io"
	"net/http"
	"net/url"
	"strconv"
	"unsafe"
)

type packetHeader struct {
	versionB0 byte
	versionB1 byte
	pktType   packetType
	flag      byte
	pktLen    int32
}

func (self *packetHeader) getFlag(f byte) byte {
	return self.flag & f
}

func (self *packetHeader) flagIsSet(f byte) bool {
	return self.getFlag(f) != 0
}

func (self *packetHeader) verify(t packetType, r io.Reader, b []byte) (int, binary.ByteOrder, error) {
	self.versionB0 = b[0]
	self.versionB1 = b[1]
	self.pktType = packetType(b[2])
	self.flag = b[3]
	if self.versionB0 != 'L' || self.versionB1 != 'S' {
		return 0, nil, errors.New("lsapi: invalid version")
	} else if self.pktType != t {
		return 0, nil, errors.New("lsapi: invalid packet type")
	}

	var packetEndian binary.ByteOrder
	packetEndianIsBig := self.flagIsSet(lsapiEndianBit)
	if (lsapiEndian == binary.BigEndian) == packetEndianIsBig {
		packetEndian = lsapiEndian
	} else if packetEndianIsBig {
		packetEndian = binary.BigEndian
	} else {
		packetEndian = binary.LittleEndian
	}
	self.pktLen = int32(packetEndian.Uint32(b[4:]))
	return int(self.pktLen), packetEndian, nil
}

type packetRequestHeader struct {
	httpHeaderLen     int32
	reqBodyLen        int32
	scriptFileOff     int32
	scriptNameOff     int32
	queryStringOff    int32
	requestMethodOff  int32
	cntUnknownHeaders int32
	cntEnv            int32
	cntSpecialEnv     int32
}

func unsafeGetNextInt32(b []byte, order binary.ByteOrder, index int) int32 {
	j := index + 1
	return int32(order.Uint32(b[sizeofInt32*index : sizeofInt32*j]))
}

func unsafeGetNextInt16(b []byte, order binary.ByteOrder, index int) int16 {
	j := index + 1
	return int16(order.Uint16(b[sizeofInt16*index : sizeofInt16*j]))
}

func (self *packetRequestHeader) validateHeader(payload []byte, order binary.ByteOrder) error {
	totalSize := int32(len(payload))
	b := payload[sizeofPacketHeader:]

	i := 0
	self.httpHeaderLen = unsafeGetNextInt32(b, order, i)
	i++
	self.reqBodyLen = unsafeGetNextInt32(b, order, i)
	i++
	self.scriptFileOff = unsafeGetNextInt32(b, order, i)
	i++
	self.scriptNameOff = unsafeGetNextInt32(b, order, i)
	i++
	self.queryStringOff = unsafeGetNextInt32(b, order, i)
	i++
	self.requestMethodOff = unsafeGetNextInt32(b, order, i)
	i++
	self.cntUnknownHeaders = unsafeGetNextInt32(b, order, i)
	i++
	self.cntEnv = unsafeGetNextInt32(b, order, i)
	i++
	self.cntSpecialEnv = unsafeGetNextInt32(b, order, i)

	if self.scriptFileOff < 0 ||
		self.scriptFileOff >= totalSize ||
		self.scriptNameOff < 0 ||
		self.scriptNameOff >= totalSize ||
		self.queryStringOff < 0 ||
		self.queryStringOff >= totalSize ||
		self.requestMethodOff < 0 ||
		self.requestMethodOff >= totalSize {
		return errors.New("lsapi: offset greater than length")
	}
	return nil
}

type packet struct {
	header      packetHeader
	reqHeader   packetRequestHeader
	readLen     int
	consumedLen int
	payload     [lsapiMaxPacketLen]byte
}

func (self *packet) HasBufferedPayload() bool {
	return (self.readLen > self.consumedLen)
}

func (self *packet) ReadBufferedPayload(p []byte) int {
	n := copy(p, self.payload[self.consumedLen:self.readLen])
	self.consumedLen += n
	return n
}

func (self *packet) read(r io.Reader) (err error) {
	b := self.payload[:sizeofPacketHeader]
	// Read in a packet header
	self.readLen, err = io.ReadAtLeast(r, self.payload[:], sizeofPacketHeader)
	if err != nil {
		return err
	}
	var packetEndian binary.ByteOrder
	self.consumedLen, packetEndian, err = self.header.verify(lsapiBeginRequest, r, b)
	if err != nil {
		return err
	}
	if self.consumedLen < 0 {
		return errors.New("lsapi: payload len was negative.")
	} else if self.consumedLen > C.LSAPI_MAX_HEADER_LEN {
		return errors.New("lsapi: payload len too long.")
	}
	if self.readLen < self.consumedLen {
		// Read the rest of the payload.
		var n int
		if n, err = io.ReadAtLeast(r, self.payload[self.readLen:],
			self.consumedLen-self.readLen); err != nil {
			return err
		}
		self.readLen += n
	}
	if err = self.reqHeader.validateHeader(self.payload[:], packetEndian); err != nil {
		return err
	}
	return nil
}

func parseEnv(b []byte, cnt int32) (map[string]string, int, error) {
	var m map[string]string
	initLen := len(b)
	for i := int32(0); i < cnt; i++ {
		if len(b) < 4 {
			return nil, -1, errors.New("lsapi: cannot extract env, too few bytes")
		}
		keyLen := (int(b[0]) << 8) + int(b[1])
		valLen := (int(b[2]) << 8) + int(b[3])
		b = b[4:]
		if int(keyLen+valLen) > len(b) {
			return nil, -1, errors.New("lsapi: key len and val len are too long.")
		} else if keyLen == 0 || valLen == 0 {
			return nil, -1, errors.New("lsapi: key len or val len is zero.")
		}
		key := string(b[:keyLen-1])
		b = b[keyLen:]
		val := string(b[:valLen-1])
		b = b[valLen:]
		if m == nil {
			m = make(map[string]string)
		}
		m[key] = val
	}

	if bytes.Compare(b[0:4], []byte("\000\000\000\000")) != 0 {
		return nil, -1, errors.New("lsapi: missing spacer")
	}
	return m, (initLen - len(b[4:])), nil

}

// The following two functions are direct copies from the golang math/bits package.
// The latest Golang version that FreeBSD 9.3 supports is golang1.7.4.
// The math/bits package was not yet introduced in that version.

// ReverseBytes16 returns the value of x with its bytes in reversed order.
func reverseBytes16(x uint16) uint16 {
	return x>>8 | x<<8
}

// ReverseBytes32 returns the value of x with its bytes in reversed order.
func reverseBytes32(x uint32) uint32 {
	const m3 = 0x00ff00ff00ff00ff
	const m = 1<<32 - 1
	x = x>>8&(m3&m) | x&(m3&m)<<8
	return x>>16 | x<<16
}

func maybeFix32(i C.int32_t, fix bool) int32 {
	if !fix {
		return int32(i)
	}
	return int32(reverseBytes32(uint32(i)))
}

// max, off, len
func validOffAndLen(m, o, l int32) bool {
	return (m < o+l)
}

func (self *packet) handleNewRequest(http_req *http.Request) (*http.Request, error) {

	var packetEndian binary.ByteOrder
	initLen := int(self.header.pktLen)

	if self.header.flagIsSet(lsapiEndianBit) {
		packetEndian = binary.BigEndian
	} else {
		packetEndian = binary.LittleEndian
	}
	fixEndian := (packetEndian != lsapiEndian)

	{
		b := self.payload[self.reqHeader.requestMethodOff:]
		end := bytes.IndexByte(b, '\000')
		http_req.Method = string(b[:end])
	}

	ptr := self.payload[sizeofPacketHeader+sizeofRequestHeader : initLen]
	var specialEnvs, envs map[string]string
	// First are the special environments.
	{
		var incr int
		var err error
		specialEnvs, incr, err = parseEnv(ptr, self.reqHeader.cntSpecialEnv)
		if err != nil {
			return nil, err
		}
		ptr = ptr[incr:]
	}

	// Next are the regular environments.
	{
		var incr int
		var err error
		envs, incr, err = parseEnv(ptr, self.reqHeader.cntEnv)
		if err != nil {
			return nil, err
		}
		ptr = ptr[incr:]

		if envs == nil {
			return nil, errors.New("lsapi: Required environments not set.")
		}
		uri, err := url.ParseRequestURI(envs["REQUEST_URI"])
		if err != nil {
			return nil, err
		}
		http_req.URL = uri

		http_req.Proto = envs["SERVER_PROTOCOL"]
		protoMajor, protoMinor, ok := http.ParseHTTPVersion(http_req.Proto)
		if !ok {
			return nil, errors.New("lsapi: failed to set server protocol")
		}
		http_req.ProtoMajor = protoMajor
		http_req.ProtoMinor = protoMinor

		http_req.RemoteAddr = envs["REMOTE_ADDR"] + ":" + envs["REMOTE_PORT"]
	}

	http_req.Header = http.Header{}

	// Offsets from beginning of payload.
	offHdrIdx := ((initLen - len(ptr) + 7) &^ 0x7)
	offUnknHdrList := offHdrIdx + C.sizeof_struct_lsapi_http_header_index
	offHdrs := offUnknHdrList + C.sizeof_struct_lsapi_header_offset*
		int(self.reqHeader.cntUnknownHeaders)

	hdrBegin := self.payload[offHdrs:initLen]

	// Max header offset + length
	offHdrEnd := int32(len(hdrBegin))

	if offHdrEnd != self.reqHeader.httpHeaderLen {
		return nil, errors.New("lsapi: Request header does not match header size.")
	}

	// Known headers
	{
		const hdrCnt = int(C.H_TRANSFER_ENCODING + 1)

		hdrIdxPtr := self.payload[offHdrIdx:]
		hdrIdx := C.lsgo_get_hdr_idx_ptr(C.uintptr_t(uintptr(unsafe.Pointer(&hdrIdxPtr[0]))))

		for i := 0; i < hdrCnt; i++ {
			hdrOff := int32(hdrIdx.m_headerOff[i])
			hdrLen := int16(hdrIdx.m_headerLen[i])
			if hdrOff != 0 {
				if fixEndian {
					hdrOff = int32(reverseBytes32(uint32(hdrOff)))
					hdrLen = int16(reverseBytes16(uint16(hdrLen)))
				}

				if hdrOff > offHdrEnd || hdrOff+int32(hdrLen) > offHdrEnd {
					return nil, errors.New("lsapi: known header length exceeded total.")
				}

				hdrVal := string(hdrBegin[hdrOff : hdrOff+int32(hdrLen)])
				http_req.Header.Add(httpHeaders[i], hdrVal)
				if i == int(C.H_HOST) {
					http_req.Host = hdrVal
				}
			}
		}
	}

	{
		if self.reqHeader.reqBodyLen == -2 {
			length := http_req.Header.Get("Content-Length")
			if len_int64, err := strconv.ParseInt(length, 0, 64); err == nil {
				http_req.ContentLength = len_int64
			} else {
				http_req.ContentLength = -1
			}
		} else {
			http_req.ContentLength = int64(self.reqHeader.reqBodyLen)
		}
	}

	// Unknown headers
	{
		// If offsets are equal, no unknown headers.
		for ; offUnknHdrList < offHdrs; offUnknHdrList += C.sizeof_struct_lsapi_header_offset {
			unknHdr := C.lsgo_get_unkn_hdr_list_ptr(C.uintptr_t(
				uintptr(unsafe.Pointer(&self.payload[offUnknHdrList]))))
			nameOff := maybeFix32(unknHdr.nameOff, fixEndian)
			nameLen := maybeFix32(unknHdr.nameLen, fixEndian)
			valOff := maybeFix32(unknHdr.valueOff, fixEndian)
			valLen := maybeFix32(unknHdr.valueLen, fixEndian)

			if nameOff > offHdrEnd || nameOff+nameLen > offHdrEnd ||
				valOff > offHdrEnd || valOff+valLen > offHdrEnd {
				return nil, errors.New("lsapi: unknown header length exceeded total.")
			}

			hdrName := string(hdrBegin[nameOff : nameOff+int32(nameLen)])
			hdrVal := string(hdrBegin[valOff : valOff+int32(valLen)])
			http_req.Header.Add(hdrName, hdrVal)
		}
	}

	for k, v := range specialEnvs {
		envs[k] = v
	}

	ctx := context.WithValue(http_req.Context(), keyLsapiReqEnv, envs)
	http_req = http_req.WithContext(ctx)

	return http_req, nil
}
