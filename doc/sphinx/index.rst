..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Test Environment
.. _doxid-group__te:

Test Environment
================

.. toctree::
	:hidden:

    pages/group_terminology
    pages/group_te_ts
    pages/group_te_user
    pages/group_te_engine
    pages/group_te_agents
    pages/group_te_tools
    pages/selftest
    pages/add_doc
    generated/global

.. _doxid-group__te_1te_introduction:

Introduction
~~~~~~~~~~~~

OKTET Labs Test Environment (TE) is a software product that is intended to ease creating automated test suites. The history of TE goes back to 2000 year when the first prototype of software was created. At that time the product was used for testing SNMP MIBs and CLI commands. Two years later (in 2002) software was extended to support testing of IPv6 protocol.

Few years of intensive usage the software in testing projects showed that a deep re-design was necessary to make the architecture flexible and extandable for new and new upcoming features. In 2003 year it was decided that the redesign should be fulfilled. Due to the careful and well-thought design decisions made in 2003 year, the overall TE architecture (main components and interconnections between them) are still valid even though a lot of new features has been added since then.





.. _doxid-group__te_1te_conventions:

Conventions
~~~~~~~~~~~

Throught the documentation the following conventions are used:

* directory paths or file names marked as: ``/usr/bin/bash``, ``Makefile``;

* program names (scripts or binary executables) marked as: ``gdb``, ``dispatcher.sh``;

* program options are maked as: ``help``, ``-q``;

* environment variables are marked as: ``TE_BASE``, ``${PATH}``;

* different directives in configuration files are marked as: ``TE_LIB_PARMS``, ``include``;

* configuration pathes as maked as: ``/agent/interface``, ``/local:/net:A``;

* names of different attributes (mainly names of XML element attributes) are marked as: ``type``, ``src``;

* values of different attributes are marked as: ``unix``, ``${TE_BASE}/suites/ipv6-demo``;

* different kind of commands or modes are marked as: ``Unregister``, ``live``.





.. _doxid-group__te_1te_abrev_term:

Abbreviations and Terminology
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :ref:`Terminology <doxid-group__terminology>` for the abbreviations and terminology.





.. _doxid-group__te_1te_architecture:

Test Environment Architecture
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

From the high level point of view TE consists of two main parts:

* :ref:`Test Engine <doxid-group__te__engine>`;

* :ref:`Test Agents <doxid-group__te__agents>`.

The following picture shows high level decomposition of TE components:

.. image:: /static/image/hl-decomposition.png
	:alt: High Level Decomposition of Test Environment components

There are several kinds of Test Agents (see the diagram below):

* Test Agent running on an auxiliary station and controlling this station: Test Protocol is used over a network and all commands are applied to the station on which Test Agent runs.

* Test Agent running on an auxiliary station and controlling the NUT: :ref:`Test Protocol <doxid-group__te__engine__rcf_1te_engine_rcf_test_proto>` is used over a network and all commands are applied to the NUT (Test Agent interacts with the NUT via a NUT-specific interface).

* Test Agent running on a NUT and controlling the NUT: Test Protocol is used over a network or over some other transport (for example, over a serial port or Ethernet) and all commands are applied to the NUT.

Seletion of appropriate usage scenario depends on the object being tested. If it's a network device the Test Agent can run either on the device itself or (if black box testing is performed or device hardware is not capable of running the agent) on an auxiliary station. If a network library is tested then the agent should probably be running on the NUT itself (in such case NUT is the host on which the service that uses the library is running).

Apart from these three kinds of agents (or should they be called 'Test Agent usage scenarios') there can exist arbitrary number of agents running on other equipments which participates in the testing process. Examples include:

* other hosts which are used for traffic generation;

* proxy agents which are used to control various sevices used for network envrionment creation or DUT configuration via managemnt protocols;

* agents used for debug information collection, for instance serial logs.

The below diagram shows various scenarios and Test Agents locations.

.. image:: /static/image/te_agent_types.png
	:alt: Stations and applications participating in the testing process

TE subsystems and :ref:`Test Agents <doxid-group__te__agents>` are distributed among several applications running on different stations. Apart from other things :ref:`Test Engine <doxid-group__te__engine>` controls :ref:`Test Agents <doxid-group__te__agents>` by means of so-called :ref:`Test Protocol <doxid-group__te__engine__rcf_1te_engine_rcf_test_proto>`

