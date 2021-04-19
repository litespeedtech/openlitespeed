/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <time.h>
#include <signal.h>

char *pchSendBuffer = NULL;
int  iSendBuffer = 0;
int  iTestNumber = 0;

typedef struct postData_ {
    int m_isocket;          // The communications socket.
    int m_iHowMuch;         // Total
    int m_iBuffer;          // Buffer size
    int m_iuDelay;          // in microseconds
    int m_iCloseAfter;      // in bytes
    int m_iCloseAfterRecv;  // in bytes
    int m_bStarted;         // Whether the thread started
    int m_bOk;              // Whether it ran to expected completion
} postData_t_;

char *strTime(char *pchBuffer) {
    struct timeb timebNow;
    struct tm    tmNow;
    ftime(&timebNow);
    localtime_r(&timebNow.time,&tmNow);
    sprintf(pchBuffer,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tmNow.tm_year + 1900,
            tmNow.tm_mon + 1,
            tmNow.tm_mday,
            tmNow.tm_hour,
            tmNow.tm_min,
            tmNow.tm_sec,
            timebNow.millitm);
    return(pchBuffer);
}



void *sendThread(void *pvPostData) {
    postData_t_ *pPostData = (postData_t_ *)pvPostData;
    char *pBuffer;
    char *pTempBuffer;
    int  iCounter = 0;
    int  i = 0;
    int  isent = 0;
    int  iBlock = 0;
    char chTime[80];

    pPostData->m_bStarted = 1;
    pBuffer = (char *)malloc(pPostData->m_iBuffer);
    if (!pBuffer) {
        printf("SendThread error in malloc of %d bytes of data\n",pPostData->m_iBuffer);
        return(NULL);
    }
    pTempBuffer = pBuffer;
    while (iCounter < pPostData->m_iBuffer) {
        (*pTempBuffer) = (iCounter % 26) + 'A';
        iCounter++;
        pTempBuffer++;
    }
    for (i = 0; i < pPostData->m_iHowMuch; i += isent) {
        char chBlock[80];
        int iSend;
        iBlock++;
        // WARNING Block counters start at 1!
        sprintf(chBlock,"%08d",iBlock);
        iSend = (pPostData->m_iBuffer > (pPostData->m_iHowMuch - i)) ? (pPostData->m_iHowMuch - i) : pPostData->m_iBuffer;
        memcpy(pBuffer,chBlock,(8 < iSend) ? 8 : iSend);
        isent = send(pPostData->m_isocket, pBuffer, iSend, 0);
        if (isent < 0) {
            printf("%s Error in send of byte %d of %d: %s\n", strTime(chTime), i, pPostData->m_iHowMuch, strerror(errno));
            break;
        }
        if ((pPostData->m_iCloseAfter) && (i + isent >= pPostData->m_iCloseAfter)) {
            printf("%s Breaking connection early after sending %d bytes\n", strTime(chTime), i + isent);
            //close(pPostData->m_isocket);
            pPostData->m_bOk = 1;
            break;
        }
        if (pPostData->m_iuDelay) {
            usleep(pPostData->m_iuDelay);
        }
    }
    if (i + 1 >= pPostData->m_iHowMuch) {
        pPostData->m_bOk = 1;
        printf("Send thread ok\n");
    }
    free(pBuffer);
    // Close in the main program....
    return(pPostData->m_bOk ? pPostData : NULL);
}


