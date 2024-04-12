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
#include "ssiscript.h"
#include "ssiconfig.h"

#include <http/requestvars.h>
#include <log4cxx/logger.h>
#include <lsr/ls_fileio.h>
#include <util/blockbuf.h>
#include <util/gpointerlist.h>
#include <util/pcregex.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>

#include <pcreposix.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


static SubstFormat *parseFormat(const char *pBegin, int len)
{
    SubstFormat *pFormat = new SubstFormat();
    pFormat->parse(pBegin, pBegin, pBegin + len, 1);
    return pFormat;
}




SsiComponent::~SsiComponent()
{
    LinkedObj *pNext, *p1 = m_pAttrs;
    while (p1)
    {
        pNext = p1->next();
        //delete p1
        delete(SubstItem *)p1;
        p1 = pNext;
    }
}


void SsiComponent::appendParsed(LinkedObj *p)
{
    if (!p)
        return;
    if (!m_pAttrs)
        m_pAttrs = p;
    else
    {
        LinkedObj *p1 = m_pAttrs;
        while (p1->next())
            p1 = p1->next();
        p1->setNext(p);
    }
}


// Ssi_If::Ssi_If()
//     : m_blockIf(NULL)
//     , m_blockElse(NULL)
// {}
//
//
// Ssi_If::~Ssi_If()
// {
//     if (m_blockIf)
//         delete m_blockIf;
//     if (m_blockElse)
//         delete m_blockElse;
// }


SsiScript::SsiScript()
    : m_iParserState(0)
    , m_lModify(0)
    , m_lSize(0)
    , m_pCurComponent(NULL)
    , m_pCurBlock(NULL)
    , m_pContent(NULL)
{
}


SsiScript::~SsiScript()
{
    if (m_pContent)
        delete m_pContent;
}


int SsiScript::appendHtmlContent(int offset, int len)
{
    if (m_pCurComponent)
    {
        if (m_pCurComponent->getContentEndOffset() == offset)
        {
            m_pCurComponent->setContentLen(len + m_pCurComponent->getContentLen());
            return LS_OK;
        }
        else
            m_pCurComponent = NULL;
    }
    if ((!m_pCurComponent) ||
        (m_pCurComponent->getType() != SsiComponent::SSI_String))
    {
        m_pCurComponent = new SsiComponent();
        if (!m_pCurComponent)
            return LS_FAIL;
        m_pCurComponent->setType(SsiComponent::SSI_String);
        m_pCurBlock->append(m_pCurComponent);
    }
    //m_pCurComponent->getContentBuf()->append( pBegin, pEnd - pBegin );
    m_pCurComponent->setContentOffset(offset);
    m_pCurComponent->setContentLen(len);
    return LS_OK;
}


static const char *s_SSI_Cmd[] =
{
    "N/A",
    "config", "echo", "exec",
    "fsize", "flastmod", "include", "printenv",
    "set", "if", "else", "elif", "endif"
};
static int s_SSI_Cmd_len[] =
{   0, 6, 4, 4, 5, 8, 7, 8, 3, 2, 4, 4, 5 };

static const char *s_SSI_Attrs[] =
{
    "N/A", "echomsg", "errmsg", "sizefmt", "timefmt",
    "var", "encoding", "cgi", "cmd", "file", "virtual",
    "value", "expr", "var", "none", "url", "entity"
};

static int s_SSI_Attrs_len[] =
{   0, 7, 6, 7, 7, 3, 8, 3, 3, 4, 7, 5, 4, 3, 4, 3, 6  };


