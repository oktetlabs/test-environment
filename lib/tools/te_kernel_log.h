/** @file
 * @brief Unix Kernel Logger
 *
 * Unix Kernel Logger header file
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

#ifndef __TE_TOOLS_KERNEL_LOG_H__
#define __TE_TOOLS_KERNEL_LOG_H__

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Map Logger level name to the value
 * 
 * @param name  Name of the level
 * 
 * @return Level number
 */
extern int map_name_to_level(const char *name);

/**
 * Get addresses by host name.
 *
 * @param host_name     Host name
 * @param host_ipv4     Here IPv4 address will be saved if not NULL
 * @param ipv4_found    Will be set to TRUE if IPv4 address found
 * @param host_ipv6     Here IPv6 address will be saved if not NULL
 * @param ipv6_found    Will be set to TRUE if IPv6 address found
 *
 * @return -1 on failue, 0 on success
 */
extern int te_get_host_addrs(const char *host_name,
                             struct sockaddr_in *host_ipv4,
                             te_bool *ipv4_found,
                             struct sockaddr_in6 *host_ipv6,
                             te_bool *ipv6_found);

/**
 * Log kernel output via Logger component
 *
 * @param ready       Parameter release semaphore
 * @param argc        Number of string arguments
 * @param argv        String arguments:
 *                    - log user
 *                    - log level
 *                    - message interval
 *                    - tty name
 *                    (if it does not start with "/", it is interpreted
 *                     as a conserver connection designator)
 *                    - sharing mode (opt)
 */
extern int log_serial(void *ready, int argc, char *argv[]);

/**
 * Set function to be used as system(),
 *
 * @param p     Function pointer
 */
extern void te_kernel_log_set_system_func(void *p);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_KERNEL_LOG_H__ */
