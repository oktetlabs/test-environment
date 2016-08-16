/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/systeminfo.h.
 * 
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#if HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_systeminfo.h"


/* See the description in te_rpc_sys_systeminfo.h */
const char *
sysinfo_command_rpc2str(rpc_sysinfo_command command)
{
    switch (command)
    {
        RPC2STR(SI_SYSNAME);
        RPC2STR(SI_HOSTNAME);
        RPC2STR(SI_RELEASE);
        RPC2STR(SI_VERSION);
        RPC2STR(SI_MACHINE);
        RPC2STR(SI_ARCHITECTURE);
        RPC2STR(SI_HW_SERIAL);
        RPC2STR(SI_HW_PROVIDER);
        RPC2STR(SI_SRPC_DOMAIN);
        RPC2STR(SI_SET_HOSTNAME);
        RPC2STR(SI_SET_SRPC_DOMAIN);
        RPC2STR(SI_PLATFORM);
        RPC2STR(SI_ISALIST);
        RPC2STR(SI_DHCP_CACHE);
        RPC2STR(SI_ARCHITECTURE_32);
        RPC2STR(SI_ARCHITECTURE_64);
        RPC2STR(SI_ARCHITECTURE_K);
        RPC2STR(SI_ARCHITECTURE_NATIVE);
        default:
            return "<unknown sysinfo() command>";
    }
}

/* See the description in te_rpc_sys_systeminfo.h */
int
sysinfo_command_rpc2h(rpc_sysinfo_command command)
{
    switch (command)
    {
#ifdef HAVE_SYS_SYSTEMINFO_H
        RPC2H_CHECK(SI_SYSNAME);
        RPC2H_CHECK(SI_HOSTNAME);
        RPC2H_CHECK(SI_RELEASE);
        RPC2H_CHECK(SI_VERSION);
        RPC2H_CHECK(SI_MACHINE);
        RPC2H_CHECK(SI_ARCHITECTURE);
        RPC2H_CHECK(SI_HW_SERIAL);
        RPC2H_CHECK(SI_HW_PROVIDER);
        RPC2H_CHECK(SI_SRPC_DOMAIN);
        RPC2H_CHECK(SI_SET_HOSTNAME);
        RPC2H_CHECK(SI_SET_SRPC_DOMAIN);
#ifdef __sun__
        RPC2H_CHECK(SI_PLATFORM);
        RPC2H_CHECK(SI_ISALIST);
        RPC2H_CHECK(SI_DHCP_CACHE);
        RPC2H_CHECK(SI_ARCHITECTURE_32);
        RPC2H_CHECK(SI_ARCHITECTURE_64);
        RPC2H_CHECK(SI_ARCHITECTURE_K);
        RPC2H_CHECK(SI_ARCHITECTURE_NATIVE);
#endif
#endif
        default:
            return -1;
    }
}

/* See the description in te_rpc_sys_systeminfo.h */
rpc_sysinfo_command
sysinfo_command_h2rpc(int command)
{
    switch (command)
    {
#ifdef HAVE_SYS_SYSTEMINFO_H
        H2RPC_CHECK(SI_SYSNAME);
        H2RPC_CHECK(SI_HOSTNAME);
        H2RPC_CHECK(SI_RELEASE);
        H2RPC_CHECK(SI_VERSION);
        H2RPC_CHECK(SI_MACHINE);
        H2RPC_CHECK(SI_ARCHITECTURE);
        H2RPC_CHECK(SI_HW_SERIAL);
        H2RPC_CHECK(SI_HW_PROVIDER);
        H2RPC_CHECK(SI_SRPC_DOMAIN);
        H2RPC_CHECK(SI_SET_HOSTNAME);
        H2RPC_CHECK(SI_SET_SRPC_DOMAIN);
#ifdef __sun__
        H2RPC_CHECK(SI_PLATFORM);
        H2RPC_CHECK(SI_ISALIST);
        H2RPC_CHECK(SI_DHCP_CACHE);
        H2RPC_CHECK(SI_ARCHITECTURE_32);
        H2RPC_CHECK(SI_ARCHITECTURE_64);
        H2RPC_CHECK(SI_ARCHITECTURE_K);
        H2RPC_CHECK(SI_ARCHITECTURE_NATIVE);
#endif
#endif
        default:
            return -1;
    }
}
