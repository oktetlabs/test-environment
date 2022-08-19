..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Route Gateway TAPI
.. _doxid-group__route__gw:

Route Gateway TAPI
==================

.. toctree::
	:hidden:

"Route Gateway" TAPI allows to create testing configuration where two hosts, IUT and Tester, are connected not directly but through third gateway host forwarding packets between them. This configuration allows to break network connection in any direction between IUT and Tester, which is useful for testing what happens when reply to a packet is delayed.

This TAPI is defined in ``lib/tapi/tapi_route_gw.h``.



.. _doxid-group__route__gw_1route_gw_init:

Initialization and configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To store all variables related to route gateway configuration, :ref:`tapi_route_gateway <doxid-structtapi__route__gateway>` structure is defined. Declare variable of this type in your test to use this TAPI.

Macro TAPI_DECLARE_ROUTE_GATEWAY_PARAMS can be used to declare all test parameters related to route gateway configuration, such as RPC servers, network interfaces and addresses. It should be used together with macro TAPI_GET_ROUTE_GATEWAY_PARAMS which allows to get all these parameters from environment. Also macro TAPI_INIT_ROUTE_GATEWAY is declared which can be used to initialize route gateway variable with values obtained by TAPI_GET_ROUTE_GATEWAY_PARAMS.

After initializing route gateway, use :ref:`tapi_route_gateway_configure() <doxid-group__ts__tapi__route__gw_1ga777597392d0e6b782559c646b1b601ae>` to configure forwarding through gateway host. It creates two routes, from IUT to Tester on IUT and from Tester to IUT on Tester, and enables IPv4 forwarding on gateway host.

.. ref-code-block:: cpp

	TAPI_DECLARE_ROUTE_GATEWAY_PARAMS;

	tapi_route_gateway gw;

	TEST_START;
	TAPI_GET_ROUTE_GATEWAY_PARAMS;

	TAPI_INIT_ROUTE_GATEWAY(gw);

	CHECK_RC(tapi_route_gateway_configure(&gw));

These marcos can be used only if your environment defines parameters with names hardcoded in these macros (for example RPC server on IUT is named pco_iut). If this is not true, you should instead declare and obtain all test parameters manually and then use :ref:`tapi_route_gateway_init() <doxid-group__ts__tapi__route__gw_1ga426f99cafa42a0c0a1aa0ac37fdd769c>` instead of TAPI_INIT_ROUTE_GATEWAY to initialize route gateway.





.. _doxid-group__route__gw_1route_gw_usage:

Breaking and repairing network connection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Route Gateway TAPI allows to break network connection with the following methods:

* :ref:`tapi_route_gateway_set_forwarding() <doxid-group__ts__tapi__route__gw_1ga53c749056c8609e85d8efe699db131ce>` allows to disable forwarding on gateway, breaking connection in both directions.

* :ref:`tapi_route_gateway_break_gw_iut() <doxid-group__ts__tapi__route__gw_1ga2a0ec27813846514f565805f44539a30>` breaks connection gateway -> IUT.

* :ref:`tapi_route_gateway_break_iut_gw() <doxid-group__ts__tapi__route__gw_1ga5f863da0a72cfbf616941a87d70677a3>` breaks connection IUT -> gateway.

* :ref:`tapi_route_gateway_break_gw_tst() <doxid-group__ts__tapi__route__gw_1ga0f87a7796970d8b8033cc910456d9488>` breaks connection gateway -> Tester.

* :ref:`tapi_route_gateway_break_tst_gw() <doxid-group__ts__tapi__route__gw_1ga3a7ef4de8a787d4246574b889728d879>` breaks connection Tester -> gateway.

Broken network connection then can be repaired by using corresponding repair method:

* :ref:`tapi_route_gateway_set_forwarding() <doxid-group__ts__tapi__route__gw_1ga53c749056c8609e85d8efe699db131ce>` with enabled=TRUE.

* :ref:`tapi_route_gateway_repair_gw_iut() <doxid-group__ts__tapi__route__gw_1ga85e6d5265adf3479f7fca727deab7b8a>`.

* :ref:`tapi_route_gateway_repair_iut_gw() <doxid-group__ts__tapi__route__gw_1gaa0b1ff414b349a0788573e217ed54ae2>`.

* :ref:`tapi_route_gateway_repair_gw_tst() <doxid-group__ts__tapi__route__gw_1ga2df28ba9498f94670450f4e7234e7a85>`.

* :ref:`tapi_route_gateway_repair_tst_gw() <doxid-group__ts__tapi__route__gw_1ga7a815dfc1cfefb74d5b7785b1a286f79>`.





.. _doxid-group__route__gw_1route_gw_destroy:

Destroying route gateway configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Currently there is no method to destroy route gateway configuration. It is supposed that Configurator will do it in cleanup; to prevent test fail due to changed configuration, use track_conf attribute in package.xml:

.. ref-code-block:: cpp

	<script name="derived_epoll" track_conf="silent"/>





.. _doxid-group__route__gw_1route_gw_example:

Example
~~~~~~~

.. ref-code-block:: cpp

	int
	main(int argc, char *argv[])
	{
	    TAPI_DECLARE_ROUTE_GATEWAY_PARAMS;

	    int iut_s = -1;
	    int tst_s_aux = -1;
	    int tst_s = -1;

	    tapi_route_gateway gw;

	    TEST_START;
	    TAPI_GET_ROUTE_GATEWAY_PARAMS;

	    TAPI_INIT_ROUTE_GATEWAY(gw);

	    CHECK_RC(tapi_route_gateway_configure(&gw));
	    CFG_WAIT_CHANGES;

	    iut_s = rpc_socket(pco_iut,
	                       rpc_socket_domain_by_addr(iut_addr),
	                       RPC_SOCK_STREAM, RPC_PROTO_DEF);
	    rpc_bind(pco_iut, iut_s, iut_addr);

	    tst_s_aux = rpc_socket(pco_tst,
	                           rpc_socket_domain_by_addr(tst_addr),
	                           RPC_SOCK_STREAM, RPC_PROTO_DEF);
	    rpc_bind(pco_tst, tst_s_aux, tst_addr);
	    rpc_listen(pco_tst, tst_s_aux, 1);

	    /*
	     * Break connection Gateway -> IUT, so that replies
	     * from Tester does not reach IUT.
	     */
	    CHECK_RC(tapi_route_gateway_break_gw_iut(&gw));
	    CFG_WAIT_CHANGES;

	    /* Call nonblocking connect() from IUT. */
	    rpc_fcntl(pco_iut, iut_s, RPC_F_SETFL, RPC_O_NONBLOCK);
	    rpc_connect(pco_iut, iut_s, tst_addr);

	    /*
	     * Now iut_s should be in SYN_SENT state because
	     * SYN-ACK from Tester cannot be received. Here some
	     * actions can be made on a socket in this state.
	     * After that you can repair connection and finish
	     * TCP connection establishment if you need it.
	     */

	    /* Repair connection Gateway -> IUT. */
	    CHECK_RC(tapi_route_gateway_repair_gw_iut(&gw));
	    CFG_WAIT_CHANGES;

	    /* Wait until TCP connection is established. */
	    tst_s = rpc_accept(pco_tst, tst_s_aux, NULL, NULL);

	    TEST_SUCCESS;

	cleanup:

	    CLEANUP_RPC_CLOSE(pco_iut, iut_s);
	    CLEANUP_RPC_CLOSE(pco_tst, tst_s_aux);
	    CLEANUP_RPC_CLOSE(pco_tst, tst_s);

	    TEST_END;
	}

