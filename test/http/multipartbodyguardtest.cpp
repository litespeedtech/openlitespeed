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
#ifdef RUN_TEST

#include <http/multipartbodyguard.h>
#include <http/httpreq.h>

#include "unittest-cpp/UnitTest++.h"

#include <string.h>
#include <string>


static const char s_ctype[] = "multipart/form-data; boundary=BOUND";


// Feed a body to a freshly armed guard, optionally split at every offset, and
// report whether it ended up blocked.
static bool runGuardWithType(const std::string &body, size_t split,
                             const char *contentType)
{
    MultipartBodyGuard guard;
    guard.init(contentType, strlen(contentType));
    bool blocked = false;
    if (split > body.size())
        split = body.size();
    blocked = guard.inspect(body.data(), split);
    if (!blocked)
        blocked = guard.inspect(body.data() + split, body.size() - split);
    return blocked;
}


static bool runGuard(const std::string &body, size_t split)
{
    return runGuardWithType(body, split, s_ctype);
}


static bool runGuardOneByte(const std::string &body)
{
    MultipartBodyGuard guard;
    guard.init(s_ctype, sizeof(s_ctype) - 1);
    for (size_t i = 0; i < body.size(); ++i)
        if (guard.inspect(body.data() + i, 1))
            return true;
    return false;
}


// Build one multipart part with the given Content-Disposition and raw value.
static std::string part(const std::string &disposition,
                        const std::string &value)
{
    std::string s = "--BOUND\r\n";
    s += "Content-Disposition: ";
    s += disposition;
    s += "\r\n\r\n";
    s += value;
    s += "\r\n";
    return s;
}

static const std::string s_end = "--BOUND--\r\n";


TEST(MultipartBodyGuard_blocksTargetFieldWithNul)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("abc\0def", 7)) + s_end;
    CHECK(runGuard(body, 0));
}


TEST(MultipartBodyGuard_matchesCaseInsensitiveBoundaryParameter)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("abc\0def", 7)) + s_end;
    CHECK(runGuardWithType(body, 0,
                           "multipart/form-data; BOUNDARY=BOUND"));
    CHECK(runGuardWithType(body, 0,
                           "multipart/form-data; boundary =BOUND"));
    CHECK(runGuardWithType(body, 0,
                           "multipart/form-data; BOUNDARY=WRONG; "
                           "boundary=BOUND"));
}


TEST(MultipartBodyGuard_prefersLowercaseBoundaryLikePhp)
{
    std::string value = "PHP_SESSION_UPLOAD_PROGRESS";
    value.append(1, '\0');
    std::string body = part("form-data; name=\"other\"", value) + s_end;
    CHECK(!runGuardWithType(body, 0,
                            "multipart/form-data; BOUNDARY=WRONG; "
                            "boundary=BOUND"));
}


TEST(MultipartBodyGuard_allowsTargetFieldWithoutNul)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            "harmless value") + s_end;
    CHECK(!runGuard(body, 0));
}


// A binary upload routinely carries NUL bytes; it must not be blocked when it
// is not the upload-progress field.
TEST(MultipartBodyGuard_allowsBinaryUploadWithNul)
{
    std::string body = part(
        "form-data; name=\"file\"; filename=\"a.bin\"",
        std::string("\x00\x01\x02\x00img", 7)) + s_end;
    CHECK(!runGuard(body, 0));
}


// NUL in a different field must not trip the guard.
TEST(MultipartBodyGuard_allowsNulInOtherField)
{
    std::string body = part("form-data; name=\"other\"",
                            std::string("x\0y", 3))
                     + part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            "clean") + s_end;
    CHECK(!runGuard(body, 0));
}


// The signature is meaningful only in headers or uncertain framing. Ordinary
// field/file data containing the text must not arm the guard.
TEST(MultipartBodyGuard_doesNotArmOnSignatureInValue)
{
    std::string value = "PHP_SESSION_UPLOAD_PROGRESS";
    value.append(1, '\0');
    value += "tail";
    std::string body = part("form-data; name=\"other\"", value) + s_end;
    CHECK(!runGuard(body, 0));
    CHECK(!runGuardOneByte(body));
}


// Exercise the bulk body-scan path and ensure it still finds the next part.
TEST(MultipartBodyGuard_blocksTargetAfterLargeFile)
{
    std::string fileData(256 * 1024, 'x');
    std::string body = part(
        "form-data; name=\"file\"; filename=\"large.bin\"", fileData);
    body += part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                 std::string("a\0b", 3));
    body += s_end;
    CHECK(runGuard(body, 0));
}


// The field name split across folded header lines still reassembles to the
// target in PHP, so it must still be caught.
TEST(MultipartBodyGuard_catchesFoldedFieldName)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; name=\"PHP_SESSION_UPLOAD_\r\n"
        "PROGRESS\"\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// An unquoted field name terminated by the end of the headers is matched too.
