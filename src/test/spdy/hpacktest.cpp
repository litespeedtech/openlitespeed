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
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"
#include "spdy/hpack.h"
#include <stdlib.h>
#include <stdio.h>

void addEntry( HPackDynamicTable &dynTab, const char *name, const char *value, uint8_t nameId )
{
    dynTab.pushEntry((char *)name, strlen(name), (char *)value, strlen(value), nameId);
}

void printTable( HPackDynamicTable &dynTab)
{
    printf("Dynamic Table : \n");
    int count = dynTab.getEntryCount();
    for (int i= 1 ; i<=count; ++i)
    {
        int tabId = i + HPackStaticTableCount;
        DynamicTableEntry *pEntry = (DynamicTableEntry *)dynTab.getEntryByTabId(tabId);
        
        printf("[%3d]  (s = %3d) %s: %s\n",
               i, pEntry->getEntrySize(), pEntry->getName(), pEntry->getValue());
    }
    printf("\tTable size: %d\n", dynTab.getTotalTableSize());
}

TEST(hapck_test_1)
{
    HPackDynamicTable dynTable;
    dynTable.updateMaxCapacity(256);

    addEntry( dynTable, ":authority", "www.example.com", 1);
    printTable( dynTable);
    
    addEntry( dynTable, "cache-control", "no-cache", 24);
    printTable( dynTable);
    
    addEntry( dynTable, "custom-key", "custom-value", 0);
    printTable( dynTable);
    
    addEntry( dynTable, "custom-key2", "custom-value22321grotretretuerotiueroituerotureotuouertoueirtoeirutoierutoeirt3213123123213213213213", 0);
    printTable( dynTable);
    
//    int index = 0;
//     DynamicTableEntry *pEntry = dynTable.find((char *)"custom-key", sizeof("custom-key") -1, index);
//     if (pEntry)
//         dynTable.removeEntry(pEntry);
    printTable( dynTable);
    
    dynTable.reset();
    printTable( dynTable);
    dynTable.updateMaxCapacity(256);
    printTable( dynTable);
    
    addEntry( dynTable, ":status", "302", 0);
    addEntry( dynTable, "cache-control", "private", 24);
    addEntry( dynTable, "date", "Mon, 21 Oct 2013 20:13:21 GMT", 33);
    addEntry( dynTable, "location", "https://www.example.com", 46);
    printTable( dynTable);
    
    addEntry( dynTable, ":status", "307", 0);
    printTable( dynTable);
    
    addEntry( dynTable, "date", "Mon, 21 Oct 2013 20:13:22 GMT", 33);
    printTable( dynTable);
    
    addEntry( dynTable, "content-encoding", "gzip", 0);
    printTable( dynTable);
    
    addEntry( dynTable, "set-cookie", "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", 55);
    printTable( dynTable);
    
    dynTable.updateMaxCapacity(110);
    printTable( dynTable);
    
    dynTable.updateMaxCapacity(256);
    printTable( dynTable);
    
    
    
    
    
    
    
    printTable( dynTable);
}

void testNameValue(const char *name, const char *val, int result, int val_match_result)
{
    int val_match;
    int index = Hpack::getStaticTableId( (char *)name, (uint16_t)strlen(name) , (char *)val, (uint16_t)strlen(val), val_match);
    printf("name: %s, val: %s, index = %d\n", name, val, index);
    CHECK( index == result && val_match == val_match_result);
}


TEST(hapck_test_2)
{
    testNameValue(":authority", "www.example.com", 1, 0);
    testNameValue(":authority1", "www.example.com", 0, 0);
    testNameValue(":authority", "www.example.com", 1, 0);
    testNameValue(":authority1", "www.example.com", 0, 0);
    
    testNameValue(":method", "GET", 2, 1);
    testNameValue(":method", "gET", 2, 1);
    testNameValue(":method", "PURGE", 2, 0);
    testNameValue(":method", "POST", 3, 1);
    
    testNameValue(":scheme", "http", 6, 1);
    testNameValue(":scheme", "HTTP", 6, 1);
    testNameValue(":scheme", "https", 7, 1);
    testNameValue(":scheme", "httPS", 7, 1);
    
//     testNameValue("scheme", "http", 0, 0);
//     testNameValue("scheme", "https", 0, 0);
    
    testNameValue(":status", "200", 8, 1);
    testNameValue(":status", "401", 8, 0);
    testNameValue(":status", "500", 14, 1);
//    testNameValue(":status-xxx", "200", 0, 0);
    
    testNameValue("accept-encoding", "gzip, deflate", 16, 1);
    testNameValue("accept-encoding", "gzip", 16, 0);

    testNameValue("accept-charset", "en", 15, 0);
    testNameValue("accept-charset", "EN", 15, 0);
    
    testNameValue("age", "100", 21, 0);
    testNameValue("age", "10000", 21, 0);
    
    testNameValue("www-authenticate", "wwwewe", 61, 0);
    testNameValue("www-authenticate-", "dtertete", 0, 0);
    
}

