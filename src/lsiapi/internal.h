
#ifndef LSAPI_INTERNAL_H
#define LSAPI_INTERNAL_H
class ModuleConfig;

class LsiSession
{
public:
    LsiSession(){};
    virtual ~LsiSession(){};
    ModuleConfig * getModuleConfig()    { return m_pModuleConfig; };
    
protected:
    ModuleConfig * m_pModuleConfig;
};


#endif