int SsiScript::getAttr(const char *&pBegin, const char *pEnd,
                       char *pAttrName, const char *&pValue, int &valLen)
{
    while ((pBegin < pEnd) && isspace(*pBegin))
        ++pBegin;
    if (pBegin >= pEnd)
        return -2;
    if (*pBegin == '=')
        return LS_FAIL;
    const char *pAttrEnd = (const char *)memchr(pBegin, '=', pEnd - pBegin);
    if (!pAttrEnd)
        return LS_FAIL;
    pValue = pAttrEnd + 1;
    while (isspace(pAttrEnd[ -1 ]))
        --pAttrEnd;
    if (pAttrEnd - pBegin > 80)
        return LS_FAIL;
    memmove(pAttrName, pBegin, pAttrEnd - pBegin);
    pAttrName[pAttrEnd - pBegin] = 0;

    while (isspace(*pValue))
        ++pValue;
    pAttrEnd = StringTool::strNextArg(pValue, NULL);
    if (!pAttrEnd)
        pAttrEnd = pEnd;
    valLen = pAttrEnd - pValue;
    pBegin = pAttrEnd + 1;
    return 0;

}


int SsiScript::parseAttrs(int cmd, const char *pBegin, const char *pEnd)
{
    char achAttr[100];
    const char *pValue;
    int  valLen;
    int ret;
    while (1)
    {
        ret = getAttr(pBegin, pEnd, achAttr, pValue, valLen);
        if (ret == -1)
            return LS_FAIL;
        if (ret == -2)
            break;
        int attr = 1;
        for (; attr <= SSI_ATTR_EXPR; attr++)
        {
            if (strcasecmp(s_SSI_Attrs[attr], achAttr) == 0)
                break;
        }
        if (attr > SSI_ATTR_EXPR)
        {
            //error: unknown SSI attribute
            continue;
        }
        if (attr == SSI_ATTR_ENCODING)
        {
            attr = SSI_ATTR_ENC_NONE;
            for (; attr <= SSI_ATTR_ENC_ENTITY; attr++)
            {
                if ((strncasecmp(s_SSI_Attrs[attr], pValue, valLen) == 0)
                    && (s_SSI_Attrs_len[attr] == valLen))
                    break;
            }
            if (attr > SSI_ATTR_ENC_ENTITY)
                continue;
        }
        if (attr == SSI_ATTR_EXPR)
        {
            if ((cmd != SsiComponent::SSI_If) &&
                (cmd != SsiComponent::SSI_Elif))
                continue;
        }


        SubstItem *pItem = new SubstItem();

        if (attr == SSI_ATTR_EXPR)
        {
            Expression *pExpr = new Expression();
            pExpr->parse(SubstFormat::SFT_SSI, pValue, pValue + valLen);
            pItem->setType(REF_EXPR);
            pItem->setAny(pExpr);

        }
        else if (attr == SSI_ATTR_ECHO_VAR)
        {
            if (cmd == SsiComponent::SSI_Set)
            {
                attr = SSI_ATTR_SET_VAR;
                pItem->setType(REF_STRING);
                pItem->setStr(pValue, valLen);
            }
            else
            {
                if (pItem->parseServerVar2(pValue, pValue, valLen, 1) == LS_FAIL)
                {
                    pItem->setType(REF_SSI_VAR);
                    pItem->setStr(pValue, valLen);
                }
            }
        }
        else if (attr < SSI_ATTR_ENC_NONE)
        {
            if (memchr(pValue, '$', valLen))
            {
                SubstFormat *pFormat = parseFormat(pValue, valLen);
                if (pFormat)
                {
                    pItem->setType(REF_FORMAT_STR);
                    pItem->setAny(pFormat);
                }
            }
            else
            {
                pItem->setType(REF_STRING);
                pItem->setStr(pValue, valLen);
            }
        }
        pItem->setSubType(attr);

        m_pCurComponent->appendParsed(pItem);

    }
    return 0;
}


// int SsiScript::addBlock(Ssi_If *pSSI_If, int is_else)
// {
//     SsiBlock *pBlock = new SsiBlock(m_pCurBlock, pSSI_If);
//     if (!is_else)
//         pSSI_If->setIfBlock(pBlock);
//     else
//         pSSI_If->setElseBlock(pBlock);
//     m_pCurComponent = NULL;
//     m_pCurBlock = pBlock;
//     return 0;
// }