void displayHeader(unsigned char *buf, int len)
{
    int i;
    for ( i =0;i<len /16; ++i)
    {
        printf("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x | \n",
                buf[i * 16 + 0],buf[i * 16 + 1],buf[i * 16 + 2],buf[i * 16 + 3],
                buf[i * 16 + 4],buf[i * 16 + 5],buf[i * 16 + 6],buf[i * 16 + 7],
                buf[i * 16 + 8],buf[i * 16 + 9],buf[i * 16 + 10],buf[i * 16 + 11],
                buf[i * 16 + 12],buf[i * 16 + 13],buf[i * 16 + 14],buf[i * 16 + 15]);
    }
    
    i *= 16;
    for (; i<len; ++i)
    {
        printf( ((i % 2 == 0) ? "%02x" : "%02x "), buf[i]);
    }
    printf("\n");

}

#define STR_TO_IOVEC_TEST(a) ((char *)a), (sizeof(a) -1)
TEST(hapck_test_RFC_EXample)
{
    unsigned char respBuf[8192] = {0};
    unsigned char * respBufEnd = respBuf + 8192;
    Hpack hpack;
    hpack.getRespDynamicTable().updateMaxCapacity(256);
    
    unsigned char *pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("302"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("private"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("date"), STR_TO_IOVEC_TEST("Mon, 21 Oct 2013 20:13:21 GMT"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("location"), STR_TO_IOVEC_TEST("https://www.example.com"));
    displayHeader(respBuf, pBuf - respBuf);
    printTable( hpack.getRespDynamicTable());
    char bufSample1[] = 
"\x48\x82\x64\x02\x58\x85\xae\xc3\x77\x1a\x4b\x61\x96\xd0\x7a\xbe"
"\x94\x10\x54\xd4\x44\xa8\x20\x05\x95\x04\x0b\x81\x66\xe0\x82\xa6"
"\x2d\x1b\xff\x6e\x91\x9d\x29\xad\x17\x18\x63\xc7\x8f\x0b\x97\xc8"
"\xe9\xae\x82\xae\x43\xd3";
    CHECK(memcmp(bufSample1, respBuf, pBuf - respBuf) == 0);
    
    
    pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("307"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("private"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("date"), STR_TO_IOVEC_TEST("Mon, 21 Oct 2013 20:13:21 GMT"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("location"), STR_TO_IOVEC_TEST("https://www.example.com"));
    displayHeader(respBuf, pBuf - respBuf);
    printTable( hpack.getRespDynamicTable());
    char bufSample2[] = "\x48\x83\x64\x0e\xff\xc1\xc0\xbf";
    CHECK(memcmp(bufSample2, respBuf, pBuf - respBuf) == 0);
    
    pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("200"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("private"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("date"), STR_TO_IOVEC_TEST("Mon, 21 Oct 2013 20:13:22 GMT"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("location"), STR_TO_IOVEC_TEST("https://www.example.com"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("content-encoding"), STR_TO_IOVEC_TEST("gzip"));
    printTable( hpack.getRespDynamicTable());
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("set-cookie"), STR_TO_IOVEC_TEST("foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1"));
    printTable( hpack.getRespDynamicTable());
    displayHeader(respBuf, pBuf - respBuf);
    
    
    char bufSample3[] = 
"\x88\xc1\x61\x96\xd0\x7a\xbe\x94\x10\x54\xd4\x44\xa8\x20\x05\x95"
"\x04\x0b\x81\x66\xe0\x84\xa6\x2d\x1b\xff\xc0\x5a\x83\x9b\xd9\xab"
"\x77\xad\x94\xe7\x82\x1d\xd7\xf2\xe6\xc7\xb3\x35\xdf\xdf\xcd\x5b"
"\x39\x60\xd5\xaf\x27\x08\x7f\x36\x72\xc1\xab\x27\x0f\xb5\x29\x1f"
"\x95\x87\x31\x60\x65\xc0\x03\xed\x4e\xe5\xb1\x06\x3d\x50\x07";
    CHECK(memcmp(bufSample3, respBuf, pBuf - respBuf) == 0);
    
    
    
    /****************************
     * decHeader testing
     * 
     ****************************/
    unsigned char *bufSamp4 = (unsigned char *)"\x82\x86\x84\x41\x8c\xf1\xe3\xc2\xe5\xf2\x3a\x6b\xa0\xab\x90\xf4\xff";
    unsigned char *pSrc = bufSamp4;
    unsigned char* bufEnd =  bufSamp4 + strlen((const char *)bufSamp4);
    int rc;
    char name[1024];
    char val[1024];
    uint16_t name_len=1024;
    uint16_t val_len = 1024;
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());
    
    
    unsigned char *bufSamp = (unsigned char *)"\x82\x86\x84\xbe\x58\x86\xa8\xeb\x10\x64\x9c\xbf";
    pSrc = bufSamp;
    bufEnd =  bufSamp + strlen((const char *)bufSamp);
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());
    
    bufSamp = (unsigned char *)"\x82\x87\x85\xbf\x40\x88\x25\xa8\x49\xe9\x5b\xa9\x7d\x7f\x89\x25\xa8\x49\xe9\x5b\xb8\xe8\xb4\xbf";
    pSrc = bufSamp;
    bufEnd =  bufSamp + strlen((const char *)bufSamp);
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());
    
}


TEST(hapck_self_enc_dec_test)
{
    unsigned char respBuf[8192] = {0};
    unsigned char * respBufEnd = respBuf + 8192;
    Hpack hpack;
    hpack.getRespDynamicTable().updateMaxCapacity(256);
    hpack.getReqDynamicTable().updateMaxCapacity(256);
 
    //1:
    unsigned char *pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("200"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("public, max-age=1000"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("date"), STR_TO_IOVEC_TEST("Mon, 21 Oct 2013 20:13:23 GMT"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("allow"), STR_TO_IOVEC_TEST("*.*"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("location"), STR_TO_IOVEC_TEST("https://www.example.com"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key"), STR_TO_IOVEC_TEST("my-test-values1111"));
    displayHeader(respBuf, pBuf - respBuf);
    printTable( hpack.getRespDynamicTable());
    
    
     /****************************
     * decHeader testing
     ****************************/
    unsigned char *pSrc = respBuf;
    unsigned char* bufEnd =  pBuf;
    int rc;
    char name[1024];
    char val[1024];
    uint16_t name_len=1024;
    uint16_t val_len = 1024;
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());

    
    
 
    //2:
    pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("404"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("public, max-age=1000"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("allow"), STR_TO_IOVEC_TEST("*.*"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("from"), STR_TO_IOVEC_TEST("123456@mymail.com"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("host"), STR_TO_IOVEC_TEST("www.host.com"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("vary"), STR_TO_IOVEC_TEST("wsdsdsdfdsfues1111"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("via"), STR_TO_IOVEC_TEST("m"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("range"), STR_TO_IOVEC_TEST("1111"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key2"), STR_TO_IOVEC_TEST("my-test-values22222222222222"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key3333"), STR_TO_IOVEC_TEST("my-test-va3"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key44"), STR_TO_IOVEC_TEST("my-test-value444"));
    displayHeader(respBuf, pBuf - respBuf);
    printTable( hpack.getRespDynamicTable());
    
    
     /****************************
     * decHeader testing
     ****************************/
    pSrc = respBuf;
    bufEnd =  pBuf;
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());
    
    //3:
    pBuf = respBuf;
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST(":status"), STR_TO_IOVEC_TEST("307"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("cache-control"), STR_TO_IOVEC_TEST("public, max-age=1000"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("allow"), STR_TO_IOVEC_TEST("*.*.*.*"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("from"), STR_TO_IOVEC_TEST("123456@mymail.com"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("host"), STR_TO_IOVEC_TEST("www.host.com"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key3333"), STR_TO_IOVEC_TEST("my-test-va3"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("vary"), STR_TO_IOVEC_TEST("wsdsdsdfdsfues1111"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("via"), STR_TO_IOVEC_TEST("mmmmm"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key2"), STR_TO_IOVEC_TEST("my-test-values22222222222222"));
    pBuf = hpack.encHeader(pBuf, respBufEnd, STR_TO_IOVEC_TEST("my-test_key44"), STR_TO_IOVEC_TEST("my-test-value444"));
    displayHeader(respBuf, pBuf - respBuf);
    printTable( hpack.getRespDynamicTable());
    
    
     /****************************
     * decHeader testing
     ****************************/
    pSrc = respBuf;
    bufEnd =  pBuf;
    while((rc = hpack.decHeader(pSrc, bufEnd, name, name_len, val, val_len)) > 0)
    {
        name[name_len] = 0x00;
        val[val_len] = 0x00;
        printf("[%d %d]%s: %s\n", name_len, val_len, name, val);
        name_len=1024;
        val_len = 1024;
    }
    printTable( hpack.getReqDynamicTable());
    
    
    
    
}




#endif
