..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te
  te-order: 40

.. index:: pair: group; Test Agents
.. _doxid-group__te__agents:

Test Agents
===========

.. include:: _toctree/te_agents.inc

.. _doxid-group__te__agents_1te_agt_introduction:

Introduction
~~~~~~~~~~~~

Test Agent is a piece of software that in general runs on dedicated host and exports functionality necessary to configure test set-up, generate some traffic, or receive some statistics from device under test or auxiliary testing components. Depending on the particular type of an Agent, it can operate as a single process or as a set of processes or threads.

The Agent is a component that is intended for future amendments, extensions and porting to different platforms.

At the time or writing this document TE has Test Agents available for the following Operating Systems:

* Linux;

* FreBSD;

* Windows.


.. image:: /static/image/agt_types.png
	:alt: Possible physical location of Test Agents

Depending on the physical location of Test Agent and the device it controls we can differentiate Agents as following:

* **Auxiliary Agent**

  Test Agent running on an auxiliary station and controlling this station.

  Test Protocol is used over a network and all commands are applied to the station on which Test Agent runs. Such Agents are necessary when we do Black Box testing:

  * to emulate presence of some software or hardware component in the test set-up;

  * network protocol testing (sending/receiving protocol data);

  * in interoperability testing;

  * external interface testing (SNMP, TR-069, CLI).

  Please note that a Test Agent can be run on the same station as :ref:`Test Engine <doxid-group__te__engine>`, the only difference is that reboot command should never be applied to this Test Agent;

* **Proxy Agent**

  Test Agent running on an auxiliary station and controlling the DUT.

  Test Protocol is used over a network and all commands are applied to the DUT (Test Agent interacts with the DUT via a DUT-specific interface).

  This kind of Agent is necessary when we need to be able to configure DUT during test scenario or when we want to control some external equipment that provides facilities necessary in the particular test scenarios, but at the same time we only have some external interfaces exported by DUT or auxiliary equipment.

* **Embedded Agent**

  Test Agent running on a DUT and controlling the DUT.

  Test Protocol is used over a network or over some other transport (for example, over a serial port) and all commands are applied to the NUT.

  This kind of Test Agent is necessary when we want to do White Box testing (i.e. when we are able to integrate our software components into the DUT):

  * Module testing;

  * DUP API testing;

  * DUT-specific management.

  All communication with Test Agents performed via :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` components. Interactions are done with text-based Test Protocol.


.. _doxid-group__te__agents_1te_agt_organization:

Test Agent Organization
~~~~~~~~~~~~~~~~~~~~~~~

Basically Test Agent consists of:

* start-up code;

* generic part - Portable Command Handler (PCH) (sources under lib/rcfpch directory);

* OS-specific part - Command Handler (CH) - functions called by PCH (for more information see :ref:`Test Agents: Command Handler <doxid-group__rcf__ch>`);

* OS-specific implementation of callbacks for configurator tree (see :ref:`Command Handler: Configuration support <doxid-group__rcf__ch__cfg>` interface);

* a set of functions used in RPC;

* a number of standard RCF Services supported in separate libraries (like TAD).


.. image:: /static/image/ta_decomp.png
	:alt: Test Agent decomposition

On start-up Test Agent does the following things:

* calls :cref:`ta_log_init` to initialize logger API;

* if a Test Agent supports process or thread creation (:cref:`rcf_ch_start_process`, and :cref:`rcf_ch_start_thread` are supported), then it is necessary to create a separate thread or process that will receive log messages from different processes and threads run on behalf of Test Agent. lib/loggerta library exports :cref:`logfork_entry` function that should be used as an entry point for such a thread/process;

* calls :cref:`rcf_pch_run` function that passes control to RCF Portable Command Handler.

In RCF PCH Test Agent passively waits for :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` commands. RCF commands are handled in the main loop of RCF PCH library. Depending on the command type it can be handled either by generic part of RCF PCH (located under lib/rcfpch) or it can be directed to Agent specific parts of code (located under agents/[type]).

Functions declared in ``lib/rcfpch/rcf_ch_api.h`` should be exported by a Test Agent, though they can return -1 telling that an operation is not supported.


.. _doxid-group__te__agents_1te_agt_types:

Types of supported Test Agents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Unix Test Agent is the agent used in practice:

* :ref:`Unix Test Agent <doxid-group__te__agents__unix>`.


.. _doxid-group__te__agents_1te_agt_extending:

Extending Test Agent with new functionality
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. _doxid-group__te__agents_1te_agt_extending_rpc:

Adding new RPC calls
--------------------

* :ref:`RPC Development Framework <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework>` section gives guidelines on how to add support for a new RPC call.


.. _doxid-group__te__agents_1te_agt_extending_cfg:

Adding new configuration nodes
------------------------------

* :ref:`Adding configuration nodes <doxid-group__te__agents__conf>` page gives guidelines on how to add new configuration nodes to Test Agent;

* :ref:`Using the Configurator API <doxid-group__confapi>` page gives guidelines on how to work with API from test scenarios.