// Ssi_If *SsiScript::getComponentIf(SsiBlock *&pBlock)
// {
//     Ssi_If *pComp = m_pCurBlock->getParentComp();
//     pBlock = m_pCurBlock->getParentBlock();
//     while (pComp &&
//            (pComp->getType() != SsiComponent::SSI_If))
//     {
//         pComp = pBlock->getParentComp();
//         pBlock = pBlock->getParentBlock();
//     }
//     return pComp;
// }


int SsiScript::parseIf(int cmd, const char *pBegin, const char *pEnd)
{
    int falseBlock = 0;
    if ((cmd == SsiComponent::SSI_Endif)
        || (cmd == SsiComponent::SSI_Else)
        || (cmd == SsiComponent::SSI_Elif))
    {
        int blockType = m_pCurBlock->getType();
        if ((blockType == SsiComponent::SSI_If)
            || (blockType == SsiComponent::SSI_Else)
            || (blockType == SsiComponent::SSI_False)
            || (blockType  == SsiComponent::SSI_Elif))
        {
            m_pCurComponent = m_pCurBlock;
            m_pCurBlock = m_pCurBlock->getParentBlock();
        }
    }

    if (m_pCurBlock->getType() != SsiComponent::SSI_Switch)
    {
        if (cmd == SsiComponent::SSI_Else)
            falseBlock = 1;
        if (cmd == SsiComponent::SSI_Endif)
            return 0;
        if (cmd == SsiComponent::SSI_Elif)
            cmd = SsiComponent::SSI_If;
    }
    if (cmd == SsiComponent::SSI_If)
    {
        SsiBlock *pBlock = new SsiBlock(m_pCurBlock);
        pBlock->setType(SsiComponent::SSI_Switch);
        m_pCurBlock->append(pBlock);
        m_pCurBlock = pBlock;
    }

    if (cmd == SsiComponent::SSI_Endif)
    {
        m_pCurComponent = m_pCurBlock;
        m_pCurBlock = m_pCurBlock->getParentBlock();
        return 0;
    }
    SsiBlock *pBlock = new SsiBlock(m_pCurBlock);
    m_pCurBlock->append(pBlock);
    m_pCurBlock = pBlock;
    m_pCurComponent = pBlock;

    if (cmd != SsiComponent::SSI_Else)
    {
        while (isspace(*pBegin))
            ++pBegin;
        parseAttrs(cmd, pBegin, pEnd);
    }
    else
    {
        if (falseBlock)
            cmd = SsiComponent::SSI_False;
    }
    pBlock->setType(cmd);

    return 0;
//         SsiBlock *pBlock = m_pCurBlock->getParentBlock();
//         Ssi_If *pSSI_If = m_pCurBlock->getParentComp();
//         if (!pSSI_If)
//         {
//             if (cmd == SsiComponent::SSI_Elif)
//             {
//                 //Error: Elif block without matching If, treated as If block
//                 cmd = SsiComponent::SSI_If;
//             }
//             else
//             {
//                 //Error: Else block without matching If, ignore
//                 return 0;
//             }
//         }
//         else
//         {
//             m_pCurComponent = pSSI_If;
//             m_pCurBlock = pBlock;
//             addBlock(pSSI_If, 1);
//         }
//     }

//     if (cmd == SsiComponent::SSI_Endif)
//     {
//         SsiBlock *pBlock;
//         Ssi_If *pSSI_If = getComponentIf(pBlock);
//         if (!pSSI_If)
//         {
//             //Error: Endif without matching If, ignore
//             return 0;
//         }
//         m_pCurBlock = pBlock;
//         m_pCurComponent = NULL;
//         return 0;
//     }

//     if ((cmd == SsiComponent::SSI_Elif) ||
//         (cmd == SsiComponent::SSI_If))
//     {
//         Ssi_If *pSSI_If = new Ssi_If();
//         m_pCurComponent = pSSI_If;
//         m_pCurComponent->setType(cmd);
//         m_pCurBlock->append(m_pCurComponent);
//         while (isspace(*pBegin))
//             ++pBegin;
//         parseAttrs(cmd, pBegin, pEnd);
//
//         addBlock(pSSI_If, 0);
//     }
    return 0;
}


