..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; TCP socket emulation
.. _doxid-group__tcp__sock__emulation:

TCP socket emulation
====================

.. toctree::
	:hidden:

TCP socket emulation gives us full control over TCP behavior, so that we can choose when and how to respond to packets from a peer, send malformed packets to it, etc. Some use cases:

* Test TCP states which are usually hard to observe. For example, delay reply to SYN to observe TCP_SYN_SENT state on peer.

* Send reordered packets and check that they are correctly processed by a peer.

* Check what happens when overlapped TCP fragments are sent.

TCP socket emulation API is defined in ``lib/tapi_tad/ipstack/tapi_tcp.h``



.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_init:

Creating and destroying TCP socket emulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TCP socket emulation can be created with :ref:`tapi_tcp_init_connection() <doxid-group__tapi__tad__tcp_1ga6b4e5e9b2155fe822f46c0ea81b16506>`, which returns handler of type tapi_tcp_handler_t.

This function allows to specify local and remote IP and MAC addresses. It is important that local MAC address we use should not be assigned to any host in our network, including host on which we create TCP socket emulation. Otherwise Linux can disturb our connection by replying on packets from peer. For example, it can send RST to SYN sent from peer because it does not have any listener socket for that address and port and have no idea about our TCP socket emulation.

For alien MAC address to work, we should create an ARP entry for local IP address we use associating it with this MAC address. This ARP entry can be created either on test host where peer socket resides, or on a gateway host via which our hosts are connected.

:ref:`Route Gateway TAPI <doxid-group__route__gw>` uses creating such ARP entry with alien MAC address to break network connection in a specified direction between two hosts connected via gateway host. So it can be easily used with TCP socket emulation: break connection from a host where tested socket resides to a host where TCP socket emulation should be created, and then use :ref:`tapi_tcp_init_connection() <doxid-group__tapi__tad__tcp_1ga6b4e5e9b2155fe822f46c0ea81b16506>` passing to it the same alien MAC address as a local MAC.

Emulated TCP connection may be established either actively or passively.

Passive connection establishment:

#. Call :ref:`tapi_tcp_init_connection() <doxid-group__tapi__tad__tcp_1ga6b4e5e9b2155fe822f46c0ea81b16506>` with mode=TAPI_TCP_SERVER.

#. Call non-blocking connect() from peer.

#. Call :ref:`tapi_tcp_wait_open() <doxid-group__tapi__tad__tcp_1ga2471181d512d3d25ae294407d6ad51a0>`.

#. Check than connect() on peer returns success.

Active connection establishment:

#. Create listener socket on peer.

#. Call :ref:`tapi_tcp_init_connection() <doxid-group__tapi__tad__tcp_1ga6b4e5e9b2155fe822f46c0ea81b16506>` with mode=TAPI_TCP_CLIENT.

#. Call :ref:`tapi_tcp_wait_open() <doxid-group__tapi__tad__tcp_1ga2471181d512d3d25ae294407d6ad51a0>`.

#. Call accept() on peer.

TCP socket emulation can be destroyed with :ref:`tapi_tcp_destroy_connection() <doxid-group__tapi__tad__tcp_1ga17c0870454417b5a99d55bc93e79be5d>`.





.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_recv:

Receiving
~~~~~~~~~

* :ref:`tapi_tcp_wait_msg() <doxid-group__tapi__tad__tcp_1ga63dc0d47f9bdad9ae2b367abdaca7fe3>` - wait for new packet for a specified amount of time. It ignores retransmits. When a packet is received, information about the last received SEQN/ACKN, FIN, RST, etc. is updated.

* :ref:`tapi_tcp_recv_msg() <doxid-group__tapi__tad__tcp_1gaf9db6ec07b55d8e64a03b2d8b6c0c20b>` - retrieve payload and ACKN/SEQN of the next TCP packet; send ACK to it if requested. If there is no packet in queue, it waits for it for a specified amount of time.

