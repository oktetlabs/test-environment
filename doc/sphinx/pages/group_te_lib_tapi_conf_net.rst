..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Network topology configuration of Test Agents
.. _doxid-group__te__lib__tapi__conf__net:

Network topology configuration of Test Agents
=============================================

.. toctree::
	:hidden:



.. _doxid-group__te__lib__tapi__conf__net_1te_lib_tapi_conf_net_introduction:

Introduction
~~~~~~~~~~~~

In most cases a test scenario requires a special phisical set-up to be prepared. The simplest example is when a test expects two nodes to have a physical connection. From test point of view it does not matter where these nodes reside - the same test should work on different physical set-ups.

Partly physical set-up for a test run is configured via :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` where we specify te_agents to run together with their physical locations (network hosts).

Each host can have a number of network interfaces some of which can be connected with interfaces of another hosts mentioned in :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>`. If we specify relation between network interfaces we would clarify network topology, not just a list of hosts used in test procedure.

.. image:: /static/image/te_lib_tapi_net_cfg_rcf_only.png
	:alt: Network topology from RCF point of view

From :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` point of view all hosts where we run te_agents should be accessible from a host where we run :ref:`Test Engine <doxid-group__te__engine>`. It is not necessary to be a direct link access - for example we can run :ref:`Test Engine <doxid-group__te__engine>` on our local PC, but te_agents can reside on remote hosts reside in another countries accessed via the Internet.

To specify network topology of hosts used in test set-up we need to specify relationship between network interfaces.

.. image:: /static/image/te_lib_tapi_net_cfg_nets.png
	:alt: Network topology between Test Agent hosts

Later the information about network interfaces can be used by test scenario to prepare necessary test set-up.





.. _doxid-group__te__lib__tapi__conf__net_1te_lib_tapi_conf_net_basic_cfg_tree:

Basic configuration tree nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

From test scenarios point of view network configuration can be analized with the help of functions exported by :ref:`Network topology configuration of Test Agents <doxid-group__tapi__conf__net>` library.

The library gets information from local configurator management tree that shall be prepared by hands for each physical set-up.

Configuration nodes responsible for :ref:`Network topology configuration of Test Agents <doxid-group__tapi__conf__net>` are:

* /net - root object of network configuration tree. Instance name represents network name - an arbitrary string that is associated with a group of nodes belonging to the network.

* /net/node - network node object. Represents a point of network configuration that reside in corresponding network. Instance name is an arbitrary string, but the value should keep a reference to configuration tree node associated with this network point.

  More often the value has the following format:

  .. code-block:: none


    /agent:<agent name>="">/interface:<interface name>="">

  E.g.: /agent:Agt_A/interface:eth0, which means a nodes is associated with interface eth0 that reside on the host where Test Agent Agt_A runs (to find out host name where Test Agent runs we should have a look at :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>`). You can extend the format of /net/node value if necessary, for example for the case when a node is assocated with a bridge port the value would be:

  .. code-block:: none


    /agent:<agent name>="">/port:<port id>="">

  E.g.: /agent:bridge/port:20.

* /net/node/type - type of the network node. Each node is assigned with its logical type value:

  * NUT - :ref:`Node <doxid-structNode>` Under Test - node of this type exports functionality that we are to test. In configuration tree the value of this node is of integer type and should be equal to 1;

  * TST - Tester :ref:`Node <doxid-structNode>` - node of this type provides auxiliary services to make it possible to do the testing of functionality exported by DUT or NUT. In configuration tree the value of this node is of integer type and should be equal to 0.