int SsiScript::parseSsiDirective(const char *pBegin, const char *pEnd)
{
    while (isspace(*pBegin))
        ++pBegin;
    if (pBegin >= pEnd)
        return 0;
    int cmd = 1;
    for (; cmd < (int)(sizeof(s_SSI_Cmd) / sizeof(const char *)); cmd++)
    {
        if (strncasecmp(s_SSI_Cmd[cmd], pBegin,
                        s_SSI_Cmd_len[cmd]) == 0)
        {
            if (isspace(*(pBegin + s_SSI_Cmd_len[cmd])))
                break;
        }
    }
    if (cmd > SsiComponent::SSI_Endif)
    {
        //error: unknown SSI command
        return LS_FAIL;
    }
    pBegin += s_SSI_Cmd_len[cmd] + 1;
    while (isspace(*pBegin))
        ++pBegin;
    if ((cmd == SsiComponent::SSI_If) ||
        (cmd == SsiComponent::SSI_Elif) ||
        (cmd == SsiComponent::SSI_Else) ||
        (cmd == SsiComponent::SSI_Endif))
        return parseIf(cmd, pBegin, pEnd);

    m_pCurComponent = new SsiComponent();
    if (!m_pCurComponent)
        return LS_FAIL;
    m_pCurComponent->setType(cmd);
    m_pCurBlock->append(m_pCurComponent);
    return parseAttrs(cmd, pBegin, pEnd);
}


int SsiScript::parse(SsiTagConfig *pConfig, const char *pBegin,
                     const char *pEnd, int finish, int curOffset)
{
    static AutoStr2 sStart("<!--#");
    static AutoStr2 sEnd("-->");
    const AutoStr2 *pattern[2] = { NULL, NULL };
    const char *p = pBegin;
    const char *pTag;
    const char *pContentBegin = pBegin;
    if (pConfig)
    {
        pattern[0] = &pConfig->getStartTag();
        pattern[1] = &pConfig->getEndTag();
    }
    if (!pattern[0] || !pattern[0]->c_str())
        pattern[0] = &sStart;
    if (!pattern[1] || !pattern[1]->c_str())
        pattern[1] = &sEnd;

    while (p < pEnd + 1 - pattern[m_iParserState]->len())
    {
        pTag = (char *)memchr(p, *(pattern[m_iParserState]->c_str()),
                              pEnd + 1 - pattern[m_iParserState]->len() - p);
        if (pTag)
        {
            if (memcmp(pTag, pattern[m_iParserState]->c_str(),
                       pattern[m_iParserState]->len()) == 0)
            {
                if (m_iParserState)
                    parseSsiDirective(pContentBegin, pTag);
                else
                {
                    if (pContentBegin != pTag)
                        appendHtmlContent(pContentBegin - pBegin + curOffset,
                                          pTag - pContentBegin);
                }
                p = pTag + pattern[m_iParserState]->len();
                pContentBegin = p;
                m_iParserState = !m_iParserState;
            }
            else
                p = pTag + 1;
            continue;
        }
        else
            break;
    }

    if (m_iParserState)
    {
        //looking for the end tag
    }
    else
    {
        //looking for the start tag
        if (finish)
            p = pEnd;
        else
        {
            if (pEnd - p > pattern[m_iParserState]->len() - 1)
                p = pEnd + 1 - pattern[m_iParserState]->len();
        }
        if (p > pContentBegin)
            appendHtmlContent(pContentBegin - pBegin + curOffset,
                              p - pContentBegin);
        pContentBegin = p;
    }
    return pContentBegin - pBegin;
}


