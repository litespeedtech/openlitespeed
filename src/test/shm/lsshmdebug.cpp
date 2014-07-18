/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#include <http/httpglobals.h>
#include <http/httplog.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <shm/lsshm.h>
#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include "lsshmdebug.h"


FILE * debugBase::m_fp = NULL;
char * debugBase::m_outfile = NULL;

void debugBase::setup(const char * filename)
{
    if (m_fp)
        return;
    m_outfile = strdup(filename);
    m_fp = fopen(m_outfile, "w");
}

void debugBase::done()
{
    if (m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    if (m_outfile)
    {
        free(m_outfile);
        m_outfile = NULL;
    }
}

void debugBase::dumpFreeBlock( LsShm * pShm, LsShm_offset_t offset)
{
    register lsShmFreeTop_t * ap;
    register lsShmFreeBot_t * bp;
    
    ap = (lsShmFreeTop_t *)pShm->offset2ptr(offset);
    bp = (lsShmFreeBot_t *)pShm->offset2ptr(offset + ap->x_freeSize - sizeof(lsShmFreeBot_t));
    
    fprintf(m_fp, "FLINK %s%8X %8X ->%8X %8X<- %s%s\n",
            (ap->x_aMarker != LSSHM_FREE_AMARKER) ? "E" : "+",
        offset, ap->x_freeSize,
        ap->x_freeNext, ap->x_freePrev,
            (bp->x_bMarker != LSSHM_FREE_BMARKER) ? "E" : "+",
        bp->x_freeOffset != offset ? "O" : " "
    );
}

void debugBase::dumpMapFreeList( LsShm* pShm )
{
    register LsShm_offset_t offset;
    register lsShmFreeTop_t * ap;
    register lsShmMap_t *hp;
    hp = pShm->m_pShmMap;
    
    for (offset = hp->x_freeOffset; offset; )
    {
        ap = (lsShmFreeTop_t *)pShm->offset2ptr(offset);
        dumpFreeBlock(pShm, offset);
        offset = ap->x_freeNext;
    }
}

void debugBase::dumpMapCheckFree( LsShm* pShm )
{
    register lsShmFreeTop_t *ap;
    register lsShmFreeBot_t *bp;
    LsShm_offset_t offset;
    
    fprintf(m_fp, "FREE LINK\n");
    dumpMapFreeList(pShm);
    fprintf(m_fp, "FREE MAP\n");
    
    for (offset = pShm->getShmMap()->x_xdataOffset + pShm->getShmMap()->x_xdataSize; 
                offset < pShm->getShmMap()->x_size; 
                )
    {
        ap = (lsShmFreeTop_t *)pShm->offset2ptr(offset);
        if (ap->x_aMarker == LSSHM_FREE_AMARKER)
        {
            bp = (lsShmFreeBot_t *)pShm->offset2ptr(offset + ap->x_freeSize - sizeof(lsShmFreeBot_t));
            if (bp->x_bMarker == LSSHM_FREE_BMARKER)
            {
                dumpFreeBlock(pShm, offset);
                offset += ap->x_freeSize;
                continue;
            }
        }
        offset += pShm->getShmMap()->x_unitSize;
    }
}

void debugBase::dumpMapHeader( LsShm* pShm )
{
    register lsShmMap_t *hp;
    hp = pShm->m_pShmMap;
    fprintf(m_fp, "%.12s %X %d.%d.%d CUR %8X %8X DATA %8X AVAIL %X UNIT %X FREE %X\n",
            hp->x_name,
            hp->x_magic,
            (int)hp->x_version.x.major,
            (int)hp->x_version.x.minor,
            (int)hp->x_version.x.rel,
            hp->x_size,
            hp->x_maxSize,
            hp->x_xdataOffset,
            hp->x_xdataSize,
            hp->x_unitSize,
            hp->x_freeOffset
        );
    fprintf(m_fp, "REG NUMPERPAGE %X SIZE %d E%d H%d R%d  OFFSET %X %X MAX %d\n",
            hp->x_numRegPerBlk,
            
            sizeof(lsShmRegBlkHdr_t),
            sizeof(lsShmReg_t),
            sizeof(lsShmRegElem_t),
            sizeof(lsShmRegBlk_t),
            
            hp->x_regBlkOffset,
            hp->x_regLastBlkOffset,
            hp->x_maxRegNum
        );
}

void debugBase::dumpHeader( LsShm* pShm )
{
    fprintf(m_fp, "%d %s %p %p MAXSIZE[%X] %p hdrsize %X\n",
            pShm->m_status,
            pShm->m_fileName,
            pShm->m_pShmMap,
            pShm->m_pShmMap_o,
            pShm->m_maxSize_o,
            pShm->m_pShmData,
            (unsigned int)pShm->m_pShmData - (unsigned int)(pShm->m_pShmMap)
        );
}

void debugBase::dumpShm( LsShmPool * pool, int mode, void* udata )
{
    LsShm * pShm;
    pShm = pool->m_pShm;
    fprintf(m_fp, ">>>>>>>>>>>>>>>>START SHARE-MEMORY-MAP-DUMP\n");
    dumpHeader(pShm);
    dumpMapHeader(pShm);
    dumpMapCheckFree(pShm);
    fprintf(m_fp, "END <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
    fflush(m_fp);
}

//
//  print status 
//
int debugBase::checkStatus( const char* tag, const char* mapName, LsShm_status_t status )
{
    switch(status)
    {
    case LSSHM_READY:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s READY\r\n",
                   tag, mapName);
        return 0;
        
    case LSSHM_NOTREADY:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s NOTREADY\r\n",
                   tag, mapName);
        break;
    case LSSHM_BADMAPFILE:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s BADMAPFILE\r\n",
                   tag, mapName);
        break;
    case LSSHM_BADPARAM:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s BADPARAM\r\n",
                   tag, mapName);
        break;
    case LSSHM_BADVERSION:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s BADVERSION\r\n",
                   tag, mapName);
        break;
    default:
        HttpLog::log(LSI_LOG_NOTICE,  "%s %s UNKNOWN %d\r\n",
                   tag, mapName, status);
        break;
    }
    return -1;
}

