/*
Copyright (c) 2014, LiteSpeed Technologies Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer. 
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution. 
    * Neither the name of the Lite Speed Technologies Inc nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.  

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/
/*******************************************************************
 * How to test this sample
 * 1, build it, put the file reqinfomodule.so to $LSWS_HOME/modules
 * 2, curl -F file=@/home/.../test1 http://localhost:8088/ 
 * 3, if test1 contain at least one word in blockWords, will get 403
 *    otherwise, will get 200
 ********************************************************************/
#define _GNU_SOURCE 
#include "../include/ls.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "loopbuff.h"

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       reqobservmodule
/////////////////////////////////////////////////////////////////////////////

//Test if the request body contains below words, if so 403
const char *blockWords[] = {
    "badword1",
    "badword2",
    "badword3",
};

struct lsi_module_t MNAME;

static int hasBadWord(const char *s, size_t len)
{
    int i;
    int ret = 0;
    for (i=0; i<sizeof(blockWords)/ sizeof( char *); ++i)
    {
        if (memmem(s, len, blockWords[i], strlen(blockWords[i])) != NULL )
        {
            ret = 1;
            break;
        }
    }
    
    return ret;   
}


int check_file(int fd)
{
    char buf[8192] = {0};
    int n;
    //const int maxBadWordLength = 20;
    int ret = LSI_RET_OK;
    
    //seek to begin of the file
    lseek(fd, 0, SEEK_SET);
    
    while((n = read(fd, buf, 8192)) != 0)
    {
//         if (n == 8192)
//             lseek(fd, -1 * maxBadWordLength, SEEK_CUR);
        if (hasBadWord(buf, n))
        {
            ret = LSI_RET_ERROR;
            break;
        }
    }
    
    return ret;
}       
    
    
int check_req_whole_body(struct lsi_cb_param_t *rec)
{
    int fd = g_api->get_req_body_file_fd( rec->_session );
    if (fd != -1)
    {
        return check_file(fd);
    }
    return LSI_RET_OK;
}

static int _init()
{
    g_api->add_hook( LSI_HKPT_RECVED_REQ_BODY, &MNAME, check_req_whole_body, LSI_HOOK_EARLY , 0);
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, };

