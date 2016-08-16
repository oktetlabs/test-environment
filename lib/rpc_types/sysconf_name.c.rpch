/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of sysconf() constant names.
 * 
 * 
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sysconf.h"

/* See the description in te_rpc_sysconf.h */
const char *
sysconf_name_rpc2str(rpc_sysconf_name name)
{
    switch (name)
    {
        RPC2STR(SC_ARG_MAX);
        RPC2STR(SC_CHILD_MAX);
        RPC2STR(SC_HOST_NAME_MAX);
        RPC2STR(SC_OPEN_MAX);
        RPC2STR(SC_PAGESIZE);
        RPC2STR(SC_UNKNOWN);
        default: return "<SC_FATAL_ERROR>";
    }
}

/* See the description in te_rpc_sysconf.h */
int
sysconf_name_rpc2h(rpc_sysconf_name name)
{
#define SYSCONF_RPC2H(name_) \
    case RPC ## name_: return name_

    switch (name)
    {
#ifdef _SC_ARG_MAX
        SYSCONF_RPC2H(_SC_ARG_MAX);
#endif
#ifdef _SC_CHILD_MAX
        SYSCONF_RPC2H(_SC_CHILD_MAX);
#endif
#ifdef _SC_HOST_NAME_MAX
        SYSCONF_RPC2H(_SC_HOST_NAME_MAX);
#endif
#ifdef _SC_OPEN_MAX
        SYSCONF_RPC2H(_SC_OPEN_MAX);
#endif
#ifdef _SC_PAGESIZE
        SYSCONF_RPC2H(_SC_PAGESIZE);
#endif
        default: return -1;
    }
#undef SYSCONF_RPC2H
}

/* See the description in te_rpc_sysconf.h */
rpc_sysconf_name
sysconf_name_h2rpc(int name)
{
#define SYSCONF_H2RPC(name_) \
    case name_: return RPC ## name_

    switch (name)
    {
#ifdef _SC_ARG_MAX
        SYSCONF_H2RPC(_SC_ARG_MAX);
#endif
#ifdef _SC_CHILD_MAX
        SYSCONF_H2RPC(_SC_CHILD_MAX);
#endif
#ifdef _SC_HOST_NAME_MAX
        SYSCONF_H2RPC(_SC_HOST_NAME_MAX);
#endif
#ifdef _SC_OPEN_MAX
        SYSCONF_H2RPC(_SC_OPEN_MAX);
#endif
#ifdef _SC_PAGESIZE
        SYSCONF_H2RPC(_SC_PAGESIZE);
#endif
        default: return RPC_SC_UNKNOWN;
    }
#undef SYSCONF_H2RPC
}
