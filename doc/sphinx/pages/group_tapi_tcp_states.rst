.. index:: pair: group; TAPI for testing TCP states
.. _doxid-group__tapi__tcp__states:

TAPI for testing TCP states
===========================

.. toctree::
	:hidden:

	/generated/group_tapi_tcp_states_def.rst

tapi_tcp_states (TSA) library can be used to test a TCP socket in various TCP states. It is used to check that every TCP state can be reached according to TCP states diagram, and to check what happens if we perform some actions while being in some TCP state.

Its interface is defined in ``lib/tapi_tcp_states/tapi_tcp_states.h``.



.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_how:

How it works
~~~~~~~~~~~~

In this library it is supposed that we have a TCP socket under test, which we move to a desired TCP state, and its TCP peer, which plays auxiliary role (such as sending some packets to move the socket under test to a desired state). The peer may be not an usual TCP socket, but its emulation with help of RAW sockets. Usually the socket under test and its peer reside on different hosts - the socket under test is on IUT, whereas the peer is on Tester. However both may be on the same host if we test loopback.

Not all TCP states exist for a long time. Some of them usually take place only for a short time interval, until a peer replies to a packet which was sent to it by a socket under test. For example, a socket sends SYN to reach SYN_SENT state, and then quickly reaches ESTABLISHED state when SYN-ACK arrives from a peer.

To make such a transient TCP state stable for a while, we can either break network connection from Tester to IUT, so that no reply is received from the peer, or use tcp_sock_emulation instead of a normal socket on Tester, so that we can determine the exact moment when a reply is sent.





.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_init:

Initialization
~~~~~~~~~~~~~~

The library has three modes of operation:

* ``TSA_TST_SOCKET``. A real socket is used on Tester, and there is a gateway host connecting IUT and Tester, which is used to break connection when necessary.

* ``TSA_TST_CSAP``. tcp_sock_emulation is used on Tester; for this to work an ARP entry associating the peer address with an alien (i.e. not belonging to any existing host in the network) MAC address is created on IUT.

* ``TSA_TST_GW_CSAP``. Again, tcp_sock_emulation is used on Tester, however there is a gateway host in the middle connecting IUT and Tester, and an ARP entry is added on it. This mode is better than ``TSA_TST_CSAP`` because it requires no ARP changes on IUT.

To use the library, follow these steps:

#. Declare variable of type :ref:`tsa_session <doxid-structtsa__session>`, which will be used to store the session context.

#. Specify the mode of operation to :ref:`tsa_state_init() <doxid-group__tapi__tcp__states__def_1ga900fafc922c95fd441defff2fde6ed77>`.

#. Use :ref:`tsa_iut_set() <doxid-group__tapi__tcp__states__def_1ga5c2055d56f0240b8b9cdf20238f3cb3b>` and :ref:`tsa_tst_set() <doxid-group__tapi__tcp__states__def_1ga3b4d0924922d5f7a6d142d2ed9b6063f>` to specify IUT and Tester properties. If the selected mode of operation requires a gateway, use :ref:`tsa_gw_set() <doxid-group__tapi__tcp__states__def_1gadc3e55803f99b7436d7ab92b720a67c7>` too.

#. Use CFG_WAIT_CHANGES macro to wait until configuration changes are made.

#. Call :ref:`tsa_create_session() <doxid-group__tapi__tcp__states__def_1ga112ea2149a0b7098b1a2b92378f23679>` to create a socket under test on IUT and its peer on Tester. The socket under test is initially in TCP_CLOSE state.





.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_fini:

Finalization
~~~~~~~~~~~~

:ref:`tsa_destroy_session() <doxid-group__tapi__tcp__states__def_1ga89edeca462df2cc3657847a9d1a33664>` should be used to perform cleanup after using the library.





.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_use:

Usage
~~~~~

The following functions can be used to perform transition to desired TCP state following a chosen path:

* :ref:`tsa_do_tcp_move() <doxid-group__tapi__tcp__states__def_1ga446dccab8ee2b79fbe8e740f33b95dad>` can be used to move from the current TCP state to one of the next states, according to the TCP state machine.

* :ref:`tsa_do_moves() <doxid-group__tapi__tcp__states__def_1gac34c52e94b29e6e361db65f33a82bad2>` can be used to move from the current TCP state to the desired one through specified intermediate states. This function takes variable number of arguments, and intermediary and final TCP states are specified as its last arguments. The final argument should be ``RPC_TCP_UNKNOWN``, it denotes end of the list of TCP states.

* :ref:`tsa_do_moves_str() <doxid-group__tapi__tcp__states__def_1ga2596e97a37da6fc2ee5458b2b97cceb1>` is similar to :ref:`tsa_do_moves() <doxid-group__tapi__tcp__states__def_1gac34c52e94b29e6e361db65f33a82bad2>`, but instead of variable number of arguments it takes string as it last argument, which can describe desired TCP states sequence like "TCP_CLOSE -> TCP_SYN_SENT -> TCP_ESTABLISHED -> TCP_FIN_WAIT1".

The following functions can be used to obtain the socket under test and its peer, in order to perform some actions on them when the desired TCP state is reached:

* :ref:`tsa_iut_sock() <doxid-group__tapi__tcp__states__def_1gae02d0584f6527a483f125d4492d66a84>` returns an IUT socket.

