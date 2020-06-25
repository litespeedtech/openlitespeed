
#ifndef LSXPACK_ERROR_CODE_H_v200
#define LSXPACK_ERROR_CODE_H_v200

#ifdef __cplusplus
extern "C" {
#endif

/**
 * When headers are processed, various errors may occur.  They are listed
 * in this enum.
 */
enum lsxpack_err_code
{
    /** header is good. */
    LSXPACK_OK = 0,
    /** Duplicate pseudo-header */
    LSXPACK_ERR_DUPLICATE_PSDO_HDR,
    /** Not all request pseudo-headers are present */
    LSXPACK_ERR_INCOMPL_REQ_PSDO_HDR,
    /** Unnecessary request pseudo-header present in the response */
    LSXPACK_ERR_UNNEC_REQ_PSDO_HDR,
    /** Prohibited header in request */
    LSXPACK_ERR_BAD_REQ_HEADER,
    /** Not all response pseudo-headers are present */
    LSXPACK_ERR_INCOMPL_RESP_PSDO_HDR,
    /** Unnecessary response pseudo-header present in the response. */
    LSXPACK_ERR_UNNEC_RESP_PSDO_HDR,
    /** Unknown pseudo-header */
    LSXPACK_ERR_UNKNOWN_PSDO_HDR,
    /** Uppercase letter in header */
    LSXPACK_ERR_UPPERCASE_HEADER,
    /** Misplaced pseudo-header */
    LSXPACK_ERR_MISPLACED_PSDO_HDR,
    /** Missing pseudo-header */
    LSXPACK_ERR_MISSING_PSDO_HDR,
    /** Header or headers are too large */
    LSXPACK_ERR_HEADERS_TOO_LARGE,
    /** Cannot allocate any more memory. */
    LSXPACK_ERR_NOMEM,
};

#ifdef __cplusplus
}
#endif

#endif //LSXPACK_ERROR_CODE_H_v200

