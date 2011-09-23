/*
 * IPv6 Demo Test Suite
 * IPv6 specific socket options
 *
 * Copyright (C) 2007 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * $Id$
 */

/** @page opt_ipv6_only IPV6_V6ONLY socket option with IPv4 nodes (SOCK_STREAM).
 *
 * @objective Verify that IPV6_V6ONLY socket option can be used to 
 *            deny stream connections with IPv4 peers.
 *
 * @type conformance
 *
 * @reference @ref RFC3493
 *
 * @param pco_iut       PCO on IUT
 * @param pco_tst       PCO on Tester
 * @param iut_addr4     IPv4 address on IUT
 * @param iut_addr6     IPv6 address on IUT
 * 
 * @par Test sequence:
 * -# Create @p iut_s6 socket of type @p sock_type from @c PF_INET6 domain
 *    on @p pco_iut;
 * -# Create @p tst_s4 socket of type @p sock_type from @c PF_INET domain
 *    on @p pco_tst (it will be used as an IPv4 peer of IPv6 socket 
 *    created on @p pco_iut);
 * -# Create @p tst_s6 socket of type @p sock_type from @c PF_INET6 domain
 *    on @p pco_tst (it will be used as an IPv6 peer of IPv6 socket 
 *    created on @p pco_iut);
 * -# Enable @c IPV6_V6ONLY socket option on @p pco_iut socket;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# @b bind() @p iut_s6 socket to IPv6 wildcard address;
 * -# @b listen(@p iut_s6, @c 1);
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# @b connect() @p tst_s4 socket to @p iut_addr4;
 * -# Check that @p iut_s6 socket is not readable;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# @b connect() @p tst_s6 socket to @p iut_addr6;
 * -# Check that @p iut_s6 socket is readable and open a new connection;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# Disable @c IPV6_V6ONLY socket option on @p pco_iut socket;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# @b connect() @p tst_s4 socket to @p iut_addr4;
 * -# Check that @p iut_s6 socket is readable and open a new connection;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# @b connect() @p tst_s6 socket to @p iut_addr6;
 * -# Check that @p iut_s6 socket is readable and open a new connection;
 *    \n @htmlonly &nbsp; @endhtmlonly
 * -# Close all sockets.
 * 
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "sockets/opt_ipv6_only"

#include "ipv6-demo-test.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_tst = NULL;
    int             iut_s6 = -1;
    int             tst_s4 = -1;
    int             tst_s6 = -1;
    int             conn_s = -1;

    const struct sockaddr *iut_addr4 = NULL;
    const struct sockaddr *iut_addr6 = NULL;
    struct sockaddr_in6    wild_addr;

    int       opt_val;
    in_port_t port1;
    in_port_t port2;
    

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(pco_iut, iut_addr4);
    TEST_GET_ADDR(pco_iut, iut_addr6);

    port1 = SIN(iut_addr4)->sin_port;
    port2 = SIN6(iut_addr6)->sin6_port;

    iut_s6 = rpc_socket(pco_iut, RPC_PF_INET6, RPC_SOCK_STREAM, RPC_PROTO_DEF);

    tst_s4 = rpc_socket(pco_tst, RPC_PF_INET, RPC_SOCK_STREAM, RPC_PROTO_DEF);
    tst_s6 = rpc_socket(pco_tst, RPC_PF_INET6, RPC_SOCK_STREAM, RPC_PROTO_DEF);

    /***** Enable IPV6_V6ONLY option *****/
    opt_val = TRUE;
    rpc_setsockopt(pco_iut, iut_s6, RPC_IPV6_V6ONLY, &opt_val);
 
    /*
     * Prepare wildcard address and set up the same port value
     * in all addresses.
     */
    memset(&wild_addr, 0, sizeof(wild_addr));
    wild_addr.sin6_family = AF_INET6;
    te_sockaddr_set_wildcard(SA(&wild_addr));

    SIN(iut_addr4)->sin_port = wild_addr.sin6_port = 
            SIN6(iut_addr6)->sin6_port = port1;

    rpc_bind(pco_iut, iut_s6, SA(&wild_addr));
    rpc_listen(pco_iut, iut_s6, 1);

    /* 
     * Check that it is denied to open 
     * IPv4 socket --> IPv6 socket connection.
     */
    RPC_AWAIT_IUT_ERROR(pco_tst);
    rc = rpc_connect(pco_tst, tst_s4, iut_addr4);

    if (rc != -1)
        TEST_FAIL("IUT accepts IPv4 connections on PF_INET6 socket, "
                  "although RPC_IPV6_V6ONLY socket option is ON");

    CHECK_RPC_ERRNO(pco_tst, RPC_ECONNREFUSED, "connect() returns -1, but");

    SLEEP(3);
    CHECK_READABILITY(pco_iut, iut_s6, FALSE);

    /* 
     * Check that it is still possible to open 
     * IPv6 socket --> IPv6 socket connection.
     */
    rpc_connect(pco_tst, tst_s6, iut_addr6);

    /* Open a new connection */
    CHECK_READABILITY(pco_iut, iut_s6, TRUE);
    conn_s = rpc_accept(pco_iut, iut_s6, NULL, NULL);
    RPC_CLOSE(pco_iut, conn_s);

    /* Recreate a socket as it is already connected */
    RPC_CLOSE(pco_tst, tst_s6);
    tst_s6 = rpc_socket(pco_tst, RPC_PF_INET6, RPC_SOCK_STREAM, RPC_PROTO_DEF);

    /***** Disable IPV6_V6ONLY option *****/
    opt_val = FALSE;

    /*
     * On some systems it is not allowed to change IPV6_V6ONLY option
     * on active (server) socket, which is why we need to recreate 
     * our server.
     */
    RPC_AWAIT_IUT_ERROR(pco_iut);
    rc = rpc_setsockopt(pco_iut, iut_s6, RPC_IPV6_V6ONLY, &opt_val);

    if (rc == -1)
    {
        CHECK_RPC_ERRNO(pco_iut, RPC_EINVAL, "setsockopt() returns -1, but");

        RPC_CLOSE(pco_iut, iut_s6);

        /*
         * In order to avoid problems with EADDRINUSE while binding we
         * should change port number on IPv6 server socket, and as a result
         * in all address structures used to connect to the server.
         */
        SIN(iut_addr4)->sin_port = wild_addr.sin6_port = 
                SIN6(iut_addr6)->sin6_port = port2;

        iut_s6 = rpc_socket(pco_iut, RPC_PF_INET6, RPC_SOCK_STREAM,
                            RPC_PROTO_DEF);
        rpc_bind(pco_iut, iut_s6, SA(&wild_addr));
        rpc_listen(pco_iut, iut_s6, 1);
    }

    /*
     * Check that now it is possible to open 
     * IPv4 socket --> IPv6 socket connection.
     */
    rpc_connect(pco_tst, tst_s4, iut_addr4);

    /* Open a new connection */
    CHECK_READABILITY(pco_iut, iut_s6, TRUE);
    conn_s = rpc_accept(pco_iut, iut_s6, NULL, NULL);
    RPC_CLOSE(pco_iut, conn_s);

    /*
     * Check that it is still possible to open 
     * IPv6 socket --> IPv6 socket connection.
     */
    rpc_connect(pco_tst, tst_s6, iut_addr6);

    /* Open a new connection */
    CHECK_READABILITY(pco_iut, iut_s6, TRUE);
    conn_s = rpc_accept(pco_iut, iut_s6, NULL, NULL);
    RPC_CLOSE(pco_iut, conn_s);

    TEST_SUCCESS;
cleanup:
    CLEANUP_RPC_CLOSE(pco_iut, iut_s6);
    CLEANUP_RPC_CLOSE(pco_iut, conn_s);
    CLEANUP_RPC_CLOSE(pco_tst, tst_s4);
    CLEANUP_RPC_CLOSE(pco_tst, tst_s6);

    TEST_END;
}
