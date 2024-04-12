

#include <expression.h>
#include <http/httpsession.h>
#include <http/requestvars.h>
#include <log4cxx/logger.h>
#include <util/accessdef.h>
#include <util/accesscontrol.h>
#include <util/autostr.h>
#include <util/pcregex.h>
#include <util/stringlist.h>
#include <util/stringtool.h>

#include <ctype.h>
#include <fnmatch.h>

ExprToken::~ExprToken()
{
    if (!m_obj)
        return;
    switch (m_type)
    {
    case EXP_STRING:
        delete m_str;
        break;
    case EXP_FMT:
        delete m_fmt;
        break;
    case EXP_IN:
    case EXP_LIST:
        delete m_list;
        break;
    case EXP_IPMATCH:
    case EXP_IPMATCH2:
        delete m_acl;
        break;
    case EXP_REGEX:
    case EXP_REGEX_NOMATCH:
        if (m_dynamic)
            delete m_fmt;
         break;
    default:
        break;
    }
    return;
}


int ExprToken::s_priority[FUNC_END] =
{
    0, 0, 0, 0,
    6, // EXP_OPENP,
    1, // EXP_CLOSEP,
    4, // EXP_REGEX,
    4, // EXP_EQ,
    4, // EXP_NE,
    4, // EXP_LESS,
    4, // EXP_LE,
    4, // EXP_GREAT,
    4, // EXP_GE,
    3, // EXP_NOT,
    2, // EXP_AND,
    2,  // EXP_OR,
    4,      // EXP_REGEX_NOMATCH,
    4,      // EXP_IN
    4,      // EXP_EQ_NUM,
    4,      // EXP_NE_NUM,
    4,      // EXP_LESS_NUM,
    4,      // EXP_LE_NUM,
    4,      // EXP_GREAT_NUM,
    4,      // EXP_GE_NUM,
    4,      // EXP_IPMATCH,
    4,      // EXP_STR_MATCH,
    4,      // EXP_STR_CMATCH,
    4,      // EXP_FNMATCH,
    4,      // EXP_DIR_EXIST
    4,      // EXP_FILE_DIR_EXIST
    4,      // EXP_REGULR_FILE_EXIST
    4,      // EXP_NON_EMPTY_FILE
    4,      // EXP_SYMLINK
    4,      // EXP_SYMLINK2
    4,      // EXP_FILE_ALLOW_ACCESS
    4,      // EXP_URL_ALLOW_ACCESS
    4,      // EXP_URL_ALLOW_ACCESS2
    4,      // EXP_NOT_EMPTY_STR
    4,      // EXP_EMPTY_STR
    4,      // EXP_TEST_BOOL
    4,      // EXP_IPMATCH2

    0,      // EXP_END,

    5,      // FUNC_REQ,
    5,      // FUNC_HTTP,
    5,      // FUNC_REQ_NOVARY,
    5,      // FUNC_RESP,
    5,      // FUNC_REQENV,
    5,      // FUNC_OSENV,
    5,      // FUNC_NOTE,
    5,      // FUNC_ENV,
    5,      // FUNC_TOLOWER,
    5,      // FUNC_TOUPPER,
    5,      // FUNC_ESCAPE,
    5,      // FUNC_UNESCAPE,
    5,      // FUNC_BASE64,
    5,      // FUNC_UNBASE64,
    5,      // FUNC_MD5,
    5,      // FUNC_SHA1,
    5,      // FUNC_FILE,
    5,      // FUNC_FILEMOD,
    5,      // FUNC_FILESIZE,
    //    FUNC_END,

};


static const char *s_operator[ExprToken::EXP_END] =
{
    NULL, NULL, NULL, NULL,
    "(",    // EXP_OPENP,
    ")",    // EXP_CLOSEP,
    "=~",   // EXP_REGEX
    "==",   // EXP_EQ,
    "!=",   // EXP_NE,
    "<",    // EXP_LESS,
    "<=",   // EXP_LE,
    ">",    // EXP_GREAT,
    ">=",   // EXP_GE,
    "!",    // EXP_NOT,
    "&&",   // EXP_AND,
    "||",   // EXP_OR,
    "!~",   // EXP_REGEX_NOMATCH,
    "in",   // EXP_IN
    "-eq",  // EXP_EQ_NUM,
    "-ne",  // EXP_NE_NUM,
    "-lt",  // EXP_LESS_NUM,
    "-le",  // EXP_LE_NUM,
    "-gt",  // EXP_GREAT_NUM,
    "-ge",  // EXP_GE_NUM,
    "-ipmatch",     // EXP_IPMATCH,
    "-strmatch",    // EXP_STR_MATCH,
    "-strcmatch",   // EXP_STR_CMATCH,
    "-fnmatch",     // EXP_FNMATCH,
    "-d",   // EXP_DIR_EXIST
    "-e",   // EXP_FILE_DIR_EXIST
    "-f",   // EXP_REGULR_FILE_EXIST
    "-s",   // EXP_NON_EMPTY_FILE
    "-L",   // EXP_SYMLINK
    "-h",   // EXP_SYMLINK2
    "-F",   // EXP_FILE_ALLOW_ACCESS
    "-U",   // EXP_URL_ALLOW_ACCESS
    "-A",   // EXP_URL_ALLOW_ACCESS2
    "-n",   // EXP_NOT_EMPTY_STR
    "-z",   // EXP_EMPTY_STR
    "-T",   // EXP_TEST_BOOL
    "-R",   // EXP_IPMATCH2
};


