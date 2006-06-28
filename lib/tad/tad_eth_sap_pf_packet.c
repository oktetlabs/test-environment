/** @file
 * @brief TAD Data Link Provider Interface
 *
 * Implementation routines to access media through AF_PACKET sockets.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD PF_PACKET"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef PF_PACKET

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"

#include "tad_eth_sap.h"


/** Internal data of Ethernet service access point via PF_PACKET sockets */
typedef struct tad_eth_sap_pf_packet_data {
    unsigned int    ifindex;    /**< Interface index */
    int             in;         /**< Input socket (for receive) */
    int             out;        /**< Output socket (for send) */
} tad_eth_sap_pf_packet_data;


/**
 * Close PF_PACKET socket.
 *
 * @param sock      Location of the socket file descriptor
 *
 * @return Status code.
 */
static te_errno
close_socket(int *sock)
{
    assert(sock != NULL);
    if (*sock >= 0)
    {
        if (close(*sock) != 0)
        {
            te_errno rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);

            ERROR("%s(): close() failed: %r", __FUNCTION__, rc);
            return rc;
        }
        INFO("PF_PACKET socket %d closed", *sock);
        *sock = -1;
    }
    return 0;
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_attach(const char *ifname, tad_eth_sap *sap)
{
    tad_eth_sap_pf_packet_data *data;
    int                         cfg_socket;
    struct ifreq                if_req;
    te_errno                    rc;

    if (name == NULL || sap == NULL) 
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return TE_RC(TE_TAD_PF_PACKET, TE_EFAULT);
    }
    if (strlen(ifname) >= MIN(sizeof(if_req.ifr_name),
                              sizeof(data->name)))
    {
        ERROR("%s(): Too long interface name", __FUNCTION__);
        return TE_RC(TE_TAD_PF_PACKET, TE_E2BIG);
    }

    cfg_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (cfg_socket < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): socket(AF_INET, SOCK_DGRAM, 0) failed: %r",
              __FUNCTION__, rc);
        return rc;
    }

    memset(&if_req, 0, sizeof(if_req));
    strncpy(if_req.ifr_name, name, sizeof(if_req.ifr_name));

    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): ioctl(SIOCGIFHWADDR) failed: %r",
              __FUNCTION__, rc);
        close(cfg_socket);
        return rc;
    }

    memcpy(sap->local_addr, if_req.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
    strncpy(if_req.ifr_name, name, sizeof(if_req.ifr_name));

    if (ioctl(cfg_socket, SIOCGIFINDEX, &if_req))
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): ioctl(SIOCGIFINDEX) failed: %r", __FUNCTION__, rc);
        close(cfg_socket);
        return rc;
    }

    close(cfg_socket);

    assert(sap->data == NULL);
    sap->data = data = TE_ALLOC(sizeof(*data));
    if (data == NULL)
        return TE_RC(TE_TAD_PF_PACKET, TE_ENOMEM);

    data->ifindex = if_req.ifr_ifindex;
    data->in = data->out = -1;
    
    strcpy(sap->name, ifname);

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_open(tad_eth_sap *sap, tad_eth_sap_send_mode mode)
{
    tad_eth_sap_pf_packet_data *data;
    te_errno                    rc;
    struct sockaddr_ll          bind_addr;

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    assert(data->out == -1);

    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - do not receive any packets
     */
    data->out = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->out < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("socket(PF_PACKET, SOCK_RAW, 0) failed: %r", rc);
        return rc;
    }

    /* 
     * Bind PF_PACKET socket:
     *  - sll_protocol: 0 - do not receive any packets
     *  - sll_hatype. sll_pkttype, sll_halen, sll_addr are not used for
     *    binding
     */
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(0);
    bind_addr.sll_ifindex = ifdescr.if_index;

    if (bind(data->out, SA(&bind_addr), sizeof(bind_addr)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("Failed to bind PF_PACKET socket: %r", rc);
        if (close(data->out) < 0)
            assert(FALSE);
        data->out = -1;
        return rc;
    }

    INFO("PF_PACKET socket %d opened and bound for send", data->out);

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt)
{
    tad_eth_sap_pf_packet_data *data;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_close(tad_eth_sap *sap)
{
    tad_eth_sap_pf_packet_data *data;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    return close_socket(&data->out);
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_open(tad_eth_sap *sap, tad_eth_sap_recv_mode mode)
{
    tad_eth_sap_pf_packet_data *data;
    te_errno                    rc;
    struct sockaddr_ll          bind_addr;
    struct packet_mreq          mr;

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    assert(data->in == -1);

    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - receive nothing before bind to interface
     */
    data->in = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->in < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("socket(PF_PACKET, SOCK_RAW, ETH_P_ALL) failed: %r", rc);
        return rc;
    }

    /*
     * Enable promiscuous mode for the socket on specified interface.
     */
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = data->ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(data->in, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                   &mr, sizeof(mr)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt: PACKET_ADD_MEMBERSHIP failed: %r",
              __FUNCTION__, rc); 
        if (close(data->in) < 0)
            assert(FALSE);
        data->in = -1;
        return rc;
    }

    /* 
     * Bind PF_PACKET socket:
     *  - sll_protocol: ETH_P_ALL - receive everything
     *  - sll_hatype. sll_pkttype, sll_halen, sll_addr are not used for
     *    binding
     */
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(ETH_P_ALL);
    bind_addr.sll_ifindex = data->ifindex;

    if (bind(data->in, SA(&bind_addr), sizeof(bind_addr)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("Failed to bind PF_PACKET socket: %r", rc);
        if (close(data->in) < 0)
            assert(FALSE);
        data->in = -1;
        return rc;
    }

    INFO("PF_PACKET socket %d opened and bound for receive", data->in);

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                 const tad_pkt *pkt, size_t *pkt_len)
{
    tad_eth_sap_pf_packet_data *data;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_close(tad_eth_sap *sap)
{
    tad_eth_sap_pf_packet_data *data;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    return close_socket(&data->in);
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_detach(tad_eth_sap *sap)
{
    tad_eth_sap_pf_packet_data *data;
    te_errno                    result = 0;
    te_errno                    rc;
    
    assert(sap != NULL);
    data = sap->data;
    sap->data = NULL;
    assert(data != NULL);

    if (data->in != -1)
    {
        WARN("Force close of input PF_PACKET socket on detach");
        rc = close_socket(&data->in);
        TE_RC_UPDATE(result, rc);
    }
    if (data->out != -1)
    {
        WARN("Force close of output PF_PACKET socket on detach");
        rc = close_socket(&data->out);
        TE_RC_UPDATE(result, rc);
    }

    free(data);

    return result;
}

#endif /* PF_PACKET */
