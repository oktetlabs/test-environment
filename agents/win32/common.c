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

#include "winsock2.h"
#include "te_defs.h"
#include "ta_common.h"

extern void *rcf_ch_symbol_addr_auto(const char *name, te_bool is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);

/* See description in rcf_ch_api.h */
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    return rcf_ch_symbol_addr_auto(name, is_func);
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
int 
setenv(const char *name, const char *value, int overwrite)
{
    return SetEnvironmentVariable(name, value) ? 0 : -1;
}

/** Replaces cygwin function */
void 
unsetenv(const char *name)
{
    SetEnvironmentVariable(name, NULL);
}

