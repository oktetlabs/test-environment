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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id: $
 */
 
#ifndef __TE_RPC_SYS_RESOURCE_H__
#define __TE_RPC_SYS_RESOURCE_H__

#include "te_rpc_defs.h"

#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

/**
 * TA-independent rlimit resource types.
 */

typedef enum rpc_rlimit_resource {
  RPC_RLIMIT_CPU,
  RPC_RLIMIT_FSIZE,
  RPC_RLIMIT_DATA,
  RPC_RLIMIT_STACK,
  RPC_RLIMIT_CORE,
  RPC_RLIMIT_RSS,
  RPC_RLIMIT_NOFILE,
  RPC_RLIMIT_AS,
  RPC_RLIMIT_NPROC,
  RPC_RLIMIT_MEMLOCK,
  RPC_RLIMIT_LOCKS,
  RPC_RLIMIT_SIGPENDING,
  RPC_RLIMIT_MSGQUEUE,
  RPC_RLIMIT_NLIMITS,
} rpc_rlimit_resource;

/** Convert RPC resource type (setrlimit/getrllimit) to native resource type */
static inline int
rlimit_resource_rpc2h(rpc_rlimit_resource resource)
{
    switch (resource)
    {
        RPC2H(RLIMIT_CPU);
        RPC2H(RLIMIT_FSIZE);
        RPC2H(RLIMIT_DATA);
        RPC2H(RLIMIT_STACK);
        RPC2H(RLIMIT_CORE);
        RPC2H(RLIMIT_RSS);
        RPC2H(RLIMIT_NOFILE);
        RPC2H(RLIMIT_AS);
        RPC2H(RLIMIT_NPROC);
        RPC2H(RLIMIT_MEMLOCK);
        RPC2H(RLIMIT_LOCKS);
        default: return RLIMIT_NLIMITS;
    }
}

/** Convert native resource type (setrlimit/getrllimit) to RPC resource type */
static inline rpc_rlimit_resource
rlimit_resource_h2rpc(int resource)
{
    switch (resource)
    {
        H2RPC(RLIMIT_CPU);
        H2RPC(RLIMIT_FSIZE);
        H2RPC(RLIMIT_DATA);
        H2RPC(RLIMIT_STACK);
        H2RPC(RLIMIT_CORE);
        H2RPC(RLIMIT_RSS);
        H2RPC(RLIMIT_NOFILE);
        H2RPC(RLIMIT_AS);
        H2RPC(RLIMIT_NPROC);
        H2RPC(RLIMIT_MEMLOCK);
        H2RPC(RLIMIT_LOCKS);
        default: return RPC_RLIMIT_NLIMITS;
    }
}

/** Convert RPC resource type (setrlimit/getrllimit) to string representation */
static inline const char *
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
        default: return "<RLIMIT_NLIMITS>";
    }
}

#endif /* !__TE_RPC_SYS_RESOURCE_H__ */
