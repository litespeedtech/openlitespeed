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
#ifndef LSIAPIHOOKS_H
#define LSIAPIHOOKS_H

//#include "lsiapi/modulemanager.h"
#include "lsiapi/internal.h"
#include "lsiapi/lsiapi.h"
#include "lsiapi/lsimoduledata.h"
#include <stdlib.h>


class LsiSession;

template< int B, int S >
class SessionHooks;

typedef struct lsiapi_hook_s
{
    const lsi_module_t *module;
    lsi_callback_pf     cb;
    short               priority;
    short               flag;
} lsiapi_hook_t;

#define LSI_HKPT_L4_COUNT       (LSI_HKPT_HTTP_BEGIN - LSI_HKPT_L4_BEGINSESSION)
#define LSI_HKPT_HTTP_COUNT     (LSI_HKPT_HTTP_END - LSI_HKPT_HTTP_BEGIN + 1)
#define LSI_HKPT_SERVER_COUNT   (LSI_HKPT_TOTAL_COUNT - LSI_HKPT_L4_COUNT - LSI_HKPT_HTTP_COUNT)

typedef SessionHooks<0, LSI_HKPT_L4_COUNT>   IolinkSessionHooks;
typedef SessionHooks<LSI_HKPT_HTTP_BEGIN, LSI_HKPT_HTTP_COUNT>
HttpSessionHooks;
typedef SessionHooks<LSI_HKPT_MAIN_INITED, LSI_HKPT_SERVER_COUNT>
ServerSessionHooks;

class LsiApiHooks;

typedef struct lsiapi_hookinfo_s
{
    const LsiApiHooks  *hooks;
    int8_t             *enable_array;
    filter_term_fn      term_fn;
} lsiapi_hookinfo_t;


class LsiApiHooks
{
public:
    explicit LsiApiHooks(int capacity = 4)
        : m_pHooks(NULL)
        , m_iCapacity(0)
        , m_iBegin(0)
        , m_iEnd(0)
        , m_iFlag(0)
    {
        reallocate(capacity);
    }

    ~LsiApiHooks()
    {
        if (m_pHooks)
            free(m_pHooks);
    }

    LsiApiHooks(const LsiApiHooks &other);

    short size() const              {   return m_iEnd - m_iBegin;   }
    short capacity() const          {   return m_iCapacity;         }

    short getGlobalFlag() const     {   return m_iFlag;             }
    void  setGlobalFlag(short f)    {   m_iFlag |= f;               }

    short add(const lsi_module_t *pModule, lsi_callback_pf cb, short priority,
              short flag = 0);

    int remove(lsiapi_hook_t *pHook);
    int remove(const lsi_module_t *pModule);

    lsiapi_hook_t *find(const lsi_module_t *pModule, lsi_callback_pf cb);
    lsiapi_hook_t *find(const lsi_module_t *pModule) const;
    lsiapi_hook_t *find(const lsi_module_t *pModule, int *index) const;


    lsiapi_hook_t *get(int index) const
    {
        return m_pHooks + index;
    }
    lsiapi_hook_t *begin() const  {   return m_pHooks + m_iBegin; }
    lsiapi_hook_t *end() const    {   return m_pHooks + m_iEnd;   }
    LsiApiHooks *dup() const
    {   return new LsiApiHooks(*this);    }

    int copy(const LsiApiHooks &other);


    int runCallback(int level, lsi_cb_param_t *param) const;
    int runCallback(int level, int8_t *pEnableArray, LsiSession *session,
                    void *param1, int paramLen1, int *flag_out, int flag_in,
                    lsi_module_t *pModule = NULL) const
    {
        if (pModule && find(pModule) == NULL)
        {
            //ERROR
            pModule = NULL;
        }

        lsiapi_hookinfo_t info = { this, pEnableArray, NULL };
        lsi_cb_param_t param =
        {
            session,
            &info,
            (pModule ?  find(pModule) + 1 : begin()),
            param1,
            paramLen1,
            flag_out,
            flag_in
        };

        return runCallback(level, &param);
    }

