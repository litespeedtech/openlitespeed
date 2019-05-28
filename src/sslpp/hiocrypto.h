#ifndef HIOCRYPTO_H
#define HIOCRYPTO_H
#include <unistd.h>
typedef struct x509_st X509;
class HioCrypto
{
public:
    enum ENV
    {
        CRYPTO_VERSION,
        SESSION_ID,
        CIPHER,
        CIPHER_USEKEYSIZE,
        CIPHER_ALGKEYSIZE,
        CLIENT_CERT,
        TRANS_PROTOCOL_VERSION
    };
    HioCrypto()             {}
    virtual ~HioCrypto()    {}
    
    virtual int getEnv(HioCrypto::ENV id, char *&val,int maxValLen) = 0;
    virtual X509 *getPeerCertificate() const
    {   return NULL;    }
    virtual int  getVerifyMode() const
    {   return 0;       }
    virtual int  isVerifyOk() const
    {   return 0;       }
    int  buildVerifyErrorString(char *pBuf, int len) const
    {   return 0;       }
    
};


#endif // HIOCRYPTO_H