* :ref:`tapi_tcp_recv_msg_gen() <doxid-group__tapi__tad__tcp_1ga4d3a7a5a995e6374309eb359659053a6>` - the same as :ref:`tapi_tcp_recv_msg() <doxid-group__tapi__tad__tcp_1gaf9db6ec07b55d8e64a03b2d8b6c0c20b>`, but it allows to filter out retransmits.

* :ref:`tapi_tcp_recv_data() <doxid-group__tapi__tad__tcp_1gac24feae401f1f5fec66c1b1e5c718546>` - append all payload data received from peer to specified dynamic buffer.





.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_send:

Sending
~~~~~~~

* :ref:`tapi_tcp_send_msg() <doxid-group__tapi__tad__tcp_1ga685264289c925ac4e83e5395bc529737>` - send TCP packet with specified payload.

* :ref:`tapi_tcp_send_template() <doxid-group__tapi__tad__tcp_1gafd5fcebd30ac2ed12a0acbdbb7b810db>` - send TCP packet according to template described in ASN. Template can be created with :ref:`tapi_tcp_conn_template() <doxid-group__tapi__tad__tcp_1gac1555426b20da4d4f114ace3fc2626a7>`.



.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_send_spec:

Sending TCP packets with special flags
--------------------------------------

* :ref:`tapi_tcp_send_fin() <doxid-group__tapi__tad__tcp_1gae87b9e19165410c3ebb1567d8dff2170>` - send FIN.

* :ref:`tapi_tcp_send_fin_ack() <doxid-group__tapi__tad__tcp_1ga371f68c420593772845b32e7178753a5>` - send FIN-ACK.

* :ref:`tapi_tcp_send_rst() <doxid-group__tapi__tad__tcp_1ga8045232fef58c66c0344c0d791cd2c7e>` - send RST.

* :ref:`tapi_tcp_send_ack() <doxid-group__tapi__tad__tcp_1ga0473edd00b7e1efd0382979b6acabe65>` - send ACK with specified ACKN.

* :ref:`tapi_tcp_ack_all() <doxid-group__tapi__tad__tcp_1ga4b6668322aa74b1a1e9c4859ea07e1cf>` - send ACK to all data received.







.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_info:

Obtaining information about TCP connection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* :ref:`tapi_tcp_last_seqn_got() <doxid-group__tapi__tad__tcp_1ga510b0a559657e89d3a8a5523e978e93e>` - get last SEQN received from a peer.

* :ref:`tapi_tcp_last_ackn_got() <doxid-group__tapi__tad__tcp_1ga88936949864dff594247d94f62379364>` - get last ACKN received from a peer.

* :ref:`tapi_tcp_last_win_got() <doxid-group__tapi__tad__tcp_1ga5bc524091e89d7c5997a09bcab7b0f21>` - get last TCP window size received from a peer.

* :ref:`tapi_tcp_fin_got() <doxid-group__tapi__tad__tcp_1ga834b79120ee2b94c8e9072c69f8f6778>` - check whether FIN was received from a peer.

* :ref:`tapi_tcp_rst_got() <doxid-group__tapi__tad__tcp_1ga7d3c823280f11e6b7d2cbac44e881ebe>` - check wheter RST was received from a peer.

* :ref:`tapi_tcp_last_seqn_sent() <doxid-group__tapi__tad__tcp_1ga3772fb87b1fefcd895cfd54636c58114>` - get last SEQN sent to a peer.

* :ref:`tapi_tcp_last_ackn_sent() <doxid-group__tapi__tad__tcp_1ga2e6edecb6a2d79a3d77e01b351876be9>` - get last ACKN sent to a peer.

* :ref:`tapi_tcp_next_seqn() <doxid-group__tapi__tad__tcp_1ga06dc8b36c7367d02e2b342beeab9a3f5>` - next SEQN to be sent to a peer.

* :ref:`tapi_tcp_next_ackn() <doxid-group__tapi__tad__tcp_1ga7c07230bc630c873fa9c8e701d66d062>` - next ACKN to be sent to a peer.





.. _doxid-group__tcp__sock__emulation_1tcp_sock_emulation_example:

Example
~~~~~~~

