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
#include <util/xmlnode.h>
#include <util/autostr.h>
#include <util/autobuf.h>
#include <util/hashstringmap.h>

#include <assert.h>
#include <ctype.h>
#include <expat.h>
#include <stdio.h>
#include <string.h>

#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <lsr/ls_strtool.h>

#define BUF_SIZE 8192

class Attr
{
    AutoStr m_name;
    AutoStr m_value;
public:
    Attr(const char *name, const char *value);
    Attr() {};
    ~Attr() {};
    const char *getName() const     {   return m_name.c_str();  }
    const char *getValue() const    {   return m_value.c_str(); }

};

Attr::Attr(const char *name, const char *value)
    : m_name(name), m_value(value)
{}

class AttrMap: public HashStringMap<Attr *>
{
public:
    AttrMap()
        : HashStringMap<Attr * >(5)
    {}
    ~AttrMap();
};

AttrMap::~AttrMap()
{
    release_objects();
}

class NodeMap: public HashStringMap<XmlNodeList *>
{
public:

    static int releaseNode(GHash::iterator iter)
    {
        ((XmlNodeList *)iter->second())->release_objects();
        return 0;
    }

    NodeMap()
        : HashStringMap<XmlNodeList * >(10)
    {}

    NodeMap(GHash::hasher hf, GHash::kcmp_ne vc)
        : HashStringMap<XmlNodeList * >(10, hf, vc)
    {}

    ~NodeMap()
    {
        GHash::for_each(begin(), end(), releaseNode);
        release_objects();
    }
    int addChild(const char *name, XmlNode *pChild);

};

int NodeMap::addChild(const char *name, XmlNode *pChild)
{
    iterator iter = find(name);
    if (iter != end())
        iter.second()->push_back(pChild);
    else
    {
        XmlNodeList *pList = new XmlNodeList(4);
        if (pList)
        {
            pList->push_back(pChild);
            insert(name, pList);
        }
        else
            return -1;
    }
    return 0;
}

static int s_case_insensitive = 1;

class XmlNodeImpl
{
    friend class XmlNode;
    friend class XmlTree;

    AutoStr m_sName;
    AutoStr2 m_sValue;
    XmlNode *m_pParentNode;
    NodeMap *m_pChildrenMap;
    AttrMap *m_pAttrMap;

    XmlNodeImpl()
        : m_pParentNode(NULL)
        , m_pChildrenMap(NULL)
        , m_pAttrMap(NULL)
    {};
    ~XmlNodeImpl()
    {
        if (m_pAttrMap)
            delete m_pAttrMap;
        if (m_pChildrenMap)
            delete m_pChildrenMap;
    };

    int init(const char *el, const char **attr);
    int addChild(const char *name, XmlNode *pChild)
    {
        if (!m_pChildrenMap)
        {
            if (s_case_insensitive)
                m_pChildrenMap = new NodeMap(GHash::hfCiString, GHash::cmpCiString);
            else
                m_pChildrenMap = new NodeMap();
        }
        return m_pChildrenMap->addChild(name, pChild);
    }

    int addChildCi(const char *name, XmlNode *pChild)
    {
        if (!m_pChildrenMap)
            m_pChildrenMap = new NodeMap(GHash::hfCiString, GHash::cmpCiString);
        return m_pChildrenMap->addChild(name, pChild);
    }

};



int XmlNodeImpl::init(const char *el, const char **attr)
{
    m_sName = el;
    if ((attr[0]) && (!m_pAttrMap))
        m_pAttrMap = new AttrMap();
    for (int i = 0 ; attr[i] ; i += 2)
    {
        Attr *pAttr = new Attr(attr[i], attr[i + 1]);
        m_pAttrMap->insert(pAttr->getName(), pAttr);
    }
    return 0;
}

XmlNode::XmlNode()
    : m_impl(NULL)
{
    m_impl = new XmlNodeImpl();
}
XmlNode::~XmlNode()
{
    delete m_impl;
}


int XmlNode::init(const char *name, const char **attr)
{
    return m_impl->init(name, attr);
}

const char *XmlNode::getName() const
{
    return m_impl->m_sName.c_str();
}

const char *XmlNode::getValue() const
{
    return m_impl->m_sValue.c_str();
}


int XmlNode::getValueLen() const
{
    return m_impl->m_sValue.len();
}


int XmlNode::addChild(const char *name, XmlNode *pChild)
{
    m_impl->addChild(name, pChild);
    assert(pChild->m_impl->m_pParentNode == NULL);
    pChild->m_impl->m_pParentNode = this;
    return 0;
}

XmlNode *XmlNode::getParent() const
{
    return m_impl->m_pParentNode;
}

int XmlNode::setValue(const char *value, int len)
{
    m_impl->m_sValue.setStr(value, len);
    return 0;
}


