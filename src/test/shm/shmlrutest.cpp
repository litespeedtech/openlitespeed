/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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

#include <shm/lsshmlruhash.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

static const char *shmdir = LSSHM_SYSSHM_DIR1;
static const char *shmname = "SHMXLRUTEST";
static shmlru_t shmlru[2];
static struct mylru
{
    shmlru_t *base;
    int mode;
    const char *name;
} mylru[] =
{
    { &shmlru[0],    LSSHM_LRU_MODE2,    "SHMWLRU" },
    { &shmlru[1],    LSSHM_LRU_MODE3,    "SHMXLRU" },
};

static void doit(struct mylru *pLru);

TEST(ls_ShmLruTest_test)
{
    int ret;
    char shmfilename[255];
    char lockfilename[255];
    snprintf(shmfilename, sizeof(shmfilename), "%s/%s.shm", shmdir, shmname);
    snprintf(lockfilename, sizeof(lockfilename), "%s/%s.lock", shmdir,
             shmname);
    unlink(shmfilename);
    unlink(lockfilename);

    CHECK((ret = shmlru_init(mylru[0].base, shmname, 0,
                      mylru[0].name, 0, LsShmHash::hashXXH32, LsShmHash::compBuf,
                      mylru[0].mode)) == 0);
    if (ret != 0)
        return;
    CHECK((ret = shmlru_init(mylru[1].base, shmname, 0,
                      mylru[1].name, 0, LsShmHash::hashXXH32, LsShmHash::compBuf,
                      mylru[1].mode)) == 0);
    if (ret != 0)
        return;

    if (unlink(shmfilename) != 0)
        perror(shmfilename);
    if (unlink(lockfilename) != 0)
        perror(lockfilename);

    doit(&mylru[0]);
    doit(&mylru[1]);
}