    int runCallbackNoParam(int level, int8_t *pEnableArray,
                           LsiSession *session,
                           lsi_module_t *pModule = NULL) const
    {
        return runCallback(level, pEnableArray, session, NULL,
                           0, NULL, 0, pModule);
    }


    static int runForwardCb(lsi_cb_param_t *param);
    static int runBackwardCb(lsi_cb_param_t *param);



    static ServerSessionHooks *s_pServerSessionHooks;
    static ServerSessionHooks *getServerSessionHooks()
    {   return s_pServerSessionHooks;    }


    static const LsiApiHooks *getGlobalApiHooks(int index)
    {   return &s_hooks[index];         }
    static LsiApiHooks *getReleaseDataHooks(int index);
    static inline const char *getHkptName(int index)
    {   return s_pHkptName[ index ];    }
    static void initGlobalHooks();


private:

    LsiApiHooks &operator=(const LsiApiHooks &other);
    bool operator==(const LsiApiHooks &other) const;

    int reallocate(int capacity, int newBegin = -1);

private:
    lsiapi_hook_t   *m_pHooks;
    short            m_iCapacity;
    short            m_iBegin;
    short            m_iEnd;
    short            m_iFlag;

public:
    static const char  *s_pHkptName[LSI_HKPT_TOTAL_COUNT];
    static LsiApiHooks  s_hooks[LSI_HKPT_TOTAL_COUNT];
};


template< int B, int S >
class SessionHooks
{
    enum
    {
        UNINIT = 0,
        INITED,
        HASOWN,
    };



private:
    int8_t *m_pEnableArray[S];
    short   m_iFlag[S];
    short   m_iStatus;


    void inheritFromParent(SessionHooks<B, S> *parentSessionHooks)
    {
        assert(m_iStatus != UNINIT);
        if (m_iStatus == HASOWN)
            return;

        for (int i = 0; i < S; ++i)
        {
            int level = B + i;
            int level_size = LsiApiHooks::getGlobalApiHooks(level)->size();
            memcpy(m_pEnableArray[i], parentSessionHooks->getEnableArray(level),
                   level_size * sizeof(int8_t));
        }
        memcpy(m_iFlag, parentSessionHooks->m_iFlag, S * sizeof(short));
    }

    void updateFlag(int level)
    {
        int index = level - B;
        m_iFlag[index] = 0;
        int level_size = LsiApiHooks::getGlobalApiHooks(level)->size();
        lsiapi_hook_t *pHook = LsiApiHooks::getGlobalApiHooks(level)->begin();
        int8_t *pEnableArray = m_pEnableArray[index];

        for (int j = 0; j < level_size; ++j)
        {
            //For the disabled hook, just ignor it
            if (pEnableArray[j])
                m_iFlag[index] |= pHook->flag | LSI_HOOK_FLAG_ENABLED;
            ++pHook;
        }
    }

    void updateFlag()
    {
        for (int i = 0; i < S; ++i)
            updateFlag(B + i);
    }

    void inheritFromGlobal()
    {
        int level_size;
        lsiapi_hook_t *pHook;
        int8_t *pEnableArray;
        assert(m_iStatus != UNINIT);
        if (m_iStatus == HASOWN)
            return;

        for (int i = 0; i < S; ++i)
        {
            level_size = LsiApiHooks::getGlobalApiHooks(B + i)->size();
            pHook = LsiApiHooks::getGlobalApiHooks(B + i)->begin();
            pEnableArray = m_pEnableArray[i];
            for (int j = 0; j < level_size; ++j)
            {
                pEnableArray[j] = ((pHook->flag & LSI_HOOK_FLAG_ENABLED)
                                   ? 1 : 0);
                ++pHook;
            }
        }
        updateFlag();
    }

