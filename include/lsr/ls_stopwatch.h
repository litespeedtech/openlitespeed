/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2023  LiteSpeed Technologies, Inc.                 *
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
#ifndef LS_STOPWATCH_H
#define LS_STOPWATCH_H

#include <stddef.h>

#include <lsdef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ls_stopwatch_s
{
    uint64_t tmStartMs;
    uint64_t tmStopMs;
};

typedef struct ls_stopwatch_s ls_stopwatch_t;

/** @ls_stopwatch_reset
 * @brief Zeroes the start and stop times.
 *
 * @param[in,out] pWatch - The watch to reset.
 */
ls_inline void ls_stopwatch_reset(ls_stopwatch_t *pWatch)
{
    pWatch->tmStartMs = 0;
    pWatch->tmStopMs = 0;
}

/** @ls_stopwatch_start
 * @brief Sets the start time.
 *
 * @param[in,out] pWatch - The watch to start.
 */
void ls_stopwatch_start(ls_stopwatch_t *pWatch);

/** @ls_stopwatch_stop
 * @brief Sets the stop time.
 *
 * @param[in,out] pWatch - The watch to stop.
 */
void ls_stopwatch_stop(ls_stopwatch_t *pWatch);

/** @ls_stopwatch_get_tm_start
 * @brief Zeroes the start and stop times.
 *
 * @param[in] pWatch - The watch.
 */
ls_inline long ls_stopwatch_get_tm_start(const ls_stopwatch_t *pWatch)
{
    return pWatch->tmStartMs;
}

/** @ls_stopwatch_get_tm_stop
 * @brief Zeroes the start and stop times.
 *
 * @param[in] pWatch - The watch.
 */
ls_inline long ls_stopwatch_get_tm_stop(const ls_stopwatch_t *pWatch)
{
    return pWatch->tmStopMs;
}

/** @ls_stopwatch_dup
 * @brief Copies the start and stop times from src into dst.
 *
 * @param[in,out] dst - The destination watch.
 * @param[in] src - The source watch.
 */
ls_inline void ls_stopwatch_dup(ls_stopwatch_t *dst, const ls_stopwatch_t *src)
{
    dst->tmStartMs = src->tmStartMs;
    dst->tmStopMs = src->tmStopMs;
}

/** @ls_stopwatch_get_tm_since_start
 * @brief Zeroes the start and stop times.
 *
 * @param[in] pWatch - The watch.
 */
long ls_stopwatch_get_tm_since_start(const ls_stopwatch_t *pWatch);

/** @ls_stopwatch_get_tm_dur
 * @brief Zeroes the start and stop times.
 *
 * @param[in] pWatch - The watch.
 */
ls_inline long ls_stopwatch_get_tm_dur(const ls_stopwatch_t *pWatch)
{
    long diff = pWatch->tmStopMs - pWatch->tmStartMs;
    return (diff > 0) ? diff : 0;
}

/** @ls_stopwatch_get_tm_dur_str
 * @brief Zeroes the start and stop times.
 *
 * @param[in] pWatch - The watch.
 */
size_t ls_stopwatch_get_tm_dur_str(const ls_stopwatch_t *pWatch, char *pBuf, size_t iBufLen, char bIncludeUnits);

#ifdef __cplusplus
}
#endif


#endif //LS_STOPWATCH_H
