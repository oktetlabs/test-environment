.. index:: pair: group; Test Engine
.. _doxid-group__te__engine:

Test Engine
===========

.. toctree::
    :hidden:

    group_kernel_log.rst
    group_te_engine_builder.rst
    group_te_engine_conf.rst
    group_te_engine_logger.rst
    group_te_engine_dispatcher.rst
    group_te_engine_rcf.rst
    group_te_engine_tester.rst
    group_rgt.rst
    group_trc.rst



.. _doxid-group__te__engine_1te_eng_introduction:

Introduction
~~~~~~~~~~~~

Test Engine is a set of software components that provide essential features of Test Environment. It is unlikely that you will need to update any of Test Engine components, but more likely you will need to implement some helper libraries that utilize services provided by Test Engine or you will need to add some functionality in te_agents.

.. image:: /static/image/ten_decomposition.png
	:alt: High Level decomposition of Test Engine Components

Test Engine consists of the following components:

* :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`, which is responsible for configuring and starting of another subsystems;

* :ref:`Builder <doxid-group__te__engine__builder>`, which is responsible for preparing libraries and executables for te_agents and TE Subsystems as well as NUT bootable images and building the tests;

* :ref:`Configurator <doxid-group__te__engine__conf>`, which is responsible for configuring the environment, providing configuration information to tests and for recovering the configuration after failures. Moreover it supports some TEN-local database used by Test Packages to store shared data;

* :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, which is responsible for starting te_agents and for all interactions between :ref:`Test Engine <doxid-group__te__engine>` and te_agents on behalf of other subsystems and tests;

* :ref:`Tester <doxid-group__te__engine__tester>`, which is responsible for running a set of Test Packages specified by a user in the mode specified by a user (one-by-one, simultaneous, debugging);

* :ref:`Logger <doxid-group__te__engine__logger>`, which provides logging facilities for Test Environment (:ref:`Test Engine <doxid-group__te__engine>` and te_agents) and tests, and log processing tools for users.

The following diagram gives more detailed information on relations between Test Environment components:

.. image:: /static/image/ten_interconnections.png
	:alt: Interconnections of Test Engine components

|	:ref:`Builder<doxid-group__te__engine__builder>`
|	:ref:`Configurator<doxid-group__te__engine__conf>`
|		:ref:`API Usage: Configurator API<doxid-group__confapi>`
|			:ref:`API: Configurator<doxid-group__confapi__base>`
|				:ref:`Configuration backup manipulation<doxid-group__confapi__base__backup>`
|				:ref:`Configuration tree traversal<doxid-group__confapi__base__traverse>`
|				:ref:`Contriguration tree access operations<doxid-group__confapi__base__access>`
|				:ref:`Synchronization configuration tree with Test Agent<doxid-group__confapi__base__sync>`
|				:ref:`Test Agent reboot<doxid-group__confapi__base__reboot>`
|			:ref:`TAPI: Test API for configuration nodes<doxid-group__tapi__conf>`
|				:ref:`ARL table configuration<doxid-group__tapi__conf__arl>`
|				:ref:`Agent namespaces configuration<doxid-group__tapi__namespaces>`
|				:ref:`Agents, namespaces and interfaces relations<doxid-group__tapi__host__ns>`
|				:ref:`Bonding and bridging configuration<doxid-group__tapi__conf__aggr>`
|				:ref:`CPU topology configuration of Test Agents<doxid-group__tapi__conf__cpu>`
|				:ref:`Command monitor TAPI<doxid-group__tapi__cmd__monitor__def>`
|				:ref:`DHCP Server configuration<doxid-group__tapi__conf__dhcp__serv>`
|				:ref:`DUT serial console access<doxid-group__tapi__conf__serial>`
|				:ref:`Environment variables configuration<doxid-group__tapi__conf__sh__env>`
|				:ref:`Ethernet PHY configuration<doxid-group__tapi__conf__phy>`
|				:ref:`Ethernet interface features configuration<doxid-group__tapi__conf__eth>`
|				:ref:`IP rules configuration<doxid-group__tapi__conf__ip__rule>`
|				:ref:`IPv6 specific configuration<doxid-group__tapi__conf__ip6>`
|				:ref:`Kernel modules configuration<doxid-group__tapi__conf__modules>`
|				:ref:`L2TP configuration<doxid-group__tapi__conf__l2tp>`
|				:ref:`Local subtree access<doxid-group__tapi__conf__local>`
|				:ref:`Manipulation of network address pools<doxid-group__tapi__conf__net__pool>`
|				:ref:`NTP daemon configuration<doxid-group__tapi__conf__ntpd>`
|				:ref:`Neighbour table configuration<doxid-group__tapi__conf__neigh>`
|				:ref:`Network Base configuration<doxid-group__tapi__conf__base__net>`
|				:ref:`Network Emulator configuration<doxid-group__tapi__conf__netem>`
|				:ref:`Network Interface configuration<doxid-group__tapi__conf__iface>`
|				:ref:`Network Switch configuration<doxid-group__tapi__conf__switch>`
|				:ref:`Network sniffers configuration<doxid-group__tapi__conf__sniffer>`
|				:ref:`Network statistics access<doxid-group__tapi__conf__stats>`
|				:ref:`Network topology configuration of Test Agents<doxid-group__tapi__conf__net>`
|				:ref:`PPPoE Server configuration<doxid-group__tapi__conf__pppoe>`
|				:ref:`Processes configuration<doxid-group__tapi__conf__process>`
|				:ref:`Queuing Discipline configuration<doxid-group__tapi__conf__qdisc>`
|					:ref:`tc qdisc TBF configuration<doxid-group__tapi__conf__tbf>`
|				:ref:`Routing Table configuration<doxid-group__tapi__conf__route>`
|				:ref:`Serial console parsers configuration<doxid-group__tapi__conf__serial__parse>`
|				:ref:`Solarflare PTP daemon configuration<doxid-group__tapi__conf__sfptpd>`
|				:ref:`System parameters configuration<doxid-group__tapi__cfg__sys>`
|				:ref:`TA system options configuration (OBSOLETE)<doxid-group__tapi__conf__proc>`
|				:ref:`TR-069 Auto Configuration Server Engine (ACSE) configuration<doxid-group__tapi__conf__acse>`
|				:ref:`Test API to handle a cache<doxid-group__tapi__cache>`
|				:ref:`VTund configuration<doxid-group__tapi__conf__vtund>`
|				:ref:`Virtual machines configuration<doxid-group__tapi__conf__vm>`
|				:ref:`XEN configuration<doxid-group__tapi__conf__xen>`
|				:ref:`iptables configuration<doxid-group__tapi__conf__iptable>`
|	:ref:`Dispatcher<doxid-group__te__engine__dispatcher>`
|	:ref:`Logger<doxid-group__te__engine__logger>`
|		:ref:`API: Logger<doxid-group__logger__api>`
|		:ref:`API: Logger messages stack<doxid-group__te__log__stack>`
|	:ref:`Remote Control Facility (RCF)<doxid-group__te__engine__rcf>`
|		:ref:`API Usage: Remote Control Facility API<doxid-group__rcfapi>`
|			:ref:`API: RCF<doxid-group__rcfapi__base>`
|		:ref:`API Usage: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc>`
|			:ref:`API: RCF RPC<doxid-group__te__lib__rcfrpc>`
|			:ref:`RPC pointer namespaces<doxid-group__rpc__type__ns>`
|			:ref:`TAPI: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc__tapi>`
|				:ref:`Macros for socket API remote calls<doxid-group__te__lib__rpcsock__macros>`
|				:ref:`TAPI for RTE EAL API remote calls<doxid-group__te__lib__rpc__rte__eal>`
|				:ref:`TAPI for RTE Ethernet Device API remote calls<doxid-group__te__lib__rpc__rte__ethdev>`
|				:ref:`TAPI for RTE FLOW API remote calls<doxid-group__te__lib__rpc__rte__flow>`
|				:ref:`TAPI for RTE MBUF API remote calls<doxid-group__te__lib__rpc__rte__mbuf>`
|				:ref:`TAPI for RTE MEMPOOL API remote calls<doxid-group__te__lib__rpc__rte__mempool>`
|				:ref:`TAPI for RTE mbuf layer API remote calls<doxid-group__te__lib__rpc__rte__mbuf__ndn>`
|				:ref:`TAPI for RTE ring API remote calls<doxid-group__te__lib__rpc__rte__ring>`
|				:ref:`TAPI for TR-069 ACS<doxid-group__te__lib__rpc__tr069>`
|				:ref:`TAPI for asynchronous I/O calls<doxid-group__te__lib__rpc__aio>`
|				:ref:`TAPI for directory operation calls<doxid-group__te__lib__rpc__dirent>`
|				:ref:`TAPI for interface name/index calls<doxid-group__te__lib__rpc__ifnameindex>`
|				:ref:`TAPI for miscellaneous remote calls<doxid-group__te__lib__rpc__misc>`
|				:ref:`TAPI for name/address resolution remote calls<doxid-group__te__lib__rpc__netdb>`
|				:ref:`TAPI for remote calls of Winsock2-specific routines<doxid-group__te__lib__rpc__winsock2>`
|				:ref:`TAPI for remote calls of dynamic linking loader<doxid-group__te__lib__rpc__dlfcn>`
|				:ref:`TAPI for remote calls of power switch<doxid-group__te__lib__rpc__power__sw>`
|				:ref:`TAPI for remote calls of telephony<doxid-group__te__lib__rpc__telephony>`
|				:ref:`TAPI for signal and signal sets remote calls<doxid-group__te__lib__rpc__signal>`
|				:ref:`TAPI for socket API remote calls<doxid-group__te__lib__rpc__socket>`
|				:ref:`TAPI for some file operations calls<doxid-group__te__lib__rpc__unistd>`
|				:ref:`TAPI for standard I/O calls<doxid-group__te__lib__rpc__stdio>`
|	:ref:`Report Generator Tool<doxid-group__rgt>`
|	:ref:`Test Results Comparator<doxid-group__trc>`
|	:ref:`Tester<doxid-group__te__engine__tester>`


