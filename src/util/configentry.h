#ifndef CONFIGENTRY_H
#define CONFIGENTRY_H

#include <util/autostr.h>
#include <inttypes.h>

class ConfigValidator
{
public:
    ConfigValidator( long long min, long long max, long long def )
    {}
    int operator()(long long val )const 
    {}
};

class ConfigIntValidation
{
    long long m_iDefault;
    long long m_iMin;
    long long m_iMax;
};


class ConfigEntry
{

public:
    ConfigEntry();
    ~ConfigEntry();
    
public:
    enum
    {
        NUMERIC,
        STRING,
        PATH,
        FILE,
        VALID_PATH,
        VALID_FILE,
        
    };

private:
    ConfigEntry& operator=(const ConfigEntry& other);
    ConfigEntry(const ConfigEntry& other);

private:
    AutoStr     m_sName;
    short       m_iValueType;
    short       m_iOptinal;
    long long   m_iDefault; 
    long long   m_iMin;
    long long   m_iMax;
    AutoStr     m_sRegexValidate;
    
    
};

#endif // CONFIGENTRY_H
