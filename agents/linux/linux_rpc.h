/** @file
 * @brief Linux Test Agent
 *
 * Linux RCF RPC definitions
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_TA_LINUX_RPC_H__
#define __TE_TA_LINUX_RPC_H__

/** Obtain RCF RPC errno code */
#define RPC_ERRNO errno_h2rpc(errno)


/** Logging path */
extern struct sockaddr_un ta_log_addr;


/** Function to start RPC server after execve */
void tarpc_init(int argc, char **argv);

/**
 * Destroy all RPC server processes and release the list of RPC servers.
 */
extern void tarpc_destroy_all(void);

/** RPC server entry point */
extern void *tarpc_server(const void *arg);


#endif /* __TE_TA_LINUX_RPC_H__ */
