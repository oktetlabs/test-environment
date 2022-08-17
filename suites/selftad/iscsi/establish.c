/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * iSCSI CSAP and TAPI test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "iscsi/establish"

#define TE_LOG_LEVEL 0xff

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "tapi_rpc.h"

#include "tapi_test.h"
#include "tapi_tcp.h"
#include "tapi_iscsi.h"

#include "te_bufs.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "ndn_iscsi.h"

uint8_t tx_buffer[10000];
uint8_t rx_buffer[10000];



int
main(int argc, char *argv[])
{
    csap_handle_t iscsi_csap = CSAP_INVALID_HANDLE;
    csap_handle_t listen_csap = CSAP_INVALID_HANDLE;
    csap_handle_t acc_csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *rpc_srv = NULL;
    tarpc_pid_t pid;

    char *buf = NULL;

    int acc_sock;

    char  ta[32];
    char *agt_a = ta;
    char *agt_b;

    size_t  len = sizeof(ta);

    struct sockaddr_in csap_addr;


    in_addr_t csap_ip_addr = inet_addr("192.168.37.18");

    TEST_START;

    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len)
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &rpc_srv)) != 0)
        TEST_FAIL("Cannot create server %x", rc);

    rpc_srv->def_timeout = 5000;

    rcf_rpc_setlibname(rpc_srv, NULL);

    csap_addr.sin_family = AF_INET;
    csap_addr.sin_addr.s_addr = csap_ip_addr;
    csap_addr.sin_port = htons(3260);



    rc = tapi_tcp_server_csap_create(agt_a, 0, &csap_addr, &listen_csap);
    if (rc != 0)
        TEST_FAIL("server csap create failed: %r", rc);

    pid = rpc_ta_shell_cmd_ex(rpc_srv, "/tmp/ini-conn-up", -1, NULL, NULL);

    RING("shell_cmd_ex pid: %d", pid);


    rc = tapi_tcp_server_recv(agt_a, 0, listen_csap, 1000, &acc_sock);
    if (rc != 0)
        TEST_FAIL("recv accepted socket failed: %r", rc);

    RING("acc socket: %d", acc_sock);

    rc = tapi_tcp_socket_csap_create(agt_a, 0, acc_sock, &acc_csap);
    if (rc != 0)
        TEST_FAIL("create CSAP over accepted socket failed, %r", rc);

    rc = tapi_iscsi_csap_create(agt_a, 0, &iscsi_csap);
    if (rc != 0)
        TEST_FAIL("iSCSI csap create failed: %r", rc);


    /*
     * TCP connection єstablished, start data transmit
     */

    do {
        /* I->T */
        len = sizeof(rx_buffer);
        memset(rx_buffer, 0, len);
        rc = tapi_tcp_buffer_recv(agt_a, 0, acc_csap, 2000,
                                  CSAP_INVALID_HANDLE, FALSE,
                                  rx_buffer, &len);
        if (rc != 0)
            TEST_FAIL("recv from NET failed: %r", rc);
        RING("received %d bytes from NET", len);

        rc = tapi_iscsi_send_pkt(agt_a, 0, iscsi_csap, NULL,
                                 rx_buffer, len);
        if (rc != 0)
            TEST_FAIL("send to TARGET failed: %r", rc);

        /* T->I */
        len = sizeof(tx_buffer);
        memset(tx_buffer, 0, len);
        rc = tapi_iscsi_recv_pkt(agt_a, 0, iscsi_csap, 2000,
                                  CSAP_INVALID_HANDLE, NULL,
                                  tx_buffer, &len);
        if (rc != 0)
            TEST_FAIL("recv on CSAP failed: %r", rc);

        INFO("+++++++++++ data from TARGET to NET: %Tm", tx_buffer, len);

        rc = tapi_tcp_buffer_send(agt_a, 0, acc_csap,
                                  tx_buffer, len);
        if (rc != 0)
            TEST_FAIL("recv on CSAP failed: %r", rc);
    } while (1);

#if 0
    /* temporary check, while there is dummy 'target' on TA */
    rc = memcmp(tx_buffer, rx_buffer, len);
    if (rc != 0)
        TEST_FAIL("iSCSI: sent and received data differ, rc = %d", rc);
#endif


    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(rcf_ta_csap_destroy(agt_a, 0, iscsi_csap));

    free(buf);
    TEST_END;
}