// Returns the chunk size or -1 if there's an error
int chunkedHeader(postData_t_  *pPostData) {
    char chChunkHeader[80];
    int  iChunkHeader = 0;
    int iRecvd;
    int iChunkSize;
    int bBeginSize = 0;
    int bBeginData = 0;
    char chTime[80];
    char chTimePrior[80];
    do {
        if (iChunkHeader > 20) {
            printf("%s Past reasonable point in header for a size\n", strTime(chTime));
            return(-1);
        }
        strTime(chTimePrior);
        iRecvd = recv(pPostData->m_isocket,&chChunkHeader[iChunkHeader],1,0);
        if ((iChunkHeader == 0) &&
            (((chChunkHeader[0] >= '0') && (chChunkHeader[0] <= '9')) ||
             ((chChunkHeader[0] >= 'A') && (chChunkHeader[0] <= 'F')) ||
             ((chChunkHeader[0] >= 'a') && (chChunkHeader[0] <= 'f')))) {
            bBeginSize = 1;
            iChunkHeader++;
            chChunkHeader[iChunkHeader] = 0;
            continue;
        }
        if (iRecvd == 0) {
            printf("%s Received unexpected EOF reading chunk header (entered recv at %s)\n",strTime(chTime), chTimePrior);
            return(-1);
        }
        if (iRecvd < 0) {
            printf("%s Error receiving: %s, entered receive at: %s\n",strTime(chTime),strerror(errno), chTimePrior);
            return(-1);
        }
        if (chChunkHeader[iChunkHeader] == '\r') {
            // Filler, ignore it.
        }
        else if (chChunkHeader[iChunkHeader] == '\n') {
            if (bBeginSize) {
                bBeginData = 1;
                chChunkHeader[iChunkHeader] = 0;
            }
            else {
                bBeginSize = 1;
            }
        }
        else if (bBeginSize) {
            iChunkHeader++;
            chChunkHeader[iChunkHeader] = 0;
        }
    } while (!bBeginData);
    iChunkSize = (int)strtol(chChunkHeader,NULL,16);
    if (iChunkSize < 0) {
        printf("%s Unexpeced negative chunk size: %s, entered recv at: %s\n",strTime(chTime),strerror(errno), chTimePrior);
        return(-1);
    }
    //printf("ChunkSize: %s, %d\n", chChunkHeader, iChunkSize);
    return(iChunkSize);
}


