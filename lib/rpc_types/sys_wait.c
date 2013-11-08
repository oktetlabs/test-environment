/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/wait.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_wait.h"


/** Convert RPC waitpid options to native options */
int
waitpid_opts_rpc2h(rpc_waitpid_opts opts)
{
    return (!!(opts & RPC_WNOHANG) * WNOHANG) |
           (!!(opts & RPC_WUNTRACED) * WUNTRACED) |
#ifdef WCONTINUED
           (!!(opts & RPC_WCONTINUED) * WCONTINUED) |
#endif
           0;
}


/** Convert status flag to string */
const char *
wait_status_flag_rpc2str(rpc_wait_status_flag flag)
{
    return
        flag == RPC_WAIT_STATUS_EXITED ? "EXITED" :
        flag == RPC_WAIT_STATUS_SIGNALED ? "SIGNALED" :
        flag == RPC_WAIT_STATUS_STOPPED ? "STOPPED" :
        flag == RPC_WAIT_STATUS_RESUMED ? "RESUMED" :
        flag == RPC_WAIT_STATUS_CORED ? "CORED" : "UNKNOWN";
}

/** Convert native status value to RPC status structure */
rpc_wait_status
wait_status_h2rpc(int st)
{
    rpc_wait_status ret = {RPC_WAIT_STATUS_UNKNOWN, 0};

    if (WIFEXITED(st))
    {
        ret.flag = RPC_WAIT_STATUS_EXITED;
        ret.value = WEXITSTATUS(st);
    } 
    else if (WIFSIGNALED(st))
    {
        ret.value = signum_h2rpc(WTERMSIG(st));
#ifdef WCOREDUMP
        if (WCOREDUMP(st))
            ret.flag = RPC_WAIT_STATUS_CORED;
        else
#endif
            ret.flag = RPC_WAIT_STATUS_SIGNALED;
    }
    else if (WIFSTOPPED(st))
    {
        ret.flag = RPC_WAIT_STATUS_STOPPED;
        ret.value = signum_h2rpc(WSTOPSIG(st));
    }
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(st))
        ret.flag = RPC_WAIT_STATUS_RESUMED;
#endif

    return ret;
}