void debugBase::dumpPoolPage( LsShmPool* pool )
{
    ;
}

void debugBase::dumpPoolDataFreeList( LsShmPool* pool )
{
    LsShm_offset_t offset;
    lsShmFreeList_t * pFree;
    
    fprintf(debugBase::fp(), "FreeList %8X ", 
            pool->m_dataMap->x_freeList );
    for (offset = pool->m_dataMap->x_freeList; offset; )
    {
        pFree = (lsShmFreeList_t *) pool->offset2ptr(offset);
        fprintf(debugBase::fp(), "<%4X %4X %4X>", 
                pFree->x_prev, pFree->x_size, pFree->x_next );
        offset = pFree->x_next;
    }
    fprintf(debugBase::fp(), "\n");
}

void debugBase::dumpPoolDataFreeBucket( LsShmPool* pool )
{
    LsShm_offset_t offset;
    LsShm_offset_t * pBucket;
    register int i;
    pBucket = pool->m_dataMap->x_freeBucket;
    for (i = 0; i < LSSHM_POOL_NUMBUCKET; i++)
    {
        if ( (offset = *pBucket) )
        {
            fprintf(debugBase::fp(), "Bucket[%2d]->%X", i, offset);
            register int num = 0;
            while ((num < 8) && offset)
            {
                LsShm_offset_t * xp;
                xp = (LsShm_offset_t *) pool->offset2ptr(offset);
                offset = *xp;
                fprintf(debugBase::fp(), " %X", offset);
                num++;
            }
            fprintf(debugBase::fp(), "\n");
        }
        pBucket++;
    }
}

void debugBase::dumpPoolDataCheck( LsShmPool* pool )
{
    ;
}

void debugBase::dumpPoolData( LsShmPool* pool )
{
    ;
}

void debugBase::dumpPoolHeader( LsShmPool* pool )
{
    fprintf(debugBase::fp(), "=====================================================\n");
    fprintf(debugBase::fp(), "LsShmPool %p %.12s %s %p %p [%p %p]\n",
            pool, pool->name(),
            (pool->status() == LSSHM_READY) ? "READY" : "NOTREADY",
            pool->m_pShm,
            pool->m_pool,
            pool->m_pageMap,
            pool->m_dataMap
           );
    
    fprintf(debugBase::fp(), "MAGIC %8X SIZE %8X AVAIL %8X FREE %8X\n", 
            pool->m_pool->x_magic,
            pool->m_pool->x_size,
            pool->m_pShm->getShmMap()->x_xdataSize,
            pool->m_pShm->getShmMap()->x_xdataSize - pool->m_pool->x_size
           );
    
    fprintf(debugBase::fp(), "DATA UNIT [%X %X] NUMBUCKET %X CHUNK [%X %X] FREE %X\n",
            pool->m_dataMap->x_unitSize,
            pool->m_dataMap->x_maxUnitSize,
            pool->m_dataMap->x_numFreeBucket,
            pool->m_dataMap->x_chunk.x_start,
            pool->m_dataMap->x_chunk.x_end,
            pool->m_dataMap->x_freeList
            );
    fprintf(debugBase::fp(), "=====================================================\n");
}