int processChunked(postData_t_ *pPostData,
                   char        *pBuffer,
                   char        *pBuffer2)
{
    // Counters
    int  iInBuffer = 0;
    int  iChunkPos = 0;
    int  iChunkSize = 0;
    int  iTotalReceived = 0;
    int  iBlock = 0;
    char chTime[80];
    // State controllers
#define STATE_GET_CHUNK_HEADER  0
#define STATE_GET_BUFFER        1
#define STATE_CHECK_BUFFER      2
#define STATE_DONE              3
    int  iState = STATE_GET_CHUNK_HEADER;
    printf("Receiving chunked data\n");
    do {
        if (iState == STATE_GET_CHUNK_HEADER) {
            int iPriorChunkSize = iChunkSize;
            iChunkSize = chunkedHeader(pPostData);

            if (iChunkSize < 0) {
                printf("Prior error reported  Details on location: iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d, PriorChunkSize: %d\n", iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize, iPriorChunkSize);
                return(-1);
            }
            iChunkPos = 0;
            if (iChunkSize == 0) {
                printf("%s Chunk size is 0, unexpected, %d bytes remaining\n",strTime(chTime),(pPostData->m_iHowMuch - iTotalReceived));
                return(-1);
            }
            if (iChunkSize > pPostData->m_iBuffer) {
                printf("%s chunk size: %d greater than block size: %d at %d\n", strTime(chTime), iChunkSize, pPostData->m_iBuffer, iTotalReceived);
                return(-1);
            }
            iState++;
        }
        if (iState == STATE_GET_BUFFER) {
            int iSpaceInBuffer = pPostData->m_iBuffer - iInBuffer;
            int iRemainingData = pPostData->m_iHowMuch - iTotalReceived;
            int iChunkRemaining = iChunkSize - iChunkPos;
            // Take the smallest of the 3 above to read
            int iRead;
            //int iNextState;
            int iDidRead;
            if (!iRemainingData) {
                printf("%s Thought we needed to get data, but we really don't!\n",strTime(chTime));
                iState = STATE_DONE;
                continue;
            }
            if (!iSpaceInBuffer) {
                if (iInBuffer) {
                    printf("%s Thought we needed to get data, but we have a full left over buffer.  Check it. (iBuffer: %d, iInBuffer: %d, TotalReceived: %d, ChunkSize: %d, ChunkPos: %d)\n", strTime(chTime), pPostData->m_iBuffer, iInBuffer, iTotalReceived, iChunkSize, iChunkPos);
                }
                else {
                    printf("%s Thought we needed to get data, but we have an empty left over buffer.  Check it. (iBuffer: %d, iInBuffer: %d)\n", strTime(chTime), pPostData->m_iBuffer, iInBuffer);
                }
                iState = STATE_CHECK_BUFFER;
                continue;
            }
            if (!iChunkRemaining) {
                printf("%s Thought we needed to get buffer, but we're out of chunk - so get some of that!\n", strTime(chTime));
                iState = STATE_GET_CHUNK_HEADER;
                continue;
            }
            // 3 possible values, total choices 6
            //printf("Choosing: iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d\n", iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize);
            //printf("iSpaceInBuffer: %d, iRemainingData: %d, iChunkRemaining: %d\n", iSpaceInBuffer, iRemainingData, iChunkRemaining);
            if ((iSpaceInBuffer <= iRemainingData) &&
                (iRemainingData <= iChunkRemaining)) {
                iRead = iSpaceInBuffer;
            }
            else if ((iSpaceInBuffer <= iChunkRemaining) &&
                     (iChunkRemaining <= iRemainingData)) {
                iRead = iSpaceInBuffer;
            }
            else if ((iRemainingData <= iSpaceInBuffer) &&
                     (iSpaceInBuffer <= iChunkRemaining)) {
                iRead = iRemainingData;
            }
            else if ((iRemainingData <= iChunkRemaining) &&
                     (iChunkRemaining <= iSpaceInBuffer)) {
                iRead = iRemainingData;
            }
            else if ((iChunkRemaining <= iRemainingData) &&
                     (iRemainingData <= iSpaceInBuffer)) {
                iRead = iChunkRemaining;
            }
            else {
                iRead = iChunkRemaining;
            }
            //printf("Do read for %d, starting at %d of %d\n",iRead, iInBuffer, pPostData->m_iBuffer);
            iDidRead = recv(pPostData->m_isocket, &pBuffer[iInBuffer], iRead, 0);
            if (iDidRead == 0) {
                printf("%s Reading data, unexpected EOF reading data, tried to read: %d\n", strTime(chTime),iRead);
                printf("Unexpected state: iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d\n", iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize);
                return(-1);
            }
            else if (iDidRead < 0) {
                printf("%s Reading data, error: %s\n", strTime(chTime), strerror(errno));
                return(-1);
            }
            iTotalReceived += iDidRead;
            iInBuffer += iDidRead;
            iChunkPos += iDidRead;
            if ((pPostData->m_iCloseAfterRecv) && (iTotalReceived >= pPostData->m_iCloseAfterRecv)) {
                printf("%s CloseAfterRecv (%d) reached in receive thread (received: %d)\n", strTime(chTime), pPostData->m_iCloseAfterRecv, iTotalReceived);
                return(iTotalReceived);
            }
            if ((iInBuffer >= pPostData->m_iBuffer) ||
                (iTotalReceived >= pPostData->m_iHowMuch)) {
                iState = STATE_CHECK_BUFFER;
            }
            else if (iChunkPos >= iChunkSize) {
                iState = STATE_GET_CHUNK_HEADER;
            }
            else {
                // Leave the state alone and continue to come right back here to receive using a different size.
                continue;
                //printf("%s Unexpected state: iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d\n", strTime(chTime), iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize);
                //return(-1);
            }
        }
        if (iState == STATE_CHECK_BUFFER) {
            char chBlock[80];
            if ((!iInBuffer) || (iInBuffer > pPostData->m_iBuffer)) {
                printf("%s Invalid iInBuffer in CheckBuffer. iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d\n", strTime(chTime), iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize);
                return(-1);
            }
            iBlock++;
            sprintf(chBlock,"%08d",iBlock);
            memcpy(pBuffer2,chBlock,(iInBuffer > 8) ? 8 : iInBuffer);
            if (memcmp(pBuffer, pBuffer2, iInBuffer)) {
                int iIndex;
                char chBlockReceived[9];
                memcpy(chBlockReceived,pBuffer,8);
                chBlockReceived[8] = 0;
                printf("%s Unexpected data. Block #%d iInBuffer: %d, iTotalReceived: %d, iChunkPos: %d, BufferSize: %d, TotalToReceive: %d, ChunkSize: %d, Expected Block: %s, ReceivedBlock: %s\n", strTime(chTime), iBlock, iInBuffer, iTotalReceived, iChunkPos, pPostData->m_iBuffer, pPostData->m_iHowMuch, iChunkSize, chBlock, chBlockReceived);
                for (iIndex = 0; iIndex < iInBuffer; iIndex++) {
                    if (pBuffer[iIndex] != pBuffer2[iIndex]) {
                        printf("Mismatch at index #%d, '%c' != '%c'\n", iIndex, pBuffer[iIndex], pBuffer2[iIndex]);
                        break;
                    }
                }
                return(-1);
            }
            iInBuffer = 0;
            if (iTotalReceived >= pPostData->m_iHowMuch) {
                printf("Completed receive of %d bytes successfully\n", iTotalReceived);
                iState = STATE_DONE;
                break;
            }
            else if (iChunkPos >= iChunkSize) {
                iState = STATE_GET_CHUNK_HEADER;
            }
            else {
                iState = STATE_GET_BUFFER;
            }
        }
    } while (iTotalReceived < pPostData->m_iHowMuch);
    return(iTotalReceived);
}



