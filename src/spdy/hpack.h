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
#ifndef HPACK_H
#define HPACK_H

#include <util/loopbuf.h>
#include <util/ghash.h>
#include <util/autobuf.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define INITIAL_DYNAMIC_TABLE_SIZE  4096
#define HPackStxTabCount   61



enum
{
    HPACK_HUFFMAN_FLAG_ACCEPTED = 0x01,
    HPACK_HUFFMAN_FLAG_SYM = 0x02,
    HPACK_HUFFMAN_FLAG_FAIL = 0x04,
};

//static table
struct HPackHeaderTable_t
{
    const char *name;
    uint16_t name_len;
    const char *val;
    uint16_t val_len;
};

struct HPackHuffEncode_t
{
    uint32_t code;
    int      bits;
};

struct HPackHuffDecode_t
{
    uint8_t state;
    uint8_t flags;
    uint8_t sym;
};

struct HPackHuffDecodeStatus_t
{
    uint8_t state;
    uint8_t eos;
};

class DynTabEntry
{
public:
    DynTabEntry(char *name, uint32_t name_len, char *val,
                      uint32_t val_len, uint8_t stxTabId)
    {
        reset();
        init(name, name_len, val, val_len, stxTabId);
    }

    ~DynTabEntry()
    {
        if (m_valLen && m_val)
            delete []m_val;

        //only when name in static table, use a pointer to it, otherwise new one
        if (m_nameLen && m_nameStxTabId == 0 && m_name)
            delete []m_name;
        reset();
    };

    uint32_t getEntrySize()         { return m_valLen + m_nameLen + 32; }
    char       *getName() const     { return m_name;   }
    uint16_t    getNameLen()        { return m_nameLen; }
    char       *getValue() const    { return m_val;     }
    uint16_t    getValueLen()       { return m_valLen; }

    void init(char *name, uint32_t name_len, char *val, uint32_t val_len,
              uint8_t stxTabId);

private:
    char       *m_name;
    uint8_t     m_nameStxTabId  : 6;  // < 64, if in StxTab, is 1~61; other 0.
    uint16_t    m_nameLen       : 13; // < 8192
    uint16_t    m_valLen        : 13; // < 8192
    char       *m_val;

    void reset()
    {
        memset(&m_name, 0, (char *)&m_val + sizeof(char *) - (char *)&m_name);
    }

};


#define ENTRYPSIZE (sizeof(DynTabEntry *))
class HPackDynTab
{
public:
    HPackDynTab();
    ~HPackDynTab();


    size_t getTotalTableSize()  { return m_curCapacity; }
    size_t getEntryCount()      { return (uint32_t)(m_loopbuf.size()) / ENTRYPSIZE; }
    void updateMaxCapacity(size_t maxCapacity)
    {
        m_maxCapacity = maxCapacity;
        removeOverflowEntries();
    }
    void reset();

    int getDynTabId(char *name, uint16_t name_len, char *value, 
                    uint16_t value_len, int &val_matched, uint8_t stxTabId);

    DynTabEntry *getEntry(uint32_t dynTabId)
    {
        if (dynTabId < HPackStxTabCount + 1
            || dynTabId > HPackStxTabCount + getEntryCount())
            return NULL;
        return getEntryInternal(dynTabIdToInternalIndex(dynTabId));
    }

    void removeNameValueHashTEntry(DynTabEntry *pEntry);
    void removeNameHashTEntry(DynTabEntry *pEntry);
    void popEntry();
    void pushEntry(char *name, uint16_t name_len, char *val, uint16_t val_len,
                   uint32_t nameIndex);

public:
    static hash_key_t hfName(const void *__s);
    static hash_key_t hfNameVal(const void *__s);
    static int cmpName(const void *pVal1, const void *pVal2);
    static int cmpNameVal(const void *pVal1, const void *pVal2);

private:
    void        removeOverflowEntries();
    DynTabEntry *getEntryInternal(int index)
    {
//      if (index < 0 || index * (int)ENTRYPSIZE >= m_loopbuf.size())
//          return NULL;
        DynTabEntry **pEntry =
            (DynTabEntry **)(m_loopbuf.getPointer(index * ENTRYPSIZE));
        return *pEntry;
    }

    int dynTabIdToInternalIndex(uint32_t dynTabId)
    {
        return getEntryCount() - (dynTabId - HPackStxTabCount);
    }

private:
    size_t      m_maxCapacity;  //set by SETTINGS_HEADER_TABLE_SIZE
    size_t      m_curCapacity;
    uint32_t    m_nextFlowId;

    //It contains DynamicTableEntry * as each chars
    LoopBuf     m_loopbuf;
    GHash      *m_pNameHashT;
    GHash      *m_pNameValueHashT;

};


/*******************************************************************************************
 * Comments about huffman encode and decode
 * The encoding compression ratio is in the range of (0.625, 3.75), so that it is safe to
 * malloc the buffer
 * 1, times 4 of the original clear text buffer when encoding
 * 2, times 2 of the encoded text buffer when decoding
 *
*******************************************************************************************/

class HuffmanCode
{
public:
    HuffmanCode() {};
    ~HuffmanCode() {};

    static size_t calcHuffmanEncBufSize(const unsigned char *src,
                                    const unsigned char *src_end);
    static int huffmanEnc(const unsigned char *src, const unsigned char *src_end,
                          unsigned char *dst, int dst_len);
    static unsigned char *huffmanDec4bits(uint8_t src_4bits, unsigned char *dst,
                                          HPackHuffDecodeStatus_t &status);
    static int huffmanDec(unsigned char *src, int src_len, unsigned char *dst,
                          int dst_len);

    static HPackHuffEncode_t m_HPackHuffEncode_t[257];
    static HPackHuffDecode_t m_HPackHuffDecode_t[256][16];

};


class Hpack
{
public:
    Hpack() {};
    ~Hpack() {};

    HPackDynTab &getReqDynTab()  { return m_reqDynTab;    }
    HPackDynTab &getRespDynTab() { return m_respDynTab;   }

    static uint8_t getStxTabId(char *name, uint16_t name_len, char *val,
                                uint16_t val_len, int &val_matched);

    unsigned char *encInt(unsigned char *dst, uint32_t value,
                                 uint32_t prefix_bits);
    int decInt(unsigned char *&src, const unsigned char *src_end,
                           uint32_t prefix_bits, uint32_t& value);
    int encStr(unsigned char *dst, size_t dst_len,
                      const unsigned char *str, uint16_t str_len);
    int decStr(unsigned char *dst, size_t dst_len, unsigned char *&src,
                      const unsigned char *src_end);

    //indexedType: 0, Add, 1,: without, 2: never
    unsigned char *encHeader(unsigned char *dst, unsigned char *dstEnd,
                             char *name, uint16_t nameLen, char *value,
                             uint16_t valueLen, int indexedType = 0);
    int decHeader(unsigned char *&src, unsigned char *srcEnd, AutoBuf &nameValBuf,
                  uint16_t &name_len, uint16_t &val_len);


private:
    HPackDynTab m_reqDynTab;
    HPackDynTab m_respDynTab;

};

#endif // HPACK_H
