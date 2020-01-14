#include <util/filtermatch.h>
#include <ctype.h>
#include <string.h>
#include <fnmatch.h>

FilterMatch::FilterMatch(const char * filter)
    : m_negate(false)
    , m_igncase(false)
    , m_reg()
    , m_fnmatchFlags(0)
    , m_filter(NULL)
    , m_filterLen(0)
    , m_type(STRCMP)
{
    init(filter);
}

FilterMatch::~FilterMatch()
{
    delete[] m_filter;
    // do we need to do anything with m_reg?
}

void FilterMatch::init(const char * filter)
{
    // filter string formats:
    // optional leading '!' = negate comparison
    // s... - simple string compare
    // i... - simple string compare, ignore case
    // fd.. - fnmatch, d= or'd 1, 2, 4 to enable FNM_NOESCAPE, FNM_PATHNAME, FNM_PERIOD
    // gd.. - same as fd + FNM_CASEFOLD
    // r... - regex

    if (!filter)
    {
        return;
    }
    size_t filter_len = strlen(filter); // (without the terminating NUL)
    if ('!' == *filter)
    {
        m_negate = true;
        filter++;
        filter_len--;
    }

    int flags = 0;
    switch(*filter)
    {
    case 'i':
        m_igncase = true;
        // fall through
    case 's':
        m_filter = new char[filter_len + 1];
        memcpy((void *)m_filter, filter+1, filter_len); // skip the first char, include the NUL
        m_filterLen = filter_len - 1;
        break;
    case 'g':
        m_fnmatchFlags |= FNM_CASEFOLD;
        // fall through
    case 'f':
        m_type = FNMATCH;

        flags = *(filter + 1) - '0';
        if (flags & 0x1)
        {
            m_fnmatchFlags |= FNM_NOESCAPE;
        }
        if (flags & 0x2)
        {
            m_fnmatchFlags |= FNM_PATHNAME;
        }
        if (flags & 0x4)
        {
            m_fnmatchFlags |= FNM_PERIOD;
        }
        m_filter = new char[filter_len - 1];
        memcpy((void *)m_filter, filter+2, filter_len - 1); // skip the first 2 chars, include the NUL
        m_filterLen = filter_len - 2;
        break;
    case 'r':
        m_type = REGEX;
        // we don't really need to save filter - look at removing this?
        m_filter = new char[filter_len];
        memcpy((void *)m_filter, filter+1, filter_len); // skip the first char, include the NUL
        m_filterLen = filter_len - 1;
        // no support for options yet
        m_reg.compile(filter + 1, 0);
        break;
    }
}

bool FilterMatch::match(const char * val, size_t val_len)
{
    bool ret = false;
    if (!val)
    {
        // null val doesn't match anything, if negated make it true
        // don't continue to strlen on NULL ptr
        return m_negate;
    }
    if (0 == val_len)
    {
        val_len = strlen(val);
    }
    char buf[val_len + 1]; // we don't know if val is NUL terminated, fnmatch has no count limit

    switch(m_type)
    {
    case STRCMP:
        if (m_filterLen != val_len) {
            ret = false;
            break;
        }
        if (m_igncase)
        {
            ret = (0 == strncasecmp(m_filter, val, m_filterLen));
        }
        else
        {
            ret = (0 == strncmp(m_filter, val, m_filterLen));
        }
        break;
    case FNMATCH:
        memcpy((void *)buf, val, val_len);
        buf[val_len] = '\0';
        ret = (0 == fnmatch(m_filter, buf, m_fnmatchFlags));
        break;
    case REGEX:
        int ovec[30];
        int rexec = m_reg.exec(val, val_len, 0, 0, ovec, 30);
        ret = (rexec >= 0);
        break;
    }
    return m_negate ? !ret : ret;
}