void *recvThread(void *pvPostData) {
    postData_t_ *pPostData = (postData_t_ *)pvPostData;
    char *pBuffer;
    char *pBuffer2;
    char *pTempBuffer;
    int  iCounter = 0;
    int  i = 0;
    int  isent = 0;
    int  iheader = 0;
    char chBuffer[1024];
    char chTime[80];
    int  iBlock = 0;
    char chBlock[80];
    int  iAllocate;

    pPostData->m_bStarted = 1;
    // For now receive a buffer and see if it's the header.
    if (pPostData->m_iCloseAfter) {
        printf("%s Early close (send), reading header\n", strTime(chTime) );
    }
    while ((isent = recv(pPostData->m_isocket, &chBuffer[iheader], 1, 0)) == 1) {
        iheader++;
        if ((iheader > 4) &&
            (chBuffer[iheader - 1] == '\n') &&
            (chBuffer[iheader - 2] == '\r') &&
            (chBuffer[iheader - 3] == '\n') &&
            (chBuffer[iheader - 4] == '\r')) {
            chBuffer[iheader] = 0;
            //printf("RecvThread got header after %d bytes (%s)\n",iheader,chBuffer);
            break;
        }
    }
    if (isent == 0) {
        printf("%s RecvThread end of data reading header after %d bytes\n",strTime(chTime), iheader);
        return(NULL);
    }
    chBuffer[iheader] = 0;
    iAllocate = pPostData->m_iBuffer;
    if (iAllocate < 8) {
        iAllocate = 8;
    }
    pBuffer = (char *)malloc(iAllocate + 1);
    pBuffer2 = (char *)malloc(iAllocate + 1);
    if ((!pBuffer) || (!pBuffer2)) {
        printf("%s RecvThread error in malloc of %d bytes of data\n",strTime(chTime), pPostData->m_iBuffer);
        if (pBuffer) {
            free(pBuffer);
        }
        if (pBuffer2) {
            free(pBuffer2);
        }
        return(NULL);
    }
    pBuffer[iAllocate] = 0;
    pBuffer2[iAllocate] = 0;
    pBuffer[pPostData->m_iBuffer] = 0;
    pBuffer2[pPostData->m_iBuffer] = 0;
    /* Initialize a second buffer for comparison purposes! */
    pTempBuffer = pBuffer2;
    while (iCounter < pPostData->m_iBuffer) {
        (*pTempBuffer) = (iCounter % 26) + 'a';
        iCounter++;
        pTempBuffer++;
    }
    if (strstr(chBuffer,"Transfer-Encoding: chunked")) {
        int result;
        result = processChunked(pPostData,pBuffer,pBuffer2);
        free(pBuffer);
        free(pBuffer2);
        if (result >= 0) {
            return(pPostData);
        }
        return(NULL);
    }
    else for (i = 0; i < pPostData->m_iHowMuch; i += isent) {
        int size = (pPostData->m_iBuffer > (pPostData->m_iHowMuch - i)) ? (pPostData->m_iHowMuch - i) : pPostData->m_iBuffer;
        int recvd = 0;
        int irecv = 0;
        while (recvd < size) {
            int recvSize = size - recvd;
            //printf("Receive %d bytes starting at %d in buffer\n", recvSize, recvd);
            irecv = recv(pPostData->m_isocket, &pBuffer[recvd], recvSize, 0);
            if (irecv < 0) {
                printf("%s Error in recv of byte %d of %d: %s\n", strTime(chTime), i + recvd, recvSize, strerror(errno));
                break;
            }
            if (irecv == 0) {
                printf("%s Unexpected EOF in data at %d\n",strTime(chTime), i);
                break;
            }
            recvd += irecv;
        }
        if (irecv <= 0) {
            break;
        }
        isent = recvd;
        if ((pPostData->m_iCloseAfterRecv) && (i + isent >= pPostData->m_iCloseAfterRecv)) {
            printf("%s Breaking connection early after receiving %d bytes\n", strTime(chTime), i + isent);
            pPostData->m_bOk = 1;
            break;
        }
        pBuffer[recvd] = 0;
        iBlock++;
        if (pPostData->m_iBuffer >= 8) {
            sprintf(chBlock,"%08d",iBlock);
            memcpy(pBuffer2,chBlock,(recvd > 8) ? 8 : recvd);
            if (memcmp(pBuffer,pBuffer2,recvd)) {
                char chBlockReceive[9];
                memcpy(chBlockReceive,pBuffer,8);
                chBlockReceive[8] = 0;
                printf("%s Mismatch in data in buffer starting block #%d at %d (at %d, got: %c, expected: %c), received block: %s, expected block: %s: %s\n",strTime(chTime), iBlock, i,i,pBuffer[0],pBuffer2[0],chBlockReceive, chBlock, pBuffer);
                break;
            }
        }
        else {
            // Just a couple of bytes, don't try to validate it.
        }
        if (pPostData->m_iuDelay) {
            usleep(pPostData->m_iuDelay);
        }
    }
    if (i + 1 >= pPostData->m_iHowMuch) {
        pPostData->m_bOk = 1;
        printf("recvThread ok\n");
    }
    free(pBuffer);
    free(pBuffer2);
    // Close in the main program....
    printf("Return from recv thread, Ok: %s\n", pPostData->m_bOk ? "YES" : "NO");
    return(pPostData->m_bOk ? pPostData : NULL);
}



