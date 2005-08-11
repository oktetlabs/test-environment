/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP, stack-related callbacks.
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <netinet/in.h>
#include <netinet/ether.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/if.h>

#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <assert.h>

#include <string.h>

#define TE_LGR_USER     "TAD packet sockets"
#include "logger_ta.h"

#include "pkt_socket.h"


#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) do {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; } while (0)
/* macros to remove element p from queue  */
#define REMQUE(p) do {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; } while(0)
#endif

                   

typedef struct iface_user_rec {
    struct iface_user_rec *next, *prev;
    eth_interface_t        descr;
} iface_user_rec;

static iface_user_rec iface_users_root = 
    {&iface_users_root, &iface_users_root, {{0,}, 0, {0,}}};

static iface_user_rec *
find_iface_user_rec(const char *ifname)
{
    iface_user_rec *ir;

    for (ir = iface_users_root.next; ir != &iface_users_root; ir = ir->next)
        if (strncmp(ir->descr.name, ifname, IFNAME_SIZE) == 0)
            return ir;

    return NULL;
}


#define USE_PROMISC_MODE 1

 
/* See description in pkt_socket.h */
int 
open_packet_socket(const char *ifname, int *sock)
{
    int  rc;
    char err_buf[200];

    eth_interface_t     ifdescr;
    struct sockaddr_ll  bind_addr;

    struct packet_mreq  mr;

    memset (&bind_addr, 0, sizeof(bind_addr));

    rc = eth_find_interface(ifname,  &ifdescr);
    if (rc != 0)
    {
        ERROR("%s(): find interface %s failed 0x%X", 
              __FUNCTION__, ifname, rc);
        return rc;
    }

    if (sock == NULL)
    {
        ERROR("Location for socket is NULL");
        return TE_EINVAL;
    }

    /* Create packet socket */
    *sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    VERB("OPEN SOCKET (%d): %s:%d", *sock,
               __FILE__, __LINE__);

    VERB("socket system call returns %d\n", *sock);

    if (*sock < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("Socket creation failed, errno %d, \"%s\"\n", 
                rc, err_buf);
        return rc;
    }

    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(ETH_P_ALL);
    bind_addr.sll_ifindex = ifdescr.if_index;
    bind_addr.sll_hatype = ARPHRD_ETHER;
    bind_addr.sll_pkttype = 0;
    bind_addr.sll_halen = ETH_ALEN;

    VERB("RAW Socket opened");

#if 0
    switch (dir)
    {
        case IN_PACKET_SOCKET:
            bind_addr.sll_pkttype = pkt_type;
            shutdown_method = SHUT_WR;
            break;
        case OUT_PACKET_SOCKET:
            shutdown_method = SHUT_RD;
            break;
    }

    if (shutdown (*sock, shutdown_method) < 0)
    {
        perror ("ETH packet socket shutdown");
        return errno;
    }
#endif

    if (bind(*sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("Socket bind failed, errno %d, \"%s\"\n", 
                rc, err_buf);

        VERB("CLOSE SOCKET (%d): %s:%d", 
                *sock, __FILE__, __LINE__);
        if (close(*sock) < 0)
            assert(0);

        *sock = -1;
        return rc;
    }
#if 1
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifdescr.if_index;
    mr.mr_type    = PACKET_MR_PROMISC;
    if (setsockopt(*sock, SOL_PACKET,
        PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        ERROR("setsockopt: PACKET_ADD_MEMBERSHIP failed");
    }
#endif
    return 0;
}


/* See description in pkt_socket.h */
int
close_packet_socket(const char* ifname, int sock)
{
    iface_user_rec *ir;
    int rc; 

    ir = find_iface_user_rec(ifname);
    if (ir == NULL)
    {
        ERROR("%s(): iface %d not used before for create packet socket", 
              __FUNCTION__, ifname);
        return TE_EINVAL;
    }

    rc = eth_free_interface(&(ir->descr));
    if (rc != 0)
    {
        ERROR("%s(): error free interface %d",
              __FUNCTION__, ir->descr.name);
        return rc;
    }

    close(sock);

    return 0;
}

/**
 * Find ethernet interface by its name and initialize specified
 * structure with interface parameters
 *
 * @param name      symbolic name of interface to find (e.g. eth0, eth1)
 * @param iface     pointer to interface structure to be filled
 *                  with digged parameters (OUT)
 *
 * @return zero on success or one of the error codes
 *
 * @retval zero                    on success
 * @retval ETH_IFACE_NOT_FOUND     if config socket error occured or interface 
 *                                 can't be found by specified name
 * @retval ETH_IFACE_HWADDR_ERROR  if hardware address can't be extracted   
 * @retval ETH_IFACE_IFINDEX_ERROR if interface index can't be extracted
 */
int
eth_find_interface(const char *name, eth_interface_p ifdescr)
{
    char err_buf[200];
    int    cfg_socket;
    struct ifreq  if_req;
    int rc;
    iface_user_rec *ir;

    if (name == NULL) 
    {
       return TE_EINVAL;
    }

    VERB("%s('%s') start", __FUNCTION__, name);

    if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return errno;

    ir = find_iface_user_rec(name);
    if (ir != NULL)
    {
        memcpy(ifdescr, &(ir->descr), sizeof(*ifdescr));
        goto promisc_mode;
    }

    ir = calloc(1, sizeof(*ir));

    memset(&if_req, 0, sizeof(if_req));
    strcpy(if_req.ifr_name, name);

    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if addr error %d \"%s\"", rc, err_buf);
        close(cfg_socket);
        return rc;
    }

    memcpy(ifdescr->local_addr, if_req.ifr_hwaddr.sa_data, ETH_ALEN);
    strncpy(if_req.ifr_name, name, sizeof(if_req.ifr_name));

    if (ioctl(cfg_socket, SIOCGIFINDEX, &if_req))
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if index error %d \"%s\"", rc, err_buf);
        close(cfg_socket);
        return rc;
    }
    
    /* saving if index  */
    ifdescr->if_index = if_req.ifr_ifindex;
    
    /* saving name     */
    strncpy(ifdescr->name, if_req.ifr_name, (IFNAME_SIZE - 1));
    ifdescr->name[IFNAME_SIZE - 1] = '\0';


    memcpy(&(ir->descr), ifdescr, sizeof(*ifdescr));
    INSQUE(ir, &iface_users_root); 

