/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of constant names for sysconf().
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYSCONF_H__
#define __TE_RPC_SYSCONF_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * TA-independent sysconf() names.
 */
typedef enum rpc_sysconf_name {
    RPC_SC_ARG_MAX,
    RPC_SC_CHILD_MAX,
    RPC_SC_HOST_NAME_MAX,
    RPC_SC_OPEN_MAX,
    RPC_SC_PAGESIZE,
    RPC_SC_UNKNOWN,
} rpc_sysconf_name;

/** 
 * Convert RPC sysconf() name
 * to string representation.
 */
extern const char *sysconf_name_rpc2str(rpc_sysconf_name name);

/**
 * Convert RPC sysconf() name to native one.
 */
extern int sysconf_name_rpc2h(rpc_sysconf_name name);

/**
 * Convert native sysconf() nameto RPC one.
 */
extern rpc_sysconf_name sysconf_name_h2rpc(int name);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYSCONF_H__ */