int SsiScript::processSsiFile(SsiTagConfig *pConfig, int fd,
                              struct stat *pStat)
{
    int ret;
    int finish = 1;
    char *pBegin;
    char *pEnd;
    int curOffset = 0;

    m_lModify = pStat->st_mtime;
    m_lSize = pStat->st_size;
    m_iParserState = 0;
    m_pCurComponent = NULL;
    m_pCurBlock = &m_main;

    pBegin = (char *)mmap(NULL, pStat->st_size, PROT_READ,
                          MAP_SHARED | MAP_FILE, fd, 0);

    m_pContent = new VMemBuf();
    if (!m_pContent)
    {
        LS_ERROR("Insufficient memory allocating VMemBuf in processSsiFile()");
        return -1;
    }
    m_pContent->set(VMBUF_FILE_MAP, new MmapBlockBuf(pBegin, pStat->st_size));
    m_pContent->writeUsed(pStat->st_size);

    pEnd = pBegin + pStat->st_size;

    ret = parse(pConfig, pBegin, pEnd, finish, curOffset);

//     pBufEnd = pBegin + sizeof(achBuf);
//     while (!finish)
//     {
//         ret = ls_fio_read(fd, pEnd, pBufEnd - pEnd);
//         if (ret < pBufEnd - pEnd)
//             finish = 1;
//         if (ret > 0)
//             pEnd += ret;
//         ret = parse(pConfig, pBegin, pEnd, finish);
//         pBegin += ret;
//         left = pEnd - pBegin;
//         if (left > 0)
//         {
//             if (pBegin != achBuf)
//             {
//                 memmove(achBuf, pBegin, left);
//                 pBegin = achBuf;
//                 pEnd = &achBuf[left];
//             }
//         }
//         else
//             pEnd = pBegin = achBuf;
//     }
    return ret;

}


int SsiScript::parse(SsiTagConfig *pConfig, const char *pScriptPath)
{
    struct stat st;
    int fd;
    int ret = 0;
    fd = ls_fio_open(pScriptPath, O_RDONLY, 0644);
    if (fd == -1)
    {
        LS_ERROR("[%s] Failed to open SSI script: %s",
                 pScriptPath, strerror(errno));
        return LS_FAIL;
    }
    m_sPath.setStr(pScriptPath);
    if (fstat(fd, &st) == 0)
    {
        if (st.st_size < 10240 * 1024)
            ret = processSsiFile(pConfig, fd, &st);
        else
        {
            ret = -1;
            LS_ERROR("[%s] Cannot parse SSI script, file is too big, size: %zd.",
                     pScriptPath, st.st_size);
        }
    }
    close(fd);
    return ret;

}


int SsiScript::testParse()
{
    SsiScript script;
    script.parse(NULL, "/home/gwang/proj/httpd/test_data/local_time.shtml");
    return 0;
}


// const SsiComponent *SsiScript::nextComponent(
//     const SsiComponent  *&pComponent,
//     const SsiBlock      *&pCurBlock) const
// {
//     if (!pComponent)
//         return NULL;
//     pComponent = (SsiComponent *)pComponent->next();
//     while (!pComponent)
//     {
//         if (pCurBlock)
//         {
//             pComponent = pCurBlock;
//             pCurBlock = pCurBlock->getParentBlock();
//             if (pComponent)
//                 pComponent = (SsiComponent *)pComponent->next();
//         }
//         else
//             break;
//     }
//     return pComponent;
// }


void SsiScript::setCurrentBlock(SsiBlock *pBlock)
{
    SsiComponent *pComponent = (SsiComponent *)pBlock->head()->next();
    if (!pComponent)
        return;
    m_pCurBlock = pBlock;
    m_pCurComponent = pComponent;
}

