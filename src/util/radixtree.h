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
#ifndef STRINGTREE_H
#define STRINGTREE_H

#include <lsr/ls_str.h>
#include <lsr/ls_xpool.h>


#define RTMODE_CONTIGUOUS 0
#define RTMODE_POINTER    1

#define RTFLAG_NOCONTEXT  (1 << 0)
#define RTFLAG_GLOBALPOOL (1 << 1)
#define RTFLAG_BESTMATCH  (1 << 2)
#define RTFLAG_REGEXCMP   (1 << 3)
#define RTFLAG_CICMP      (1 << 4)

typedef struct rnheader_s rnheader_t;
typedef struct rnparams_s rnparams_t;
class GHash;

//NOTICE: Should this pass in the key as well?
// Should return 0 for success.
typedef int (*rn_foreach)(void *pObj, const char *pKey, size_t iKeyLen);
typedef int (*rn_foreach2)(void *pObj, void *pUData, const char *pKey,
                           size_t iKeyLen);


class RadixNode
{
public:
    ~RadixNode()
    {
        ls_str_set(&m_label, NULL, 0);
    }

    int hasChildren()               {   return m_iNumChildren > 0 ? 1 : 0;  }
    int getNumChildren()            {   return m_iNumChildren;              }
    void incrNumChildren()          {   ++m_iNumChildren;                   }

    RadixNode *getParent() const    {   return m_pParent;                   }
    void setParent(RadixNode *p)    {   m_pParent = p;                      }

    ls_str_t *getLabel()            {   return &m_label;                    }
    void setLabel(char *pLabel, int iLabelLen)
    {   ls_str_set(&m_label, pLabel, iLabelLen);    }

    void *getObj()                  {   return m_pObj;                      }
    void setObj(void *pObj)         {   m_pObj = pObj;                      }
    void *getParentObj();


    RadixNode *insert(rnparams_t *pParams);
    // NOTICE: iFlags should be an |= of any flags needed, listed above.
    RadixNode *insert(ls_xpool_t *pool, const char *pLabel, size_t iLabelLen,
                      void *pObj, int iFlags = 0);
    void *erase(const char *pLabel, size_t iLabelLen, int iFlags = 0);
    void *update(const char *pLabel, size_t iLabelLen, void *pObj,
                 int iFlags = 0);
    void *find(const char *pLabel, size_t iLabelLen, int iFlags = 0);
    void *bestMatch(const char *pLabel, size_t iLabelLen, int iFlags = 0);
    RadixNode *findChild(const char *pLabel, size_t iLabelLen, int iFlags = 0);
    int for_each(rn_foreach fun, const char *pKey = NULL, size_t iKeyLen = 0);
    int for_each2(rn_foreach2 fun, void *pUData, const char *pKey = NULL,
                  size_t iKeyLen = 0);
    int for_each_child(rn_foreach fun);
    int for_each_child2(rn_foreach2 fun, void *pUData);


    static RadixNode *newBranch(rnparams_t *pParams, RadixNode *pParent,
                                RadixNode *&pDest);
    static RadixNode *newNode(ls_xpool_t *pool, RadixNode *pParent,
                              void *pObj);

    void printChildren(unsigned long offset);

private:

    RadixNode(RadixNode *pParent, void *pObj = NULL);
    RadixNode(const RadixNode &rhs);
    void *operator=(const RadixNode &rhs);

    int getState()                  {   return m_iState;                    }
    void setState(int iState)       {   m_iState = iState;                  }

    RadixNode *checkLevel(rnparams_t *pParams, RadixNode *pNode,
                          int iHasChildren);
    RadixNode *fillNode(rnparams_t *pParams, rnheader_t *pHeader,
                        int iHasChildren, size_t iChildLen);
    RadixNode *insertCArray(rnparams_t *pParams, rnparams_t *myParams,
                            int iHasChildren, size_t iChildLen);
    RadixNode *insertPArray(rnparams_t *pParams, rnparams_t *myParams,
                            int iHasChildren, size_t iChildLen);
    RadixNode *insertHash(rnparams_t *pParams, rnparams_t *myParams,
                          int iHasChildren, size_t iChildLen);


    RadixNode *findArray(const char *pLabel, size_t iLabelLen,
                         size_t iChildLen, int iHasChildren, int iFlags);
    RadixNode *findChildData(const char *pLabel, size_t iLabelLen,
                             RadixNode *pNode, int iHasChildren, int iFlags);

    static int printHash(const void *key, void *data, void *extra);

    int              m_iNumChildren;
    int              m_iState;
    RadixNode       *m_pParent;
    ls_str_t         m_label;
    void            *m_pObj;
    union
    {
        rnheader_t  *m_pCHeaders;
        rnheader_t **m_pPHeaders;
        GHash       *m_pHash;
    };
};

class RadixTree
{
public:

    RadixTree(int iMode = RTMODE_CONTIGUOUS)
      : m_iMode(iMode)
      , m_iFlags(0)
      , m_pRoot(NULL)
    {   ls_xpool_init(&m_pool);             }

    ~RadixTree()
    {   ls_xpool_destroy(&m_pool);          }

    // NOTICE: If any of these are to be used, they should be set immediately.
    int setRootLabel(const char *pLabel, size_t iLabelLen);
    int getRegexCmp()               {   return m_iFlags & RTFLAG_REGEXCMP;  }
    void setRegexCmp()              {   m_iFlags |= RTFLAG_REGEXCMP;        }
    int getNoContext()              {   return m_iFlags & RTFLAG_NOCONTEXT; }
    void setNoContext();
    int getUseGlobalPool()          {   return m_iFlags & RTFLAG_GLOBALPOOL;}
    void setUseGlobalPool()         {   m_iFlags |= RTFLAG_GLOBALPOOL;      }

    RadixNode *insert(const char *pLabel, size_t iLabelLen, void *pObj);
    void *erase(const char *pLabel, size_t iLabelLen) const;
    void *update(const char *pLabel, size_t iLabelLen, void *pObj) const;
    void *find(const char *pLabel, size_t iLabelLen) const;
    void *bestMatch(const char *pLabel, size_t iLabelLen) const;

    int for_each(rn_foreach fun);
    int for_each2(rn_foreach2 fun, void *pUData);

    void printTree();

private:
    RadixTree(const RadixTree &rhs);
    void *operator=(const RadixTree &rhs);
    int checkPrefix(const char *pLabel, size_t iLabelLen) const;


    ls_xpool_t  m_pool;
    int         m_iMode;
    int         m_iFlags;
    rnheader_t *m_pRoot;
};


#endif