TEST(MultipartBodyGuard_matchesUnquotedName)
{
    std::string body = part("form-data; name=PHP_SESSION_UPLOAD_PROGRESS",
                            std::string("p\0q", 3)) + s_end;
    CHECK(runGuard(body, 0));
}


// A new header terminates the preceding Content-Disposition value. PHP keeps
// the already parsed unquoted name, so the guard must do the same.
TEST(MultipartBodyGuard_matchesUnquotedNameBeforeAnotherHeader)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; name=PHP_SESSION_UPLOAD_PROGRESS\r\n"
        "X-Test: value\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// Boundary discovery requires an exact delimiter line. A longer header line
// is continuation text in PHP and must not reset the part parser.
TEST(MultipartBodyGuard_rejectsBoundaryPrefixInHeaders)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; "
        "name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n"
        "--BOUNDX\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// PHP ignores a closing delimiter while find_boundary() is looking for an
// exact normal delimiter and will parse a later part.
TEST(MultipartBodyGuard_scansPastClosingDelimiter)
{
    std::string body = "--BOUND--\r\n";
    body += part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                 std::string("a\0b", 3));
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


TEST(MultipartBodyGuard_scansPartAfterClosingDelimiter)
{
    std::string body = part("form-data; name=\"other\"", "clean") + s_end;
    body += part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                 std::string("a\0b", 3));
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// multipart_buffer_headers() treats an exact delimiter line as a colonless
// continuation, rather than starting a new part.
TEST(MultipartBodyGuard_treatsDelimiterInHeadersAsContinuation)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; "
        "name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n"
        "--BOUND\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// The LF ending the blank header line has already been consumed when PHP's
// body reader starts searching for "\n--boundary". A boundary-looking first
// value line is therefore data, not framing.
TEST(MultipartBodyGuard_keepsBoundaryAtValueStart)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; "
        "name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n\r\n"
        "--BOUND\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// PHP ends a value at a later line's boundary prefix even when that line is
// not an exact delimiter. NUL bytes after the prefix are no longer in the
// upload-progress value and should not be attributed to it.
TEST(MultipartBodyGuard_disarmsAtLaterBoundaryPrefix)
{
    std::string body = "--BOUND\r\n"
        "Content-Disposition: form-data; "
        "name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n\r\n"
        "clean\r\n--BOUNDX\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(!runGuard(body, 0));
    CHECK(!runGuardOneByte(body));
}


// PHP emits a logical line whenever its 5120-byte multipart buffer fills,
// even without LF. Reject the physical header line at that size instead of
// letting PHP and the guard disagree about the following bytes.
TEST(MultipartBodyGuard_blocksPhpSizedHeaderLine)
{
    static const char padHeader[] = "X-Pad: ";
    std::string body = "--BOUND\r\n";
    body += padHeader;
    body.append(5120 - (sizeof(padHeader) - 1), 'x');
    body += "Content-Disposition: form-data; "
            "name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// find_boundary() uses the same 5120-byte line split. A long boundary-prefix
// line can leave an exact delimiter fragment that only PHP sees, after which
// the guard would read the real target headers as a value line.
TEST(MultipartBodyGuard_blocksPhpSizedBoundarySearchLine)
{
    std::string body = part("form-data; name=\"a\"", "x");
    body += "--BOUND";
    body.append(5120 - 7, 'A');
    body += "--BOUND\r\n"
            "Content-Disposition: form-data; name=\"b\"\r\n\r\n"
            "--BOUND\r\n\r\n";
    body += part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                 std::string("a\0b", 3));
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));

    // A line whose CRLF still fits PHP's buffer is not split.
    std::string fits = part("form-data; name=\"a\"", "x");
    fits += "--BOUND";
    fits.append(5120 - 9, 'A');
    fits += "\r\n";
    fits += part("form-data; name=\"file\"", std::string("a\0b", 3));
    fits += s_end;
    CHECK(!runGuard(fits, 0));
    CHECK(!runGuardOneByte(fits));
}


TEST(MultipartBodyGuard_stopsAtConfiguredScanLimit)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("a\0b", 3)) + s_end;
    size_t nul = body.find('\0');

    MultipartBodyGuard guard;
    guard.init(s_ctype, sizeof(s_ctype) - 1, nul);
    CHECK(!guard.inspect(body.data(), body.size()));
    CHECK(!guard.isEnabled());

    MultipartBodyGuard catchesLastByte;
    catchesLastByte.init(s_ctype, sizeof(s_ctype) - 1, nul + 1);
    CHECK(catchesLastByte.inspect(body.data(), body.size()));
}


TEST(MultipartBodyGuard_defaultScanLimitIsTenMiB)
{
    CHECK_EQUAL((int64_t)(10 * 1024 * 1024),
                MultipartBodyGuard::DEFAULT_SCAN_LIMIT);
    CHECK_EQUAL(MultipartBodyGuard::DEFAULT_SCAN_LIMIT,
                MultipartBodyGuard::getScanLimit());
}