int postData(int iHowMuch, int iSendBuffer, int iRecvBuffer, int iuDelay, int iCloseAfter, int iCloseAfterRecv)

{
    int isocket;
    int iflag = 1;
    struct sockaddr_in ssockaddr_in;
    char header[256];
    //int isent;
    pthread_attr_t pthread_attr1;
    pthread_t      pthread1;
    pthread_attr_t pthread_attr2;
    pthread_t      pthread2;
    postData_t_    postDataSend;
    postData_t_    postDataRecv;
    char           chTime[80];
    void          *pvSendResult = &postDataSend;
    void          *pvRecvResult = &postDataRecv;
    char          *pchIPAddress;
    char          *pchPort;

    printf("%s postData, %d total bytes, %d send bytes at a time, %d recv bytes at at time, %d us delay between sends, close after %d bytes send, after %d bytes recv, test #%d\n", strTime(chTime), iHowMuch, iSendBuffer, iRecvBuffer, iuDelay, iCloseAfter, iCloseAfterRecv, iTestNumber);
    isocket = socket(AF_INET, SOCK_STREAM, 0);
    if (isocket < 0) {
        printf("%s Error in simple socket call: %s\n", strTime(chTime), strerror(errno));
        return(0);
    }
    memset(&ssockaddr_in,0,sizeof(ssockaddr_in));
    pchIPAddress = getenv("LSIPADDR");
    if (pchIPAddress) {
        printf("IPAddress overridden by env variable LSIPADDR: %s\n",pchIPAddress);
    }
    else {
        pchIPAddress = (char *)"127.0.0.1";
    }
    pchPort = getenv("LSPORT");
    if (pchPort) {
        printf("Port overridden by env variable LSPORT: %s\n",pchPort);
    }
    else {
        pchPort = (char *)"8088";
    }
    ssockaddr_in.sin_family = AF_INET;
    ssockaddr_in.sin_port = htons(atoi(pchPort));
    if (inet_pton(AF_INET, pchIPAddress, &ssockaddr_in.sin_addr) < 0) {
        printf("%s Error in getting address of IPAddress: %s: %s\n", strTime(chTime), pchIPAddress, strerror(errno));
        close(isocket);
        return(0);
    }
    if (connect(isocket, (struct sockaddr *)&ssockaddr_in, sizeof(ssockaddr_in)) < 0) {
        printf("%s Error in connect to server: %s\n", strTime(chTime), strerror(errno));
        close(isocket);
        return(0);
    }
    if (setsockopt(isocket, IPPROTO_TCP, TCP_NODELAY, (char *)&iflag, sizeof(int)) < 0) {
        printf("%s Error in disabling nagle: %s\n",strTime(chTime), strerror(errno));
        close(isocket);
        return(0);
    }
    sprintf(header,"POST /mtpostmt2?sendBuffer=%d&recvBuffer=%d&pid=%d&testNumber=%d HTTP/1.1\r\nContent-Type:text/html\r\nContent-Length:%d\r\n\r\n",iRecvBuffer,iSendBuffer,(int)getpid(),iTestNumber,iHowMuch);
    if (send(isocket, header, strlen(header), 0) < 0) {
        printf("%s Error in send of header: %s\n",strTime(chTime), strerror(errno));
        close(isocket);
        return(0);
    }
    iTestNumber++;
    memset(&postDataSend,0,sizeof(postDataSend));
    postDataSend.m_isocket      = isocket;
    postDataSend.m_iBuffer      = iSendBuffer;
    postDataSend.m_iHowMuch     = iHowMuch;
    postDataSend.m_iuDelay      = iuDelay;
    postDataSend.m_iCloseAfter  = iCloseAfter;
    postDataSend.m_iCloseAfterRecv = iCloseAfterRecv;
    pthread_attr_init(&pthread_attr1);
    if (pthread_create(&pthread1,&pthread_attr1,sendThread,&postDataSend) < 0) {
        pthread_attr_destroy(&pthread_attr1);
        printf("%s Error in creating send thread: %s\n",strTime(chTime), strerror(errno));
        return(0);
    }
    pthread_attr_destroy(&pthread_attr1);
    if (!iCloseAfter) {
        memset(&postDataRecv,0,sizeof(postDataRecv));
        postDataRecv.m_isocket      = isocket;
        postDataRecv.m_iBuffer      = iRecvBuffer;
        postDataRecv.m_iHowMuch     = iHowMuch;
        postDataRecv.m_iuDelay      = iuDelay;
        postDataRecv.m_iCloseAfter  = iCloseAfter;
        postDataRecv.m_iCloseAfterRecv = iCloseAfterRecv;
        pthread_attr_init(&pthread_attr2);
        if (pthread_create(&pthread2,&pthread_attr2,recvThread,&postDataRecv) < 0) {
            pthread_attr_destroy(&pthread_attr2);
            printf("%s Error in creating recv thread: %s\n",strTime(chTime), strerror(errno));
            return(0);
        }
        pthread_attr_destroy(&pthread_attr2);
    }
    printf("Waiting on send to finish\n");
    if (pthread_join(pthread1,&pvSendResult) == -1) {
        printf("%s Error in waiting on send thread: %s\n",strTime(chTime), strerror(errno));
        pvSendResult = NULL;
    }
    printf("Send finished\n");
    if ((iCloseAfter) && (!iCloseAfterRecv)) {
        printf("%s Early close so shutdown and close socket\n", strTime(chTime));
        setsockopt(isocket, IPPROTO_TCP, TCP_NODELAY, (char *)&iflag, sizeof(int)); // Supposed to force a flush
        sleep(1);
        close(isocket);
        pvRecvResult = pvSendResult;
    }
    else if (!iCloseAfter) {
        printf("Waiting for receive to finish\n");
        if (pthread_join(pthread2,&pvRecvResult) == -1) {
            printf("%s Error in waiting on recv thread: %s\n",strTime(chTime), strerror(errno));
            pvRecvResult = NULL;
        }
        printf("%s Receive finished\n", strTime(chTime));
        setsockopt(isocket, IPPROTO_TCP, TCP_NODELAY, (char *)&iflag, sizeof(int)); // Supposed to force a flush
        sleep(1);
        close(isocket);
    }
    if (!pvSendResult) {
        printf("Send result shows failure\n");
    }
    if (!pvRecvResult) {
        printf("Recv result shows failure\n");
    }
    return((int)(pvSendResult && pvRecvResult));
}




