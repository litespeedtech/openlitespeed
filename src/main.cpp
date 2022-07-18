/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <main/lshttpdmain.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>


//#define WANT_LSAN 1
#if !WANT_LSAN

int ls_lsan_off = 1;

extern "C" const char* __asan_default_options()
{ return "disable_coredump=0:unmap_shadow_on_exit=1:detect_leaks=0:abort_on_error=1"; }

#else

int ls_lsan_off = 0;

extern "C" const char* __asan_default_options()
{ return "disable_coredump=0:unmap_shadow_on_exit=1:abort_on_error=1"; }


extern "C" const char* __lsan_default_suppressions()
{
    return "leak:chunk_alloc";
};


#endif

extern "C" int __lsan_is_turned_off() { return ls_lsan_off; }


static LshttpdMain *s_pLshttpd = NULL;
int main(int argc, char *argv[])
{
    s_pLshttpd = new LshttpdMain();
    if (!s_pLshttpd)
    {
        perror("new LshttpdMain()");
        exit(1);
    }
    int ret = s_pLshttpd->main(argc, argv);
    delete s_pLshttpd;
    exit(ret);
}



