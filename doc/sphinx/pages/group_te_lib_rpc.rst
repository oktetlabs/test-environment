..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; API Usage: Remote Procedure Calls (RPC)
.. _doxid-group__te__lib__rpc:

API Usage: Remote Procedure Calls (RPC)
=======================================

.. toctree::
	:hidden:

	/generated/group_te_lib_rcfrpc.rst
	/generated/group_rpc_type_ns.rst
	group_te_lib_rpc_tapi.rst



.. _doxid-group__te__lib__rpc_1te_lib_rpc_introduction:

Introduction
~~~~~~~~~~~~

In the context of TE, Remote Procedure Calls functionality provides the ability to invoke a function on a Test Agent side. It can be obvious, but anyway it is worth nothing that in order to call a function on Test Agent a special action shall be done to tell Agent to treat that function as a function for RPC calls. I.e. RPC does not allow any function on Test Agent to be called, but only a set of functions that were told to be RPC aware.

On Test Engine side RPC functionlity is exported by RCF RPC library: :ref:`API: RCF RPC <doxid-group__te__lib__rcfrpc>`, but end users should utilize and if necessary enhance upper layer API exported by :ref:`TAPI: Remote Procedure Calls (RPC) <doxid-group__te__lib__rpc__tapi>`.

Here is the diagram of libraries and TE components that take part in RPC data flow:

.. image:: /static/image/te_lib_rpc_context.png
	:alt: Remote Procedure Call context in TE

RCF RPC library is actually reside at the same layer as RCF API library, but it is selected to a dedicated library just to split it on functionality basis.





.. _doxid-group__te__lib__rpc_1te_lib_rpc_server:

RPC Server
~~~~~~~~~~

Any RPC call is done in the context of RPC Server. RPC Server is a separate process or thread on Test Agent side in which context a function call is done. From Test code point of view RPC Server is represented by :ref:`rcf_rpc_server <doxid-structrcf__rpc__server>` data structure and it is associated with a pair of names - Test Agent name and RPC Server name (this pair of names are used in configurator tree to identify RPC Server).

In order to create or delete an RPC Server, functions from RCF RPC library should be used:

* :ref:`rcf_rpc_server_create() <doxid-group__te__lib__rcfrpc_1gadd6023c0ba484fcd9cc070c849a76cbe>` - to create an RPC Server with particular name on the particular Test Agent;

* :ref:`rcf_rpc_server_destroy() <doxid-group__te__lib__rcfrpc_1ga0148b1d62fe5a40561bad917b300f9a4>` - to destroy an RPC Server.

Creating and deleting an RPC Server is done via Configurator management tree. On Test Agent side this causes RCF PCH library to pass control to corresponding configuration model handlers - **/agent/rpcserver** node.

When a new RPC Server is being created Test Agent creates a new thread or process (depending on the desired location of RPC server). Please note that to create an RPC Server in thread context you will first need to create an RPC Server in a process context and only then you can create another RPC Server as a thread of that previously created process context RPC Server.

Each RPC Server has its own communication link with Test Agent process. On RPC Server start-up it connects to Test Agent in order to set-up that communication link.

.. image:: /static/image/te_lib_rpc_server_context.png
	:alt: Test Agent and RPC Server context

There are the following functions available for creating RPC Server in different contexts:

* :ref:`rcf_rpc_server_create() <doxid-group__te__lib__rcfrpc_1gadd6023c0ba484fcd9cc070c849a76cbe>`;

* :ref:`rcf_rpc_server_thread_create() <doxid-group__te__lib__rcfrpc_1gac587e23c562e4d4a0fdae00e35e0a8be>`;

* :ref:`rcf_rpc_server_fork() <doxid-group__te__lib__rcfrpc_1gadc722d5beae0739de16066207f5d8516>`;

* :ref:`rcf_rpc_server_fork_exec() <doxid-group__te__lib__rcfrpc_1ga485c827d96d8eb7cfbb7aed42aa19921>`;

