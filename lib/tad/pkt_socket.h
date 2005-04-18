/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_PKT_SOCKET_H__
#define __TE_TAD_PKT_SOCKET_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "tad_common.h"
#include "rcf_ch_api.h"


#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif

#ifndef ETH_IFACE_OK
#define ETH_IFACE_OK 0
#endif

#ifndef ETH_IFACE_NOT_FOUND
#define ETH_IFACE_NOT_FOUND 1
#endif

#ifndef ETH_IFACE_HWADDR_ERROR
#define ETH_IFACE_HWADDR_ERROR 2
#endif

#ifndef ETH_IFACE_IFINDEX_ERROR
#define ETH_IFACE_IFINDEX_ERROR 3
#endif

#ifdef __cplusplus
extern "C" {
#endif







/* ============= Types and structures definitions =============== */


/* 
 * Ethernet interface related data
 */
struct eth_interface;
typedef struct eth_interface *eth_interface_p;
typedef struct eth_interface
{
    char  name[IFNAME_SIZE];    /**< Etherner interface name (e.g. eth0) */
    int   if_index;             /**< Interface index                     */
    
    char  local_addr[ETH_ALEN]; /**< Hardware address of the home interface
                                     Extracted through ioctls if not defined
                                     by the user in the configuration param.
                                     (may differ from real hardware address)
                                 */
    
} eth_interface_t;


/**
 * Create and bind packet socket to operate with network interface
 *
 * @param ifname    name of network interface
 * @param sock      pointer to place where socket handler will be saved
 *
 * @param 0 on succees, error code otherwise
 */
extern int open_packet_socket(const char* ifname, int *sock);


/**
 * Close packet socket
 *
 * @param ifname    name of network interface socket bound to
 * @param sock      pointer to place where socket handler will be saved
 *
 * @param 0 on succees, error code otherwise
 */
extern int close_packet_socket(const char* ifname, int sock);

/**
 * Find ethernet interface by its name and initialize specified
 * structure with interface parameters
 *
 * @param name      symbolic name of interface to find (e.g. eth0, eth1)
 * @param iface     pointer to interface structure to be filled
 *                  with found parameters
 *
 * @return ETH_IFACE_OK on success or one of the error codes
 *
 * @retval ETH_IFACE_OK            on success
 * @retval ETH_IFACE_NOT_FOUND     if config socket error occured or
 *                                 interface can't be found by name
 * @retval ETH_IFACE_HWADDR_ERROR  if hardware address can't be extracted   
 * @retval ETH_IFACE_IFINDEX_ERROR if interface index can't be extracted
 */
extern int eth_find_interface(const char *name, eth_interface_p if_descr); 


/**
 * Free ethernet interface by its descriptor. 
 * Mainly, this method drop promiscuous mode on interface.
 *
 * @param iface         interface structure descriptor
 *
 * @return error status code
 */
extern int eth_free_interface(eth_interface_p iface);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_PKT_SOCKET_H__ */