* a text-based protocol used in communication between :ref:`Test Engine <doxid-group__te__engine>` and :ref:`Test Agents <doxid-group__te__agents>`.

According to initial design :ref:`Test Engine <doxid-group__te__engine>` runs on Linux platform and there is no intend to port it to any other OS type, but :ref:`Test Agents <doxid-group__te__agents>` can be run on different platforms providing particular set of mandatory features for :ref:`Test Engine <doxid-group__te__engine>`. :ref:`Test Agents <doxid-group__te__agents>` are the subject to be ported to hardware/software specific components of test infrastructure.





.. _doxid-group__te_1te_tools:

Test Environment Tools
~~~~~~~~~~~~~~~~~~~~~~

Apart from main components, TE provides the following set of tools:

* :ref:`Report Generator Tool <doxid-group__rgt>` representing logs in different formats;

* :ref:`Test Results Comparator <doxid-group__trc>` utility that can be used to check the difference between different test runs (TRC tool);

* :ref:`Packet Sniffer <doxid-group__sniffer>` to be used for traffic capture on the test agents;

* **Testing Coverage Estimation** tool that gives some report about the quality of test suite we run against the software under test;

* **Test Package Generator** tool.

|	:ref:`Common tools API<doxid-group__te__tools>`
|		:ref:`Call shell commands<doxid-group__te__tools__te__shell__cmd>`
|		:ref:`Date, time<doxid-group__te__tools__te__time>`
|		:ref:`Dymanic array<doxid-group__te__tools__te__vec>`
|		:ref:`Dynamic buffers<doxid-group__te__tools__te__dbuf>`
|		:ref:`Dynamic strings<doxid-group__te__tools__te__string>`
|		:ref:`Execute a program in a child process<doxid-group__te__tools__te__exec__child>`
|		:ref:`File operations<doxid-group__te__tools__te__file>`
|		:ref:`Format string parsing<doxid-group__te__tools__te__format>`
|		:ref:`IP stack headers<doxid-group__te__tools__te__ipstack>`
|		:ref:`Key-value pairs<doxid-group__te__tools__te__kvpair>`
|		:ref:`Log buffers<doxid-group__te__tools__log__bufs>`
|		:ref:`Log format string processing<doxid-group__te__tools__te__log__fmt>`
|		:ref:`Logger subsystem<doxid-group__te__tools__logger__file>`
|		:ref:`Machine interface data logging<doxid-group__te__tools__te__mi__log>`
|			:ref:`MI measurement-specific declarations<doxid-group__te__tools__te__mi__log__meas>`
|		:ref:`Mapping of unix signal names and numbers<doxid-group__te__tools__te__sigmap>`
|		:ref:`Regular binary buffers<doxid-group__te__tools__te__bufs>`
|		:ref:`Regular strings<doxid-group__te__tools__te__str>`
|		:ref:`Safe memory allocation<doxid-group__te__tools__te__alloc>`
|		:ref:`Serial console<doxid-group__te__tools__te__serial>`
|		:ref:`Serial console common tools<doxid-group__te__tools__te__serial__common>`
|		:ref:`Serial console parser<doxid-group__te__tools__te__serial__parser>`
|		:ref:`Sleep<doxid-group__te__tools__te__sleep>`
|		:ref:`Sockaddr<doxid-group__te__tools__te__sockaddr>`
|		:ref:`Tail queue of strings<doxid-group__te__tools__tq__string>`
|		:ref:`Toeplitz hash<doxid-group__te__tools__te__toeplitz>`
|		:ref:`Unit-conversion<doxid-group__te__tools__te__units>`
|		:ref:`Unix Kernel Logger<doxid-group__te__tools__te__kernel__log>`
|		:ref:`iSCSI<doxid-group__te__tools__te__iscsi>`
|	:ref:`TE: User Guide<doxid-group__te__user>`
|		:ref:`Network namespaces<doxid-group__te__netns>`
|		:ref:`TE Config Files Basics<doxid-group__te__user__run__details>`
|	:ref:`Terminology<doxid-group__terminology>`
|	:ref:`Test Agents<doxid-group__te__agents>`
|		:ref:`Test Agents: Command Handler<doxid-group__rcf__ch>`
|			:ref:`Command Handler: Configuration support<doxid-group__rcf__ch__cfg>`
|				:ref:`Configuration node definition macros<doxid-group__rcf__ch__cfg__node__def>`
|			:ref:`Command Handler: File maniputation support<doxid-group__rcf__ch__file>`
|			:ref:`Command Handler: Function call support<doxid-group__rcf__ch__func>`
|			:ref:`Command Handler: Process/thread support<doxid-group__rcf__ch__proc>`
|			:ref:`Command Handler: Reboot and shutdown support<doxid-group__rcf__ch__reboot>`
|			:ref:`Command Handler: Symbol name and address resolver support<doxid-group__rcf__ch__addr>`
|			:ref:`Command Handler: Traffic Application Domain (TAD) support<doxid-group__rcf__ch__tad>`
|			:ref:`Command Handler: Variables support<doxid-group__rcf__ch__var>`
|		:ref:`Test Agents: Creating new configuration nodes in Test Agent<doxid-group__te__agents__conf>`
|		:ref:`Test Agents: Portable Commands Handler<doxid-group__rcf__pch>`
|			:ref:`API: Shared TA resources<doxid-group__rcf__pch__rsrc>`
|		:ref:`Test Agents: Unix Test Agent<doxid-group__te__agents__unix>`
|		:ref:`Test Agents: Windows Test Agent<doxid-group__te__agents__win>`
|	:ref:`Test Engine<doxid-group__te__engine>`
|		:ref:`Builder<doxid-group__te__engine__builder>`
|		:ref:`Configurator<doxid-group__te__engine__conf>`
|			:ref:`API Usage: Configurator API<doxid-group__confapi>`
|				:ref:`API: Configurator<doxid-group__confapi__base>`
|					:ref:`Configuration backup manipulation<doxid-group__confapi__base__backup>`
|					:ref:`Configuration tree traversal<doxid-group__confapi__base__traverse>`
|					:ref:`Contriguration tree access operations<doxid-group__confapi__base__access>`
|					:ref:`Synchronization configuration tree with Test Agent<doxid-group__confapi__base__sync>`
|					:ref:`Test Agent reboot<doxid-group__confapi__base__reboot>`
|				:ref:`TAPI: Test API for configuration nodes<doxid-group__tapi__conf>`
|					:ref:`ARL table configuration<doxid-group__tapi__conf__arl>`
|					:ref:`Agent namespaces configuration<doxid-group__tapi__namespaces>`
|					:ref:`Agents, namespaces and interfaces relations<doxid-group__tapi__host__ns>`
|					:ref:`Bonding and bridging configuration<doxid-group__tapi__conf__aggr>`
|					:ref:`CPU topology configuration of Test Agents<doxid-group__tapi__conf__cpu>`
|					:ref:`Command monitor TAPI<doxid-group__tapi__cmd__monitor__def>`
|					:ref:`DHCP Server configuration<doxid-group__tapi__conf__dhcp__serv>`
|					:ref:`DUT serial console access<doxid-group__tapi__conf__serial>`
|					:ref:`Environment variables configuration<doxid-group__tapi__conf__sh__env>`
|					:ref:`Ethernet PHY configuration<doxid-group__tapi__conf__phy>`
|					:ref:`Ethernet interface features configuration<doxid-group__tapi__conf__eth>`
|					:ref:`IP rules configuration<doxid-group__tapi__conf__ip__rule>`
|					:ref:`IPv6 specific configuration<doxid-group__tapi__conf__ip6>`
|					:ref:`Kernel modules configuration<doxid-group__tapi__conf__modules>`
|					:ref:`L2TP configuration<doxid-group__tapi__conf__l2tp>`
|					:ref:`Local subtree access<doxid-group__tapi__conf__local>`
|					:ref:`NTP daemon configuration<doxid-group__tapi__conf__ntpd>`
|					:ref:`Network Base configuration<doxid-group__tapi__conf__base__net>`
|					:ref:`Network Emulator configuration<doxid-group__tapi__conf__netem>`
|					:ref:`Network Switch configuration<doxid-group__tapi__conf__switch>`
|					:ref:`Network sniffers configuration<doxid-group__tapi__conf__sniffer>`
|					:ref:`Network statistics access<doxid-group__tapi__conf__stats>`
|					:ref:`Network topology configuration of Test Agents<doxid-group__tapi__conf__net>`
|					:ref:`PPPoE Server configuration<doxid-group__tapi__conf__pppoe>`
|					:ref:`Queuing Discipline configuration<doxid-group__tapi__conf__qdisc>`
|					:ref:`Routing Table configuration<doxid-group__tapi__conf__route>`
|						:ref:`Manipulation of network address pools<doxid-group__tapi__conf__net__pool>`
|						:ref:`Neighbour table configuration<doxid-group__tapi__conf__neigh>`
|						:ref:`Network Interface configuration<doxid-group__tapi__conf__iface>`
|					:ref:`Serial console parsers configuration<doxid-group__tapi__conf__serial__parse>`
|					:ref:`Solarflare PTP daemon configuration<doxid-group__tapi__conf__sfptpd>`
|					:ref:`System parameters configuration<doxid-group__tapi__cfg__sys>`
|					:ref:`TA system options configuration (OBSOLETE)<doxid-group__tapi__conf__proc>`
|					:ref:`Test API to handle a cache<doxid-group__tapi__cache>`
|					:ref:`VTund configuration<doxid-group__tapi__conf__vtund>`
|					:ref:`Virtual machines configuration<doxid-group__tapi__conf__vm>`
|					:ref:`XEN configuration<doxid-group__tapi__conf__xen>`
|					:ref:`iptables configuration<doxid-group__tapi__conf__iptable>`
|		:ref:`Dispatcher<doxid-group__te__engine__dispatcher>`
|		:ref:`Kernel Logging<doxid-group__kernel__log>`
|			:ref:`Console Log Level Configuration<doxid-group__console__ll>`
|			:ref:`Packet Serial Parser<doxid-group__serial>`
|		:ref:`Logger<doxid-group__te__engine__logger>`
|			:ref:`API: Logger<doxid-group__logger__api>`
|			:ref:`API: Logger messages stack<doxid-group__te__log__stack>`
|			:ref:`Command Output Logging<doxid-group__te__cmd__monitor>`
|			:ref:`Packet Sniffer<doxid-group__sniffer>`
|		:ref:`Remote Control Facility (RCF)<doxid-group__te__engine__rcf>`
|			:ref:`API Usage: Remote Control Facility API<doxid-group__rcfapi>`
|				:ref:`API: RCF<doxid-group__rcfapi__base>`
|			:ref:`API Usage: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc>`
|				:ref:`API: RCF RPC<doxid-group__te__lib__rcfrpc>`
|				:ref:`RPC pointer namespaces<doxid-group__rpc__type__ns>`
|				:ref:`TAPI: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc__tapi>`
|					:ref:`Macros for socket API remote calls<doxid-group__te__lib__rpcsock__macros>`
|					:ref:`TAPI for RTE EAL API remote calls<doxid-group__te__lib__rpc__rte__eal>`
|					:ref:`TAPI for RTE Ethernet Device API remote calls<doxid-group__te__lib__rpc__rte__ethdev>`
|					:ref:`TAPI for RTE FLOW API remote calls<doxid-group__te__lib__rpc__rte__flow>`
|					:ref:`TAPI for RTE MBUF API remote calls<doxid-group__te__lib__rpc__rte__mbuf>`
|					:ref:`TAPI for RTE MEMPOOL API remote calls<doxid-group__te__lib__rpc__rte__mempool>`
|					:ref:`TAPI for RTE mbuf layer API remote calls<doxid-group__te__lib__rpc__rte__mbuf__ndn>`
|					:ref:`TAPI for RTE ring API remote calls<doxid-group__te__lib__rpc__rte__ring>`
|					:ref:`TAPI for asynchronous I/O calls<doxid-group__te__lib__rpc__aio>`
|					:ref:`TAPI for directory operation calls<doxid-group__te__lib__rpc__dirent>`
|					:ref:`TAPI for interface name/index calls<doxid-group__te__lib__rpc__ifnameindex>`
|					:ref:`TAPI for miscellaneous remote calls<doxid-group__te__lib__rpc__misc>`
|					:ref:`TAPI for name/address resolution remote calls<doxid-group__te__lib__rpc__netdb>`
|					:ref:`TAPI for remote calls of Winsock2-specific routines<doxid-group__te__lib__rpc__winsock2>`
|					:ref:`TAPI for remote calls of dynamic linking loader<doxid-group__te__lib__rpc__dlfcn>`
|					:ref:`TAPI for remote calls of power switch<doxid-group__te__lib__rpc__power__sw>`
|					:ref:`TAPI for signal and signal sets remote calls<doxid-group__te__lib__rpc__signal>`
|					:ref:`TAPI for socket API remote calls<doxid-group__te__lib__rpc__socket>`
|					:ref:`TAPI for some file operations calls<doxid-group__te__lib__rpc__unistd>`
|					:ref:`TAPI for standard I/O calls<doxid-group__te__lib__rpc__stdio>`
|		:ref:`Report Generator Tool<doxid-group__rgt>`
|		:ref:`Test Results Comparator<doxid-group__trc>`
|		:ref:`Tester<doxid-group__te__engine__tester>`
|	:ref:`Test Suite<doxid-group__te__ts>`
|		:ref:`Network topology configuration of Test Agents<doxid-group__te__lib__tapi__conf__net>`
|		:ref:`Test API<doxid-group__te__ts__tapi>`
|			:ref:`Agent job control<doxid-group__tapi__job>`
|				:ref:`Helper functions for handling options<doxid-group__tapi__job__opt>`
|					:ref:`convenience macros for option<doxid-group__tapi__job__opt__bind__constructors>`
|					:ref:`functions for argument formatting<doxid-group__tapi__job__opt__formatting>`
|			:ref:`BPF/XDP configuration of Test Agents<doxid-group__tapi__bpf>`
|			:ref:`Control NVMeOF<doxid-group__tapi__nvme>`
|				:ref:`Kernel target<doxid-group__tapi__nvme__kern__target>`
|				:ref:`ONVMe target<doxid-group__tapi__nvme__onvme__target>`
|			:ref:`Control network channel using a gateway<doxid-group__ts__tapi__route__gw>`
|			:ref:`DPDK RTE flow helper functions TAPI<doxid-group__tapi__rte__flow>`
|			:ref:`DPDK helper functions TAPI<doxid-group__tapi__dpdk>`
|			:ref:`DPDK statistics helper functions TAPI<doxid-group__tapi__dpdk__stats>`
|			:ref:`Engine and TA files management<doxid-group__ts__tapi__file>`
|			:ref:`FIO tool<doxid-group__tapi__fio>`
|				:ref:`Control a FIO tool<doxid-group__tapi__fio__fio>`
|				:ref:`FIO tool internals<doxid-group__tapi__fio__internal>`
|			:ref:`GTest support<doxid-group__tapi__gtest>`
|			:ref:`High level TAPI to configure network<doxid-group__ts__tapi__network>`
|			:ref:`High level TAPI to work with sockets<doxid-group__ts__tapi__sockets>`
|			:ref:`I/O multiplexers<doxid-group__tapi__iomux>`
|			:ref:`Macros to get test parameters on a gateway configuration<doxid-group__ts__tapi__gw>`
|			:ref:`Network Traffic TAPI<doxid-group__net__traffic>`
|				:ref:`CSAP TAPI<doxid-group__csap>`
|				:ref:`Route Gateway TAPI<doxid-group__route__gw>`
|				:ref:`TAPI for testing TCP states<doxid-group__tapi__tcp__states>`
|					:ref:`TAPI definitions for testing TCP states<doxid-group__tapi__tcp__states__def>`
|				:ref:`TCP socket emulation<doxid-group__tcp__sock__emulation>`
|			:ref:`RADIUS Server Configuration and RADIUS CSAP<doxid-group__tapi__radius>`
|			:ref:`Stack of jumps<doxid-group__ts__tapi__jmp>`
|			:ref:`Target requirements modification<doxid-group__ts__tapi__reqs>`
|			:ref:`Test<doxid-group__te__ts__tapi__test>`
|				:ref:`Network environment<doxid-group__tapi__env>`
|				:ref:`Test execution flow<doxid-group__te__ts__tapi__test__log>`
|				:ref:`Test misc<doxid-group__te__ts__tapi__test__misc>`
|				:ref:`Test parameters<doxid-group__te__ts__tapi__test__param>`
|				:ref:`Test run status<doxid-group__te__ts__tapi__test__run__status>`
|			:ref:`Test API to control a network throughput test tool<doxid-group__tapi__performance>`
|			:ref:`Test API to operate the UPnP<doxid-group__tapi__upnp>`
|				:ref:`Test API for DLNA UPnP commons<doxid-group__tapi__upnp__common>`
|				:ref:`Test API to operate the DLNA UPnP Content Directory Resources<doxid-group__tapi__upnp__cd__resources>`
|				:ref:`Test API to operate the DLNA UPnP Content Directory Service<doxid-group__tapi__upnp__cd__service>`
|				:ref:`Test API to operate the DLNA UPnP Device information<doxid-group__tapi__upnp__device__info>`
|				:ref:`Test API to operate the DLNA UPnP Service information<doxid-group__tapi__upnp__service__info>`
|				:ref:`Test API to operate the UPnP Control Point<doxid-group__tapi__upnp__cp>`
|			:ref:`Test API to operate the media data<doxid-group__tapi__media>`
|				:ref:`Test API to control a media player<doxid-group__tapi__media__player>`
|				:ref:`Test API to control the mplayer media player<doxid-group__tapi__media__player__mplayer>`
|				:ref:`Test API to operate the DLNA media files<doxid-group__tapi__media__dlna>`
|				:ref:`Test API to operate the media files<doxid-group__tapi__media__file>`
|			:ref:`Test API to operate the storage<doxid-group__tapi__storage>`
|				:ref:`Test API to control the local storage device<doxid-group__tapi__local__storage__device>`
|				:ref:`Test API to control the storage FTP client<doxid-group__tapi__storage__client__ftp>`
|				:ref:`Test API to control the storage client<doxid-group__tapi__storage__client>`
|				:ref:`Test API to control the storage server<doxid-group__tapi__storage__server>`
|				:ref:`Test API to perform the generic operations over the storage<doxid-group__tapi__storage__wrapper>`
|			:ref:`Test API to use memory-related functions conveniently<doxid-group__tapi__mem>`
|			:ref:`Test API to use packetdrill test tool<doxid-group__tapi__packetdrill>`
|			:ref:`Tools to work with "struct sockaddr"<doxid-group__ts__tapi__sockaddr>`
|			:ref:`Traffic Application Domain<doxid-group__tapi__tad__main>`
|				:ref:`ARP<doxid-group__tapi__tad__arp>`
|				:ref:`ATM<doxid-group__tapi__tad__atm>`
|				:ref:`CLI<doxid-group__tapi__tad__cli>`
|				:ref:`DHCP<doxid-group__tapi__tad__dhcp>`
|				:ref:`Ethernet<doxid-group__tapi__tad__eth>`
|				:ref:`Forwarder module<doxid-group__tapi__tad__forw>`
|				:ref:`Generic TAD API<doxid-group__tapi__tad>`
|				:ref:`IGMP<doxid-group__tapi__tad__igmp>`
|				:ref:`IP stack<doxid-group__tapi__tad__ipstack>`
|					:ref:`Common functions for IPv4 and IPv6<doxid-group__tapi__tad__ip__common>`
|					:ref:`ICMP<doxid-group__tapi__tad__icmp>`
|					:ref:`ICMP4<doxid-group__tapi__tad__icmp4>`
|					:ref:`ICMP6<doxid-group__tapi__tad__icmp6>`
|					:ref:`IPv4<doxid-group__tapi__tad__ip4>`
|					:ref:`IPv6<doxid-group__tapi__tad__ip6>`
|					:ref:`Socket<doxid-group__tapi__tad__socket>`
|					:ref:`TCP<doxid-group__tapi__tad__tcp>`
|					:ref:`UDP<doxid-group__tapi__tad__udp>`
|				:ref:`Network Data Notation (NDN)<doxid-group__tapi__ndn>`
|				:ref:`PCAP<doxid-group__tapi__tad__pcap>`
|				:ref:`PPP<doxid-group__tapi__tad__ppp>`
|				:ref:`PPPOE<doxid-group__tapi__tad__pppoe>`
|				:ref:`Packet flow processing<doxid-group__tapi__tad__flow>`
|				:ref:`RTE mbuf<doxid-group__tapi__tad__rte__mbuf>`
|				:ref:`SNMP<doxid-group__tapi__tad__snmp>`
|				:ref:`STP<doxid-group__tapi__tad__stp>`
|				:ref:`iSCSI<doxid-group__tapi__tad__iscsi>`
|			:ref:`tool functions TAPI<doxid-group__tapi__wrk>`
|			:ref:`tool functions TAPI<doxid-group__tapi__netperf>`
|		:ref:`The format of test suite scenarios<doxid-group__te__scenarios>`


