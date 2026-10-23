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

#include "multipartbodyguard.h"

#include <stdlib.h>
#include <string.h>


static const char s_targetName[] = "PHP_SESSION_UPLOAD_PROGRESS";
static const uint8_t s_targetFailure[] =
{
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0
};

#define TARGET_NAME_LEN ((int)(sizeof(s_targetName) - 1))
#define PHP_MULTIPART_BOUNDARY_MAX (5120 - 4)
#define PHP_MULTIPART_BUFFER_SIZE 5120


const int64_t MultipartBodyGuard::DEFAULT_SCAN_LIMIT = 10 * 1024 * 1024;
int8_t MultipartBodyGuard::s_blockEnabled = 1;
int64_t MultipartBodyGuard::s_scanLimit =
    MultipartBodyGuard::DEFAULT_SCAN_LIMIT;


void MultipartBodyGuard::setBlockEnabled(int enabled)
{
    s_blockEnabled = enabled != 0;
}


int MultipartBodyGuard::getBlockEnabled()
{
    return s_blockEnabled;
}


void MultipartBodyGuard::setScanLimit(int64_t limit)
{
    s_scanLimit = limit;
}


int64_t MultipartBodyGuard::getScanLimit()
{
    return s_scanLimit;
}


static inline char lowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}


void MultipartBodyGuard::clearBoundary()
{
    if (m_pDelim)
    {
        free(m_pDelim);
        m_pDelim = NULL;
    }
    m_delimLen = 0;
}


void MultipartBodyGuard::disable()
{
    clearBoundary();
    m_active = 0;
    m_invalidBoundary = 0;
}


void MultipartBodyGuard::resetLine()
{
    m_delimMatch = 0;
    m_lineDelimFull = 0;
    m_lineDelimBroken = 0;
    m_lineHasContent = 0;
    m_lineBytes = 0;
}


void MultipartBodyGuard::reset()
{
    clearBoundary();
    m_section = SEC_SCAN;
    m_active = 0;
    m_invalidBoundary = 1;
    m_unlimitedScan = 1;
    m_armed = 0;
    m_valueFirstLine = 0;
    m_sawCR = 0;
    m_phpLineLimit = PHP_MULTIPART_BUFFER_SIZE;
    m_scanRemaining = 0;
    m_signatureMatch = 0;
    resetLine();
}


// Extract the boundary the same way PHP's rfc1867 parser does: prefer the
// first lowercase "boundary", fall back to a case-insensitive search, use the
// first following '=', and terminate an unquoted value only at ',' or ';'. An
// unusable boundary leaves the guard in its conservative any-NUL fallback.
void MultipartBodyGuard::init(const char *pContentType, int ctLen,
                              uint64_t maxScanBytes)
{
    reset();
    m_active = 1;
    m_unlimitedScan = maxScanBytes == 0;
    m_scanRemaining = maxScanBytes;
    if (!pContentType || ctLen <= 0)
        return;

    const char *pEnd = pContentType + ctLen;
    const char *p = NULL;
    const char *pScan = pContentType;
    while (pEnd - pScan >= 8)
    {
        if (memcmp(pScan, "boundary", 8) == 0)
        {
            p = pScan;
            break;
        }
        ++pScan;
    }
    if (!p)
    {
        pScan = pContentType;
        while (pEnd - pScan >= 8)
        {
            int i = 0;
            while (i < 8 && lowerAscii(pScan[i]) == "boundary"[i])
                ++i;
            if (i == 8)
            {
                p = pScan;
                break;
            }
            ++pScan;
        }
    }
    if (!p)
        return;

    p += 8;
    while (p < pEnd && *p != '=')
        ++p;
    if (p == pEnd || ++p == pEnd)
        return;

    const char *pBoundary;
    const char *pBoundaryEnd;
    if (*p == '"')
    {
        pBoundary = ++p;
        while (p < pEnd && *p != '"')
            ++p;
        if (p == pEnd)
            return;
        pBoundaryEnd = p;
    }
    else
    {
        pBoundary = p;
        while (p < pEnd && *p != ',' && *p != ';')
            ++p;
        pBoundaryEnd = p;
    }

    int boundaryLen = (int)(pBoundaryEnd - pBoundary);
    if (boundaryLen <= 0 || boundaryLen > PHP_MULTIPART_BOUNDARY_MAX)
        return;

    m_delimLen = boundaryLen + 2;
    m_pDelim = (char *)malloc(m_delimLen);
    if (!m_pDelim)
    {
        m_delimLen = 0;
        return;
    }
    m_pDelim[0] = '-';
    m_pDelim[1] = '-';
    memcpy(m_pDelim + 2, pBoundary, boundaryLen);
    m_invalidBoundary = 0;
    m_phpLineLimit = m_delimLen + 4;
    if (m_phpLineLimit < PHP_MULTIPART_BUFFER_SIZE)
        m_phpLineLimit = PHP_MULTIPART_BUFFER_SIZE;
}


void MultipartBodyGuard::signatureByte(char c)
{
    // PHP's default quoted-string decoder only removes escapes before a
    // backslash or quote. The fixed ASCII target contains neither, so an exact
    // decoded default field name contains this literal byte sequence. Wide
    // encodings also encounter the header-NUL rejection before they evade it.
    while (m_signatureMatch
           && c != s_targetName[m_signatureMatch])
        m_signatureMatch = s_targetFailure[m_signatureMatch - 1];

    if (c == s_targetName[m_signatureMatch])
    {
        if (++m_signatureMatch == TARGET_NAME_LEN)
        {
            m_armed = 1;
            m_signatureMatch = s_targetFailure[TARGET_NAME_LEN - 1];
        }
    }
}


