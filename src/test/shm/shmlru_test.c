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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shm/lsshmlru.h>


char *argv0 = NULL;

#ifdef notdef
static char *shmdir = "/dev/shm/LiteSpeed/";
#endif
static shmlru_t ShmLru = { 0 };


/*
 */
static int pr_data(void *ptr)
{
    shmlru_data_t *pData = (shmlru_data_t *)ptr;
    fprintf(stdout, "[%.*s] (%d) %s",
            pData->size, pData->data, pData->count,
            ctime(&pData->lasttime));
    return 0;
}


/*
 */
int main(int ac, char *av[])
{
    int ret;
    char *p;
    char buf[80];
    char buf1[16 + 1];
    char buf2[16 + 1];
    char buf3[16 + 1];

    if (ac != 4)
    {
        fprintf(stderr, "usage: shmlru_test mode shmpoolname hashname\n");
        return 1;
    }
#ifdef notdef
    char shmfilename[256];
    snprintf(shmfilename, sizeof(shmfilename), "%s%s.shm", shmdir, av[1]);
    unlink(shmfilename);
    snprintf(shmfilename, sizeof(shmfilename), "%s%s.lock", shmdir, av[1]);
    unlink(shmfilename);
#endif
    if (shmlru_init(&ShmLru,
                    av[2], 0, av[3], 0, NULL, (lsi_val_comp)memcmp, atoi(av[1])) != 0)
    {
        fprintf(stderr, "shmlru_init failed!\n");
        return 1;
    }

    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        if (strcmp(buf, "quit\n") == 0)
            break;
        if (strcmp(buf, "check\n") == 0)
        {
            if ((ret = shmlru_check(&ShmLru)) == SHMLRU_CHECKOK)
            {
                LsHashLruInfo *pHdr = (LsHashLruInfo *)
                                      lsi_shmhash_datakey2ptr(ShmLru.phash, ShmLru.offhdr);
                fprintf(stdout, "check: nval=%d, ndata=%d.\n",
                        pHdr->nvalset, pHdr->ndataset);
            }
            else
                fprintf(stdout, "Check failed! [ret=%d].\n", ret);
        }
        else if (sscanf(buf, "%16s%16s%16s", buf1, buf2, buf3) != 2)
        {
            if ((p = strchr(buf, (int)'\n')) != NULL)
                * p = '\0';
            fprintf(stderr, "Bad input [%s].\n", buf);
        }
        else if (strcmp(buf1, "trim") == 0)
        {
            ret = shmlru_trim(&ShmLru, (time_t)atol(buf2), NULL, NULL);
            fprintf(stdout, "Trim: [ret=%d].\n", ret);
        }
        else if (strcmp(buf1, "get") == 0)
        {
            lsi_shm_off_t retbuf[3];
            if ((ret = shmlru_get(&ShmLru, (uint8_t *)buf2, strlen(buf2),
                                  retbuf, sizeof(retbuf) / sizeof(retbuf[0]))) < 0)
                fprintf(stderr, "Get failed! [%s].\n", buf2);
            else
            {
                lsi_shm_off_t *pRet = retbuf;
                while (--ret >= 0)
                {
                    pr_data((void *)
                            lsi_shmhash_datakey2ptr(ShmLru.phash, *pRet++));
                }
            }
        }
        else if (strcmp(buf1, "getptrs") == 0)
        {
            if ((ret = shmlru_getptrs(&ShmLru,
                                      (uint8_t *)buf2, strlen(buf2), pr_data)) < 0)
                fprintf(stderr, "Getptrs failed! [%s].\n", buf2);
        }
        else if (shmlru_set(&ShmLru,
                            (uint8_t *)buf1, strlen(buf1), (uint8_t *)buf2, strlen(buf2)) < 0)
            fprintf(stderr, "Set failed! [%s/%s].\n", buf1, buf2);
    }

    return 0;
}