For our sample configuration we would add the following lines in :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>`

* Register necessary object:

  .. ref-code-block:: xml

    <register>
      <object oid="/net" access="read_create" type="none"/>
      <object oid="/net/node" access="read_create" type="string"/>
      <object oid="/net/node/type" access="read_write" type="integer"/>
    </register>

* Add set-up specific instances:

  .. ref-code-block:: xml

    <add>
      <instance oid="/net:net1"/>
      <instance oid="/net:net1/node:A"
                value="/agent:Agt_A/interface:eth1"/>
      <instance oid="/net:net1/node:B"
              value="/agent:Agt_C/interface:hme2"/>
    </add>
    <set>
      <instance oid="/net:net1/node:A/type:" value="0"/>
      <instance oid="/net:net1/node:B/type:" value="1"/>
    </set>

    <add>
      <instance oid="/net:net2"/>
      <instance oid="/net:net2/node:A"
                value="/agent:Agt_B/interface:eth2"/>
      <instance oid="/net:net2/node:B"
              value="/agent:Agt_C/interface:hme1"/>
    </add>
    <set>
      <instance oid="/net:net2/node:A/type:" value="0"/>
      <instance oid="/net:net2/node:B/type:" value="1"/>
    </set>

Please note that in our sample we have two te_agents running on host h1. Depending on our design and the type of te_agents we can use particular combinations of (Test Agent, interface name) pairs (for example Agt_B may be a dedicated Test Agent that does not support interface configuration, i.e. it is not expected to have instances of /agent/interface object).

Assuming we have only Agt_A that exports instances of /agent/interface configuration tree we would have:

.. ref-code-block:: xml

	<add>
	  <instance oid="/net:net1"/>
	  <instance oid="/net:net1/node:A"
	            value="/agent:Agt_A/interface:eth1"/>
	  <instance oid="/net:net1/node:B"
	          value="/agent:Agt_C/interface:hme2"/>
	</add>
	<set>
	  <instance oid="/net:net1/node:A/type:" value="0"/>
	  <instance oid="/net:net1/node:B/type:" value="1"/>
	</set>

	<add>
	  <instance oid="/net:net2"/>
	  <instance oid="/net:net2/node:A"
	            value="/agent:Agt_A/interface:eth2"/>
	  <instance oid="/net:net2/node:B"
	          value="/agent:Agt_C/interface:hme1"/>
	</add>
	<set>
	  <instance oid="/net:net2/node:A/type:" value="0"/>
	  <instance oid="/net:net2/node:B/type:" value="1"/>
	</set>





.. _doxid-group__te__lib__tapi__conf__net_1te_lib_tapi_conf_net_addr_cfg_tree:

Network addresses configuration tree nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Apart from basic network topology configuration we can specify pools of network addresses that can be used for address asignment on network nodes (network interfaces specified as the value of instances of /net/node object).

First group of nodes relates to the pool of networks:

* /net_pool - root object of network address pools. Instance name is expected to be either ip4 or ip6 depending on the type of test subnetworks kept in the pool;

* /net_pool/entry - pool entry that specifies a subnetwork. Instance name is expected to be a valid IPv4 or IPv6 address depending on the name of /net_pool instance;

* /net_pool/entry/prefix - prefix length for the address specified as instance name of /net_pool/entry object.

* /net_pool/entry/pool - a node to keep track of allocated network addresses;

* /net_pool/entry/pool/entry - network address entry allocated from the pool /net_pool/entry.

Another group of nodes is an extension of /net objects that save information about assigned network addresses:

* /net/ip4_subnet - track IPv4 subnets whose addresses are asssigned to nodes of this /net;

* /net/ip6_subnet - the same as /net/ip4_subnet, but for IPv6 addresses;

* /net/node/ip4_address - track of IPv4 addresses assigned to the particular network node;

* /net/node/ip6_address - the same as /net/node/ip4_address, but for IPv6 addresses.

Please note that the only nodes we should take care of are:

* /net_pool;

* /net_pool/entry;

* /net_pool/entry/prefix.

All the other nodes are tracked by :ref:`Network topology configuration of Test Agents <doxid-group__tapi__conf__net>` library internally.

To specify pools of networks we would write something like the following in our :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>`

.. ref-code-block:: xml

	<register>
	  <object oid="/net" access="read_create" type="none"/>
	  <object oid="/net/ip4_subnet" access="read_create" type="address"/>
	  <object oid="/net/ip6_subnet" access="read_create" type="address"/>
	  <object oid="/net/node" access="read_create" type="string"/>
	  <object oid="/net/node/type" access="read_write" type="integer"/>
	  <object oid="/net/node/ip4_address" access="read_create" type="address"/>
	  <object oid="/net/node/ip6_address" access="read_create" type="address"/>
	</register>

	<register>
	  <object oid="/net_pool" access="read_create" type="none"/>
	  <object oid="/net_pool/entry" access="read_create" type="integer"/>
	  <object oid="/net_pool/entry/prefix" access="read_write" type="integer"/>
	  <object oid="/net_pool/entry/n_entries"
	          access="read_write" type="integer"/>
	  <object oid="/net_pool/entry/pool" access="read_write" type="none"/>
	  <object oid="/net_pool/entry/pool/entry"
	          access="read_create" type="integer"/>
	</register>

	<add>
	  <instance oid="/net_pool:ip4"/>
	  <instance oid="/net_pool:ip6"/>
	</add>

	<add>
	  <instance oid="/net_pool:ip4/entry:10.38.10.0" value="0"/>
	  <instance oid="/net_pool:ip4/entry:10.38.11.0" value="0"/>
	</add>
	<set>
	  <instance oid="/net_pool:ip4/entry:10.38.10.0/prefix:" value="24"/>
	  <instance oid="/net_pool:ip4/entry:10.38.11.0/prefix:" value="24"/>
	</set>

	<add>
	  <instance oid="/net_pool:ip6/entry:fec0:0:0::" value="0"/>
	  <instance oid="/net_pool:ip6/entry:fec0:0:1::" value="0"/>
	</add>
	<set>
	  <instance oid="/net_pool:ip6/entry:fec0:0:0::/prefix:" value="48"/>
	  <instance oid="/net_pool:ip6/entry:fec0:0:1::/prefix:" value="48"/>
	</set>

To manipulate network topology and pools of subnetworks use:

* :ref:`Network topology configuration of Test Agents <doxid-group__tapi__conf__net>`;

* :ref:`Manipulation of network address pools <doxid-group__tapi__conf__net__pool>`.