    //int and set the disable array from the global(in the LsiApiHook flag)
    int initSessionHooks()
    {
        if (m_iStatus != UNINIT)
            return 1;

        for (int i = 0; i < S; ++i)
        {
            //int level = base + i;
            int level_size = LsiApiHooks::getGlobalApiHooks(B + i)->size();
            m_pEnableArray[i] = new int8_t[level_size];
        }
        m_iStatus = INITED;

        return 0;
    }


    SessionHooks(const SessionHooks &rhs);
    void operator=(const SessionHooks &rhs);
public:
    //with the globalHooks for determine the base and size
    SessionHooks() : m_iStatus(UNINIT)  {}

    ~SessionHooks()
    {
        if (m_iStatus != UNINIT)
        {
            for (int i = 0; i < S; ++i)
                delete []m_pEnableArray[i];
        }
    }

    void disableAll()
    {
        int level_size;
        if (m_iStatus > UNINIT)
        {
            for (int i = 0; i < S; ++i)
            {
                level_size = LsiApiHooks::getGlobalApiHooks(B + i)->size();
                memset(m_pEnableArray[i], 0, level_size);
                m_iFlag[i] = 0;
            }
            m_iStatus = INITED;
        }
    }

    void inherit(SessionHooks<B, S> *parentRt, int isGlobal)
    {
        switch (m_iStatus)
        {
        case UNINIT:
            initSessionHooks();
        //No break, follow with the next
        case INITED:
            if (parentRt && !parentRt->isAllDisabled())
                inheritFromParent(parentRt);
            else if (isGlobal)
                inheritFromGlobal();
            else
                disableAll();
            break;
        case HASOWN:
        default:
            break;
        }
    }



    int8_t *getEnableArray(int level)
    {   return m_pEnableArray[level - B]; }

    int setEnable(int level, const lsi_module_t *pModule, int enable)
    {
        if (m_iStatus < INITED)
            return LS_FAIL;

        int level_index = -1;
        lsiapi_hook_t *pHook;
        pHook = LsiApiHooks::getGlobalApiHooks(level)->find(pModule,
                &level_index);
        if (pHook == NULL)
            return LS_FAIL;

        assert(level_index >= 0);
        int8_t *pEnableArray = m_pEnableArray[level - B];
        pEnableArray[level_index] = (enable ? 1 : 0);

        if (enable)
            m_iFlag[level - B] |= (pHook->flag | LSI_HOOK_FLAG_ENABLED);
        else
            updateFlag(level);
        m_iStatus = HASOWN;
        return 0;
    }

    void setModuleEnable(const lsi_module_t *pModule, int enable)
    {
        for (int level = B; level < B + S; ++level)
            setEnable(level, pModule, enable);
    }


    void reset()
    {
        if (m_iStatus >= INITED)
            m_iStatus = INITED;
    };

    int getBase()   { return B;  }
    int getSize()   { return S;  }

    int isNotInited() const   {   return (m_iStatus == UNINIT); }

    int isAllDisabled() const   {   return isNotInited(); }


    short getFlag(int hookLevel) { return m_iFlag[hookLevel - B]; }
//     void  setFlag( int hookLevel, short f )
//     {   m_iFlag[hookLevel - B] |= f;               }


    int isDisabled(int hookLevel) const
    {
        if (isAllDisabled())
            return 1;
        return ((m_iFlag[hookLevel - B] & LSI_HOOK_FLAG_ENABLED) == 0);
    }

    int isEnabled(int hookLevel) const {  return !isDisabled(hookLevel); }


//     int runCallback( int level, LsiSession *session, void *param1,
//                      int paramLen1, int *param2, int paramLen2) const
//     {
//         return LsiApiHooks::getGlobalApiHooks(level)->runCallback( level,
//                 m_pEnableArray[level - B], session, param1, paramLen1,
//                 param2, paramLen2 );
//     }

    int runCallbackNoParam(int level, LsiSession *session,
                           lsi_module_t *pModule = NULL) const
    {
        return LsiApiHooks::getGlobalApiHooks(level)->runCallbackNoParam(level,
                m_pEnableArray[level - B], session, pModule);
    }

};




#endif // LSIAPIHOOKS_H
