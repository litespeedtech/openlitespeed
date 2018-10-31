/*
 * 
 */

#include "sslcontextconfig.h"



SslContextConfig::SslContextConfig()
{
    LS_ZERO_FILL(m_iKeyCerts, m_iOcspMaxAge);
}

SslContextConfig::~SslContextConfig()
{

}
