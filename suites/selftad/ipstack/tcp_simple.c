/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple UDP CSAP test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "ipstack/tcp_simple"

#define TE_LOG_LEVEL 0xff

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "conf_api.h"
#include "tapi_rpc.h"

#include "tapi_test.h"
#include "tapi_ip4.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    rcf_rpc_server *srv_src = NULL, *srv_dst = NULL;
    int  sid;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    size_t  len = sizeof(ta);

    char path[1000];


    TEST_START;

    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len)
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            TEST_FAIL("rcf_ta_create_session failed");
            return 1;
        }
        INFO("Test: Created session: %d", sid);
    }

    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &srv_src)) != 0)
    {
        TEST_FAIL("Cannot create server %x", rc);
    }
    srv_src->def_timeout = 5000;

    rcf_rpc_setlibname(srv_src, NULL);


    if ((rc = rcf_rpc_server_create(agt_a, "SECOND", &srv_dst)) != 0)
    {
        TEST_FAIL("Cannot create server %x", rc);
    }
    srv_dst->def_timeout = 5000;

    rcf_rpc_setlibname(srv_dst, NULL);

    do {
        asn_value *csap_spec, *pattern;

        int sock_src;
        int sock_dst;

        int csap;
        int num;
        int rc_mod;

        if ((sock_src = rpc_socket(srv_src, RPC_AF_INET, RPC_SOCK_STREAM,
                            RPC_IPPROTO_TCP)) < 0 || srv_src->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %r", srv_src->_errno);
        }

        if ((sock_dst = rpc_socket(srv_dst, RPC_AF_INET, RPC_SOCK_STREAM,
                            RPC_IPPROTO_TCP)) < 0 || srv_dst->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %r", srv_dst->_errno);
        }

        rc = asn_parse_value_text("{layers {tcp:{local-port plain:0}, "
                                  " ip4:{max-packet-size plain:100000},"
                                  " eth:{device-id plain:\"eth0\"}}}",
                                  ndn_csap_spec, &csap_spec, &num);
        VERB("CSAP spec parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error");

        strcpy(path, "/tmp/te_tcp_csap_create.XXXXXX");
        mkstemp(path);
        VERB("file name for csap spec: '%s'", path);

        asn_save_to_file(csap_spec, path);


        rc = rcf_ta_csap_create(ta, sid, "tcp.ip4.eth", path, &csap);
        INFO("csap_create rc: %d, csap id %d\n", rc, csap);
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            INFO ("rc from module %d is %r\n",
                        rc_mod, TE_RC_GET_ERROR(rc));
        }

        strcpy(path, "/tmp/te_tcp_pattern.XXXXXX");
        mkstemp(path);
        VERB("file name for tcp pattern: '%s'", path);

        num = 0;
        rc = asn_parse_value_text(
"{{ action function:\"tadf_forw_packet:a\","
" pdus {tcp:{dst-port plain:6100},"
" ip4:{protocol plain:6}, eth:{length-type plain:2048}}}}",
                                  ndn_traffic_pattern, &pattern, &num);
        VERB("Pattern parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error");

        asn_save_to_file(pattern, path);
#if 1
        rc = rcf_ta_trrecv_start(ta, sid, csap, path, TAD_TIMEOUT_INF, 0,
                                 RCF_TRRECV_COUNT);
        INFO("trrecv_start: %r \n", rc);
        if (rc) break;

#if 1
        sleep(5);
        INFO ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, sid, csap, NULL, NULL, &num);
        INFO("trrecv_get: %r num: %d\n", rc, num);
        if (rc) break;

#endif
        num = 10;
        INFO ("sleep %d secs before stop\n", num);
        sleep (num);

        INFO ("try to stop\n");
        rc = rcf_ta_trrecv_stop(ta, sid, csap, NULL, NULL, &num);
        INFO("trrecv_stop: %r num: %d\n", rc, num);

#endif
        rc = rcf_ta_csap_destroy(ta, sid, csap);
        INFO("csap %d destroy: %r ", csap, rc);

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %X", rc);

    TEST_SUCCESS;

cleanup:
    if (srv_dst && (rcf_rpc_server_destroy(srv_dst) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }

    if (srv_src && (rcf_rpc_server_destroy(srv_src) != 0))
    {
        WARN("Cannot delete src RPC server\n");
    }

    TEST_END;
}