bool MultipartBodyGuard::countLineByte()
{
    if (++m_lineBytes < m_phpLineLimit)
        return false;

    // PHP's get_line() would split the physical line at this point, both in
    // headers and while find_boundary() searches for the next delimiter; the
    // split remainder can then become an exact delimiter only PHP sees.
    // Rejecting the unusual line keeps later framing from diverging.
    m_section = SEC_BLOCKED;
    return true;
}


void MultipartBodyGuard::processLineByte(char c)
{
    if (m_section == SEC_HEADER)
    {
        if (c == '\0')
        {
            m_section = SEC_BLOCKED;
            return;
        }
        signatureByte(c);
        m_lineHasContent = 1;
        return;
    }

    if (m_section == SEC_SCAN)
    {
        signatureByte(c);
        if (m_armed && c == '\0')
        {
            m_section = SEC_BLOCKED;
            return;
        }

        if (!m_lineDelimBroken && !m_lineDelimFull)
        {
            if (c == m_pDelim[m_delimMatch])
            {
                if (++m_delimMatch == m_delimLen)
                    m_lineDelimFull = 1;
                return;
            }
            m_lineDelimBroken = 1;
        }
        else if (m_lineDelimFull)
            m_lineDelimBroken = 1;

        m_lineHasContent = 1;
        return;
    }

    // The blank header line's LF is already consumed before PHP starts its
    // "\n--boundary" search, so the first value line cannot end the value.
    if (m_valueFirstLine || m_lineDelimBroken)
    {
        if (m_armed && c == '\0')
            m_section = SEC_BLOCKED;
        else
        {
            m_lineDelimBroken = 1;
            m_lineHasContent = 1;
        }
        return;
    }

    if (c == m_pDelim[m_delimMatch])
    {
        if (++m_delimMatch == m_delimLen)
        {
            // PHP ends the value as soon as the prefix matches. Only the rest
            // of the line determines whether a new header block follows.
            m_lineDelimFull = 1;
            m_section = SEC_SCAN;
            m_armed = 0;
            m_signatureMatch = 0;
            // find_boundary() reads this line from the delimiter onward.
            m_lineBytes = m_delimLen;
        }
        return;
    }

    m_lineDelimBroken = 1;
    if (m_armed && c == '\0')
        m_section = SEC_BLOCKED;
    else
        m_lineHasContent = 1;
}


void MultipartBodyGuard::finishLine()
{
    if (m_section == SEC_HEADER)
    {
        if (!m_lineHasContent)
        {
            m_section = SEC_VALUE;
            m_valueFirstLine = 1;
            m_signatureMatch = 0;
        }
    }
    else if (m_section == SEC_SCAN)
    {
        if (m_lineDelimFull && !m_lineDelimBroken)
            m_section = SEC_HEADER;
    }
    else if (m_section == SEC_VALUE)
        m_valueFirstLine = 0;

    m_sawCR = 0;
    resetLine();
}


bool MultipartBodyGuard::inspect(const char *pBuf, size_t len)
{
    if (m_section == SEC_BLOCKED)
        return true;
    if (!m_active || !pBuf || len == 0)
        return false;

    size_t scanLen = len;
    if (!m_unlimitedScan)
    {
        if (scanLen > m_scanRemaining)
            scanLen = (size_t)m_scanRemaining;
        m_scanRemaining -= scanLen;
    }

    const char *p = pBuf;
    const char *pEnd = pBuf + scanLen;

    if (m_invalidBoundary)
    {
        if (memchr(p, '\0', scanLen))
        {
            m_section = SEC_BLOCKED;
            return true;
        }
        if (!m_unlimitedScan && m_scanRemaining == 0)
            disable();
        return false;
    }

    while (p < pEnd)
    {
        // Most upload bytes are value bytes on a line already known not to be
        // a delimiter. Search directly for only the bytes that can matter.
        if (m_section == SEC_VALUE && !m_sawCR
            && (m_valueFirstLine || m_lineDelimBroken))
        {
            const char *pLf = (const char *)memchr(p, '\n', pEnd - p);
            const char *pNul = NULL;
            if (m_armed)
                pNul = (const char *)memchr(p, '\0',
                                            (pLf ? pLf : pEnd) - p);
            if (pNul && (!pLf || pNul < pLf))
            {
                m_section = SEC_BLOCKED;
                return true;
            }
            if (!pLf)
            {
                if (p < pEnd)
                {
                    m_lineDelimBroken = 1;
                    m_lineHasContent = 1;
                }
                p = pEnd;
                break;
            }

            if (pLf > p)
            {
                m_lineDelimBroken = 1;
                m_lineHasContent = 1;
            }
            p = pLf + 1;
            finishLine();
            continue;
        }

        char c = *p++;
        if (m_sawCR)
        {
            m_sawCR = 0;
            if (c == '\n')
            {
                finishLine();
                continue;
            }
            processLineByte('\r');
            if (m_section == SEC_BLOCKED)
                return true;
        }

        if (c == '\r')
        {
            if (m_section != SEC_VALUE && countLineByte())
                return true;
            m_sawCR = 1;
            continue;
        }
        if (c == '\n')
        {
            finishLine();
            continue;
        }

        if (m_section != SEC_VALUE && countLineByte())
            return true;
        processLineByte(c);
        if (m_section == SEC_BLOCKED)
            return true;
    }

    if (!m_unlimitedScan && m_scanRemaining == 0)
        disable();
    return false;
}
