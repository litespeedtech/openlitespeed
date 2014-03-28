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
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux
#include <sched.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#endif


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

int PCUtil::getNumProcessors()
{
    int numCPU;
#if defined(linux) || defined(__linux) || defined(__linux__)
    numCPU = sysconf( _SC_NPROCESSORS_ONLN );
#else
    int mib[2];
    size_t len = sizeof(numCPU);
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &numCPU, &len, NULL, 0);
    if ( numCPU <= 0 )
    {
        mib[1] = HW_NCPU;
        sysctl( mib, 2, &numCPU, &len, NULL, 0);
    }
#endif
    return ( (numCPU <= 0) ? 0 : numCPU);
}

void PCUtil::getAffinityMask( int iCpuCount, int iProcessNum, int iNumCoresToUse, cpu_set_t *mask)
{
    CPU_ZERO(mask); 
    if(iCpuCount <= iNumCoresToUse)
        for(int i = 0; i < iCpuCount; i++)
            CPU_SET( i, mask );
    else
        for(int i = 0; i < iNumCoresToUse; i++)
            CPU_SET( 
                ((((iProcessNum + (i*3) )%iCpuCount ) / (iCpuCount/2) ) + (2 * (iProcessNum + (i*3))) % iCpuCount), 
                    mask
                   );
    return;
}

int PCUtil::setCpuAffinity(cpu_set_t *mask)
{
#ifdef __FreeBSD__
    return cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(cpu_set_t), mask);
#elif defined(linux) || defined(__linux) || defined(__linux__)
    return sched_setaffinity(0, sizeof(cpu_set_t), mask);
#endif
    return 0;
}