static int s_op_len[ExprToken::EXP_END] =
{
    0, 0, 0, 0,
    1,      // EXP_OPENP,
    1,      // EXP_CLOSEP,
    2,      // EXP_REGEX
    2,      // EXP_EQ,
    2,      // EXP_NE,
    1,      // EXP_LESS,
    2,      // EXP_LE,
    1,      // EXP_GREAT,
    2,      // EXP_GE,
    1,      // EXP_NOT,
    2,      // EXP_AND,
    2,      // EXP_OR,
    2,      // EXP_REGEX_NOMATCH,
    2,      // EXP_IN
    3,      // EXP_EQ_NUM,
    3,      // EXP_NE_NUM,
    3,      // EXP_LESS_NUM,
    3,      // EXP_LE_NUM,
    3,      // EXP_GREAT_NUM,
    3,      // EXP_GE_NUM,
    8,      // EXP_IPMATCH,
    9,      // EXP_STR_MATCH,
    10,     // EXP_STR_CMATCH,
    8,      // EXP_FNMATCH,
    2,      // EXP_DIR_EXIST
    2,      // EXP_FILE_DIR_EXIST
    2,      // EXP_REGULR_FILE_EXIST
    2,      // EXP_NON_EMPTY_FILE
    2,      // EXP_SYMLINK
    2,      // EXP_SYMLINK2
    2,      // EXP_FILE_ALLOW_ACCESS
    2,      // EXP_URL_ALLOW_ACCESS
    2,      // EXP_URL_ALLOW_ACCESS2
    2,      // EXP_NOT_EMPTY_STR
    2,      // EXP_EMPTY_STR
    2,      // EXP_TEST_BOOL
    2,      // EXP_IPMATCH2
};


static const char *s_func_names[] =
{
    "req(",
    "http(",
    "req_novary(",
    "resp(",
    "reqenv(",
    "osenv(",
    "note(",
    "env(",
    "tolower(",
    "toupper(",
    "escape(",
    "unescape(",
    "base64(",
    "unbase64(",
    "md5(",
    "sha1(",
    "file(",
    "filemod(",
    "filesize(",
};


static int s_func_name_len[] =
{
    4,  // "req(",
    5,  // "http(",
    11, // "req_novary(",
    5,  // "resp(",
    7,  // "reqenv(",
    6,  // "osenv(",
    5,  // "note(",
    4,  // "env(",
    8,  // "tolower(",
    8,  // "toupper(",
    7,  // "escape(",
    9,  // "unescape(",
    7,  // "base64(",
    9,  // "unbase64(",
    4,  // "md5(",
    5,  // "sha1(",
    5,  // "file(",
    8,  // "filemod(",
    9,  // "filesize(",
};


ExprToken::type ExprToken::parseFunc(const char *pBegin, int len)
{
    int i;
    for(i = FUNC_REQ; i < FUNC_FILESIZE; ++i)
    {
        int idx = i - FUNC_REQ;
        if (len >= s_func_name_len[idx]
            && strncasecmp(s_func_names[idx], pBegin, s_func_name_len[idx]) == 0)
        {
            return (ExprToken::type)i;
        }
    }
    return EXP_NONE;
}


ExprToken::type ExprToken::parse(const char *pBegin, int len)
{
    int i;
    if (*pBegin == '-')
    {
        for(i = EXP_BEGIN_WITH_DASH; i < EXP_END; ++i)
        {
            if (len == s_op_len[i]
                && strncasecmp(s_operator[i], pBegin, s_op_len[i]) == 0)
                return (ExprToken::type)i;
        }
    }
    else
    {
        if (len == 1 && *pBegin == '=')
            return ExprToken::EXP_EQ;
        for(i = EXP_REGEX; i < EXP_BEGIN_WITH_DASH; ++i)
        {
            if (len == s_op_len[i]
                && strncasecmp(s_operator[i], pBegin, s_op_len[i]) == 0)
                return (ExprToken::type)i;
        }
    }
    return ExprToken::EXP_NONE;
}


int ExprToken::addAcl(const char *pBegin, int len)
{
    assert(m_type == EXP_IPMATCH || m_type == EXP_IPMATCH2);
    if (m_acl == NULL)
        m_acl = new AccessControl();
    if (m_acl)
        return m_acl->addList(pBegin, AC_ALLOW);
    return -1;
}