int XmlNode::appendValue(const char *value, int len)
{
    m_impl->m_sValue.append(value, len);
    return m_impl->m_sValue.len();
}



int XmlNode::hasChild()
{
    if (!m_impl->m_pChildrenMap)
        return 0;
    return !m_impl->m_pChildrenMap->empty();
}


const XmlNode *XmlNode::getChild(const char *name, int bOptional) const
{
    NodeMap::iterator pos;
    if (!m_impl->m_pChildrenMap)
        return NULL;
    pos = m_impl->m_pChildrenMap->find(name);
    if (pos != m_impl->m_pChildrenMap->end())
        return *(pos.second()->begin());
    if (bOptional)
        return this;
    return NULL;
}


const XmlNode *XmlNode::getChild(const char *name) const
{
    NodeMap::iterator pos;
    if (!m_impl->m_pChildrenMap)
        return NULL;
    pos = m_impl->m_pChildrenMap->find(name);
    if (pos != m_impl->m_pChildrenMap->end())
        return *(pos.second()->begin());
    else
        return NULL;
}


const char *XmlNode::getChildValue(const char *name, int bKeyName) const
{
    if (bKeyName)
    {
        const char *p = getValue();
        if (p)
            return p;
    }
    const XmlNode *pNode = getChild(name);
    if (pNode)
        return pNode->getValue();
    return NULL;
}


const char *XmlNode::getChildValue(const char *name) const
{
    const XmlNode *pNode = getChild(name);
    if (pNode)
        return pNode->getValue();
    return NULL;
}


int XmlNode::getChildValueLen(const char *name, int bKeyName) const
{
    if (bKeyName)
        return getValueLen();
    const XmlNode *pNode = getChild(name);
    if (pNode)
        return pNode->getValueLen();
    return 0;
}


XmlNode *XmlNode::getChild(const char *name)
{
    return (XmlNode *)((const XmlNode *)this)->getChild(name);
}

const XmlNodeList *XmlNode::getChildren(const char *name) const
{
    NodeMap::iterator pos;
    if (!m_impl->m_pChildrenMap)
        return NULL;
    pos = m_impl->m_pChildrenMap->find(name);
    if (pos != m_impl->m_pChildrenMap->end())
        return pos.second();
    else
        return NULL;
}



static long long getLongValue(const char *pValue, int base = 10)
{
    long long l = strlen(pValue);
    long long m = 1;
    char ch = *(pValue + l - 1);
    if (ch == 'G' || ch == 'g')
        m = 1024 * 1024 * 1024;
    else if (ch == 'M' || ch == 'm')
        m = 1024 * 1024;
    else if (ch == 'K' || ch == 'k')
        m = 1024;
    return strtoll(pValue, (char **)NULL, base) * m;
}


long long XmlNode::getLongValue(const char *pTag,
                                long long min, long long max, long long def, int base) const
{
    const char *pValue = getChildValue(pTag);
    long long val;
    if (!pValue)
        return def;
    val = ::getLongValue(pValue, base);
    if ((max == INT_MAX) && (val > max))
        val = max;
    if (((min != LLONG_MIN) && (val < min)) ||
        ((max != LLONG_MAX) && (val > max)))
    {
        //LS_WARN( "[%s] invalid value of <%s>:%s, use default=%ld",
        //         getLogId(), pTag, pValue, def ));
        return def;
    }
    return val;
}




int XmlNode::getAllChildren(XmlNodeList &list)
{
    return ((const XmlNode *)this)->getAllChildren(list);
}

int XmlNode::getAllChildren(XmlNodeList &list) const
{
    int count = 0;
    NodeMap::iterator pos;
    if (!m_impl->m_pChildrenMap)
        return 0;
    for (pos = m_impl->m_pChildrenMap->begin();
         pos != m_impl->m_pChildrenMap->end();
         pos = m_impl->m_pChildrenMap->next(pos))
    {
        const XmlNodeList *pList = pos.second();
        count += pList->size();
        list.push_back(*pList);
    }
    return count;
}

const char *XmlNode::getAttr(const char *name) const
{
    AttrMap::iterator pos;
    if (!m_impl->m_pAttrMap)
        return NULL;
    pos = m_impl->m_pAttrMap->find(name);
    if (pos != m_impl->m_pAttrMap->end())
        return pos.second()->getValue();
    else
        return NULL;
}

const AttrMap *XmlNode::getAllAttr() const
{
    return m_impl->m_pAttrMap;
}

