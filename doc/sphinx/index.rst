..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Test Environment
.. _doxid-group__te:

Test Environment
================

.. include:: pages/_toctree/te.inc

.. Not a documented group of its own: the flat Doxygen dump of
   everything that is not filed under one.

.. toctree::
	:hidden:

	generated/global

.. _doxid-group__te_1te_introduction:

Introduction
~~~~~~~~~~~~

OKTET Labs Test Environment (TE) is a software product that is intended to ease creating automated test suites. The history of TE goes back to 2000 year when the first prototype of software was created. At that time the product was used for testing SNMP MIBs and CLI commands. Two years later (in 2002) software was extended to support testing of IPv6 protocol.

Few years of intensive usage the software in testing projects showed that a deep re-design was necessary to make the architecture flexible and extandable for new and new upcoming features. In 2003 year it was decided that the redesign should be fulfilled. Due to the careful and well-thought design decisions made in 2003 year, the overall TE architecture (main components and interconnections between them) are still valid even though a lot of new features has been added since then.


.. _doxid-group__te_1te_conventions:

Conventions
~~~~~~~~~~~

Through the documentation the following conventions are used:

* directory paths or file names marked as: ``/usr/bin/bash``, ``Makefile``;

* program names (scripts or binary executables) marked as: ``gdb``, ``dispatcher.sh``;

* program options are marked as: ``help``, ``-q``;

* environment variables are marked as: ``TE_BASE``, ``${PATH}``;

* different directives in configuration files are marked as: ``TE_LIB_PARMS``, ``include``;

* configuration paths as marked as: ``/agent/interface``, ``/local:/net:A``;

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

Selection of appropriate usage scenario depends on the object being tested. If it's a network device the Test Agent can run either on the device itself or (if black box testing is performed or device hardware is not capable of running the agent) on an auxiliary station. If a network library is tested then the agent should probably be running on the NUT itself (in such case NUT is the host on which the service that uses the library is running).

Apart from these three kinds of agents (or should they be called 'Test Agent usage scenarios') there can exist arbitrary number of agents running on other equipments which participates in the testing process. Examples include:

* other hosts which are used for traffic generation;

* proxy agents which are used to control various services used for network environment creation or DUT configuration via management protocols;

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
