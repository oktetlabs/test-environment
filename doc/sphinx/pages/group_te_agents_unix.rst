..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Test Agents: Unix Test Agent
.. _doxid-group__te__agents__unix:

Test Agents: Unix Test Agent
============================

.. toctree::
	:hidden:



.. _doxid-group__te__agents__unix_1te_agent_unix_introduction:

Introduction
~~~~~~~~~~~~

The most functional and frequently used agent type is UNIX Agent. Even though it tends to be generic UNIX Agent, it is better adapted for Linux OS.

Sources of UNIX Test Agent can be found under agents/unix directory of TE tree.

Unix Test Egent supports the following features:

* RPC Server functionality;

* Process/thread creation feature;

* Rich configuration tree support:

* Daemons configuration (dhcp, dns, ldap, pppoe, radius, radvd, vtund, 802.1x supplicants);

* Sniffer support;

* Control of Power Switch devices;

* Traffic Application Domain (TAD) support for network protocols (ARP, ATM, DHCP, Ethernet, IGMP, UDP, TCP, IPv4, ICMPv4, IPv6, ICMPv6, PPP, SNMP, iSCSI);

* Periodically running specified commands and logging their output (te_cmd_monitor).





.. _doxid-group__te__agents__unix_1te_agent_unix_source:

Source organization
~~~~~~~~~~~~~~~~~~~

The sources of UNIX Test Agent located under agents/unix directory of TE tree:

* agents/unix/rpc - implementation of RPC Server calls.

  Please refer to :ref:`RPC Development Framework <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework>` for the details on how to add new RPC calls into :ref:`Test Agents <doxid-group__te__agents>`;

* agents/unix/main.c - implementation of entry point of Test Agent as well as functions of RCF PCH interface;

* agents/unix/conf implementation of configuration nodes for UNIX Test Agent:

  * agents/unix/conf/base - support of base configuration nodes (network interface configuration);

  * agents/unix/conf/daemons - support of network daemons configuration;

  * agents/unix/conf/route - support of network routing table configuration;

  * agents/unix/conf/util - support of configuration models of external utilities integrated in TE (sniffer);

  Please refer to :ref:`Test Agents: Creating new configuration nodes in Test Agent <doxid-group__te__agents__conf>` for the details on how to add new configuration nodes into :ref:`Test Agents <doxid-group__te__agents>`.

