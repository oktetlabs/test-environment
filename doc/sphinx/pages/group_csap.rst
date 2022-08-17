..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; CSAP TAPI
.. _doxid-group__csap:

CSAP TAPI
=========

.. toctree::
	:hidden:

Communication Service Access Point (CSAP) is an object which can be created on TA to send/receive some data (network packets, CLI commands, etc.). Most often it is used to sniff network packets or to send packets constructed in a special way (having some flags set, malformed, etc). :ref:`TCP socket emulation <doxid-group__tcp__sock__emulation>` is based on CSAP TAPI.



.. _doxid-group__csap_1csap_create:

Creating CSAP object
~~~~~~~~~~~~~~~~~~~~

CSAP is created on TA for specific protocol stack (such as TCP/IPv4/Ethernet). There is a generic function for creating CSAP, :ref:`tapi_tad_csap_create() <doxid-group__tapi__tad_1ga72802448118bc3ec04d5d77cc0556542>`. However more often helper functions defined for specific protocol stacks are used. For example, for TCP/IPv4/Ethernet there is :ref:`tapi_tcp_ip4_eth_csap_create() <doxid-group__tapi__tad__tcp_1ga4170174a26d6c4ae03c7745ff33be4bf>`. These helper functions can be found in header files under lib/tapi_tad/.





.. _doxid-group__csap_1csap_sniff:

Sniffing network traffic
~~~~~~~~~~~~~~~~~~~~~~~~

Once CSAP is created, you can start sniffing network packets matching its properties (protocols, addresses, ports, etc.) with :ref:`tapi_tad_trrecv_start() <doxid-group__tapi__tad_1gab222bbc26dac1ed366a9873608419ea7>`.

To process packets sniffed by CSAP, use either :ref:`tapi_tad_trrecv_stop() <doxid-group__tapi__tad_1ga85799ed356025b67deb86bed410ceadc>` (which stops sniffing) or :ref:`tapi_tad_trrecv_get() <doxid-group__tapi__tad_1ga5a0556cc873b92a43ac50ff55da29b8d>` (which gets already received packets without stopping sniffing for new ones). These functions report number of packets received; also they allow to specify callback which will be called for every obtained packet.

These two functions are just simple wrappers for :ref:`rcf_ta_trrecv_stop() <doxid-group__rcfapi__base_1ga5e2f63e40f7068b0f3c9b9eb83715403>` and :ref:`rcf_ta_trrecv_get() <doxid-group__rcfapi__base_1ga42a701d6f5aa4bc46b083a78d889e3fe>` which are often used directly.





.. _doxid-group__csap_1csap_send:

Sending packets
~~~~~~~~~~~~~~~

To send packets with CSAP, use :ref:`tapi_tad_trsend_start() <doxid-group__tapi__tad_1gac320e577a868bfb8bc3a2cfe3f1aca77>`. It can be called either in blocking (RCF_MODE_BLOCKING) or nonblocking (RCF_MODE_NONBLOCKING) mode, as defined by its **blk_mode** argument.

Packets specification should be defined in ASN value of type Taffic-Pattern passed to this function. Usually not all fields of the template should be filled; for example, if you specified local and remote addresses when creating CSAP, they will be filled automatically by CSAP for outgoing packets.

Note that you cannot send packets from CSAP if it is currently sniffing packets. You should either stop sniffing by :ref:`tapi_tad_trrecv_stop() <doxid-group__tapi__tad_1ga85799ed356025b67deb86bed410ceadc>` or create a separate CSAP for sending.





.. _doxid-group__csap_1csap_destroy:

Destroying CSAP object
~~~~~~~~~~~~~~~~~~~~~~

To destroy CSAP object, use :ref:`tapi_tad_csap_destroy() <doxid-group__tapi__tad_1ga6540a89806d0460d06b778a5018d48c1>`.





.. _doxid-group__csap_1csap_example:

Example
~~~~~~~

This is an example of using CSAP to receive and send TCP/IPv4/Ethernet packets.

