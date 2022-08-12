/** @file
 * @brief Test Environment
 *
 * Simple RPC test
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_TEST_NAME    "rpctest"

#include "rpc_suite.h"

int
main(int argc, char **argv)
{
    char ta[32];
    size_t len = sizeof(ta);

    rcf_rpc_server *srv1 = NULL, *srv2 = NULL, *srv3 = NULL;

    int s1, s2, s3;

    tarpc_socket_in  in;
    tarpc_socket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    TEST_START;

    if (rcf_get_ta_list(ta, &len) != 0)
        TEST_FAIL("rcf_get_ta_list() failed");

    if ((rc = rcf_rpc_server_create(ta, "Main", &srv1)) != 0)
        TEST_FAIL("Cannot create server %r", rc);

    if ((rc = rcf_rpc_server_fork(srv1, "Forked", &srv2)) != 0)
        TEST_FAIL("Cannot fork server %r", rc);

    if ((rc = rcf_rpc_server_thread_create(srv2, "Thread", &srv3)) != 0)
        TEST_FAIL("Cannot create threadserver %r", rc);

    s1 = rpc_socket(srv1, RPC_PF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);

    s2 = rpc_socket(srv2, RPC_PF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);

    s3 = rpc_socket(srv3, RPC_PF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);

    rpc_close(srv1, s1);
    rpc_close(srv2, s2);
    rpc_close(srv3, s3);

    TEST_SUCCESS;

cleanup:
    if (srv3 != NULL && rcf_rpc_server_destroy(srv3) != 0)
        ERROR("Cannot delete thread server");

    if (srv2 != NULL && rcf_rpc_server_destroy(srv2) != 0)
        ERROR("Cannot delete forked server");

    if (srv1 != NULL && rcf_rpc_server_destroy(srv1) != 0)
        ERROR("Cannot delete main server");

    TEST_END;
}