* :ref:`rcf_rpc_server_create_process() <doxid-group__te__lib__rcfrpc_1gab9cc27bc3b60411f42baab23f0317d48>`;

Each RPC Server can be switched to use a particular dynamic library on function name resolution during RPC call operation. To change dynamic library name the following function shall be used:

* :ref:`rcf_rpc_setlibname() <doxid-group__te__lib__rcfrpc_1ga93f11072f7e2bc2f87edce186c7d33ea>`.

Once you have an RPC Server handle you are ready to do RPC call by means of the following function:

* :ref:`rcf_rpc_call() <doxid-group__te__lib__rcfrpc_1ga3175cd6f2a0dce9edbc315c7ec1709af>`.

For more information on functions exported by RCF RPC library refer to :ref:`API: RCF RPC <doxid-group__te__lib__rcfrpc>`.





.. _doxid-group__te__lib__rpc_1te_lib_rpc_tapi_section:

TAPI for RPC
~~~~~~~~~~~~

Test code should not use :ref:`rcf_rpc_call() <doxid-group__te__lib__rcfrpc_1ga3175cd6f2a0dce9edbc315c7ec1709af>` function directly, but instead it shall use functions exported by :ref:`TAPI: Remote Procedure Calls (RPC) <doxid-group__te__lib__rpc__tapi>`.

|	:ref:`API: RCF RPC<doxid-group__te__lib__rcfrpc>`
|	:ref:`RPC pointer namespaces<doxid-group__rpc__type__ns>`
|	:ref:`TAPI: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc__tapi>`
|		:ref:`Macros for socket API remote calls<doxid-group__te__lib__rpcsock__macros>`
|		:ref:`TAPI for RTE EAL API remote calls<doxid-group__te__lib__rpc__rte__eal>`
|		:ref:`TAPI for RTE Ethernet Device API remote calls<doxid-group__te__lib__rpc__rte__ethdev>`
|		:ref:`TAPI for RTE FLOW API remote calls<doxid-group__te__lib__rpc__rte__flow>`
|		:ref:`TAPI for RTE MBUF API remote calls<doxid-group__te__lib__rpc__rte__mbuf>`
|		:ref:`TAPI for RTE MEMPOOL API remote calls<doxid-group__te__lib__rpc__rte__mempool>`
|		:ref:`TAPI for RTE mbuf layer API remote calls<doxid-group__te__lib__rpc__rte__mbuf__ndn>`
|		:ref:`TAPI for RTE ring API remote calls<doxid-group__te__lib__rpc__rte__ring>`
|		:ref:`TAPI for asynchronous I/O calls<doxid-group__te__lib__rpc__aio>`
|		:ref:`TAPI for directory operation calls<doxid-group__te__lib__rpc__dirent>`
|		:ref:`TAPI for interface name/index calls<doxid-group__te__lib__rpc__ifnameindex>`
|		:ref:`TAPI for miscellaneous remote calls<doxid-group__te__lib__rpc__misc>`
|		:ref:`TAPI for name/address resolution remote calls<doxid-group__te__lib__rpc__netdb>`
|		:ref:`TAPI for remote calls of Winsock2-specific routines<doxid-group__te__lib__rpc__winsock2>`
|		:ref:`TAPI for remote calls of dynamic linking loader<doxid-group__te__lib__rpc__dlfcn>`
|		:ref:`TAPI for remote calls of power switch<doxid-group__te__lib__rpc__power__sw>`
|		:ref:`TAPI for signal and signal sets remote calls<doxid-group__te__lib__rpc__signal>`
|		:ref:`TAPI for socket API remote calls<doxid-group__te__lib__rpc__socket>`
|		:ref:`TAPI for some file operations calls<doxid-group__te__lib__rpc__unistd>`
|		:ref:`TAPI for standard I/O calls<doxid-group__te__lib__rpc__stdio>`


