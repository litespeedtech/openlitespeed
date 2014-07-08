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

#include "../include/ls.h"
#include "loopbuff.h"
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include <unistd.h>

#define     MNAME       testserverhook
lsi_module_t MNAME;

int write_log(const char *sHookName)
{
    g_api->log( NULL, LSI_LOG_DEBUG, "[Module:testserverhook] launch point %s\n", sHookName);
    return 0;
}

int writeALog1(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_MAIN_INITED"); }
int writeALog2(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_MAIN_PREFORK"); }
int writeALog3(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_MAIN_POSTFORK"); }
int writeALog4(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_WORKER_POSTFORK"); }
int writeALog5(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_WORKER_ATEXIT"); }
int writeALog6(lsi_cb_param_t * rec) {   return write_log("LSI_HKPT_MAIN_ATEXIT"); }


static lsi_serverhook_t serverHooks[] = {
    { LSI_HKPT_MAIN_INITED,    writeALog1, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_MAIN_PREFORK,      writeALog2, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_MAIN_POSTFORK,     writeALog3, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_WORKER_POSTFORK,         writeALog4, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_WORKER_ATEXIT,         writeALog5, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_MAIN_ATEXIT,         writeALog6, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t * pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "testserverhook v1.0", serverHooks};
