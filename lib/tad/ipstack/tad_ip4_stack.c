/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IPv4 CSAP layer stack-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD IPv4"

#include "te_config.h"

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
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#ifdef PF_PACKET
#if HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif
#endif

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include "logger_ta_fast.h"

#ifdef WITH_ETH
#include "../eth/tad_eth_impl.h"
#endif

#ifdef PF_PACKET
#define TAD_IP4_IFNAME_SIZE 256
#endif

#include "tad_ipstack_impl.h"


/** IPv4 layer as read/write specific data */
typedef struct tad_ip4_rw_data {
    int                 socket;
#ifdef PF_PACKET
    struct sockaddr_ll  sa_op;
#else
    struct sockaddr_in  sa_op;
#endif
} tad_ip4_rw_data;


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_rw_init_cb(csap_p csap)
{
    tad_ip4_rw_data    *spec_data;

    int    opt = 1;
    int    rc;
    size_t len;

#ifdef PF_PACKET
    char ifname[TAD_IP4_IFNAME_SIZE];
#endif

    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    csap_set_rw_data(csap, spec_data);

    /* opening incoming socket */
#ifdef PF_PACKET
    len = sizeof(ifname);
    rc = asn_read_value_field(csap->layers[csap_get_rw_layer(csap)].nds,
                              ifname, &len, "ifname");
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    spec_data->sa_op.sll_family = PF_PACKET;
    spec_data->sa_op.sll_ifindex = if_nametoindex(ifname);
    spec_data->sa_op.sll_protocol = htons(ETH_P_IP);

    len = ETHER_ADDR_LEN;
    rc = asn_read_value_field(csap->layers[csap_get_rw_layer(csap)].nds,
                              spec_data->sa_op.sll_addr, &len, "remote-hwaddr");
    if (rc != 0)
    {
        if (rc != TE_EASNINCOMPLVAL)
            return TE_RC(TE_TAD_CSAP, rc);

        spec_data->sa_op.sll_halen = 0;
    }
    else
    {
        spec_data->sa_op.sll_halen = len;
    }

    spec_data->socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
#else
    len = sizeof(struct in_addr);
    rc = asn_read_value_field(csap->layers[csap_get_rw_layer(csap)].nds,
                              &spec_data->sa_op.sin_addr.s_addr, &len,
                              "local-addr");
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    spec_data->sa_op.sin_family = AF_INET;
    spec_data->sa_op.sin_port = 0;

    spec_data->socket = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
#endif

    if (spec_data->socket < 0)
    {
        return TE_OS_RC(TE_TAD_CSAP, errno);
    }
    if (setsockopt(spec_data->socket, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0)
    {
        return TE_OS_RC(TE_TAD_CSAP, errno);
    }

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_rw_destroy_cb(csap_p csap)
{
    tad_ip4_rw_data    *spec_data = csap_get_rw_data(csap);

    if (spec_data->socket >= 0)
    {
        close(spec_data->socket);
        spec_data->socket = -1;
    }

    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    tad_ip4_rw_data    *spec_data = csap_get_rw_data(csap);
    te_errno            rc;

    rc = tad_common_read_cb_sock(csap, spec_data->socket, 0, timeout,
                                 pkt, NULL, NULL, pkt_len, NULL, NULL, NULL);

    return rc;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_write_cb(csap_p csap, const tad_pkt *pkt)
{
    tad_ip4_rw_data    *spec_data = csap_get_rw_data(csap);
    struct msghdr       msg;
    size_t              iovlen = tad_pkt_seg_num(pkt);
    struct iovec        iov[iovlen];
    te_errno            rc;
#ifdef PF_PACKET
    char ifname[TAD_IP4_IFNAME_SIZE];
    size_t len;
#endif


    if (spec_data->socket < 0)
    {
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    /* Convert packet segments to IO vector */
    if ((rc = tad_pkt_segs_to_iov(pkt, iov, iovlen)) != 0)
    {
        ERROR("Failed to convert segments to IO vector: %r", rc);
        return rc;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (struct sockaddr *)&spec_data->sa_op;
    msg.msg_namelen = sizeof(spec_data->sa_op);
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;

    if (sendmsg(spec_data->socket, &msg, 0) < 0)
    {
#ifdef PF_PACKET
        if (errno == ENXIO &&
            spec_data->sa_op.sll_family == PF_PACKET)
        {
            ERROR("%s(): sendmsg() failed with ENXIO, try "
                  " to update interface index", __FUNCTION__);
            /*
             * May be obsolete interface index.
             * Try to update it and send packet again.
             */
            len = sizeof(ifname);
            if ((rc = asn_read_value_field(
                        csap->layers[csap_get_rw_layer(csap)].nds,
                        ifname, &len, "ifname")) != 0)
            {
                return TE_RC(TE_TAD_CSAP, rc);
            }

            spec_data->sa_op.sll_ifindex = if_nametoindex(ifname);

            ERROR("%s(): try sendmsg() again after "
                  "updating index of tester's interface %s",
                  __FUNCTION__, ifname);

            if (sendmsg(spec_data->socket, &msg, 0) < 0)
            {
                ERROR("%s(): sendmsg() failed again", __FUNCTION__);
                return TE_OS_RC(TE_TAD_CSAP, errno);
            }

            return 0;
        }
#endif

        return TE_OS_RC(TE_TAD_CSAP, errno);
    }

    return 0;
}