static int  shortCurcuit(ExprToken *&pTok)
{
    int type;
    if (!pTok)
        return 0;

    type = pTok->getType();

    pTok = pTok->next();
    if (!pTok)
        return 0;

    switch (type)
    {
    // does not take extra parameter
    case ExprToken::EXP_STRING:
    case ExprToken::EXP_FMT:
    case ExprToken::EXP_IPMATCH2:
        break;
    case ExprToken::EXP_AND:
    case ExprToken::EXP_OR:
        shortCurcuit(pTok);
    //Fall through
    case ExprToken::EXP_NOT:
        shortCurcuit(pTok);
        break;

    //operator takes one parameter.
    case ExprToken::EXP_IN:
    case ExprToken::EXP_REGEX:
    case ExprToken::EXP_REGEX_NOMATCH:
    case ExprToken::EXP_IPMATCH:
    case ExprToken::EXP_DIR_EXIST:
    case ExprToken::EXP_FILE_DIR_EXIST:
    case ExprToken::EXP_REGULR_FILE_EXIST:
    case ExprToken::EXP_NON_EMPTY_FILE:
    case ExprToken::EXP_SYMLINK:
    case ExprToken::EXP_SYMLINK2:
    case ExprToken::EXP_FILE_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS2:
    case ExprToken::EXP_NOT_EMPTY_STR:
    case ExprToken::EXP_EMPTY_STR:
    case ExprToken::EXP_TEST_BOOL:
        pTok = pTok->next();
        break;
    //operator takes two parameters.
    case ExprToken::EXP_EQ:
    case ExprToken::EXP_NE:
    case ExprToken::EXP_LE:
    case ExprToken::EXP_LESS:
    case ExprToken::EXP_GE:
    case ExprToken::EXP_GREAT:
    case ExprToken::EXP_STR_CMATCH:
    case ExprToken::EXP_STR_MATCH:
    case ExprToken::EXP_FNMATCH:
        pTok = pTok->next();
        if (pTok)
            pTok = pTok->next();
        break;
    }
    return 1;
}


const Pcregex *Expression::parseRegex(const char *pBegin, const char *pEnd)
{
    const Pcregex *regex = NULL;
    char ch;
    ch = *pBegin;
    if (!(ch == '/' || (ch == 'm' && *(pBegin + 1) == '#')))
    {
        s_last_parse_error = "Not valid regular expression /.../ or m#...#";
        return NULL;
    }
    if (ch == 'm')
    {
        ++pBegin;
        ch = '#';
    }
    char *pArgEnd = (char *)memchr(pBegin + 1, ch, pEnd - pBegin);
    if (pArgEnd)
    {
        int reg_len = pArgEnd - pBegin - 1;
        if (pArgEnd + 1 < pEnd)
            ch = *(pArgEnd + 1);
        int case_ins = (ch == 'i');
        if (case_ins)
            ++pArgEnd;
        if (pArgEnd + 1 == pEnd || isspace(*(pArgEnd + 1)))
        {
            *((char *)pBegin + 1 + reg_len) = 0;
            regex = Pcregex::get(pBegin + 1, REG_EXTENDED | case_ins);
            if (!regex)
                LS_WARN("Bad regular expression: %s", pBegin + 1);
        }
    }
    else
    {
        s_last_parse_error = "Missing terminating char for REGEX /.../ or m#...#";
    }
    return regex;
}


int Expression::execRegex(HttpSession *pSession, ExprToken *&pTok,
                          ExprRunTime *runtime, const char *input, int len)
{
    int ret;
    RegexResult result;
    RegexResult *reg_result;
    const Pcregex *regex = NULL;
    if (pTok->isDynamic())
    {
        const SubstFormat *pattern = pTok->getDynamic();

        char achBuf1[40960];
        char *buf = achBuf1;
        int buf_size = sizeof(achBuf1);
        RequestVars::buildString(pattern, pSession, buf,
                                 buf_size, 0, runtime->regex_res);
        regex = parseRegex(buf, buf + buf_size);
        if (!regex)
            return -1;
    }
    else
        regex = pTok->getRegex();

    if (runtime->regex_res)
        reg_result = runtime->regex_res;
    else
        reg_result = &result;

    ret = regex->exec(input, len, 0, 0, reg_result);
    return ret;
}


