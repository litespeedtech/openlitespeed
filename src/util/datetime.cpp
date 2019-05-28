/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "datetime.h"

#include <ctype.h>
#include <string.h>
#include <lsr/ls_atomic.h>
#include <lsr/ls_threadcheck.h>

time_t DateTime::s_curTime = time(NULL);
int    DateTime::s_curTimeUs = 0;


time_t DateTime::parseHttpTime(const char *s, int len)
{
    static const unsigned int daytab[2][13] =
    {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };
    unsigned sec, min, hour, day, mon, year;
    char month[3] = { 0, 0, 0 };
    enum { D_START, D_END, D_MON, D_DAY, D_YEAR, D_HOUR, D_MIN, D_SEC };

    sec = 60;
    min = 60;
    hour = 24;
    day = 0;
    year = 0;
    {
        char ch;
        unsigned n;
        char flag;
        char state;
        char type;
        const char *end = s + len;
        type = 0;
        state = D_START;
        n = 0;
        flag = 1;
        for (ch = *s; (ch && state != D_END && s < end); ch = *s++)
        {
            switch (state)
            {
            case D_START:
                if (' ' == ch)
                {
                    state = D_MON;
                    n = 0;
                    type = 3;
                }
                else if (ch == ',') state = D_DAY;
                break;
            case D_MON:
                if ((ch == ' ') && (n == 0))
                    ;
                else if (isalpha(ch))
                {
                    if (n < 3) month[n++] = ch;
                }
                else
                {
                    if (n < 3)
                        return 0;
                    if ((1 == type) && (' ' != ch))
                        return 0;
                    if ((2 == type) && ('-' != ch))
                        return 0;
                    flag = 1;
                    state = (type == 3) ? D_DAY : D_YEAR;
                    n = 0;
                }
                break;
            case D_DAY:
                if (ch == ' ' && flag)
                    ;
                else if (isdigit(ch))
                {
                    flag = 0;
                    n = 10 * n + (ch - '0');
                }
                else
                {
                    if ((' ' != ch) && ('-' != ch))
                        return 0;
                    if (!type)
                    {
                        if (ch == ' ')
                            type = 1;
                        else if (ch == '-')
                            type = 2;
                    }
                    day = n;
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_HOUR : D_MON;
                }
                break;
            case D_YEAR:
                if (ch == ' ' && flag)
                    ;
                else if (isdigit(ch))
                {
                    flag = 0 ;
                    year = 10 * year + (ch - '0');
                }
                else
                {
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_END : D_HOUR;
                }
                break;
            case D_HOUR:
                if ((' ' == ch) && flag)
                    ;
                else if (isdigit(ch))
                {
                    n = 10 * n + (ch - '0');
                    flag = 0;
                }
                else
                {
                    if (ch != ':')
                        return 0;
                    hour = n;
                    n = 0;
                    state = D_MIN;
                }
                break;
            case D_MIN:
                if (isdigit(ch))
                    n = 10 * n + (ch - '0');
                else
                {
                    if (ch != ':')
                        return 0;
                    min = n;
                    n = 0;
                    state = D_SEC;
                }
                break;
            case D_SEC:
                if (isdigit(ch))
                    n = 10 * n + (ch - '0');
                else
                {
                    if (ch != ' ')
                        return 0;
                    sec = n;
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_YEAR : D_END;
                }
                break;
            }
        }
        if ((state != D_END) && ((type != 3) || (state != D_YEAR)))
            return 0;
    }
    if (year <= 100)
        year += (year < 70) ? 2000 : 1900;
    if (sec >= 60 || min >= 60 || hour >= 24 || day == 0 || year < 1970)
        return 0;
    switch (month[0])
    {
    case 'A':
        mon = (month[1] == 'p') ? 4 : 8;
        break;
    case 'D':
        mon = 12;
        break;
    case 'F':
        mon = 2;
        break;
    case 'J':
        mon = (month[1] == 'a') ? 1 : ((month[2] == 'l') ? 7 : 6);
        break;
    case 'M':
        mon = (month[2] == 'r') ? 3 : 5;
        break;
    case 'N':
        mon = 11;
        break;
    case 'O':
        mon = 10;
        break;
    case 'S':
        mon = 9;
        break;
    default:
        return 0;
    }
    {
        const unsigned int *pDays = daytab[year % 4 == 0] + mon - 1;
        if (day > *(pDays + 1) - *pDays)
            return 0;
        --day;
        // leap day count is correct till DC 2100. so don't worry.
        return sec + 60L * (min + 60L * (hour + 24L * (
                                             day + *pDays +
                                             365L * (year - 1970L) + ((year - 1969L) >> 2))));
    }
}


