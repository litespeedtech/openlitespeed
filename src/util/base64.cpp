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
#include <util/base64.h>

const unsigned char Base64::s_decodeTable[128] =
{
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  /* 00-0F */
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  /* 10-1F */
    255,255,255,255,255,255,255,255,255,255,255,62,255,255,255,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,255,255,255,255,255,255,          /* 30-3F */
    255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,               /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,255,255,255,255,255,           /* 50-5F */
    255,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,               /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,255,255,255,255,255            /* 70-7F */
};


Base64::Base64(){
}
Base64::~Base64(){
}

int Base64::decode( const char * encoded, int size, char * decoded )
{
    register const char * pEncoded = encoded;
    register unsigned char e1, prev_e = 0;
    register char           phase = 0;
    register unsigned char * pDecoded = (unsigned char *)decoded;
    register const char * pEnd = encoded + size ;
    
    while(  pEncoded < pEnd )
    {
        register int ch = *pEncoded++;
        if ( ch < 0 )
            continue;
        e1 = s_decodeTable[ ch ];
        if ( e1 != 255 )
        {
            switch ( phase )
            {
            case 0:
                break;
            case 1:
                *pDecoded++ = ( ( prev_e << 2 ) | ( ( e1 & 0x30 ) >> 4 ) );
                break;
            case 2:
                *pDecoded++ = ( ( ( prev_e & 0xf ) << 4 ) | ( ( e1 & 0x3c ) >> 2 ) );
                break;
            case 3:
                *pDecoded++ = ( ( ( prev_e & 0x03 ) << 6 ) | e1 );
                phase = -1;
            break;
            }
            phase++;
            prev_e = e1;
        }
    }
    *pDecoded = 0;
    return pDecoded - (unsigned char *)decoded;
}


