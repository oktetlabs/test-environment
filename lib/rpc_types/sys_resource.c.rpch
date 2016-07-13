/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/resource.h.
 * 
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_resource.h"


/* See the description in te_rpc_sys_resource.h */
const char *
rlimit_resource_rpc2str(rpc_rlimit_resource resource)
{
    switch (resource)
    {
        RPC2STR(RLIMIT_CPU);
        RPC2STR(RLIMIT_FSIZE);
        RPC2STR(RLIMIT_DATA);
        RPC2STR(RLIMIT_STACK);
        RPC2STR(RLIMIT_CORE);
        RPC2STR(RLIMIT_RSS);
        RPC2STR(RLIMIT_NOFILE);
        RPC2STR(RLIMIT_AS);
        RPC2STR(RLIMIT_NPROC);
        RPC2STR(RLIMIT_MEMLOCK);
        RPC2STR(RLIMIT_LOCKS);
        RPC2STR(RLIMIT_SIGPENDING);
        RPC2STR(RLIMIT_MSGQUEUE);
        RPC2STR(RLIMIT_SBSIZE);
        default: return "<RLIMIT_NLIMITS>";
    }
}


#ifndef RLIMIT_NLIMITS
#ifdef RLIM_NLIMITS
#define RLIMIT_NLIMITS  RLIM_NLIMITS
#else
#error RLIMIT_NLIMITS cannot be defined
#endif
#endif

/* See the description in te_rpc_sys_resource.h */
int
rlimit_resource_rpc2h(rpc_rlimit_resource resource)
{
    switch (resource)
    {
#ifdef RLIMIT_CPU
        RPC2H(RLIMIT_CPU);
#endif
#ifdef RLIMIT_FSIZE
        RPC2H(RLIMIT_FSIZE);
#endif
#ifdef RLIMIT_DATA
        RPC2H(RLIMIT_DATA);
#endif
#ifdef RLIMIT_STACK
        RPC2H(RLIMIT_STACK);
#endif
#ifdef RLIMIT_CORE
        RPC2H(RLIMIT_CORE);
#endif
#ifdef RLIMIT_RSS
        RPC2H(RLIMIT_RSS);
#endif
#ifdef RLIMIT_NOFILE
        RPC2H(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_AS
        RPC2H(RLIMIT_AS);
#endif
#ifdef RLIMIT_NPROC
        RPC2H(RLIMIT_NPROC);
#endif
#ifdef RLIMIT_MEMLOCK
        RPC2H(RLIMIT_MEMLOCK);
#endif
#ifdef RLIMIT_LOCKS
        RPC2H(RLIMIT_LOCKS);
#endif
#ifdef RLIMIT_SIGPENDING
        RPC2H(RLIMIT_SIGPENDING);
#endif
#ifdef RLIMIT_MSGQUEUE
        RPC2H(RLIMIT_MSGQUEUE);
#endif
#ifdef RLIMIT_SBSIZE
        RPC2H(RLIMIT_SBSIZE);
#endif
        default: return RLIMIT_NLIMITS;
    }
}

/* See the description in te_rpc_sys_resource.h */
rpc_rlimit_resource
rlimit_resource_h2rpc(int resource)
{
    switch (resource)
    {
#ifdef RLIMIT_CPU
        H2RPC(RLIMIT_CPU);
#endif
#ifdef RLIMIT_FSIZE
        H2RPC(RLIMIT_FSIZE);
#endif
#ifdef RLIMIT_DATA
        H2RPC(RLIMIT_DATA);
#endif
#ifdef RLIMIT_STACK
        H2RPC(RLIMIT_STACK);
#endif
#ifdef RLIMIT_CORE
        H2RPC(RLIMIT_CORE);
#endif
#if !defined RLIMIT_AS || RLIMIT_AS != RLIMIT_RSS
#ifdef RLIMIT_RSS
        H2RPC(RLIMIT_RSS);
#endif
#endif
#ifdef RLIMIT_NOFILE
        H2RPC(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_AS
        H2RPC(RLIMIT_AS);
#endif
#ifdef RLIMIT_NPROC
        H2RPC(RLIMIT_NPROC);
#endif
#ifdef RLIMIT_MEMLOCK
        H2RPC(RLIMIT_MEMLOCK);
#endif
#ifdef RLIMIT_LOCKS
        H2RPC(RLIMIT_LOCKS);
#endif
#ifdef RLIMIT_SIGPENDING
        H2RPC(RLIMIT_SIGPENDING);
#endif
#ifdef RLIMIT_MSGQUEUE
        H2RPC(RLIMIT_MSGQUEUE);
#endif
#ifdef RLIMIT_SBSIZE
        H2RPC(RLIMIT_SBSIZE);
#endif
        default: return RPC_RLIMIT_NLIMITS;
    }
}
