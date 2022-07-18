/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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

char *pchSendBuffer = NULL;
int  iSendBuffer = 0;

int postData(int iHowMuch, int iBuffer, int iDelay, int iCloseAfter)

{
    int isocket;
    int iflag = 1;
    struct sockaddr_in ssockaddr_in;
    char header[256];
    int isent;

    printf("postData, %d total bytes, %d bytes at a time, %d delay between sends, close after %d bytes\n", iHowMuch, iBuffer, iDelay, iCloseAfter);
    if (pchSendBuffer) {
        free(pchSendBuffer);
        pchSendBuffer = NULL;
    }
    if (!iBuffer)
        return 0;
    if (iBuffer) {
        pchSendBuffer = (char *)malloc(iBuffer);
        if (!pchSendBuffer) {
            printf("Error allocating data, %d bytes\n",iBuffer);
            return(0);
        }
    }
    isocket = socket(AF_INET, SOCK_STREAM, 0);
    if (isocket < 0) {
        printf("Error in simple socket call: %s\n", strerror(errno));
        return(0);
    }
    memset(&ssockaddr_in,0,sizeof(ssockaddr_in));
    ssockaddr_in.sin_family = AF_INET;
    ssockaddr_in.sin_port = htons(8088);
    if (inet_pton(AF_INET, "127.0.0.1", &ssockaddr_in.sin_addr) < 0) {
        printf("Error in getting address of localhost: %s\n", strerror(errno));
        close(isocket);
        return(0);
    }
    if (connect(isocket, (struct sockaddr *)&ssockaddr_in, sizeof(ssockaddr_in)) < 0) {
        printf("Error in connect to server: %s\n", strerror(errno));
        close(isocket);
        return(0);
    }
    if (setsockopt(isocket, IPPROTO_TCP, TCP_NODELAY, (char *)&iflag, sizeof(int)) < 0) {
        printf("Error in disabling nagle: %s\n",strerror(errno));
        close(isocket);
        return(0);
    }
    sprintf(header,"POST /mtpostmt HTTP/1.1\r\nContent-Type:text/html\r\nContent-Length:%d\r\n\r\n",iHowMuch);
    if (send(isocket, header, strlen(header), 0) < 0) {
        printf("Error in send of header: %s\n",strerror(errno));
        close(isocket);
        return(0);
    }
    if (iBuffer) {
        memset(pchSendBuffer,'0',iBuffer);
    }
    for (int i = 0; i < iHowMuch; i += isent) {
        isent = send(isocket, pchSendBuffer, (iBuffer > (iHowMuch - i)) ? (iHowMuch - i) : iBuffer, 0);
        if (isent < 0) {
            printf("Error in send of byte %d of %d: %s\n", i, iHowMuch, strerror(errno));
            break;
        }
        if ((iCloseAfter) && (i + isent >= iCloseAfter)) {
            printf("Breaking connection early after sending %d bytes\n", i + isent);
            break;
        }
        if (iDelay) {
            sleep(iDelay);
        }
    }
    close(isocket);
    return(1);
}
int main(void) {
    printf("Testing some of George's suggestions for Multi-Threaded pounding\n");
    postData(0,0,0,0);
    postData(1,1,0,0);
    postData(10000000,100000,0,0);
    postData(2,1,0,1);
    postData(100,1,0,0);
    postData(100,1,1,0);
    postData(10000000,8192,0,1000000);
    return(0);
}