static void doit(struct mylru *pLru)
{
    const char key0[] = "KEY0";
    const char keyX[] = "KEYX5678901234567";
    const char val1[] = "VAL1";
    const char val2[] = "VAL22";
    const char val3[] = "VAL333";
    const char val4[] = "VAL4";
    const char valX[] = "VALX5678901234567";
    int cnt;
    int exp;
    int ret;
    lsi_shm_off_t retbuf[10];
    shmlru_data_t *pData;

    fprintf(stdout, "shmlrutest: [%s/%s]\n", shmname, pLru->name);
    cnt = SHMLRU_MINSAVECNT;
    while (--cnt >= 0)
    {
        CHECK(shmlru_set(pLru->base,
                         (uint8_t *)key0, sizeof(key0) - 1, (uint8_t *)val1,
                         sizeof(val1) - 1) == 0);
    }
    CHECK((ret = shmlru_get(pLru->base, (uint8_t *)key0, sizeof(key0) - 1,
                            retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) == 1);
    if (ret == 1)
    {
        pData = (shmlru_data_t *)
                lsi_shmhash_datakey2ptr(pLru->base->phash, retbuf[0]);
        fprintf(stdout, "[%.*s] (%d) %s",
                pData->size, pData->data, pData->count,
                ctime(&pData->lasttime));
        CHECK(pData->count == SHMLRU_MINSAVECNT);
        CHECK(pData->size == sizeof(val1) - 1);
        CHECK(memcmp(pData->data, val1, pData->size) == 0);
    }
    lsi_shm_off_t retsave = retbuf[0];

    cnt = SHMLRU_MINSAVECNT - 1;
    while (--cnt >= 0)
    {
        CHECK(shmlru_set(pLru->base,
                         (uint8_t *)key0, sizeof(key0) - 1, (uint8_t *)val2,
                         sizeof(val2) - 1) == 0);
    }
    exp = ((pLru->mode == LSSHM_LRU_MODE2) ? 1 : 2);
    CHECK((ret = shmlru_get(pLru->base, (uint8_t *)key0, sizeof(key0) - 1,
                            retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) == exp);
    if (ret == exp)
    {
        pData = (shmlru_data_t *)
                lsi_shmhash_datakey2ptr(pLru->base->phash, retbuf[0]);
        fprintf(stdout, "[%.*s] (%d) %s",
                pData->size, pData->data, pData->count,
                ctime(&pData->lasttime));
        CHECK(pData->count == SHMLRU_MINSAVECNT - 1);
        CHECK(pData->size == sizeof(val2) - 1);
        CHECK(memcmp(pData->data, val2, pData->size) == 0);
        if (pLru->mode == LSSHM_LRU_MODE2)    /* allocs larger data, then releases old */
            CHECK(retsave != retbuf[0]);
        else
        {
            CHECK(retsave != retbuf[0]);
            CHECK(retsave == retbuf[1]);
        }
    }

    CHECK(shmlru_set(pLru->base,
                     (uint8_t *)keyX, sizeof(keyX) - 1, (uint8_t *)valX,
                     sizeof(valX) - 1) == 0);
    CHECK((ret = shmlru_get(pLru->base, (uint8_t *)keyX, sizeof(keyX) - 1,
                            retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) == 1);
    if (ret == 1)
    {
        pData = (shmlru_data_t *)
                lsi_shmhash_datakey2ptr(pLru->base->phash, retbuf[0]);
        fprintf(stdout, "[%.*s] (%d) %s",
                pData->size, pData->data, pData->count,
                ctime(&pData->lasttime));
        CHECK(pData->count == 1);
        CHECK(pData->size == sizeof(valX) - 1);
        CHECK(memcmp(pData->data, valX, pData->size) == 0);
    }

    CHECK(shmlru_set(pLru->base,
                     (uint8_t *)key0, sizeof(key0) - 1, (uint8_t *)val3,
                     sizeof(val3) - 1) == 0);
    /* should not save previous data entry, count < min */
    exp = ((pLru->mode == LSSHM_LRU_MODE2) ? 1 : 2);
    CHECK((ret = shmlru_get(pLru->base, (uint8_t *)key0, sizeof(key0) - 1,
                            retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) == exp);
    if (ret == exp)
    {
        pData = (shmlru_data_t *)
                lsi_shmhash_datakey2ptr(pLru->base->phash, retbuf[0]);
        fprintf(stdout, "[%.*s] (%d) %s",
                pData->size, pData->data, pData->count,
                ctime(&pData->lasttime));
        CHECK(pData->count == 1);
        CHECK(pData->size == sizeof(val3) - 1);
        CHECK(memcmp(pData->data, val3, pData->size) == 0);
    }
    retsave = retbuf[0];

    CHECK(shmlru_set(pLru->base,
                     (uint8_t *)key0, sizeof(key0) - 1, (uint8_t *)val4,
                     sizeof(val4) - 1) == 0);
    /* should reuse previous data entry, size < maxsize */
    exp = ((pLru->mode == LSSHM_LRU_MODE2) ? 1 : 2);
    CHECK((ret = shmlru_get(pLru->base, (uint8_t *)key0, sizeof(key0) - 1,
                            retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) == exp);
    if (ret == exp)
    {
        pData = (shmlru_data_t *)
                lsi_shmhash_datakey2ptr(pLru->base->phash, retbuf[0]);
        fprintf(stdout, "[%.*s] (%d) %s",
                pData->size, pData->data, pData->count,
                ctime(&pData->lasttime));
        CHECK(pData->count == 1);
        CHECK(pData->size == sizeof(val4) - 1);
        CHECK(memcmp(pData->data, val4, pData->size) == 0);
        CHECK(retsave == retbuf[0]);
    }

    CHECK(shmlru_check(pLru->base) == SHMLRU_CHECKOK);
    CHECK(shmlru_trim(pLru->base, time((time_t *)NULL) + 1) == 2);
    CHECK(shmlru_check(pLru->base) == SHMLRU_CHECKOK);
}

#endif