.. ref-code-block:: cpp

	#include "te_defs.h"
	#include "asn_usr.h"
	#include "tapi_tad.h"
	#include "tapi_tcp.h"

	#include "ndn.h"
	#include "ndn_ipstack.h"

	static tapi_tcp_pos_t last_ackn_got = 0;
	static tapi_tcp_pos_t last_seqn_got = 0;

	static te_bool fin_received = FALSE;

	static void
	user_pkt_handler(asn_value *packet, void *user_data)
	{
	    UNUSED(user_data);

	    int32_t pdu_field;

	    asn_value   *val1 = NULL;
	    asn_value   *val2 = NULL;
	    asn_value   *tcp_pdu = NULL;

	    CHECK_RC(asn_get_child_value(packet, (const asn_value **)&val1,
	                                 PRIVATE, NDN_PKT_PDUS));
	    CHECK_RC(asn_get_indexed(val1, &val2, 0, NULL));
	    CHECK_RC(asn_get_choice_value(val2, &tcp_pdu, NULL, NULL));
	    CHECK_RC(ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_FLAGS, &pdu_field));

	    if (pdu_field & TCP_FIN_FLAG)
	        fin_received = TRUE;

	    if (pdu_field & TCP_ACK_FLAG)
	    {
	        CHECK_RC(ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_ACKN,
	                                       &pdu_field));
	        last_ackn_got = pdu_field;
	    }

	    CHECK_RC(ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_SEQN, &pdu_field));
	    last_seqn_got = pdu_field;
	}

	int
	main(int argc, char *argv[])
	{
	    rcf_rpc_server *pco_iut = NULL;
	    rcf_rpc_server *pco_tst = NULL;

	    const struct sockaddr *iut_addr = NULL;
	    const struct sockaddr *tst_addr = NULL;
	    const struct sockaddr *iut_lladdr = NULL;
	    const struct sockaddr *tst_lladdr = NULL;

	    const struct if_nameindex *tst_if = NULL;

	    int             iut_s = -1;
	    int             tst_s = -1;

	    csap_handle_t   csap = CFG_HANDLE_INVALID;

	    tapi_tad_trrecv_cb_data cb_data;

	    struct rpc_tcp_info   tcp_info;

	    asn_value *rst_template = NULL;

	    TEST_START;
	    TEST_GET_PCO(pco_iut);
	    TEST_GET_PCO(pco_tst);
	    TEST_GET_IF(tst_if);
	    TEST_GET_ADDR(pco_iut, iut_addr);
	    TEST_GET_ADDR(pco_tst, tst_addr);
	    TEST_GET_LINK_ADDR(iut_lladdr);
	    TEST_GET_LINK_ADDR(tst_lladdr);

	    /* Create CSAP. */

	    CHECK_RC(tapi_tcp_ip4_eth_csap_create(pco_tst->ta, 0, tst_if->if_name,
	                                          TAD_ETH_RECV_DEF |
	                                          TAD_ETH_RECV_NO_PROMISC,
	                                          (const uint8_t *)tst_lladdr->sa_data,
	                                          (const uint8_t *)iut_lladdr->sa_data,
	                                          SIN(tst_addr)->sin_addr.s_addr,
	                                          SIN(iut_addr)->sin_addr.s_addr,
	                                          SIN(tst_addr)->sin_port,
	                                          SIN(iut_addr)->sin_port, &csap));


	    /* Start sniffing packets with CSAP. */
	    CHECK_RC(tapi_tad_trrecv_start(pco_tst->ta, 0, csap, NULL,
	                                   TAD_TIMEOUT_INF, -1,
	                                   RCF_TRRECV_PACKETS));

	    /* Establish TCP connection. */
	    GEN_CONNECTION(pco_iut, pco_tst, RPC_SOCK_STREAM, RPC_PROTO_DEF,
	                   iut_addr, tst_addr, &iut_s, &tst_s);

	    /* Call shutdown(WR) on IUT socket to send FIN packet. */
	    rpc_shutdown(pco_iut, iut_s, RPC_SHUT_WR);
	    TAPI_WAIT_NETWORK;

	    /*
	     * Stop sniffing packets, examine what was received
	     * by CSAP.
	     */

	    cb_data.callback = user_pkt_handler;
	    cb_data.user_data = NULL;
	    CHECK_RC(tapi_tad_trrecv_stop(
	                      pco_tst->ta, 0, csap,
	                      &cb_data, NULL));
	    if (!fin_received)
	        TEST_VERDICT("FIN packet was not received after calling "
	                     "shutdown(WR)");

	    /* Create template of TCP packet with RST flag set. */

	    CHECK_RC(tapi_tcp_template(last_ackn_got, last_seqn_got,
	                               FALSE, TRUE, NULL, 0,
	                               &rst_template));
	    CHECK_RC(asn_write_int32(rst_template, TCP_ACK_FLAG | TCP_RST_FLAG,
	                             "pdus.0.#tcp.flags.#plain"));

	    /* Send the packet. */

	    CHECK_RC(tapi_tad_trsend_start(pco_tst->ta, 0, csap,
	                                   rst_template, RCF_MODE_BLOCKING));

	    TAPI_WAIT_NETWORK;

	    rpc_getsockopt(pco_iut, iut_s, RPC_TCP_INFO, &tcp_info);
	    if (tcp_info.tcpi_state != RPC_TCP_CLOSE)
	        TEST_VERDICT("Socket is in %s state instead of TCP_CLOSE",
	                     tcp_state_rpc2str(tcp_info.tcpi_state));

	    TEST_SUCCESS;

	cleanup:

	    if (csap != CFG_HANDLE_INVALID)
	        CLEANUP_CHECK_RC(tapi_tad_csap_destroy(pco_tst->ta, 0, csap));

	    CLEANUP_RPC_CLOSE(pco_iut, iut_s);
	    CLEANUP_RPC_CLOSE(pco_tst, tst_s);

	    TEST_END;
	}