int Expression::twoParamOperator(HttpSession *pSession, ExprToken::type type,
                                 ExprToken *&pTok, ExprRunTime *runtime, int depth)
{
    int     len, len2;
    int     ret;
    int     flag;
    char achBuf2[40960] = "";
    char achBuf1[40960] = "";
    char   *p1 = achBuf1;
    char   *p2 = achBuf2;
    len = sizeof(achBuf1) - 1;
    if (pTok->getType() == ExprToken::EXP_STRING)
    {
        p1 = (char *)pTok->getStr()->c_str();
        len = pTok->getStr()->len();
        pTok = pTok->next();
    }
    else if (pTok->getType() == ExprToken::EXP_FMT)
    {
        RequestVars::buildString(pTok->getFormat(), pSession, p1, len, 0,
                                 runtime->regex_res,
                                 runtime->time_format
                                );
        p1 = achBuf1;
        pTok = pTok->next();
    }
    else if (pTok->isFunc())
    {
        len = Expression::evalOperator(pSession, pTok, runtime, p1, len, depth + 1);
        if (len < (int)sizeof(achBuf1) - 1)
            p1[len] = 0;
    }
    if (!pTok)
        return 0;
    if (type == ExprToken::EXP_IN)
    {
        if (pTok->getType() == ExprToken::EXP_LIST)
        {
            const StringList *list = pTok->getList();
            if (list)
                return (list->find(p1, len) != NULL);
        }
        return 0;
    }

    if (pTok->getType() == ExprToken::EXP_REGEX
        || pTok->getType() == ExprToken::EXP_REGEX_NOMATCH)
    {
        if (runtime->regex_input)
        {
            runtime->regex_input->setStr(p1, len);
            p1 = (char *)runtime->regex_input->c_str();
        }
        ret = execRegex(pSession, pTok, runtime, p1, len);
        if (ret == 0)
            ret = 10;
        if (ret == -1)
            ret = 0;
        if (pTok->getType() == ExprToken::EXP_REGEX_NOMATCH)
            ret = !ret;
        if (type == ExprToken::EXP_NE)
            ret = !ret;
        pTok = pTok->next();
        return ret;
    }

    len2 = sizeof(achBuf2) - 1;
    if (pTok->getType() == ExprToken::EXP_STRING)
    {
        p2 = (char *)pTok->getStr()->c_str();
        len2 = pTok->getStr()->len();
        pTok = pTok->next();
    }
    else if (pTok->getType() == ExprToken::EXP_FMT)
    {
        RequestVars::buildString(pTok->getFormat(), pSession, p2, len2, 0,
                                 runtime->regex_res);
        p2 = achBuf2;
        pTok = pTok->next();
    }
    else
    {
        len2 = Expression::evalOperator(pSession, pTok, runtime, p2, len2, depth + 1);
        if (len2 < (int)sizeof(achBuf2) - 1)
            p2[len2] = 0;
    }

    switch(type)
    {
    case ExprToken::EXP_STR_CMATCH:
    case ExprToken::EXP_STR_MATCH:
    case ExprToken::EXP_FNMATCH:
        if (ExprToken::EXP_FNMATCH == type)
            flag = FNM_PATHNAME;
        else if (ExprToken::EXP_STR_CMATCH == type)
            flag = FNM_CASEFOLD;
        else
            flag = 0;
        ret = fnmatch(p2, p1, flag);
        return ret != FNM_NOMATCH;
    default:
        break;
    }

    ret = strcmp(p1, p2);
    switch (type)
    {
    case ExprToken::EXP_EQ:
        return (ret == 0);
    case ExprToken::EXP_NE:
        return (ret != 0);
    case ExprToken::EXP_LE:
        return (ret <= 0);
    case ExprToken::EXP_LESS:
        return (ret < 0);
    case ExprToken::EXP_GE:
        return (ret >= 0);
    case ExprToken::EXP_GREAT:
        return (ret > 0);
    default:
        break;
    }
    return 0;
}


int Expression::operatorWithSingleParam(HttpSession *pSession, ExprToken *pTok,
                                        ExprRunTime *runtime, int depth)
{
    int     len;
    int     ret;
    char achBuf1[40960] = "";
    char   *p1 = achBuf1;
    StringList *list;
    ExprToken::type t = pTok->getType();
    ExprToken *param = pTok->next();
    if (!param)
        return 0;
    p1 = achBuf1;
    len = sizeof(achBuf1) - 1;
    if (param->getType() == ExprToken::EXP_STRING)
    {
        p1 = (char *)param->getStr()->c_str();
        len = param->getStr()->len();
    }
    else if (param->getType() == ExprToken::EXP_FMT)
    {
        p1 = RequestVars::buildString(param->getFormat(), pSession, p1, len, 0,
                                 runtime->regex_res, runtime->time_format
                                );
    }
    else
    {
        len = Expression::evalOperator(pSession, param, runtime, p1, len, depth + 1);
        if (len < (int)sizeof(achBuf1) - 1)
            p1[len] = 0;
    }

    assert(ExprToken::EXP_IPMATCH2 != t);

    switch(t)
    {
    case ExprToken::EXP_REGEX:
    case ExprToken::EXP_REGEX_NOMATCH:
        if (runtime->regex_input)
        {
            runtime->regex_input->setStr(p1, len);
            p1 = (char *)runtime->regex_input->c_str();
        }
        if (p1)
            ret = execRegex(pSession, pTok, runtime, p1, len);
        else
            ret = -1;
        if (ret == 0)
            ret = 10;
        if (ret == -1)
            ret = 0;
        if (t == ExprToken::EXP_REGEX_NOMATCH)
            ret = !ret;
        return ret;
    case ExprToken::EXP_IN:
        list = pTok->getList();
        if (list && p1)
            return (list->find(p1, len) != NULL);
        else
            return 0;
    case ExprToken::EXP_IPMATCH:
        if (pTok->getAcl())
            return pTok->getAcl()->hasAccess(p1);
        break;
    case ExprToken::EXP_DIR_EXIST:
    case ExprToken::EXP_FILE_DIR_EXIST:
    case ExprToken::EXP_REGULR_FILE_EXIST:
    case ExprToken::EXP_NON_EMPTY_FILE:
    case ExprToken::EXP_SYMLINK:
    case ExprToken::EXP_SYMLINK2:
    {
        struct stat st;
        if (p1)
            ret = stat(p1, &st);
        else
            return 0;
        if (ret == -1)
            return 0;
        switch(t)
        {
        case ExprToken::EXP_DIR_EXIST:
            return S_ISDIR(st.st_mode);
        case ExprToken::EXP_FILE_DIR_EXIST:
            return S_ISDIR(st.st_mode) || S_ISREG(st.st_mode);
        case ExprToken::EXP_REGULR_FILE_EXIST:
            return S_ISREG(st.st_mode);
        case ExprToken::EXP_NON_EMPTY_FILE:
            return S_ISREG(st.st_mode) && st.st_size > 0;
        case ExprToken::EXP_SYMLINK:
        case ExprToken::EXP_SYMLINK2:
            return S_ISLNK(st.st_mode);
        default:
            return 0;
        }
    }
    case ExprToken::EXP_FILE_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS2:
        //NOTE: not supported for now, always return true.
        return 1;
    case ExprToken::EXP_NOT_EMPTY_STR:
        return len > 0;
    case ExprToken::EXP_EMPTY_STR:
        return len == 0;
    case ExprToken::EXP_TEST_BOOL:
        if (!p1)
            return 0;
        return len == 0 || strcmp(p1, "0") == 0
                || strcasecmp(p1, "off") == 0
                || strcasecmp(p1, "false") == 0
                || strcasecmp(p1, "no") == 0;
    default:
        break;
    }

    return 0;
}