void debugBase::dumpShmPool (LsShmPool * pool, int mode, void * udata)
{
    dumpPoolHeader( pool );
    dumpPoolData( pool );
    dumpPoolDataFreeList( pool );
    dumpPoolDataFreeBucket( pool );
}

void debugBase::dumpRegistry( const char* tag, const lsShmReg_t* p_reg )
{
    fprintf(debugBase::fp(),
            "%s REGISTRY[%d] %.12s %X [%d]\n",
            tag ? tag : "",
            p_reg->x_regNum, 
            p_reg->x_name, 
            p_reg->x_value,
            (int)p_reg->x_flag);
}

void debugBase::dumpShmReg( LsShm* pShm )
{
    register LsShm_offset_t offset;
    register lsShmRegBlkHdr_t * p_regHdr;
    register int    regId, i;
    register lsShmRegElem_t * p_reg;
    char tag[0x100];
    
    regId = 0;
    offset = pShm->getShmMap()->x_regBlkOffset;
    while (offset)
    {
        p_reg = (lsShmRegElem_t*) pShm->offset2ptr(offset);
        p_regHdr = (lsShmRegBlkHdr_t*)p_reg;
        int maxNum = p_regHdr->x_startNum + p_regHdr->x_capacity;
        
        
        fprintf(debugBase::fp(), "%4X REG_BLK[%d] %d %d %d -> %X\n", 
                pShm->ptr2offset(p_regHdr),
                regId,
                p_regHdr->x_startNum, 
                p_regHdr->x_size,
                p_regHdr->x_capacity,
                p_regHdr->x_next);
        // regId++;
        p_reg++;
        for (i = p_regHdr->x_startNum + 1; i < maxNum; i++)
        {
            if ( p_reg->x_reg.x_name[0] )
            {
                snprintf(tag, 0x100, "%4d", regId);
                debugBase:: dumpRegistry(tag, (lsShmReg_t *)p_reg);
            }
            regId++;
            p_reg++;
        }
        offset = p_regHdr->x_next;
    }
}

// simple test decode... dont make this big!
const char* debugBase::decode( const char* p, int size )
{
    char            sbuf[0x21];
    static char     buf[0x100];
    register char   *cp, *sp;
    register int    nb;
    
    sp = sbuf;
    cp = buf;
    nb = sprintf(cp, "%08X ", (unsigned int)buf);
    cp += nb;
    if (size > 0x20)
    {
        size = 0x20;
    }
    while (--size >= 0)
    {
        nb = sprintf(cp, "%02X ", ((unsigned int)*buf)&0xff) ;
        if ((*p < ' ') || (*p >= 0x7f))
            *sp++ = '.' ;
        else
            *sp++ = *buf;
        p++;
        cp += nb;
    }
    *sp = 0;
    *cp++ = ' ';
    strcpy(cp, sbuf);
    return(buf);
}

void debugBase::dumpBuf( const char* tag, const char* buf, int size )
{
    register int    i;
    char            sbuf[0x20];

    while (size)
    {
        if (tag)
            fprintf(fp(), "%-8.8s ", tag);
        fprintf(fp(), "%08X ", (unsigned int)buf);
        for (i = 0; (i < 0x10) && size; i++, size--, buf++)
        {
            fprintf(fp(), "%02X ", ((unsigned int)*buf)&0xff) ;
            if ((*buf < ' ') || (*buf >= 0x7f))
                sbuf[i] = '.' ;
            else
                sbuf[i] = *buf;
        }
        sbuf[i--] = '\0' ;
        while (i++ < 0x10)
            fprintf(fp(), "   ") ;
        fprintf(fp(), "\t%s\n", sbuf) ;
    }
}

void debugBase::dumpIterKey( LsShmHash::iterator iter )
{
    fprintf(fp(), "KEY[%s]", decode((char *)iter->p_key(), iter->x_rkeyLen));
}

void debugBase::dumpIterValue( LsShmHash::iterator iter )
{
    fprintf(fp(), "VAL[%s]", decode((char *)iter->p_value(), iter->x_valueLen));
}

void debugBase::dumpIter( const char* tag, LsShmHash::iterator iter )
{
    if (tag)
        fprintf(fp(), "%8.8s", tag);
    dumpIterKey(iter);
    dumpIterValue(iter);
}

