..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te_engine_rcf

.. index:: pair: group; Using the RCF API
.. _doxid-group__rcfapi:

Using the RCF API
=================

.. include:: _toctree/rcfapi.inc

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

  * :cref:`rcf_ta_get_var() <rcf_ta_get_var>`;

  * :cref:`rcf_ta_set_var() <rcf_ta_set_var>`.

* manipulation of files on Test Agent side:

  * :cref:`rcf_ta_get_file() <rcf_ta_get_file>`;

  * :cref:`rcf_ta_put_file() <rcf_ta_put_file>`;

  * :cref:`rcf_ta_del_file() <rcf_ta_del_file>`.

* calling a function on Test Agent side (do not mix it with RPC calls):

  * :cref:`rcf_ta_call() <rcf_ta_call>`.

* process and thread manipulation on Test Agent side:

  * :cref:`rcf_ta_start_task() <rcf_ta_start_task>`;

  * :cref:`rcf_ta_kill_task() <rcf_ta_kill_task>`;

  * :cref:`rcf_ta_start_thread() <rcf_ta_start_thread>`;

  * :cref:`rcf_ta_kill_thread() <rcf_ta_kill_thread>`.