.. ref-code-block:: cpp

	#include "tapi_tcp.h"
	#include "tapi_route_gw.h"

	/** Maximum time connection establishment can take, in milliseconds. */
	#define TCP_CONN_TIMEOUT 500

	/** Test packet size. */
	#define PKT_SIZE 1024

	int
	main(int argc, char *argv[])
	{
	    rcf_rpc_server *pco_iut = NULL;
	    rcf_rpc_server *pco_tst = NULL;

	    const struct sockaddr *iut_addr;
	    const struct sockaddr *tst_addr;

	    int                 iut_s = -1;
	    tapi_tcp_handler_t  csap_tst_s = -1;

	    const struct sockaddr      *alien_link_addr = NULL;
	    const struct sockaddr      *iut_lladdr = NULL;

	    const struct if_nameindex *iut_if = NULL;
	    const struct if_nameindex *tst_if = NULL;

	    char snd_buf[PKT_SIZE];
	    char rcv_buf[PKT_SIZE];

	    /* Preambule */
	    TEST_START;
	    TEST_GET_PCO(pco_iut);
	    TEST_GET_PCO(pco_tst);
	    TEST_GET_ADDR(pco_iut, iut_addr);
	    TEST_GET_ADDR(pco_tst, tst_addr);
	    TEST_GET_LINK_ADDR(iut_lladdr);
	    TEST_GET_LINK_ADDR(alien_link_addr);
	    TEST_GET_IF(iut_if);
	    TEST_GET_IF(tst_if);

	    /*
	     * Add ARP entry resolving @p tst_addr to @p alien_link_addr
	     * on IUT, so that Linux on Tester will not reply to
	     * packets sent to it.
	     */
	    CHECK_RC(tapi_update_arp(pco_iut->ta, iut_if, NULL, NULL,
	                             tst_addr, alien_link_addr->sa_data, TRUE));

	    /*
	     * Create TCP socket emulation on Tester for passive connection
	     * establishment.
	     */
	    CHECK_RC(tapi_tcp_init_connection(pco_tst->ta, TAPI_TCP_SERVER,
	                                      tst_addr, iut_addr,
	                                      tst_if->if_name,
	                                      (uint8_t *)alien_link_addr->sa_data,
	                                      (uint8_t *)iut_lladdr->sa_data,
	                                      0, &csap_tst_s));

	    /* Create socket on IUT and establish TCP connection. */

	    iut_s = rpc_create_and_bind_socket(pco_iut, RPC_SOCK_STREAM,
	                                       RPC_PROTO_DEF, FALSE, FALSE,
	                                       iut_addr);

	    pco_iut->op = RCF_RPC_CALL;
	    rpc_connect(pco_iut, iut_s, tst_addr);

	    CHECK_RC(tapi_tcp_wait_open(csap_tst_s, TCP_CONN_TIMEOUT));

	    rpc_connect(pco_iut, iut_s, tst_addr);

	    te_fill_buf(snd_buf, PKT_SIZE);

	    /*
	     * Send some data from TCP socket emulation, receive and check it
	     * on peer socket.
	     */

	    CHECK_RC(tapi_tcp_send_msg(csap_tst_s, (uint8_t *)snd_buf, PKT_SIZE,
	                               TAPI_TCP_AUTO, 0,
	                               TAPI_TCP_AUTO, 0,
	                               NULL, 0));

	    rc = rpc_recv(pco_iut, iut_s, rcv_buf, PKT_SIZE, 0);
	    if (rc != PKT_SIZE ||
	        memcmp(snd_buf, rcv_buf, PKT_SIZE) != 0)
	        TEST_VERDICT("Received data does not match sent data");

	    TEST_SUCCESS;

	cleanup:

	    CLEANUP_RPC_CLOSE(pco_iut, iut_s);
	    CLEANUP_CHECK_RC(tapi_tcp_destroy_connection(csap_tst_s));
	    CLEANUP_CHECK_RC(tapi_update_arp(pco_iut->ta, iut_if,
	                                     pco_tst->ta, tst_if,
	                                     tst_addr, NULL, FALSE));

	    TEST_END;
	}

