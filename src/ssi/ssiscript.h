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
#ifndef SSISCRIPT_H
#define SSISCRIPT_H


#include <lsdef.h>
#include <http/expression.h>
#include <util/autobuf.h>

#include <util/autostr.h>
#include <util/tlinklist.h>


class HttpSession;
class SsiTagConfig;
class LinkedObj;
class SsiBlock;
class Ssi_If;
class SubstFormat;
class Pcregex;
class VMemBuf;

enum
{
    SSI_ATTR_NONE,
    SSI_ATTR_ECHOMSG,
    SSI_ATTR_ERRMSG,
    SSI_ATTR_SIZEFMT,
    SSI_ATTR_TIMEFMT,
    SSI_ATTR_ECHO_VAR,
    SSI_ATTR_ENCODING,
    SSI_ATTR_EXEC_CGI,
    SSI_ATTR_EXEC_CMD,
    SSI_ATTR_INC_FILE,
    SSI_ATTR_INC_VIRTUAL,
    SSI_ATTR_SET_VALUE,
    SSI_ATTR_EXPR,
    SSI_ATTR_SET_VAR,
    SSI_ATTR_ENC_NONE,
    SSI_ATTR_ENC_URL,
    SSI_ATTR_ENC_ENTITY,
    SSI_ATTR_COUNT
};


class SsiComponent : public LinkedObj
{
public:
    enum
    {
        SSI_String,
        SSI_Config,
        SSI_Echo,
        SSI_Exec,
        SSI_FSize,
        SSI_Flastmod,
        SSI_Include,
        SSI_Printenv,
        SSI_Set,
        SSI_If,
        SSI_Else,
        SSI_True = SSI_Else,
        SSI_Elif,
        SSI_Endif,
        SSI_Block,
        SSI_Switch,
        SSI_False
    };

    SsiComponent()
        : m_iContentOffset(0)
        , m_iContentLen(0)
        , m_iType(0)
        , m_pAttrs(NULL)
    {}
    virtual ~SsiComponent();


    int getType() const     {   return m_iType; }
    void setType(int type) {   m_iType = type; }
//     AutoBuf *getContentBuf()   {   return &m_content;  }

    const char *getContent(const char *pBegin) const
    {   return pBegin + m_iContentOffset;     }

    int getContentOffset() const    {   return m_iContentOffset;    }
    int getContentLen() const       {   return m_iContentLen;       }

    int getContentEndOffset() const {   return m_iContentOffset + m_iContentLen;   }

    void setContentOffset(int iOffset)    {   m_iContentOffset = iOffset;     }
    void setContentLen(int iLen)          {   m_iContentLen = iLen;           }

    void appendParsed(LinkedObj *p);

    LinkedObj *getFirstAttr() const
    {   return m_pAttrs;       }

    int isConditional() const
    {
        switch (m_iType)
        {
        case SSI_If:
        case SSI_Else:
        case SSI_Elif:
        case SSI_False:
            return 1;
        }
        return 0;
    }

    int isWrapper() const
    {   return (m_iType == SSI_Switch);     }

private:
//     AutoBuf m_content;
    int         m_iContentOffset;
    int         m_iContentLen;
    int         m_iType;
    LinkedObj  *m_pAttrs;
};

// class Ssi_If : public SsiComponent
// {
// public:
//     Ssi_If();
//     ~Ssi_If();
//     void setIfBlock(SsiBlock *pBlock)    {   m_blockIf = pBlock;     }
//     void setElseBlock(SsiBlock *pBlock)  {   m_blockElse = pBlock;   }
//
//     SsiBlock *getIfBlock() const       {   return m_blockIf;   }
//     SsiBlock *getElseBlock() const     {   return m_blockElse; }
//
// private:
//     SsiBlock *m_blockIf;
//     SsiBlock *m_blockElse;
//
//
// };