int XmlNode::xmlOutput(FILE *fd, int depth) const
{
    for (int i = 0 ; i <= depth ; ++ i)
        fprintf(fd, " ");
    fprintf(fd, "<%s", getName());
    if (m_impl->m_pAttrMap && !m_impl->m_pAttrMap->empty())
    {
        AttrMap::iterator pos;
        for (pos = m_impl->m_pAttrMap->begin();
             pos != m_impl->m_pAttrMap->end();
             pos = m_impl->m_pAttrMap->next(pos))
            fprintf(fd, " %s=\"%s\"", pos.first(), pos.second()->getValue());
    }
    fprintf(fd, ">");
    fprintf(fd, "%s", getValue());
    if (m_impl->m_pChildrenMap && !m_impl->m_pChildrenMap->empty())
    {
        fprintf(fd, "\n");
        NodeMap::iterator pos;
        for (pos = m_impl->m_pChildrenMap->begin();
             pos != m_impl->m_pChildrenMap->end();
             pos = m_impl->m_pChildrenMap->next(pos))
        {
            const XmlNodeList *pList = pos.second();
            XmlNodeList::const_iterator iter;
            for (iter = pList->begin(); iter != pList->end(); ++iter)
                (*iter)->xmlOutput(fd, depth + 1);
        }
        for (int i = 0 ; i <= depth ; ++ i)
            fprintf(fd, " ");
    }
    fprintf(fd, "</%s>\n", getName());
    return 0;
}

#define MAX_XML_VALUE_LEN (16384 * 1024 - 2)

class XmlTree
{
public:
    XmlTree()
        : m_pRoot(NULL)
        , m_pCurNode(NULL)
        , m_curValue(8192)
        , m_setValue(false)
    {
    }
    ~XmlTree()
    {
    };

    void startElement(const char *el, const char **attr);
    void endElement(const char *el);
    void charHandler(const char *el, int len);

    XmlNode *m_pRoot;
    XmlNode *m_pCurNode;
    AutoBuf m_curValue;
    bool    m_setValue;
};

void XmlTree::startElement(const char *el, const char **attr)
{
    XmlNode *pNewNode = new XmlNode();
    pNewNode->init(el, attr);
    if (m_pRoot == NULL)
        m_pRoot = m_pCurNode = pNewNode;
    else
    {
        m_pCurNode->addChild(pNewNode->getName(), pNewNode);
        m_pCurNode = pNewNode;
    }
}

void XmlTree::endElement(const char *el)
{
    if (m_setValue)
    {
        const char *pValue = m_curValue.begin();
        int len = m_curValue.size();
        int start = 0;
        while ((start < len) && isspace(*(pValue + start)))
            ++start;
        if (start < len)
        {
            while ((len > start) && isspace(*(pValue + len - 1)))
                --len;
            m_pCurNode->setValue(pValue + start, len - start);
            m_setValue = false;
            m_curValue.clear();
        }
    }
    m_pCurNode = m_pCurNode->getParent();
}

void XmlTree::charHandler(const char *el, int len)
{
    if (!m_setValue)
    {
        int start = 0;
        while ((start < len) && isspace(*(el + start)))
            ++start;
        if (start == len)
            return;
    }
    m_setValue = true;
    if (m_curValue.size() + len < MAX_XML_VALUE_LEN)
    {
        if (m_curValue.available() < len)
            m_curValue.reserve(m_curValue.capacity() * 2);
        m_curValue.append(el, len);
    }
}

static void startElement(void *data, const char *el, const char **attr)
{
    XmlTree *pTree = (XmlTree *)data;
    pTree->startElement(el, attr);
}

static void endElement(void *data, const char *el)
{
    XmlTree *pTree = (XmlTree *)data;
    pTree->endElement(el);
}

static void charHandler(void *data, const char *el, int len)
{
    XmlTree *pTree = (XmlTree *)data;
    pTree->charHandler(el, len);
}

XmlNode *XmlTreeBuilder::parse(const char *pFilePath, char *pError,
                               int errBufLen)
{
    int fd = open(pFilePath, O_RDONLY);
    if (fd == -1)
    {
        snprintf(pError, errBufLen, "Cannot open xml file: %s\n", pFilePath);
        return NULL;
    }

    char buf[BUF_SIZE];
    XmlTree tree;
    XML_Parser parser = XML_ParserCreate(NULL);
    int done;
    XML_SetUserData(parser, &tree);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, charHandler);

    size_t total = 0;
    do
    {
        size_t len = read(fd, buf, sizeof(buf));
        done = len < sizeof(buf);
        total += len;
        if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR)
        {
            ls_snprintf(pError, errBufLen, "[%s:%d] %s \n", pFilePath,
                        (int)XML_GetCurrentLineNumber(parser),
                        XML_ErrorString(XML_GetErrorCode(parser))
                       );
            XML_ParserFree(parser);
            close(fd);
            if (tree.m_pRoot)
                delete tree.m_pRoot;
            return NULL;
        }
    }
    while (!done);

    XML_ParserFree(parser);

    close(fd);

    return tree.m_pRoot;
}