* :ref:`tsa_tst_sock() <doxid-group__tapi__tcp__states__def_1ga9a35603d9a7584742f8d4ff403fe9bcb>` returns either a real socket or a TCP socket emulation ID on Tester, according to the chosen mode of operation.

The following functions can be used to obtain information about the TCP state of a socket under test:

* :ref:`tsa_state_cur() <doxid-group__tapi__tcp__states__def_1gae8538e7f94052047074acf93a9367bb3>` returns current TCP state.

* :ref:`tsa_state_from() <doxid-group__tapi__tcp__states__def_1ga2631e22ff93907a97534f0ff174c92ca>` returns the latest state from which a transition to the next state was attempted.

* :ref:`tsa_state_to() <doxid-group__tapi__tcp__states__def_1ga7d23909a0cafc9916328a81330086a45>` returns the latest state to which a transition was attempted.





.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_conn:

Changing network connectivity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As a result of using the library, network connectivity can be broken in the following ways:

* If either ``TSA_TST_CSAP`` or ``TSA_TST_GW_CSAP`` modes are used, network connection from IUT to Tester will be broken with the help of an ARP table entry associating the Tester address with an alien MAC.

* If ``TSA_TST_SOCKET`` mode is used, then both IUT->Tester and Tester->IUT connections may be broken in the same way, depending on the current TCP state.

The following functions can be used to repair or break again the network connectivity by changing the ARP table:

* :ref:`tsa_repair_tst_iut_conn() <doxid-group__tapi__tcp__states__def_1ga4cf772c328f23d54931321d685862b2e>`

* :ref:`tsa_repair_iut_tst_conn() <doxid-group__tapi__tcp__states__def_1gac1361613e15b201566c03bff001983f9>`

* :ref:`tsa_break_tst_iut_conn() <doxid-group__tapi__tcp__states__def_1gaa88310d2d2f9a864dc643d3a9f830c56>`

* :ref:`tsa_break_iut_tst_conn() <doxid-group__tapi__tcp__states__def_1ga1f69d97355b1e725c62eb46fdbb731f8>`

After calling these functions CFG_WAIT_CHANGES macro should be used.





.. _doxid-group__tapi__tcp__states_1tapi_tcp_states_example:

Example
~~~~~~~

.. ref-code-block:: c

	#include "tapi_tcp_states.h"

	int
	main(int argc, char *argv[])
	{
	    rcf_rpc_server *pco_iut = NULL;
	    rcf_rpc_server *pco_tst = NULL;
	    rcf_rpc_server *pco_gw = NULL;

	    const struct sockaddr *iut_addr = NULL;
	    const struct sockaddr *tst_addr = NULL;
	    const struct sockaddr *gw_iut_addr = NULL;
	    const struct sockaddr *gw_tst_addr = NULL;
	    const void            *alien_link_addr = NULL;

	    const struct if_nameindex *tst_if = NULL;
	    const struct if_nameindex *iut_if = NULL;
	    const struct if_nameindex *gw_tst_if = NULL;
	    const struct if_nameindex *gw_iut_if = NULL;

	    tsa_session ss;

	    TEST_START;
	    TEST_GET_PCO(pco_iut);
	    TEST_GET_PCO(pco_tst);
	    TEST_GET_PCO(pco_gw);
	    TEST_GET_ADDR_NO_PORT(gw_iut_addr);
	    TEST_GET_ADDR_NO_PORT(gw_tst_addr);
	    TEST_GET_IF(gw_iut_if);
	    TEST_GET_IF(gw_tst_if);
	    TEST_GET_ADDR(pco_iut, iut_addr);
	    TEST_GET_ADDR(pco_tst, tst_addr);
	    TEST_GET_LINK_ADDR(alien_link_addr);
	    TEST_GET_IF(tst_if);
	    TEST_GET_IF(iut_if);

	    CHECK_RC(tsa_state_init(&ss, TSA_TST_GW_CSAP));

	    CHECK_RC(tsa_iut_set(&ss, pco_iut, iut_if, iut_addr));
	    CHECK_RC(tsa_tst_set(&ss, pco_tst, tst_if, tst_addr,
	                         ((struct sockaddr *)
	                          alien_link_addr)->sa_data));
	    CHECK_RC(tsa_gw_set(&ss, pco_gw, gw_iut_addr, gw_tst_addr,
	                        gw_iut_if, gw_tst_if,
	                        ((struct sockaddr *)alien_link_addr)->
	                                                   sa_data));
	    CFG_WAIT_CHANGES;

	    CHECK_RC(tsa_create_session(&ss, 0));

	    /*- Check that TCP_FIN_WAIT2 can be reached according
	     * to TCP states diagram. */

	    CHECK_RC(tsa_do_moves_str(&ss, RPC_TCP_UNKNOWN, RPC_TCP_UNKNOWN,
	                              0, "TCP_CLOSE->TCP_SYN_SENT->"
	                              "TCP_ESTABLISHED->TCP_FIN_WAIT1->"
	                              "TCP_FIN_WAIT2"));

	    TEST_SUCCESS;

	cleanup:

	    CLEANUP_CHECK_RC(tsa_destroy_session(&ss));

	    TEST_END;
	}

|	:ref:`TAPI definitions for testing TCP states<doxid-group__tapi__tcp__states__def>`