class SsiBlock : public TLinkList<SsiComponent>, public SsiComponent
{
public:
    SsiBlock()
        : m_pParentBlock(NULL)
        , m_iCloseOffset(0)
        , m_iCloseLen(0)
    {}
    SsiBlock(SsiBlock *pParent)
        : m_pParentBlock(pParent)
        , m_iCloseOffset(0)
        , m_iCloseLen(0)
    {}


    ~SsiBlock() {   release_objects();  }

    SsiBlock       *getParentBlock() const   {   return m_pParentBlock;  }

    SsiComponent   *getFirstComponent() const {  return begin();         }

    void setCloseOffset(int offset, int len)
    {
        m_iCloseOffset = offset;
        m_iCloseLen = len;
    }

    int getCloseOffset() const  {   return m_iCloseOffset;  }

private:

    SsiBlock   *m_pParentBlock;
    int         m_iCloseOffset;
    int         m_iCloseLen;
//     SsiBlock()
//         : m_pParentBlock(NULL)
//         , m_pParentComp(NULL)
//     {}
//     SsiBlock(SsiBlock *pBlock, Ssi_If *pComp)
//         : m_pParentBlock(pBlock)
//         , m_pParentComp(pComp)
//     {}
//
//     ~SsiBlock() {   release_objects();  }
//
//     SsiBlock      *getParentBlock() const   {   return m_pParentBlock;  }
//     Ssi_If        *getParentComp() const    {   return m_pParentComp;   }
// private:
//     SsiBlock *m_pParentBlock;
//     Ssi_If    *m_pParentComp;

};

class SsiScript
{
public:
    SsiScript();

    ~SsiScript();

    int parse(SsiTagConfig *pConfig, const char *pScriptPath);


//     void setContent(VMemBuf *pBuf, int release)
//     {
//         m_pContent = pBuf;
//         if (release)
//             m_iFlag |= FLAG_RELEASE_CONTENT;
//     }
    VMemBuf *getContent() const            {   return m_pContent;      }


    const SsiBlock *getMainBlock() const    {   return &m_main;     }
    SsiBlock *getMainBlock()                {   return &m_main;     }

//     void resetRuntime()
//     {
//         m_pCurBlock = &m_main;
//         m_pCurComponent = (SsiComponent *)m_main.head()->next();
//     }

    SsiComponent *getCurrentComponent() const
    {   return m_pCurComponent;     }

    const AutoStr2 *getPath() const
    {   return &m_sPath;     }

    void setPath(const char *pPath, int len)
    {   m_sPath.setStr(pPath, len);     }

    void setStatusCode(int code)   {   m_iParserState = code;      }
    int  getStatusCode() const      {   return m_iParserState;      }

    long getLastMod() const         {   return m_lModify;           }

    static int testParse();

//     const SsiComponent *nextComponent(const SsiComponent  *&pComponent,
//                                       const SsiBlock      *&pCurBlock) const;
private:
    int processSsiFile(SsiTagConfig *pConfig, int fd, struct stat *pStat);
    int parse(SsiTagConfig *pConfig, const char *pBegin, const char *pEnd,
              int finish, int curOffset);
    int appendHtmlContent(int offset, int len);
    int parseSsiDirective(const char *pBegin, const char *pEnd);

    int parseIf(int cmd, const char *pBegin, const char *pEnd);
    int parseAttrs(int cmd, const char *pBegin, const char *pEnd);

    int getAttr(const char *&pBegin, const char *pEnd,
                char *pAttrName, const char *&pValue,
                int &valLen);

    void setCurrentBlock(SsiBlock *pBlock);
//     Ssi_If *getComponentIf(SsiBlock *&pBlock);


    AutoStr2    m_sPath;
    int         m_iParserState;
    long        m_lModify;
    long        m_lSize;

    SsiBlock        m_main;
    SsiComponent   *m_pCurComponent;
    SsiBlock       *m_pCurBlock;
    VMemBuf        *m_pContent;
};

#endif
