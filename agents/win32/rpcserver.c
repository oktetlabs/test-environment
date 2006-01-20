/** @file
 * @brief Windows Test Agent
 *
 * Standalone RPC server implementation 
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2006 Level5 Networks Corp.
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "tarpc_server.h"
#include "rpc_xdr.h"

DEFINE_LGR_ENTITY("(win32_rpcserver)");

extern void logfork_log_message(const char *file, unsigned int line,
                                unsigned int level,
                                const char *entity,
                                const char *user,
                                const char *fmt, ...);


te_log_message_f te_log_message = logfork_log_message;

#ifdef CL

/**
 * Convert 'struct timeval' to 'struct tarpc_timeval'.
 * 
 * @param tv_h      Pointer to 'struct timeval'
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 */
int
timeval_h2rpc(const struct timeval *tv_h, struct tarpc_timeval *tv_rpc)
{
    tv_rpc->tv_sec  = tv_h->tv_sec;
    tv_rpc->tv_usec = tv_h->tv_usec;

    return 0;
}

/**
 * Convert 'struct tarpc_timeval' to 'struct timeval'.
 * 
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 * @param tv_h      Pointer to 'struct timeval'
 */
int
timeval_rpc2h(const struct tarpc_timeval *tv_rpc, struct timeval *tv_h)
{
    tv_h->tv_sec  = tv_rpc->tv_sec;
    tv_h->tv_usec = tv_rpc->tv_usec;

    return 0;
}

int 
gettimeofday(struct timeval *tv, struct timezone *tz)
{
    SYSTEMTIME t;

    UNUSED(tz);
    
    GetSystemTime(&t);
    tv->tv_sec = time(NULL);
    tv->tv_usec = t.wMilliseconds * 1000;
}

#endif

HINSTANCE ta_hinstance;

#include "../../lib/rcfpch/rcf_pch_rpc_server.c"

int WINAPI 
WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
        LPSTR lpCmdLine, int nCmdShow) 
{
    WSADATA data;

    WSAStartup(MAKEWORD(2,2), &data);
    
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);
    
    ta_hinstance = hinstance;
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    wsa_func_handles_discover();
    
    rcf_pch_rpc_server(lpCmdLine);
    
    _exit(0);
}

