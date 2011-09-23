/*
 * IPv6 Demo Test Suite
 * Send Receive functionality
 *
 * Copyright (C) 2007 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * $Id$
 */

/** @page sockets-send_recv Basic send receive operations
 *
 * @objective Check that it is possible to send some data over socket.
 *
 * @type sockets
 *
 * @reference 
 * 
 * @param pco_iut           PCO on IUT
 * @param pco_tst           PCO on TESTER
 * 
 * @par Test sequence:
 * -# 
 * -# 
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "basic/send_recv"

#include "ipv6-demo-test.h"

int main(int argc, char *argv[])
{
    rpc_socket_type  sock_type;
    rcf_rpc_server  *pco_iut = NULL;
    rcf_rpc_server  *pco_tst = NULL;
    int              iut_s = -1;
    int              tst_s = -1;

    const struct sockaddr  *iut_addr;
    const struct sockaddr  *tst_addr;

    int     err;
    void   *rd_buf = NULL;
    void   *wr_buf = NULL;
    size_t  buf_len;


    TEST_START;

    TEST_GET_SOCK_TYPE(sock_type);

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(pco_iut, iut_addr);
    TEST_GET_ADDR(pco_tst, tst_addr);

    /* Prepare read/write buffer for data exchange */
    wr_buf = te_make_buf(20, 100, &buf_len);
    rd_buf = te_make_buf_by_len(buf_len);

    iut_s = rpc_socket(pco_iut, rpc_socket_domain_by_addr(iut_addr), 
                       sock_type, RPC_PROTO_DEF);
    tst_s = rpc_socket(pco_tst, rpc_socket_domain_by_addr(tst_addr), 
                       sock_type, RPC_PROTO_DEF);

    SLEEP(10);

    /* TST socket should be a receiving end point */
    rpc_bind(pco_tst, tst_s, tst_addr);

    if (sock_type == RPC_SOCK_STREAM)
        rpc_listen(pco_tst, tst_s, 1);

    rpc_connect(pco_iut, iut_s, tst_addr);

    if (sock_type == RPC_SOCK_STREAM)
    {
        /* Open a new connection and close server socket */
        int tmp_s;

        tmp_s = rpc_accept(pco_tst, tst_s, NULL, NULL);
        rpc_close(pco_tst, tst_s);
        tst_s = tmp_s;
    }

    rc = rpc_send(pco_iut, iut_s, wr_buf, buf_len, 0);
    if (rc != (int)buf_len)
    {
        err = RPC_ERRNO(pco_iut);
        TEST_FAIL("RPC send() on pco_iut failed RPC_errno=%X",
                  TE_RC_GET_ERROR(err));
    }

    rc = rpc_recv(pco_tst, tst_s, rd_buf, buf_len, 0);
    if (rc < 0)
    {
        err = RPC_ERRNO(pco_tst);
        TEST_FAIL("RPC recv() on pco_tst failed RPC_errno=%X",
                  TE_RC_GET_ERROR(err));
    }
    
    if (rc != (int)buf_len)
    {
        TEST_FAIL("pco_tst received %d bytes of data but expected to "
                  "receive %d bytes", rc, buf_len);
    }

    if (memcmp(rd_buf, wr_buf, buf_len) != 0)
    {
        TEST_FAIL("RX and TX data mismatch!");
    }

    TEST_SUCCESS;

cleanup:
    CLEANUP_RPC_CLOSE(pco_iut, iut_s);
    CLEANUP_RPC_CLOSE(pco_tst, tst_s);

    free(rd_buf);
    free(wr_buf);

    TEST_END;
}