int Expression::evalFunc(HttpSession *pSession, ExprToken::type func,
                         ExprToken *&pTok,
                         ExprRunTime *runtime, char *buf, int buf_size, int depth)
{
    char param_buf[40960] = "";
    int len = evalOperator(pSession, pTok, runtime, param_buf, sizeof(param_buf), depth + 1);
    if (len <= 0)
        return len;
    char *pValue;
    int ret, idx;
    HttpReq *pReq;
    pValue = buf;
    ret = buf_size;
    switch(func)
    {
    case ExprToken::FUNC_REQ:
    case ExprToken::FUNC_HTTP:
    case ExprToken::FUNC_REQ_NOVARY:
        pReq = pSession->getReq();
        idx = HttpHeader::getIndex(param_buf, len);
        if (idx < HttpHeader::H_HEADER_END)
        {
            pValue = (char *)pReq->getHeader(idx);
            if (*pValue)
                ret = pReq->getHeaderLen(idx);
            else
                ret = 0;
        }
        else
            pValue = (char *)pReq->getHeader(param_buf, len, ret);
        break;
    case ExprToken::FUNC_RESP:
        if (pSession->getResp()->getRespHeaders().getHeader(param_buf, len,
                    (const char **)&pValue, ret) == -1)
            ret = 0;
        break;
    case ExprToken::FUNC_ENV:
        pValue = (char *)RequestVars::getEnv(pSession, param_buf, len, ret);
        if (!pValue)
            ret = 0;
        break;
    case ExprToken::FUNC_REQENV:
        break;
    default:
        break;
    }
    if (pValue && ret > 0 && ret <= buf_size && pValue != buf)
        memmove(buf, pValue, ret);
    else
        return 0;
    return ret;
}

#define MAX_EXPR_RECURSIVE  50
int Expression::evalOperator(HttpSession *pSession, ExprToken *&pTok,
                             ExprRunTime *runtime, char *buf, int buf_size, int depth)
{
    char   *p1 = NULL;
    int     len;
    int     ret;
    if (!pTok)
        return 0;
    if (depth > MAX_EXPR_RECURSIVE)
        longjmp(runtime->jmp_too_many_recursive, depth);

    ExprToken::type type = pTok->getType();
    ExprToken *pCur = pTok;
    pTok = pTok->next();
    switch (type)
    {
    case ExprToken::EXP_STRING:
        if (buf)
        {
            if (buf_size >= pCur->getStr()->len())
                memcpy(buf, pCur->getStr()->c_str(), pCur->getStr()->len());
        }
        return pCur->getStr()->len();
    case ExprToken::EXP_FMT:
        {
            char achBuf1[40960];
            if (!buf)
            {
                buf = achBuf1;
                buf_size = sizeof(achBuf1);
            }
            len = buf_size;
            RequestVars::buildString(pCur->getFormat(), pSession, buf,
                                     len, 0, runtime->regex_res);
            return len;
        }
    case ExprToken::EXP_IPMATCH2:
    {
        char ip_buf[256];
        p1 = ip_buf;
        len = RequestVars::getReqVar(pSession, REF_REMOTE_ADDR, p1, sizeof(ip_buf));
        if (pCur->getAcl())
        {
            ret = pCur->getAcl()->hasAccess(p1);
            goto END;
        }
        break;
    }

    case ExprToken::EXP_REGEX:
    case ExprToken::EXP_REGEX_NOMATCH:
    case ExprToken::EXP_IPMATCH:
    case ExprToken::EXP_DIR_EXIST:
    case ExprToken::EXP_FILE_DIR_EXIST:
    case ExprToken::EXP_REGULR_FILE_EXIST:
    case ExprToken::EXP_NON_EMPTY_FILE:
    case ExprToken::EXP_SYMLINK:
    case ExprToken::EXP_SYMLINK2:
    case ExprToken::EXP_FILE_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS:
    case ExprToken::EXP_URL_ALLOW_ACCESS2:
    case ExprToken::EXP_NOT_EMPTY_STR:
    case ExprToken::EXP_EMPTY_STR:
    case ExprToken::EXP_TEST_BOOL:
        if (pTok)
            pTok = pTok->next();
        ret = operatorWithSingleParam(pSession, pCur, runtime, depth);
        goto END;
    case ExprToken::EXP_NOT:
        if (pTok)
            ret = !evalOperator(pSession, pTok, runtime, buf, buf_size, depth + 1);
        else
            ret = 1;
        goto END;
    case ExprToken::EXP_AND:
    case ExprToken::EXP_OR:
        ret = evalOperator(pSession, pTok, runtime, buf, buf_size, depth + 1);
        if (!pTok)
            return 0;
        if (((ret) && (type == ExprToken::EXP_OR)) ||
            ((!ret) && (type == ExprToken::EXP_AND)))
            shortCurcuit(pTok);
        else
            ret = evalOperator(pSession, pTok, runtime, buf, buf_size, depth + 1);
        goto END;

    default:
        if (type > ExprToken::EXP_END)
        {
            ret = evalFunc(pSession, type, pTok, runtime, buf, buf_size, depth);
            return ret;
        }
        break;
    }

    if (!pTok)
        return 0;
    ret = twoParamOperator(pSession, type, pTok, runtime, depth);
END:
    if (buf && buf_size >= 1)
        *buf = '0' + (ret > 0);
    return ret;
}


