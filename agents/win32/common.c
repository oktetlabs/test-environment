/** @file
 * @brief Windows Test Agent
 *
 * Functions used by both TA and standalone RPC server 
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

#ifdef WINDOWS
INCLUDE(winsock2.h)
INCLUDE(iphlpapi.h)
#else
#include <winsock2.h>
#include <w32api/iphlpapi.h>
#include <stdio.h>
#endif
#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "ta_common.h"

extern void *rcf_ch_symbol_addr_auto(const char *name, int is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);

/* See description in rcf_ch_api.h */
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    return rcf_ch_symbol_addr_auto(name, (int)is_func);
}

/* See description in rcf_ch_api.h */
char *
rcf_ch_symbol_name(const void *addr)
{
    return rcf_ch_symbol_name_auto(addr);
}

/** 
 * Get identifier of the current thread. 
 *
 * @return windows thread identifier
 */
uint32_t
thread_self()
{
    return GetCurrentThreadId();
}

/** 
 * Create a mutex.
 *
 * @return Mutex handle
 */
void *
thread_mutex_create(void)
{
    return (void *)CreateMutex(NULL, FALSE, NULL);
}

/** 
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
void 
thread_mutex_destroy(void *mutex)
{
    if (mutex != NULL)
        CloseHandle(mutex);
}

/** 
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 *
 */
void 
thread_mutex_lock(void *mutex)
{
    if (mutex != NULL)
        WaitForSingleObject(mutex, INFINITE);
}

/** 
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
void 
thread_mutex_unlock(void *mutex)
{
    if (mutex != NULL)
        ReleaseMutex(mutex);
}

/** Replaces cygwin function */
int __cdecl
setenv(const char *name, const char *value, int overwrite)
{
    HMODULE hCygwinDll;
    static int load_failed = 0;
    static FARPROC cygwin_setenv = NULL;

    if (!load_failed && (NULL == cygwin_setenv))
    {
        hCygwinDll = LoadLibrary("cygwin1.dll");
        if (NULL != hCygwinDll)
        {
          cygwin_setenv = GetProcAddress(hCygwinDll, "setenv");
        }
        else
        {
          load_failed = 1;
        }
    }
    if (NULL != cygwin_setenv)
    {
      /* Make sure that cygwin env. is also updated
       * (this is necessary for subsequent calls of cygwin's getenv)
       */
      cygwin_setenv(name, value, overwrite);
    }
    return SetEnvironmentVariable(name, value) ? 0 : -1;
}

/** Replaces cygwin function */
#ifndef WINDOWS
UNSETENV_RETURN_TYPE 
#endif
__cdecl
unsetenv(const char *name)
{
    SetEnvironmentVariable(name, NULL);
#ifdef UNSETENV_RETURNS_INT
    return 0;
#endif
}

/** 
 * Check if exec is requested and call appropriate function.
 *
 * @param argc  argc passed to main()
 * @param argv  argv passed to main()
 *
 * @return Status code.
 * @retval 1    exec is processed
 * @retval 0    exec is not requested
 * @retval -1   failure is happened during exec processing
 */
int
win32_process_exec(int argc, char **argv)
{
    void (* func)(int, char **);
    
    if (strcmp(argv[1], "exec") != 0)
        return 0;
        
    argc -= 2;
    argv += 2;
    
    if (strcmp(argv[0], "net_init") == 0)
    {
        WSADATA data;

        WSAStartup(MAKEWORD(2,2), &data);
        argc--;
        argv++;
    }
    
    func = rcf_ch_symbol_addr(argv[0], 1);

    if (func == NULL)
    {
        printf("Cannot resolve address of the function %s", argv[0]);
        fflush(stdout);
        return -1;
    }
    func(argc - 1, argv + 1);
    return 1;
}

/*
 * Get an IPv4 address on specified interface.
 * See description in tarpc_server.h
 */
te_errno
get_addr_by_ifindex(int if_index, struct in_addr *addr)
{
    MIB_IPADDRTABLE *table;
    int              i;

    DWORD size = 0, rc;
       
    if ((table = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IPADDRTABLE)))
            == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    if ((rc = GetIpAddrTable(table, &size, 0)) != 0)
    {
        table = realloc(table, size);
        rc = GetIpAddrTable(table, &size, 0);
        if (rc != 0)
        {
            free(table);
            return TE_RC(TE_TA_WIN32, TE_EWIN);
        }
    }
     
    if (table->dwNumEntries == 0)
    {
        free(table);
        table = NULL;
    }
   
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex == (DWORD)if_index)
        {
            memcpy(addr, &table->table[i].dwAddr, sizeof(struct in_addr));
            free(table);
            return 0;
        }
    }

    free(table);
    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

