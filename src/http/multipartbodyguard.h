/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/

#ifndef MULTIPARTBODYGUARD_H
#define MULTIPARTBODYGUARD_H

#include <stddef.h>
#include <stdint.h>


// Conservative streaming guard against the PHP upload-progress NUL attack.
// It deliberately does not parse Content-Disposition. Seeing the literal
// PHP_SESSION_UPLOAD_PROGRESS signature in a part header (or while multipart
// framing is uncertain) arms NUL detection until PHP's body delimiter prefix
// is certain to have ended that value. This can reject unusual false positives
// but keeps the security decision independent of MIME parameter syntax.
//
// The only PHP multipart behavior mirrored here is framing: exact normal
// delimiter lines start headers, header parsing ignores delimiter-looking
// lines, and a value ends at "\n--boundary" except on its first line. Header
// NULs and header or delimiter-search lines reaching PHP's input-buffer size
// are rejected so PHP's line splitting cannot make the parsers disagree about
// where headers start or end.
class MultipartBodyGuard
{
public:
    static const int64_t DEFAULT_SCAN_LIMIT;

    static void setBlockEnabled(int enabled);
    static int getBlockEnabled();
    static void setScanLimit(int64_t limit);
    static int64_t getScanLimit();

    MultipartBodyGuard()
        : m_pDelim(NULL)
    {   reset();   }

    ~MultipartBodyGuard()
    {   clearBoundary();   }

    // maxScanBytes == 0 means unlimited. Otherwise reaching the byte budget
    // disables inspection and lets the request continue. If no usable
    // boundary can be extracted, inspection falls back to blocking any NUL
    // within the same budget.
    void init(const char *pContentType, int ctLen, uint64_t maxScanBytes = 0);

    void reset();

    // Feed one decoded request body chunk. Returns true once it must be
    // blocked. Chunk boundaries may fall anywhere.
    bool inspect(const char *pBuf, size_t len);

    bool isBlocked() const      {   return m_section == SEC_BLOCKED;   }
    bool isEnabled() const      {   return m_active != 0;              }

private:
    static int8_t  s_blockEnabled;
    static int64_t s_scanLimit;

    enum
    {
        SEC_SCAN,           // preamble, epilogue, or uncertain framing
        SEC_HEADER,         // inside a part's header block
        SEC_VALUE,          // inside a part's value
        SEC_BLOCKED
    };

    void clearBoundary();
    void disable();
    void resetLine();
    void signatureByte(char c);
    void processLineByte(char c);
    void finishLine();
    bool countLineByte();

    char       *m_pDelim;       // "--" + boundary when extraction succeeded
    int         m_delimLen;
    int         m_phpLineLimit; // max(5120, boundary length + 6)
    int         m_lineBytes;
    uint64_t    m_scanRemaining;

    uint8_t     m_section;
    uint8_t     m_active;
    uint8_t     m_invalidBoundary;
    uint8_t     m_unlimitedScan;
    uint8_t     m_armed;
    uint8_t     m_valueFirstLine;
    uint8_t     m_sawCR;

    int         m_delimMatch;
    uint8_t     m_lineDelimFull;
    uint8_t     m_lineDelimBroken;
    uint8_t     m_lineHasContent;
    uint8_t     m_signatureMatch;
};

#endif