int Expression::eval(HttpSession *pSession, ExprRunTime *runtime,
                     const char *msg) const
{
    int ret = 0;
    if (setjmp(runtime->jmp_too_many_recursive) == 0)
    {
        char buf[40960];
        ExprToken *pTok = begin();
        ret = evalOperator(pSession, pTok, runtime, buf, sizeof(buf), 0);
        LS_DBG_L(pSession, "%s matches expression '%s', result: %d", msg,
                 getExprStr(), ret);
    }
    else
    {
        LS_ERROR(pSession, "Infinite recursive when process expression '%s'",
                 getExprStr());
    }
    return ret;
}


int Expression::evalValue(HttpSession *pSession, ExprRunTime *runtime,
                     const char *msg, char *buf, int buf_size) const
{
    int ret = 0;
    if (setjmp(runtime->jmp_too_many_recursive) == 0)
    {
        ExprToken *pTok = begin();
        ret = evalOperator(pSession, pTok, runtime, buf, buf_size, 0);
        LS_DBG_L(pSession, "%s matches expression '%s', result: %d", msg,
                 getExprStr(), ret);
    }
    else
    {
        LS_ERROR(pSession, "Infinite recursive when process expression '%s'",
                 getExprStr());
    }
    return ret;
}


static SubstFormat *parseFormat(int type, const char *pBegin, int len)
{
    SubstFormat *pFormat = new SubstFormat();
    pFormat->parse(pBegin, pBegin, pBegin + len, type,
                   (type == SubstFormat::SFT_REGULAR)?'%': '$');
    return pFormat;
}



int Expression::appendToken(ExprToken::type token)
{
    ExprToken *pTok = new ExprToken();
    if (!pTok)
        return -1;
    pTok->setToken(token, NULL);
    insert_front(pTok);
    return 0;
}


int Expression::appendList()
{
    ExprToken *pTok = new ExprToken();
    if (!pTok)
        return -1;
    StringList *list = new StringList();
    if (!list)
    {
        delete pTok;
        return -1;
    }
    pTok->setToken(ExprToken::EXP_LIST, list);
    insert_front(pTok);
    return 0;
}


int Expression::completeParseFunc(ExprToken *pTok, ExprToken::type type,
                                  const char *pBegin, int len)
{
    pBegin += s_func_name_len[type - ExprToken::FUNC_REQ];
    len -= s_func_name_len[type - ExprToken::FUNC_REQ];
    if (len == 0)
    {
        pTok->setToken(type, NULL);
        ExprToken *pTok2 = new ExprToken();
        if (!pTok2)
            return -1;
        pTok2->setToken(ExprToken::EXP_OPENP, NULL);
        insert_front(pTok);
        insert_front(pTok2);
        return 0;
    }
    if (*(pBegin + len - 1) != ')')
    {
        //missing close ')'
        return -1;
    }
    len -= 1;
    if (len > 2 && *pBegin == '\'' && *(pBegin + len - 1) == '\'')
    {
        pBegin ++;
        len -= 2;
    }
    pTok->setToken(type, NULL);
    AutoStr2 *pStr = new AutoStr2(pBegin, len);
    pTok->setToken(type, pStr);
    ExprToken *pTok2 = new ExprToken();
    if (!pTok2)
        return -1;
    pTok2->setToken(ExprToken::EXP_STRING, pStr);
    insert_front(pTok);
    insert_front(pTok2);
    return 0;
}


int Expression::appendString(int type, const char *pBegin, int len)
{
    ExprToken *pTok = new ExprToken();
    ExprToken::type tok = ExprToken::EXP_STRING;
    if (!pTok)
        return -1;
    void *pObj = NULL;
    if (memchr(pBegin, '$', len)
        || (type == SubstFormat::SFT_REGULAR && memchr(pBegin, '%', len)))
    {
        SubstFormat *pFormat = parseFormat(type, pBegin, len);
        if (pFormat)
        {
            tok = ExprToken::EXP_FMT;
            pObj = pFormat;
        }
    }
    else
    {
        ExprToken::type func_tok = ExprToken::parseFunc(pBegin, len);
        if (func_tok != ExprToken::EXP_NONE)
        {
            int ret = completeParseFunc(pTok, func_tok, pBegin, len);
            if (ret == -1)
                delete pTok;
            return ret;
        }
        else
        {
            AutoStr2 *pStr = new AutoStr2(pBegin, len);
            if (pStr)
                pObj = pStr;
        }
    }
    if (!pObj)
    {
        delete pTok;
        return -1;
    }
    pTok->setToken(tok, pObj);
    insert_front(pTok);
    return 0;

}

