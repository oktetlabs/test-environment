..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Network Traffic TAPI
.. _doxid-group__net__traffic:

Network Traffic TAPI
====================

.. toctree::
	:hidden:

	group_csap.rst
	group_route_gw.rst
	group_tapi_tcp_states.rst
	group_tcp_sock_emulation.rst

This page contains overview of TAPI for network traffic manipulation.



.. _doxid-group__net__traffic_1net_traffic_csap:

Communication Service Access Point (CSAP)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TE has TAPI for capturing and sending various network packets. The basic object used to perform these tasks is called CSAP (Communication Service Access Point). This object can be created on a Test Agent to send/receive some data (it may be used not only for network packets but also for CLI commands for example).





.. _doxid-group__net__traffic_1net_traffic_tcp_emulation:

TCP socket emulation
~~~~~~~~~~~~~~~~~~~~

TCP socket emulation is based on CSAP. It gives full control over TCP behavior, which is useful when we want to test how malformed packets are processed, whether all TCP states are achievable, etc.





.. _doxid-group__net__traffic_1net_traffic_route_gw:

Route Gateway
~~~~~~~~~~~~~

"Route Gateway" TAPI allows to create testing configuration where two hosts, IUT and Tester, are connected not directly but through third gateway host forwarding packets between them. TAPI has methods for breaking network connection from IUT to Tester and from Tester to IUT, which is useful when we want to check what happens when reply to a packet is delayed, or when connection is broken after establishment.

|	:ref:`CSAP TAPI<doxid-group__csap>`
|	:ref:`Route Gateway TAPI<doxid-group__route__gw>`
|	:ref:`TAPI for testing TCP states<doxid-group__tapi__tcp__states>`
|		:ref:`TAPI definitions for testing TCP states<doxid-group__tapi__tcp__states__def>`
|	:ref:`TCP socket emulation<doxid-group__tcp__sock__emulation>`


