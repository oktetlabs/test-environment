..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; API Usage: Remote Control Facility API
.. _doxid-group__rcfapi:

API Usage: Remote Control Facility API
======================================

.. toctree::
	:hidden:

	/generated/group_rcfapi_base.rst



.. _doxid-group__rcfapi_1rcfapi_introduction:

Introduction
~~~~~~~~~~~~

RCF API is an interface to access RCF services. Some functions of this API expected to be used from Test Engine components only (Logger, Configurator), but there are few that can be used from test scenarios.

The description of RCF API functions can be found at this page:

* :ref:`API: RCF <doxid-group__rcfapi__base>`

The following diagram shows components that use this or that RCF API function.

.. image:: /static/image/ten_rcfapi_users.png
	:alt: RCF API Users





.. _doxid-group__rcfapi_1rcfapi_test_user:

RCF API functions for test scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For test scenarios we can use RCF API for:

* manipulation of environment variables:

  * :ref:`rcf_ta_get_var() <doxid-group__rcfapi__base_1gaeedff701d6d9ca8ee6df9f5762081b1a>`;

  * :ref:`rcf_ta_set_var() <doxid-group__rcfapi__base_1ga5d84fcce34e3c001d3972302f7f7eed4>`.

* manipulation of files on Test Agent side:

  * :ref:`rcf_ta_get_file() <doxid-group__rcfapi__base_1gab7f10dce65461737da79bfc5f6204718>`;

  * :ref:`rcf_ta_put_file() <doxid-group__rcfapi__base_1ga5de4f5d3c247041df66c16331ee1cc68>`;

  * :ref:`rcf_ta_del_file() <doxid-group__rcfapi__base_1ga347ccc19384c7d0ccc957e538625d185>`.

* calling a function on Test Agent side (do not mix it with RPC calls):

  * :ref:`rcf_ta_call() <doxid-group__rcfapi__base_1ga2f02c1a61e6756b4c97772e5d07a31da>`.

* process and thread manipulation on Test Agent side:

  * :ref:`rcf_ta_start_task() <doxid-group__rcfapi__base_1ga6bb2cb53cce5237a8c50a0ffbd09b823>`;

  * :ref:`rcf_ta_kill_task() <doxid-group__rcfapi__base_1gafd464ab429554e9e201a738d8a9ac86c>`;

  * :ref:`rcf_ta_start_thread() <doxid-group__rcfapi__base_1ga354bb92b6a19239e5ead46a9985ed8cc>`;

  * :ref:`rcf_ta_kill_thread() <doxid-group__rcfapi__base_1gaed61242aa8027c5561ea20d64e11637d>`.

|	:ref:`API: RCF<doxid-group__rcfapi__base>`