int Expression::appendRegex(int type, ExprToken::type tok, const char *pBegin,
                            int len)
{
    const Pcregex *regex = NULL;
    ExprToken *pTok = new ExprToken();
    if (!pTok)
        return -1;
    if (memchr(pBegin, '$', len) || memchr(pBegin, '%', len))
    {
        SubstFormat *fmt;
        fmt = parseFormat(type, pBegin, len);
        if (!fmt)
        {
            LS_WARN("Bad variable format: %.*s", len, pBegin);
            delete pTok;
            return -1;
        }
        pTok->setDynamic(tok, fmt);
    }
    else
    {
        regex = parseRegex(pBegin, pBegin + len);
        if (!regex)
        {
            LS_WARN("Bad regular expression: %.*s", len, pBegin);
            delete pTok;
            return -1;
        }
        pTok->setToken(tok, (void *)regex);
    }
    insert_front(pTok);
    return 0;
}


const char *Expression::s_last_parse_error = NULL;

int Expression::parseApacheExpr(const char *expr_str, const char *expr_end)
{
    char *pBegin, *pEnd;
    char *pArgEnd;
    ExprToken::type token = ExprToken::EXP_NONE;
    ExprToken::type last_token = ExprToken::EXP_NONE;
    char ch;
    char in_list = 0;
    m_expr_str.setStr(expr_str, expr_end - expr_str);
    pBegin = m_expr_str.buf();
    pEnd = pBegin + (expr_end - expr_str);
    while (pBegin < pEnd)
    {
        while(pBegin < pEnd && isspace(*pBegin))
            ++pBegin;
        if (pBegin == pEnd)
            break;
        ch = *pBegin;
        if (last_token == ExprToken::EXP_REGEX
             || last_token == ExprToken::EXP_REGEX_NOMATCH)
        {
            if (!(ch == '/' || (ch == 'm' && *(pBegin + 1) == '#')))
            {
                s_last_parse_error = "Not valid regular expression /.../ or m#...#";
                return -1;
            }
            const char *p = pBegin;
            if (ch == 'm')
            {
                ++p;
                ch = '#';
            }
            pArgEnd = (char *)memchr(p + 1, ch, pEnd - p);
            if (pArgEnd)
            {
                int len = pArgEnd - p;
                if (len > 1
                    && (memchr(pBegin, '$', len - 1)
                        || memchr(pBegin, '%', len - 1)))
                {
                    SubstFormat *fmt;
                    fmt = parseFormat(SubstFormat::SFT_REGULAR, pBegin, len + 1);
                    if (!fmt)
                    {
                        LS_WARN("Bad variable format: %.*s", len, pBegin);
                        return -1;
                    }
                    begin()->setDynamic(last_token, fmt);
                    pBegin = pArgEnd + 1;
                    last_token = ExprToken::EXP_NONE;
                    continue;
                }
                else
                {
                    const Pcregex *regex = parseRegex(pBegin, pArgEnd + 1);
                    if (!regex)
                        return -1;
                    begin()->setRegex(regex);
                    pBegin = pArgEnd + 1;
                    last_token = ExprToken::EXP_NONE;
                    continue;
                }
            }
            else
            {
                s_last_parse_error = "Missing terminating char for REGEX /.../ or m#...#";
                return -1;
            }
        }

        pArgEnd = (char *)StringTool::memNextArg(&pBegin, pEnd - pBegin);
        if (pArgEnd)
            *pArgEnd = 0;
        else
            pArgEnd = (char *)pEnd;

        if (!in_list && ch == '{' && pArgEnd == pBegin + 1)
        {
            if (last_token == ExprToken::EXP_IN)
            {
                if (appendList() != -1)
                {
                    last_token = ExprToken::EXP_NONE;
                    in_list = 1;
                    pBegin = pArgEnd + 1;
                    continue;
                }
            }
        }
        else if (ch == '}' && pArgEnd == pBegin + 1 && in_list)
        {
            in_list = 0;
            pBegin = pArgEnd + 1;
            continue;
        }

        if ((ch == '"') || (ch == '\''))
            pBegin += StringTool::unescapeQuote((char *)pBegin, pArgEnd, ch);

        if (in_list)
        {
            if (ch != ',' || pArgEnd != pBegin + 1)
                begin()->getList()->add(pBegin, pArgEnd - pBegin);
            pBegin = pArgEnd + 1;
            if ((ch == '"') || (ch == '\''))
                ++pBegin;
            continue;
        }

        if (last_token == ExprToken::EXP_IPMATCH
            || last_token == ExprToken::EXP_IPMATCH2)
        {
            if (begin()->addAcl(pBegin, pArgEnd - pBegin) != -1)
            {
                pBegin = pArgEnd + 1;
                continue;
            }
        }

        token = ExprToken::parse(pBegin, pArgEnd - pBegin);
        if (token)
        {
            appendToken(token);
            last_token = token;
        }
        else
        {
            appendString(SubstFormat::SFT_REGULAR, pBegin, pArgEnd - pBegin);
            last_token = ExprToken::EXP_NONE;
        }
        pBegin = pArgEnd + 1;
    }
    buildPrefix();
    m_expr_str.setStr(expr_str, expr_end - expr_str);
    return 0;
}


