/** @file
 * @brief Test Environment
 *
 * Check UDP/IP4/ICMP4/IP4/ETH CSAP behaviour
 * when sendind ICMP messages with udp error replies
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
 * $Id$
 */

/** @page ipstack-ip4_send_icmp_error_udp Send ICMP datagram with udp error replu message via udp.ip4.icmp4.ip4.eth CSAP and check UDP socket error
 *
 * @objective Check that udp.ip4.icmp4.ip4.eth CSAP can be used to
 *            send ICMP datagrams with user-specified UDP error reply
 *            messages.
 *
 * @param host_csap     TA with CSAP
 * @param pco           TA with UDP socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param type          ICMP message's type
 * @param code          ICMP message's code
 *
 * @par Scenario:
 *
 * -# Create udp.ip4.icmp4.ip4.eth CSAP on @p pco_csap. 
 * -# Create datagram socket on @p pco_sock.
 * -# Send ICMP message having user-specified 
 *    udp error reply message.
 * -# Check that UDP socket has socket error appropriate to
 *    the sent error message.
 * -# Check that socket error was reset by getsockopt() call.
 * -# Call recvmsg() with flag MSG_ERRQUEUE to receive
 *    ICMP error message via socket.
 * -# Check received message to have the same IP/port, type and code
 *    fields as sent message has.
 * -# Call recvmsg() again to make sure that socket has
 *    no messages else.
 * -# Destroy CSAP and close socket
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "ipstack/ip4_send_icmp_error_udp"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#define TST_CMSG_LEN   300
#define TST_VEC        1

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#if HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif

#include <linux/types.h>
#include <linux/errqueue.h>

#include "tad_common.h"
#include "rcf_rpc.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_eth.h"
#include "tapi_ip4.h"
#include "tapi_icmp4.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"
#include "tapi_rpc_params.h"

int
main(int argc, char *argv[])
{
    const uint16_t              ip_eth = ETHERTYPE_IP;
    tapi_env_host              *host_csap = NULL;
    rcf_rpc_server             *pco = NULL;
    const struct sockaddr      *csap_addr;
    const struct sockaddr      *sock_addr;
    const struct sockaddr      *csap_hwaddr;
    const struct sockaddr      *sock_hwaddr;
    const struct if_nameindex  *csap_if;
    int                         type;
    int                         code;
    rpc_errno                   exp_errno;

    csap_handle_t               send_csap = CSAP_INVALID_HANDLE;
    asn_value                  *csap_spec = NULL;
    asn_value                  *template = NULL;
    
    int                         recv_socket = -1;

    int                         recverr = 1;
    int                         sock_error;

    uint8_t                     rx_buf[100];
    size_t                      rx_buf_len = sizeof(rx_buf);
    struct rpc_iovec            rx_vector;
    
    struct sockaddr_storage     msg_name;
    socklen_t                   msg_namelen =
                                    sizeof(struct sockaddr_storage);
    uint8_t                     cmsg_buf[TST_CMSG_LEN] = { 0, };
    struct cmsghdr             *cmsg;
    
    int                         received;
    rpc_msghdr                  rx_msghdr;
    struct sock_extended_err   *optptr;

    TEST_START; 

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(type);
    TEST_GET_INT_PARAM(code);
    TEST_GET_ERRNO_PARAM(exp_errno);

    rx_vector.iov_base = rx_buf;
    rx_vector.iov_len = rx_vector.iov_rlen = rx_buf_len;

    rx_msghdr.msg_iovlen = rx_msghdr.msg_riovlen = TST_VEC;
    rx_msghdr.msg_iov = &rx_vector;
    rx_msghdr.msg_control = cmsg_buf;
    rx_msghdr.msg_controllen = TST_CMSG_LEN;
    rx_msghdr.msg_cmsghdr_num = 1;
    rx_msghdr.msg_name = &msg_name;
    rx_msghdr.msg_namelen = rx_msghdr.msg_rnamelen = msg_namelen;
    rx_msghdr.msg_flags = 0;
    
    /* Create UDP socket */
    recv_socket = rpc_socket(pco, RPC_PF_INET, 
                             RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);
    /* Bind socket to the local IP/port */
    rpc_bind(pco, recv_socket, sock_addr);

    /* 
     * Prepare socket to receive ICMP error
     * messages
     */
    rpc_setsockopt(pco, recv_socket, RPC_IP_RECVERR, &recverr);

    /* Create CSAP */
    CHECK_RC(tapi_udp_ip4_icmp_ip4_eth_csap_create(
                                  host_csap->ta, 0,
                                  csap_if->if_name,
                                  TAD_ETH_RECV_NO,
                                  sock_hwaddr->sa_data,
                                  csap_hwaddr->sa_data,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  htons(SIN(sock_addr)->sin_port),
                                  htons(SIN(csap_addr)->sin_port),
                                  &send_csap));
    
    /* Prepare data-sending template */
    /* 
     * Prepare UDP error message's UDP header, src_port being
     * port to which UDP socket is bound
     */
    CHECK_RC(tapi_udp_add_pdu(&template, NULL,
                              FALSE, 
                              htons(SIN(sock_addr)->sin_port),
                              htons(SIN(csap_addr)->sin_port)));
    /*
     * Prepare UDP error message's IP header, src_addr
     * being IP to which UDP socket is bound
     */
    CHECK_RC(tapi_ip4_add_pdu(&template, NULL,
                              FALSE,
                              SIN(sock_addr)->sin_addr.s_addr,
                              SIN(csap_addr)->sin_addr.s_addr,
                              IPPROTO_UDP,
                              -1,
                              -1));
    /* Fill ICMP error message's type and code fields */
    CHECK_RC(tapi_icmp4_add_pdu(&template, NULL,
                                FALSE, type, code));
    /* Prepare ICMP error message's IP header */
    CHECK_RC(tapi_ip4_add_pdu(&template, NULL,
                              FALSE,
                              SIN(csap_addr)->sin_addr.s_addr,
                              SIN(sock_addr)->sin_addr.s_addr,
                              IPPROTO_ICMP,
                              -1, 
                              -1));
    /* Prepare ICMP error message's ETH header */
    CHECK_RC(tapi_eth_add_pdu(&template, NULL,
                              FALSE,
                              sock_hwaddr->sa_data,
                              csap_hwaddr->sa_data,
                              &ip_eth,
                              TE_BOOL3_ANY,
                              TE_BOOL3_ANY));
    
    /* Start sending data via CSAP */
    CHECK_RC(tapi_tad_trsend_start(host_csap->ta, 0, send_csap,
                                   template, RCF_MODE_NONBLOCKING));

    MSLEEP(100);

    /* Get socket error */
    CHECK_RC(rpc_getsockopt(pco, recv_socket,
                            RPC_SO_ERROR,
                            &sock_error));
    
    /* Check socket error */
    if (sock_error != (int)exp_errno)
        TEST_FAIL("SO_ERROR is set to %s "
                  "instead of expected %s",
                  errno_rpc2str(sock_error),
                  errno_rpc2str(exp_errno));

    /*
     * Check that socket error was reset in previous
     * getsockopt() call
     */
    CHECK_RC(rpc_getsockopt(pco, recv_socket,
                            RPC_SO_ERROR,
                            &sock_error));
    if (sock_error != 0)
        TEST_FAIL("Socket error was unexpectedly not reset in "
                  "previous getsockopt() call");

    /*
     * Receive icmp messages pending on socket
     */
    received = rpc_recvmsg(pco, recv_socket, &rx_msghdr, RPC_MSG_ERRQUEUE);

    /* 
     * Check that protocol name is equal to the
     * destination IP
     */
    if (te_sockaddrcmp(SA(&msg_name), rx_msghdr.msg_namelen,
                       csap_addr, te_sockaddr_get_size(csap_addr)) != 0)
        TEST_FAIL("Returned message name:%s is not the same as "
             "destination addr:%s reside in ICMP message payload",
             te_sockaddr2str(SA(&msg_name)), te_sockaddr2str(csap_addr));

    /* Check that message's flag is set to MSG_ERRQUEUE */
    if (!(rx_msghdr.msg_flags & RPC_MSG_ERRQUEUE))
    {
        TEST_FAIL("Unexpected msghdr.msg_flags value returned %s, "
                  "expected MSG_ERRQUEUE",
                  send_recv_flags_rpc2str(rx_msghdr.msg_flags));
    }

    /* Get returned ancillary data */
    cmsg = CMSG_FIRSTHDR(&rx_msghdr);
    if (cmsg == NULL) 
        TEST_FAIL("Ancillary data on pco_iut socket "
                  "is not recieved");

    /* Check ancilliary data */
    optptr = (struct sock_extended_err *) CMSG_DATA(cmsg);
    if (cmsg->cmsg_level != SOL_IP ||
        cmsg->cmsg_type != IP_RECVERR ||
        (errno_h2rpc(optptr->ee_errno) != exp_errno) ||
        (optptr->ee_origin != SO_EE_ORIGIN_ICMP) ||
        (optptr->ee_type != type) ||
        (optptr->ee_code != code) ||
        (optptr->ee_pad != 0))
        TEST_FAIL("Returned unexpected values of ancillary data");
    
    /* Check that error queue is empty */
    RPC_AWAIT_IUT_ERROR(pco);
    received = rpc_recvmsg(pco, recv_socket, &rx_msghdr, RPC_MSG_ERRQUEUE);
    if (received != -1)
    {
        TEST_FAIL("recvmsg() return %d, but it is expected to "
                  "return -1, because error queue is empty", received);
    }
    CHECK_RPC_ERRNO(pco, RPC_EAGAIN, "recvmsg() returns -1, but");

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco, recv_socket);

    asn_free_value(template);
    asn_free_value(csap_spec);
    
    if (host_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_csap->ta, 0, 
                                             send_csap));

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
