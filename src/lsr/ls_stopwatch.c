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
#include <lsr/ls_stopwatch.h>

#include <lsr/ls_strtool.h>

#include <sys/time.h>

static long get_tm_now_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void ls_stopwatch_start(ls_stopwatch_t *pWatch)
{
    pWatch->tmStartMs = get_tm_now_ms();
}

void ls_stopwatch_stop(ls_stopwatch_t *pWatch)
{
    pWatch->tmStopMs = get_tm_now_ms();
}

long ls_stopwatch_get_tm_since_start(const ls_stopwatch_t *pWatch)
{
    long tmStart = ls_stopwatch_get_tm_start(pWatch);
    if (tmStart)
        return get_tm_now_ms() - tmStart;
    return 0;
}

size_t ls_stopwatch_get_tm_dur_str(const ls_stopwatch_t *pWatch, char *pBuf, size_t iBufLen, char bIncludeUnits)
{
    size_t len;
    const char *pUnits = "";
    if (bIncludeUnits)
    {
        pUnits = "ms";
    }
    len = ls_snprintf(pBuf, iBufLen, "%ld%s", ls_stopwatch_get_tm_dur(pWatch), pUnits);
    return len;
}
