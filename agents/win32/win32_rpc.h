/** @file
 * @brief Windows Test Agent
 *
 * Windows RCF RPC definitions
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * $Id: win32_rpc.h 3672 2004-07-20 00:00:04Z helen $
 */
 
#ifndef __WIN32_RPC_H__
#define __WIN32_RPC_H__

/** Function to start RPC server after execve */
void tarpc_init(int argc, char **argv);

/**
 * Destroy all RPC server processes and release the list of RPC servers.
 */
extern void tarpc_destroy_all();

 
#define MY_TRACE(fmt...) \
    do {                                        \
        char buf[512] = {0,};                   \
        char *s = buf;                          \
        s += sprintf(buf, "echo \"");           \
        s += sprintf(s, fmt);                   \
        sprintf(s, "\" >> /tmp/xxx");           \
        system(buf);                            \
    } while (0)

/** Socket used by the last started RPC server */
int rpcserver_sock;

/** Name of the last started RPC server */
extern char *rpcserver_name;

/** RPC server entry point */
void *tarpc_server(void *arg);

/** Logging path */
extern struct sockaddr_in ta_log_addr;
extern int                ta_log_sock;

/** Obtain RCF RPC errno code */
#define RPC_ERRNO errno_h2rpc(errno)

#endif /* __WIN32_RPC_H__ */
