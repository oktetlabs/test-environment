..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te_ts_tapi

.. index:: pair: group; Remote calls on an agent
.. _doxid-group__te__lib__rpc:

Remote calls on an agent
========================

.. include:: _toctree/te_lib_rpc.inc

.. _doxid-group__te__lib__rpc_1te_lib_rpc_introduction:

Introduction
~~~~~~~~~~~~

In the context of TE, Remote Procedure Calls functionality provides the ability to invoke a function on a Test Agent side. It can be obvious, but anyway it is worth nothing that in order to call a function on Test Agent a special action shall be done to tell Agent to treat that function as a function for RPC calls. I.e. RPC does not allow any function on Test Agent to be called, but only a set of functions that were told to be RPC aware.

On Test Engine side RPC functionlity is exported by RCF RPC library: :ref:`API: RCF RPC <doxid-group__te__lib__rcfrpc>`, but end users should utilize and if necessary enhance upper layer API exported by :ref:`RPC call wrappers <doxid-group__te__lib__rpc__tapi>`.

Here is the diagram of libraries and TE components that take part in RPC data flow:

.. image:: /static/image/te_lib_rpc_context.png
	:alt: Remote Procedure Call context in TE

RCF RPC library is actually reside at the same layer as RCF API library, but it is selected to a dedicated library just to split it on functionality basis.


.. _doxid-group__te__lib__rpc_1te_lib_rpc_server:

RPC Server
~~~~~~~~~~

Any RPC call is done in the context of RPC Server. RPC Server is a separate process or thread on Test Agent side in which context a function call is done. From Test code point of view RPC Server is represented by :ref:`rcf_rpc_server <doxid-structrcf__rpc__server>` data structure and it is associated with a pair of names - Test Agent name and RPC Server name (this pair of names are used in configurator tree to identify RPC Server).

In order to create or delete an RPC Server, functions from RCF RPC library should be used:

* :cref:`rcf_rpc_server_create` - to create an RPC Server with particular name on the particular Test Agent;

* :cref:`rcf_rpc_server_destroy` - to destroy an RPC Server.

Creating and deleting an RPC Server is done via Configurator management tree. On Test Agent side this causes RCF PCH library to pass control to corresponding configuration model handlers - **/agent/rpcserver** node.

When a new RPC Server is being created Test Agent creates a new thread or process (depending on the desired location of RPC server). Please note that to create an RPC Server in thread context you will first need to create an RPC Server in a process context and only then you can create another RPC Server as a thread of that previously created process context RPC Server.

Each RPC Server has its own communication link with Test Agent process. On RPC Server start-up it connects to Test Agent in order to set-up that communication link.

.. image:: /static/image/te_lib_rpc_server_context.png
	:alt: Test Agent and RPC Server context

There are the following functions available for creating RPC Server in different contexts:

* :cref:`rcf_rpc_server_create`;

* :cref:`rcf_rpc_server_thread_create`;

* :cref:`rcf_rpc_server_fork`;

* :cref:`rcf_rpc_server_fork_exec`;

* :cref:`rcf_rpc_server_create_process`;

Each RPC Server can be switched to use a particular dynamic library on function name resolution during RPC call operation. To change dynamic library name the following function shall be used:

* :cref:`rcf_rpc_setlibname`.

Once you have an RPC Server handle you are ready to do RPC call by means of the following function:

* :cref:`rcf_rpc_call`.

For more information on functions exported by RCF RPC library refer to :ref:`API: RCF RPC <doxid-group__te__lib__rcfrpc>`.


.. _doxid-group__te__lib__rpc_1te_lib_rpc_tapi_section:

TAPI for RPC
~~~~~~~~~~~~

Test code should not use :cref:`rcf_rpc_call` function directly, but instead it shall use functions exported by :ref:`RPC call wrappers <doxid-group__te__lib__rpc__tapi>`.
