/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include "profiletime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ProfileTime::ProfileTime( const char * pDesc )
{
    assert( NULL != pDesc );
    m_pDesc = strdup( pDesc );
    printf( "Start executing: %s\n", pDesc );
    int ret = gettimeofday( &m_tv, NULL );
    assert( 0 == ret );
}
ProfileTime::~ProfileTime()
{
    struct timeval now;
    int ret = gettimeofday( &now, NULL );
    assert( 0 == ret );
    printf( "End executing %s, took: %ld microseconds\n", m_pDesc,
            ((long) now.tv_sec - m_tv.tv_sec ) * 1000000 +
            now.tv_usec - m_tv.tv_usec );
    free( m_pDesc );
}