promisc_mode:

#if 0
#if USE_PROMISC_MODE

    memset(&if_req, 0, sizeof(if_req));
    strcpy(if_req.ifr_name, name);

    if (ioctl(cfg_socket, SIOCGIFFLAGS, &if_req) < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if flags error %d \"%s\"", rc, err_buf);
        return rc;
    }

    VERB("SIOCGIFFLAGS, promisc mode: %d, flags: %x\n", 
                (int)(if_req.ifr_flags & IFF_PROMISC), (int)if_req.ifr_flags);

    /* set interface to promiscuous mode */
    VERB("set to promisc mode");
    if_req.ifr_flags |= IFF_PROMISC;

    if (ioctl(cfg_socket, SIOCSIFFLAGS, &if_req))
    { 
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("set PROMISC mode error %d \"%s\"", rc, err_buf);
        return rc;
    }
#endif
#endif
    
    close(cfg_socket);

    return 0;
} 



int
eth_free_interface(eth_interface_p iface)
{ 
    int rc = 0;
#if 0
#if USE_PROMISC_MODE
#if 0
    iface_user_rec *ir;
#endif
    struct ifreq  if_request;
    int cfg_socket;

    VERB("unset to promisc mode iface %s", iface->name);

    if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        ERROR("%s(): create cfg sock fails, errno %d", __FUNCTION__, rc);
        rc = errno;
    }

    memset(&if_request, 0, sizeof(if_request));
    strcpy(if_request.ifr_name, iface->name);

    if (ioctl(cfg_socket, SIOCGIFFLAGS, &if_request) < 0)
    {
        rc = errno;
        ERROR("%s(): get flags of interface failed, errno %d",
              __FUNCTION__, rc);
        close(cfg_socket);
        return rc;
    }

    /* reset interface from promiscuous mode */
    if_request.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(cfg_socket, SIOCSIFFLAGS, &if_request))
    { 
        rc = errno;
        ERROR("%s(): unset promisc mode failed %x\n", __FUNCTION__, rc);
    }
    close(cfg_socket);
#endif 
#else
    UNUSED(iface);
#endif 
    return rc;
}