int Expression::parse(int type, const char *pBegin, const char *pEnd)
{
    char achBuf[ 10240];
    char *p = achBuf;
    char quote = 0;
    char space = 1;
    char ch;
    ExprToken::type token = ExprToken::EXP_NONE;
    while (pBegin < pEnd)
    {
        ch = *pBegin++;

        if (quote)
        {
            if (ch == quote)
            {
                //end of a quote string+
                *p = 0;
                if (quote == '/')
                {
                    *p++ = '/';
                    if (*pBegin == 'i')
                    {
                        *p++ = 'i';
                        ++pBegin;
                    }
                    *p = 0;
                    appendRegex(type, token, achBuf, p - achBuf);
                }
                else
                    appendString(type, achBuf, p - achBuf);
                p = achBuf;
                quote = 0;
            }
            else
            {
                if (ch == '\\')
                {
                    if (pBegin >= pEnd)
                        break;
                    ch = *pBegin++;
                }
                *p++ = ch;
            }
            continue;
        }
        switch (ch)
        {
        case '=':
            if (*pBegin == '=' || *pBegin == '~')
                ++pBegin;
            token = ExprToken::EXP_EQ;
            break;
        case '!':
            if (*pBegin == '=' || *pBegin == '~')
            {
                ++pBegin;
                token = ExprToken::EXP_NE;
            }
            else
            {
                token = ExprToken::EXP_NOT;
            }
            break;
        case '<':
            if (*pBegin == '=')
            {
                ++pBegin;
                token = ExprToken::EXP_LE;
            }
            else
                token = ExprToken::EXP_LESS;
            break;
        case '>':
            if (*pBegin == '=')
            {
                ++pBegin;
                token = ExprToken::EXP_GE;
            }
            else
                token = ExprToken::EXP_GREAT;
            break;
        case '(':
            token = ExprToken::EXP_OPENP;
            break;
        case ')':
            token = ExprToken::EXP_CLOSEP;
            break;
        case '&':
            if (*pBegin == '&')
                ++pBegin;
            token = ExprToken::EXP_AND;
            break;
        case '|':
            if (*pBegin == '|')
                ++pBegin;
            token = ExprToken::EXP_OR;
            break;
        case ' ':
        case '\t':
            if (!space)
                space = 1;
            continue;

        case '"':
        case '\'':
            quote = ch;
            break;
        case '\\':
            if (pBegin >= pEnd)
                break;
            ch = *pBegin++;
        //fall through
        default:
            if (space)
            {
                if (p != achBuf)
                    *p++ = ' ';
                space = 0;
            }
            *p++ = ch;
            continue;
        }
        space = 0;
        if (p != achBuf)
        {
            *p = 0;
            appendString(type, achBuf, p - achBuf);
            p = achBuf;
        }
        if (token)
        {
            if ((token == ExprToken::EXP_EQ) ||
                (token == ExprToken::EXP_NE))
            {
                while (isspace(*pBegin))
                    ++pBegin;
                if (*pBegin == '/')
                {
                    ++pBegin;
                    quote = '/';
                    *p++ = '/';
                    if (token == ExprToken::EXP_EQ)
                        token = ExprToken::EXP_REGEX;
                    else
                        token = ExprToken::EXP_REGEX_NOMATCH;
                    continue;
                }
            }
            appendToken(token);
            token = ExprToken::EXP_NONE;
        }
    }
    if (p != achBuf)
    {
        appendString(type, achBuf, p - achBuf);
        p = achBuf;
    }
    buildPrefix();
    return 0;
}

/*
Algorithm
1) Reverse the input string.
2) Examine the next element in the input.
3) If it is operand, add it to output string.
4) If it is Closing parenthesis, push it on stack.
5) If it is an operator, then
i) If stack is empty, push operator on stack.
ii) If the top of stack is closing parenthesis, push operator on stack.
iii) If it has same or higher priority than the top of stack, push operator on stack.
iv) Else pop the operator from the stack and add it to output string, repeat step 5.
6) If it is a opening parenthesis, pop operators from stack and add them to output string until a closing parenthesis is encountered. Pop and discard the closing parenthesis.
7) If there is more input go to step 2
8) If there is no more input, unstack the remaining operators and add them to output string.
9) Reverse the output string.
*/

int Expression::buildPrefix()
{
    TslinkList<ExprToken> list;
    TPointerList<ExprToken> stack;
    ExprToken *tok;
    ExprToken *op;
    list.swap(*this);

    while ((tok = list.pop()) != NULL)
    {
        if (tok->getType() < ExprToken::EXP_OPENP)
            insert_front(tok);
        else if (tok->getType() == ExprToken::EXP_CLOSEP)
            stack.push_back(tok);
        else if (tok->getType() > ExprToken::EXP_CLOSEP)
        {
            while (1)
            {
                if ((stack.empty()) ||
                    (tok->getPriority() >= stack.back()->getPriority()))
                {
                    stack.push_back(tok);
                    break;
                }
                else
                {
                    op = stack.pop_back();
                    insert_front(op);
                }
            }
        }
        else if (tok->getType() == ExprToken::EXP_OPENP)
        {
            while ((!stack.empty()) &&
                   (stack.back()->getType() != ExprToken::EXP_CLOSEP))
            {
                op = stack.pop_back();
                insert_front(op);
            }
            if (!stack.empty())
            {
                op = stack.pop_back();
                delete op;
            }
            delete tok;
        }
    }
    while (!stack.empty())
    {
        op = stack.pop_back();
        if (op->getType() != ExprToken::EXP_CLOSEP)
            insert_front(op);
        else
            delete op;
    }

    return 0;
}




