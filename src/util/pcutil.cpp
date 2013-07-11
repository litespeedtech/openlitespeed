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
#include <util/pcutil.h>
#include <sys/types.h>
#include <sys/wait.h>


PCUtil::PCUtil(){
}
PCUtil::~PCUtil(){
}


int PCUtil::waitChildren()
{
    int status, pid;
    int count = 0;
    while( true )
    {
        pid = waitpid( -1, &status, WNOHANG|WUNTRACED );
        if ( pid <= 0 )
        {
            //if ((pid < 1)&&( errno == EINTR ))
            //    continue;
            break;
        }
        ++count;
    }
    return count;
}