TEST(MultipartBodyGuard_requestUsesConfiguredScanLimit)
{
    int savedBlock = MultipartBodyGuard::getBlockEnabled();
    int64_t savedLimit = MultipartBodyGuard::getScanLimit();
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("a\0b", 3)) + s_end;

    MultipartBodyGuard::setBlockEnabled(1);
    MultipartBodyGuard::setScanLimit(body.find('\0'));
    HttpReq req;
    req.updateBodyType(s_ctype, sizeof(s_ctype) - 1);
    CHECK(!req.inspectMultipartBody(body.data(), body.size()));
    CHECK(!req.isMultipartBodyBlocked());

    MultipartBodyGuard::setScanLimit(savedLimit);
    MultipartBodyGuard::setBlockEnabled(savedBlock);
}


// The relaxed scanner intentionally arms on the literal signature anywhere
// in a header block. This trades a rare false positive for independence from
// Content-Disposition parsing details.
TEST(MultipartBodyGuard_armsOnSignatureInAnyHeader)
{
    std::string body = "--BOUND\r\n"
        "X-Meta: name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n\r\n";
    body += std::string("a\0b", 3);
    body += "\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


// A longer name containing the signature is an accepted false positive.
TEST(MultipartBodyGuard_armsOnSignatureInLongerName)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESSX\"",
                            std::string("a\0b", 3)) + s_end;
    CHECK(runGuard(body, 0));
}


TEST(MultipartBodyGuard_armsOnSignatureInFilename)
{
    std::string body = part(
        "form-data; name=\"file\"; "
        "filename=\"PHP_SESSION_UPLOAD_PROGRESS.bin\"",
        std::string("a\0b", 3)) + s_end;
    CHECK(runGuard(body, 0));
}


TEST(MultipartBodyGuard_blocksNulInHeaders)
{
    std::string body = "--BOUND\r\nX-Test: value";
    body.append(1, '\0');
    body += "tail\r\n\r\nclean\r\n";
    body += s_end;
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


TEST(MultipartBodyGuard_armsWhileFramingIsUncertain)
{
    std::string body = "odd PHP_SESSION_UPLOAD_PROGRESS preamble\r\n";
    body += std::string("a\0b", 3);
    CHECK(runGuard(body, 0));
    CHECK(runGuardOneByte(body));
}


TEST(MultipartBodyGuard_blocksAcrossEverySplit)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("12\0 34", 6)) + s_end;
    for (size_t split = 0; split <= body.size(); ++split)
        CHECK(runGuard(body, split));
}


TEST(MultipartBodyGuard_blocksWithOneByteChunks)
{
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("zz\0zz", 5)) + s_end;
    CHECK(runGuardOneByte(body));
}


// Without a usable boundary, conservatively scan for any NUL rather than
// silently disabling the protection.
TEST(MultipartBodyGuard_fallsBackWithoutBoundary)
{
    MultipartBodyGuard guard;
    guard.init("multipart/form-data", 19);
    CHECK(guard.isEnabled());
    CHECK(!guard.inspect("--BOUND\r\n", 9));
    CHECK(guard.inspect("x\0y", 3));
}


TEST(MultipartBodyGuard_boundaryFallbackHonorsScanLimit)
{
    MultipartBodyGuard guard;
    guard.init("multipart/form-data", 19, 2);
    CHECK(!guard.inspect("ab\0", 3));
    CHECK(!guard.isEnabled());
}


TEST(MultipartBodyGuard_runsOnlyForMultipartRequests)
{
    HttpReq req;
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("a\0b", 3)) + s_end;

    req.updateBodyType("application/octet-stream", 24);
    CHECK(!req.inspectMultipartBody(body.data(), body.size()));

    req.updateBodyType(s_ctype, sizeof(s_ctype) - 1);
    CHECK(req.inspectMultipartBody(body.data(), body.size()));

    req.reset(0);
    CHECK(!req.inspectMultipartBody(body.data(), body.size()));
}


// The feature can be turned off; default is on.
TEST(MultipartBodyGuard_honorsConfigToggle)
{
    int saved = MultipartBodyGuard::getBlockEnabled();
    std::string body = part("form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"",
                            std::string("a\0b", 3)) + s_end;

    MultipartBodyGuard::setBlockEnabled(0);
    HttpReq off;
    off.updateBodyType(s_ctype, sizeof(s_ctype) - 1);
    CHECK(!off.inspectMultipartBody(body.data(), body.size()));

    MultipartBodyGuard::setBlockEnabled(1);
    HttpReq on;
    on.updateBodyType(s_ctype, sizeof(s_ctype) - 1);
    CHECK(on.inspectMultipartBody(body.data(), body.size()));

    MultipartBodyGuard::setBlockEnabled(saved);
}

#endif