int main(void) {
    signal(SIGPIPE,SIG_IGN); // So we don't get terminated for something we're doing on purpose.
    printf("Testing some of George's suggestions for Multi-Threaded pounding\n");
    //postData(0,0,0,0,0); Not an interesting test.
    if (!postData(1,1,1,0,0,0)) {
        printf("Failed simple one byte send/recv test\n");
        return(1);
    }
    if (!postData(10000000,100000,100000,0,0,0)) {
        printf("Failed 10MB, 100K send/recv test\n");
        return(1);
    }
    if (!postData(2,1,1,0,1,0)) {
        printf("Failed first send interruption test\n");
        return(1);
    }
    if (!postData(2,1,1,0,0,1)) {
        printf("Failed first recv interruption test\n");
        return(1);
    }
    if (!postData(2,1,1,0,1,1)) {
        printf("Failed send/recv interruption test\n");
        return(1);
    }
    if (!postData(100,1,1,0,0,0)) {
        printf("Failed 100 byte, 1 byte at a time with delay test\n");
        return(1);
    }
    if (!postData(100,1,1,10000,0,0)) {
        printf("Failed 100 byte, 1 byte at a time with longer delay test\n");
        return(1);
    }
    if (!postData(10000000,8192,8192,0,1000000,0)) {
        printf("Failed 8192 send/recv with send interrupt test\n");
        return(1);
    }
    if (!postData(10000000,8192,8192,0,0,1000000)) {
        printf("Failed 8192 send/recv with recv interrupt test\n");
        return(1);
    }
    if (!postData(10000000,20000,30000,10,0,0)) {
        printf("Failed odd size buffer with delay test\n");
        return(1);
    }
    if (!postData(1000000000,65536,32768,0,0,0)) {
        printf("Failed first big test\n");
        return(1);
    }
    if (!postData(1000000000,1000000,1000000,0,0,0)) {
        printf("Failed big buffer big data test\n");
        return(1);
    }
    if (!postData(2,1,1,0,0,1)) {
        printf("Failed recv interruption test (2)\n");
        return(1);
    }
    if (!postData(2000000000,10000000,10000000,0,0,0)) {
        printf("Failed biggest data test\n");
        return(1);
    }
    printf("Succeeded all tests\n");
    return(0);
}
