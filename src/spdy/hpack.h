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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <util/loopbuf.h>

#define INITIAL_DYNAMIC_TABLE_SIZE  4096
#define HPackStaticTableCount   61

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

class DynamicTableEntry
{
    DynamicTableEntry(const DynamicTableEntry &rhs);
    DynamicTableEntry &operator=(const DynamicTableEntry &rhs);

public:
    DynamicTableEntry() { reset();  };
    DynamicTableEntry(char *name, uint32_t name_len, char *val,
                      uint32_t val_len, uint32_t nameIndex)
    {
        reset();
        init(name, name_len, val, val_len, nameIndex);
    }

    ~DynamicTableEntry()
    {
        if (m_valLen && m_val)
            delete []m_val;
        if (m_nameLen && m_nameStxTabId == 0
            && m_name)  //only when name in static table, use a pointer to it, otherwise new one
            delete []m_name;
        reset();
    };

    //return 0: no match, 1: name match, 2: both match
    int getMatchState(char *name, uint16_t name_len, char *val,
                      uint16_t val_len)
    {
        int state = 0;
        if (m_nameLen == name_len && memcmp(m_name, name, name_len) == 0)
        {
            state = 1;
            if (val_len == m_valLen  && memcmp(m_val, val, val_len) == 0)
                state = 2;
        }
        return state;
    }

    uint32_t getEntrySize() { return m_valLen + m_nameLen + 32; }
    char       *getName()           { return m_name;   }
    uint16_t    getNameLen()        { return m_nameLen; }
    char       *getValue()          { return m_val;     }
    uint16_t    getValueLen()       { return m_valLen; }

    void init(char *name, uint32_t name_len, char *val, uint32_t val_len,
              uint32_t nameIndex);

protected:
    char       *m_name;
uint8_t     m_nameStxTabId  :
    6;  // < 64, if in StxTab, should be from 1 to 61, 0 means not in it.
    uint16_t    m_nameLen       : 13; // < 8192
    uint16_t    m_valLen        : 13; // < 8192
    char       *m_val;

    void reset() { memset(&m_name, 0, (char *)&m_val + sizeof(char *) - (char *)&m_name); }
};


#define ENTRYPSIZE (sizeof(DynamicTableEntry *))
class HPackDynamicTable
{
public:
    size_t getTotalTableSize()  { return m_curCapacity; }
    size_t getEntryCount()      { return (uint32_t)(dynamicTableLoopbuf.size()) / ENTRYPSIZE; }
    void updateMaxCapacity(size_t maxCapacity)
    {
        m_maxCapacity = maxCapacity;
        removeOverflowEntries();
    }

    void reset()
    {
        int count = getEntryCount();
        for (int i = 0; i < count; ++i)
            delete getEntry(i);

        dynamicTableLoopbuf.clear();
        m_curCapacity = 0;
        m_maxCapacity = INITIAL_DYNAMIC_TABLE_SIZE;
    }

public:
    HPackDynamicTable()     {   reset(); };
    ~HPackDynamicTable()    {   reset(); };

    //through the name to search the entry, return enrey * and index of table(base HPackStaticTableCount + 1)
    DynamicTableEntry *find(char *name, uint16_t name_len, char *value,
                            uint16_t value_len, int &index)
    {
        int state;
        index = 0;  //inited it.
        DynamicTableEntry *pEntry;
        int count = getEntryCount();

        for (int i = 0; i < count; ++i)
        {
            pEntry = getEntry(i);
            state = pEntry->getMatchState(name, name_len, value, value_len);
            if (state == 2)
            {
                index = count - i + HPackStaticTableCount;
                return pEntry;
            }
            else if (state == 1)
            {
                //If only the name match, use the lowest index
                if (index == 0)
                    index = count - i + HPackStaticTableCount;
            }
        }
        return NULL;
    }

    DynamicTableEntry *getEntryByTabId(uint32_t id);

    void popEntry() //remove oldest
    {
        DynamicTableEntry *pEntry = getEntry(0);
        dynamicTableLoopbuf.pop_front(ENTRYPSIZE);
        m_curCapacity -= pEntry->getEntrySize();
        delete pEntry;
    }

    /****
     * the new one will be append to the end of the loopbuf, so the index need to be paied more attention.
     */
    void pushEntry(char *name, uint16_t name_len, char *val, uint16_t val_len,
                   uint32_t nameIndex)
    {
        DynamicTableEntry *pEntry = new DynamicTableEntry(name, name_len, val,
                val_len, nameIndex);
        dynamicTableLoopbuf.append((char *)(&pEntry), ENTRYPSIZE);
        m_curCapacity += pEntry->getEntrySize();
        removeOverflowEntries();
    }


protected:
    size_t      m_maxCapacity;  //set by SETTINGS_HEADER_TABLE_SIZE
    size_t      m_curCapacity;
    LoopBuf
    dynamicTableLoopbuf; // it contains DynamicTableEntry * as each chars
    void        removeOverflowEntries();

    DynamicTableEntry *getEntry(int index)
    {
        if (index < 0 || index * (int)ENTRYPSIZE >= dynamicTableLoopbuf.size())
            return NULL;
        DynamicTableEntry **pEntry = (DynamicTableEntry **)(
                                         dynamicTableLoopbuf.getPointer(index * ENTRYPSIZE));
        return *pEntry;
    }

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

public:
    static size_t huffmanEncBufSize(const unsigned char *src,
                                    const unsigned char *src_end);
    static int huffmanEnc(const unsigned char *src,
                          const unsigned char *src_end, unsigned char *dst, int dst_len);
    static unsigned char *huffmanDec4bits(uint8_t src_4bits,
                                          unsigned char *dst, HPackHuffDecodeStatus_t &status);
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

    HPackDynamicTable &getReqDynamicTable()  { return m_reqDynTab;    }
    HPackDynamicTable &getRespDynamicTable() { return m_respDynTab;   }

    static int getStaticTableId(char *name, uint16_t name_len, char *val,
                                uint16_t val_len, int &val_matched);


    static unsigned char *encInt(unsigned char *dst, uint32_t value,
                                 uint32_t prefix_bits);
    static uint32_t decInt(unsigned char *&src, const unsigned char *src_end,
                           uint32_t prefix_bits);
    static int encStr(unsigned char *dst, size_t dst_len,
                      const unsigned char *str, uint16_t str_len);
    static int decStr(unsigned char *dst, size_t dst_len, unsigned char *&src,
                      const unsigned char *src_end);

    //indexedType: 0, Add, 1,: without, 2: never
    unsigned char *encHeader(unsigned char *dst, unsigned char *dstEnd,
                             char *name, uint16_t nameLen, char *value, uint16_t valueLen,
                             int indexedType = 0);
    int decHeader(unsigned char *&src, unsigned char *srcEnd, char *name,
                  uint16_t &name_len, char *val, uint16_t &val_len);


protected:
    HPackDynamicTable m_reqDynTab;
    HPackDynamicTable m_respDynTab;

};

#endif // HPACK_H
