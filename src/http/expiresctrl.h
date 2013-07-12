/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef EXPIRESCTRL_H
#define EXPIRESCTRL_H


#define EXPIRES_NOT_SET 0
#define EXPIRES_ACCESS  1
#define EXPIRES_MODIFY  2

#define CONFIG_EXPIRES  1
#define CONFIG_COMPRESS 2
#define CONFIG_HANDLER  4
#define CONFIG_IMAGE    8

class XmlNode;
class HttpContext;
  
class ExpiresCtrl
{
public: 
    ExpiresCtrl();
    ~ExpiresCtrl();
    ExpiresCtrl( const ExpiresCtrl & rhs );

    void    copyExpires( const ExpiresCtrl & rhs );
    char    isEnabled() const   {   return m_iEnabled;      }
    char    getBase() const     {   return m_iBase;         }
    int     getAge() const      {   return m_iAge;          }
    char    compressable() const{   return m_iCompressable; }
    char    cfgHandler() const  {   return m_iBits & CONFIG_HANDLER;    }
    char    cfgCompress() const {   return m_iBits & CONFIG_COMPRESS;   }
    char    cfgExpires() const  {   return m_iBits & CONFIG_EXPIRES;    }
    char    isImage() const     {   return m_iBits & CONFIG_IMAGE;      }
    
    void    enable( int enable )    {   m_iEnabled = enable;    }
    void    setBase( int base )     {   m_iBase = base;         }
    void    setAge( int age )       {   m_iAge = age;           }
    void    setCompressable(int c)  {   m_iCompressable = c;    }
    void    setBit( char bit )      {   m_iBits |= bit;         }
    void    clearBit( char bit )    {   m_iBits &= ~bit;        }
        
    int     parse( const char * pConfig );
    int     config( const XmlNode * pExpires, const ExpiresCtrl * pDefault, 
                    HttpContext * pContext );
    
private:
    char    m_iEnabled;
    char    m_iBase;
    char    m_iCompressable;
    char    m_iBits;
    int     m_iAge;
};

class ExpiresCtrlConfig
{
private:
    ExpiresCtrl * m_pDefault;
    HttpContext * m_pContext;
    
public:
    ExpiresCtrlConfig( ExpiresCtrl * pDefault, HttpContext * pContext )
        : m_pDefault( pDefault )
        , m_pContext( pContext )
        {}
    int operator()( ExpiresCtrl * pCtrl, const XmlNode * pNode ) ;

};


#endif
