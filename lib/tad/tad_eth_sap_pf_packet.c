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
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_csap_inst.h"
#include "tad_utils.h"
#include "tad_eth_sap.h"


/**
 * Number of retries to write data in low layer
 */
#define TAD_WRITE_RETRIES           128

/**
 * Default timeout for waiting write possibility. This macro should 
 * be used only for initialization of 'struct timeval' variables. 
 */
#define TAD_WRITE_TIMEOUT_DEFAULT   { 1, 0 }


/** Internal data of Ethernet service access point via PF_PACKET sockets */
typedef struct tad_eth_sap_pf_packet_data {
    unsigned int    ifindex;    /**< Interface index */
    int             in;         /**< Input socket (for receive) */
    int             out;        /**< Output socket (for send) */
    unsigned int    send_mode;  /**< Send mode */
    unsigned int    recv_mode;  /**< Receive mode */
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

    if (ifname == NULL || sap == NULL) 
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return TE_RC(TE_TAD_PF_PACKET, TE_EFAULT);
    }
    if (strlen(ifname) >= MIN(sizeof(if_req.ifr_name),
                              sizeof(sap->name)))
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
    strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));

    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): ioctl(%s, SIOCGIFHWADDR) failed: %r",
              __FUNCTION__, ifname, rc);
        close(cfg_socket);
        return rc;
    }

    memcpy(sap->addr, if_req.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
    strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));

    if (ioctl(cfg_socket, SIOCGIFINDEX, &if_req))
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): ioctl(%s, SIOCGIFINDEX) failed: %r",
              __FUNCTION__, ifname, rc);
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
tad_eth_sap_send_open(tad_eth_sap *sap, unsigned int mode)
{
    tad_eth_sap_pf_packet_data *data;
    te_errno                    rc;
    int                         buf_size;
    struct sockaddr_ll          bind_addr;

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    if (data->out >= 0)
        return 0;

    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - do not receive any packets
     */
    data->out = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->out < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): socket(PF_PACKET, SOCK_RAW, 0) failed: %r",
              __FUNCTION__, rc);
        return rc;
    }

    /*
     * Set send buffer size.
     * TODO: reasonable size of send buffer to be investigated.
     */
    buf_size = 0x100000; 
    if (setsockopt(data->out, SOL_SOCKET, SO_SNDBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(SO_SNDBUF) failed: %r", rc);
        goto error_exit;
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
    bind_addr.sll_ifindex = data->ifindex;

    if (bind(data->out, SA(&bind_addr), sizeof(bind_addr)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("Failed to bind PF_PACKET socket: %r", rc);
        goto error_exit;
    }

    data->send_mode = mode;

    INFO("PF_PACKET socket %d opened and bound for send", data->out);

    return 0;
    
error_exit:
    if (close(data->out) < 0)
        assert(FALSE);
    data->out = -1;
    return rc;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt)
{
    tad_eth_sap_pf_packet_data *data;
    size_t                      iovlen = tad_pkt_seg_num(pkt);
    struct iovec                iov[iovlen];
    te_errno                    rc;
    unsigned int                retries = TAD_WRITE_RETRIES;
    int                         ret_val;
    fd_set                      write_set;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    F_VERB("%s: writing data to socket: %d", __FUNCTION__, data->out);

    if (data->out < 0)
    {
        ERROR("%s(): no output socket", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to I/O vector: %r", rc);
        return rc;
    }

    for (retries = 0, ret_val = 0;
         ret_val <= 0 && retries < TAD_WRITE_RETRIES;
         retries++)
    {
        struct timeval timeout = TAD_WRITE_TIMEOUT_DEFAULT; 
    
        FD_ZERO(&write_set);
        FD_SET(data->out, &write_set);

        ret_val = select(data->out + 1, NULL, &write_set, NULL, &timeout);
        if (ret_val == 0)
        {
            F_INFO("%s(): select to write timed out, retry %u", retries);
            continue;
        }

        if (ret_val == 1)
            ret_val = writev(data->out, iov, iovlen);

        if (ret_val < 0)
        {
            rc = te_rc_os2te(errno);
            VERB("CSAP #%d, errno %r, retry %d",
                 sap->csap->id, rc, retries);
            switch (rc)
            {
                case TE_ENOBUFS:
                {
                    /* 
                     * It seems that 0..127 microseconds is enough
                     * to hope that buffers will be cleared and
                     * does not fall down performance.
                     */
                    struct timeval clr_delay = { 0, rand() & 0x3f };

                    select(0, NULL, NULL, NULL, &clr_delay);
                    continue;
                }

                default:
                    ERROR("%s(CSAP %d): internal error %r, socket %d", 
                          __FUNCTION__, sap->csap->id, rc, data->out);
                    return rc;
            }
        } 
    }
    if (retries == TAD_WRITE_RETRIES)
    {
        ERROR("CSAP #%d, too many retries made, failed", sap->csap->id);
        return TE_RC(TE_TAD_CSAP, TE_ENOBUFS);
    }

    F_VERB("CSAP #%d, system write return %d", 
            sap->csap->id, ret_val); 

    if (ret_val < 0)
        return TE_OS_RC(TE_TAD_CSAP, errno);

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_close(tad_eth_sap *sap)
{
    tad_eth_sap_pf_packet_data *data;
    fd_set                      write_set;
    struct timeval              timeout = TAD_WRITE_TIMEOUT_DEFAULT; 
    int                         ret_val;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    if (data->out >= 0)
    {
        /* check that all data in socket is sent */
        FD_ZERO(&write_set);
        FD_SET(data->out, &write_set);

        ret_val = select(data->out + 1, NULL, &write_set, NULL, &timeout);
        if (ret_val == 0)
        {
            WARN("Ethernet PF_PACKET (socket %d) SAP is still sending",
                 data->out);
        }
        else if (ret_val < 0)
        {
            te_errno    rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);

            ERROR("%s(): select() failed: %r", __FUNCTION__, rc);
        }
        /* Close in any case */
    }

    return close_socket(&data->out);
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_open(tad_eth_sap *sap, unsigned int mode)
{
    tad_eth_sap_pf_packet_data *data;
    te_errno                    rc;
    int                         buf_size;
    struct sockaddr_ll          bind_addr;
    struct packet_mreq          mr;

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    if (data->in >= 0)
        return 0;

    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - receive nothing before bind to interface
     */
    data->in = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->in < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("socket(PF_PACKET, SOCK_RAW, 0) failed: %r", rc);
        return rc;
    }

    /*
     * Set receive buffer size.
     * TODO: reasonable size of receive buffer to be investigated.
     */
    buf_size = 0x100000; 
    if (setsockopt(data->in, SOL_SOCKET, SO_RCVBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(SO_RCVBUF) failed: %r", rc);
        goto error_exit;
    }

    if (mode & TAD_ETH_RECV_OTHER)
    {
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
            goto error_exit;
        }
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
        goto error_exit;
    }

    data->recv_mode = mode;

    INFO("PF_PACKET socket %d opened and bound for receive", data->in);

    return 0;

error_exit:
    if (close(data->in) < 0)
        assert(FALSE);
    data->in = -1;
    return rc;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    tad_eth_sap_pf_packet_data *data;
    struct sockaddr_ll          from;
    socklen_t                   fromlen = sizeof(from);
    te_errno                    rc;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

    rc = tad_common_read_cb_sock(sap->csap, data->in, MSG_TRUNC, timeout,
                                 pkt, SA(&from), &fromlen, pkt_len);
    if (rc != 0)
        return rc;

    switch (from.sll_pkttype)
    {
        case PACKET_HOST:
            if ((data->recv_mode & TAD_ETH_RECV_HOST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_BROADCAST:
            if ((data->recv_mode & TAD_ETH_RECV_BCAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_MULTICAST:
            if ((data->recv_mode & TAD_ETH_RECV_MCAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_OTHERHOST:
            if ((data->recv_mode & TAD_ETH_RECV_OTHER) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_OUTGOING:
            if ((data->recv_mode & TAD_ETH_RECV_OUT) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        default:
            WARN("%s(): Unknown type %u of packet received",
                 __FUNCTION__, from.sll_pkttype);
            return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }

    return 0;
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
