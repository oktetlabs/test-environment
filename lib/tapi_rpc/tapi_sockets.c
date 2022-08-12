
/** @file
 * @brief Functions to opearate with socket.
 *
 * Implementation of test API for working with socket.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 */

#define TE_LGR_USER      "TAPI Socket"

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif

#include "te_errno.h"
#include "te_stdint.h"
#include "te_ipstack.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_rpc_socket.h"
#include "tad_common.h"
#include "tapi_sockets.h"

/* See description in tapi_sockets.h */
rpc_tcp_state
tapi_get_tcp_sock_state(struct rcf_rpc_server *pco,
                        int s)
{
    struct rpc_tcp_info tcpi;

    memset(&tcpi, 0, sizeof(struct tcp_info));

    rpc_getsockopt_gen(pco, s, rpc_sockopt2level(RPC_TCP_INFO),
                       RPC_TCP_INFO, &tcpi, NULL, NULL, 0);

    return tcpi.tcpi_state;
}

/* See description in tapi_sockets.h */
ssize_t
tapi_sock_read_data(rcf_rpc_server *rpcs,
                    int s,
                    te_dbuf *read_data)
{
#define READ_LEN  1024
    int   rc;
    char  data[READ_LEN];

    ssize_t  total_len = 0;

    while (TRUE)
    {
        RPC_AWAIT_ERROR(rpcs);
        rc = rpc_recv(rpcs, s, data, READ_LEN,
                      RPC_MSG_DONTWAIT);
        if (rc < 0)
        {
            if (RPC_ERRNO(rpcs) != RPC_EAGAIN)
            {
                ERROR("recv() failed with unexpected errno %r",
                      RPC_ERRNO(rpcs));
                return -1;
            }
            else
            {
                break;
            }
        }
        else if (rc == 0)
            break;

        te_dbuf_append(read_data, data, rc);
        total_len += rc;
    }

    return total_len;
}

te_errno
tapi_sock_raw_tcpv4_send(rcf_rpc_server *rpcs, rpc_iovec *iov,
                         int iovlen, int ifindex, int raw_socket,
                         te_bool remove_vlan_hdr)
{
    uint8_t                *raw_packet;
    ssize_t                 total_size = 0;
    struct sockaddr_ll      sadr_ll;
    ssize_t                 sent;
    te_errno                rc = 0;

    /* Prepare packet: headers + payload */
    total_size = rpc_iov_data_len(iov, iovlen);
    raw_packet = rpc_iovec_to_array(total_size, iov, iovlen);

    if (raw_packet == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = te_ipstack_prepare_raw_tcpv4_packet(raw_packet, &total_size,
                                             remove_vlan_hdr, &sadr_ll);
    if (rc != 0)
    {
        rc = TE_RC(TE_TAPI, rc);
        goto out;
    }

    /* Prepare destination address */
    sadr_ll.sll_ifindex = ifindex;

    /* Send prepared raw packet */
    RPC_AWAIT_ERROR(rpcs);
    sent = rpc_sendto_raw(rpcs, raw_socket, raw_packet, total_size, 0,
                          CONST_SA(&sadr_ll), sizeof(sadr_ll));
    if (sent < 0)
    {
        rc = RPC_ERRNO(rpcs);
    }
    else if (sent != total_size)
    {
        ERROR("sendto() returns %d, but expected return value is %d", sent,
               total_size);
        rc = TE_RC(TE_TAPI, TE_EFAIL);
    }

out:
    free(raw_packet);
    return rc;
}