char  *DateTime::getRFCTime(time_t t, char *buf)
{
    struct tm tmTime;
    if (gmtime_r(&t, &tmTime) != NULL)
    {
        strftime(buf, RFC_1123_TIME_LEN + 1, "%a, %d %b %Y %H:%M:%S GMT", &tmTime);
        return buf;
    }
    else
        return NULL;
}


DateTime::DateTime()
{
}
DateTime::~DateTime()
{
}
static const char aMonths[56] =
    "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";

        
char *DateTime::getLogTime(time_t lTime, char *pBuf, int bGMT)
{
    static char     bufs[2][40];
    static ls_atom_xptr_t time_and_str = {  {bufs[0], 0} };
    static char    *freeBuf = bufs[1];
    static ls_atom_xptr_t zero = {  {NULL, 0} };
    //static bool lastGMT = 0;
    ls_atom_xptr_t cur_value;
    ls_atomic_dcasv(&time_and_str, &zero, &zero, &cur_value);
    if (cur_value.m_seq == lTime)
    {
        LS_TH_IGN_RD_BEG();
        strcpy(pBuf, (char *)cur_value.m_ptr);
        LS_TH_IGN_RD_END();
        return pBuf;
    }
    else
    {
        struct tm gmt;
        int timezone = 0;
        struct tm *pTm = gmtime_r(&lTime, &gmt);
        struct tm local;
        if (!bGMT)
        {
            pTm = localtime_r(&lTime, &local);
            //NOTE: if tm_gmtoff is available, much easier
            //timezone = pTm->tm_gmtoff / 60;
            int days = local.tm_yday - gmt.tm_yday;
            int hours = ((days < -1 ? 24 : 1 < days ? -24 : days * 24)
                         + local.tm_hour - gmt.tm_hour);
            timezone = hours * 60 + local.tm_min - gmt.tm_min;
        }
        int a;
        char *p = pBuf + 30;
        *p-- = '\0';
        *p-- = '"';
        *p-- = ' ';
        *p-- = ']';
        a = (timezone > 0) ? timezone : -timezone;
        *p-- = '0' + a % 10;
        a /= 10;
        *p-- = '0' + a % 6;
        a /= 6;
        *p-- = '0' + a % 10;
        *p-- = '0' + a / 10;
        *p-- = (timezone < 0) ? '-' : '+';
        *p-- = ' ';
        a = pTm->tm_sec;
        *p-- = '0' + a % 10;
        *p-- = '0' + a / 10;
        *p-- = ':';
        a = pTm->tm_min;
        *p-- = '0' + a % 10;
        *p-- = '0' + a / 10;
        *p-- = ':';
        a = pTm->tm_hour;
        *p-- = '0' + a % 10;
        *p-- = '0' + a / 10;
        *p-- = ':';
        a = 1900 + pTm->tm_year;
        while (a)
        {
            *p-- = '0' + a % 10;
            a /= 10;
        }
        /* p points to an unused spot */
        *p-- = '/';
        p -= 2;
        memmove(p--, aMonths + 4 * (pTm->tm_mon), 3);
        *p-- = '/';
        a = pTm->tm_mday;
        *p-- = '0' + a % 10;
        *p-- = '0' + a / 10;
        *p = '[';

        //strftime( pBuf, 30, "%d/%b/%Y:%H:%M:%S", pTm );
        //sprintf( pBuf, "%s %c%02d%02d",

        // write to non-current buf - only one thread gets through
        ls_atom_xptr_t new_value;
        new_value.m_ptr = ls_atomic_setptr(&freeBuf, NULL);
        if (new_value.m_ptr)
        {
            ls_atom_xptr_t old_value;
            strcpy((char *)new_value.m_ptr, pBuf);
            new_value.m_seq = lTime;
            ls_atomic_dcasv(&time_and_str, &cur_value, &new_value, &old_value);
            ls_atomic_setptr(&freeBuf, (char *)old_value.m_ptr);
        }

        //lastGMT = bGMT;
    }
    return pBuf;
}
