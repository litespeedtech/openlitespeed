
#ifndef LS_EXPRESSION_H
#define LS_EXPRESSION_H

#include <lsdef.h>
#include <util/autostr.h>
#include <util/tlinklist.h>
#include <util/gpointerlist.h>

#include <setjmp.h>

class AccessControl;
class HttpSession;
class StringList;
class SubstFormat;
class SubstItem;
class Pcregex;
class RegexResult;


struct ExprRunTime
{
    AutoStr2    *regex_input;
    RegexResult *regex_res;
    const char  *time_format;
    jmp_buf      jmp_too_many_recursive;
    ExprRunTime()
    {
        regex_input = NULL;
        regex_res = NULL;
        time_format = NULL;
    }
};


class ExprToken : public TlinkNode<ExprToken>
{
public:
    enum type
    {
        EXP_NONE,
        EXP_STRING,
        EXP_LIST,
        EXP_FMT,
        EXP_OPENP,
        EXP_CLOSEP,
        EXP_REGEX,
        EXP_EQ,
        EXP_NE,
        EXP_LESS,
        EXP_LE,
        EXP_GREAT,
        EXP_GE,
        EXP_NOT,
        EXP_AND,
        EXP_OR,
        EXP_REGEX_NOMATCH,
        EXP_IN,
        EXP_BEGIN_WITH_DASH,
        EXP_EQ_NUM = EXP_BEGIN_WITH_DASH,
        EXP_NE_NUM,
        EXP_LESS_NUM,
        EXP_LE_NUM,
        EXP_GREAT_NUM,
        EXP_GE_NUM,
        EXP_IPMATCH,
        EXP_STR_MATCH,
        EXP_STR_CMATCH,
        EXP_FNMATCH,
        EXP_DIR_EXIST,
        EXP_FILE_DIR_EXIST,
        EXP_REGULR_FILE_EXIST,
        EXP_NON_EMPTY_FILE,
        EXP_SYMLINK,
        EXP_SYMLINK2,
        EXP_FILE_ALLOW_ACCESS,
        EXP_URL_ALLOW_ACCESS,
        EXP_URL_ALLOW_ACCESS2,
        EXP_NOT_EMPTY_STR,
        EXP_EMPTY_STR,
        EXP_TEST_BOOL,
        EXP_IPMATCH2,
        EXP_END,

        FUNC_REQ,
        FUNC_HTTP,
        FUNC_REQ_NOVARY,
        FUNC_RESP,
        FUNC_REQENV,
        FUNC_OSENV,
        FUNC_NOTE,
        FUNC_ENV,
        FUNC_TOLOWER,
        FUNC_TOUPPER,
        FUNC_ESCAPE,
        FUNC_UNESCAPE,
        FUNC_BASE64,
        FUNC_UNBASE64,
        FUNC_MD5,
        FUNC_SHA1,
        FUNC_FILE,
        FUNC_FILEMOD,
        FUNC_FILESIZE,
        FUNC_END,
    };

    ExprToken()
        : m_type(EXP_NONE)
        , m_dynamic(false)
        , m_obj(NULL)
    {}
    ~ExprToken();

    static ExprToken::type parse(const char *pBegin, int len);
    static ExprToken::type parseFunc(const char *pBegin, int len);

    int addAcl(const char *pBegin, int len);

    void setToken(ExprToken::type type, void *obj)
    {   m_type = type; m_obj = obj;     }
    ExprToken::type getType() const     {   return m_type;  }
    void *getObj() const                {   return m_obj;   }

    AutoStr2  *getStr() const
    {   assert(m_type == EXP_STRING);
        return m_str;
    }

    SubstFormat *getFormat() const
    {   assert(m_type == EXP_FMT);
        return m_fmt;
    }

    void setDynamic(ExprToken::type type, SubstFormat * fmt)
    {
        m_type = type;  m_dynamic = true;   m_fmt = fmt;
    }

    SubstFormat *getDynamic() const
    {   assert(m_dynamic);
        return m_fmt;
    }

    void setRegex(const Pcregex *regex)   {   m_pcregex = regex;  }

    const Pcregex *getRegex() const
    {   assert(m_type == EXP_REGEX || m_type == EXP_REGEX_NOMATCH);
        return m_pcregex;
    }

    StringList *getList() const
    {
        assert(m_type == EXP_LIST || m_type == EXP_IN);
        return m_list;
    }

    AccessControl *getAcl() const
    {
        assert(m_type == EXP_IPMATCH || m_type == EXP_IPMATCH2);
        return m_acl;
    }

    static int s_priority[FUNC_END];
    int getPriority() const         {   return s_priority[m_type];  }

    bool isFunc() const             {   return m_type > EXP_END;    }
    bool isDynamic() const          {   return m_dynamic;           }

private:

    type     m_type;
    bool     m_dynamic;
    union
    {
        void        *m_obj;
        AutoStr2    *m_str;
        SubstFormat *m_fmt;
        const Pcregex     *m_pcregex;
        StringList  *m_list;
        AccessControl *m_acl;
    };
};


class Expression : public LinkedObj, public TslinkList<ExprToken>
{
    int appendToken(ExprToken::type token);
    int appendString(int type, const char *pBegin, int len);
    int appendRegex(int type, ExprToken::type tok, const char *pBegin, int len);
    int appendList();
    int buildPrefix();
    static int evalOperator(HttpSession *pSession, ExprToken *&pTok,
                     ExprRunTime *runtime, char *buf, int buf_size, int depth);
    static int twoParamOperator(HttpSession *pSession, ExprToken::type type,
                                ExprToken *&pTok, ExprRunTime *runtime, int depth);
    static int operatorWithSingleParam(HttpSession *pSession, ExprToken *pTok,
                                        ExprRunTime *runtime, int depth);
    static int evalFunc(HttpSession *pSession, ExprToken::type func,
                        ExprToken *&pTok, ExprRunTime *runtime, char *buf,
                        int buf_size, int depth);
    static int execRegex(HttpSession *pSession, ExprToken *&pTok,
                         ExprRunTime *runtime, const char *input, int len);

    int parseOperand(const char *pBegin, const char *pEnd);
    int completeParseFunc(ExprToken *pTok, ExprToken::type tok,
                          const char *pBegin, int len);

public:
    Expression() {}
    ~Expression() { release_objects();  }
    int parseApacheExpr(const char *pBegin, const char *pEnd);
    int parse(int type, const char *pBegin, const char *pEnd);
    int eval(HttpSession *pSession, ExprRunTime *runtime, const char *msg) const;
    int evalValue(HttpSession *pSession, ExprRunTime *runtime, const char *msg,
        char *buf, int buf_size) const;

    const char *getExprStr() const  {   return m_expr_str.c_str();  }
    static const Pcregex *parseRegex(const char *pBegin, const char *pEnd);
    static const char *getLastParseError()
    {   return s_last_parse_error;  }
private:
    AutoStr m_expr_str;
    static const char *s_last_parse_error;
};




#endif
