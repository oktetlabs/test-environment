..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: tapi_conf

.. index:: pair: group; Using the Configurator API
.. _doxid-group__confapi:

Using the Configurator API
==========================

.. _doxid-group__confapi_1confapi_introduction:

Usage of Configurator API from test scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test scenarios should use functions exported via:

* base :ref:`API: Configurator <doxid-group__confapi__base>` (``lib/confapi/conf_api.h``);

* semantic based interface :ref:`Configuring the test bed <doxid-group__tapi__conf>`.

Here we will show how to play with samples discussed at :ref:`Creating new configuration nodes in Test Agent <doxid-group__te__agents__conf>` page.


.. _doxid-group__confapi_1confapi_usage_conf:

Tuning Configurator configuration file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to let :ref:`Configurator <doxid-group__te__engine__conf>` know about instances of new nodes we should register these new object nodes in configurator tree. Then :ref:`Configurator <doxid-group__te__engine__conf>` will be able to get instances of these objects from Test Agents. Otherwise :ref:`Configurator <doxid-group__te__engine__conf>` ignores node instances whose object nodes were not registered.

.. image:: /static/image/ten_conf_startup.png
	:alt: Configurator start-up event flow

When :ref:`Configurator <doxid-group__te__engine__conf>` starts it processes configurator configuration file that keeps object descriptions that need to be registered in local tree of objects (see arrow [2]).

Configuration file can also keep rules to add object instances, but these instances can not be applied for /agent subtree. /agent subtree is in control by Test Agents.

:ref:`Configurator <doxid-group__te__engine__conf>` should ask Test Agents about these instances that is why it call :ref:`rcf_ta_cfg_get() <doxid-group__rcfapi__base_1ga92bb850be576f887a71251e4d86ccd45>` function with wildcard object instance identifier (arrow [4] in the figure).

When :ref:`Configurator <doxid-group__te__engine__conf>` receives a reply with the list of object instance names it checks whether an instance name has corresponding object node in its local object tree. If yes, then it adds an instance into its instance configuration tree, otherwise it ignores an instance name and tests will not be able to access those instances until they register corresponding object nodes in :ref:`Configurator <doxid-group__te__engine__conf>` (see arrow [6]).

Regarding an example described at :ref:`Creating new configuration nodes in Test Agent <doxid-group__te__agents__conf>` page, we should add the following lines into :ref:`Configurator <doxid-group__te__engine__conf>` configuration file to let :ref:`Configurator <doxid-group__te__engine__conf>` know about our new supported object instances:

.. ref-code-block:: xml

	<object oid="/agent/ro_object" access="read_only" type="integer"/>
	<object oid="/agent/rw_object" access="read_write" type="address"/>
	<object oid="/agent/col_object" access="read_create" type="none"/>
	<object oid="/agent/col_object/var" access="read_only" type="string"/>

For more information on :ref:`Configurator <doxid-group__te__engine__conf>` configuration file read :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` section.


.. _doxid-group__confapi_1confapi_usage_add_del:

Adding/Deleting an entry to/from configuration tree
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For nodes of "read-create" access type it is possible to add or delete an instance in run time (from a test). Access type "read-create" does not guarantee the ability to run add or delete operation, but implementation of :ref:`rcf_pch_cfg_object::add <doxid-structrcf__pch__cfg__object_1ad7ce244750f7ef2c2b850dd98a9af7f2>` and :ref:`rcf_pch_cfg_object::del <doxid-structrcf__pch__cfg__object_1a5c5c2a064b8de217f700ff324ec8548f>` functions is required.

Please note that there can be "read-create" objects that do not provide implementation of :ref:`rcf_pch_cfg_object::add <doxid-structrcf__pch__cfg__object_1ad7ce244750f7ef2c2b850dd98a9af7f2>` or :ref:`rcf_pch_cfg_object::del <doxid-structrcf__pch__cfg__object_1a5c5c2a064b8de217f700ff324ec8548f>` functions. This mainly means that the number of instances can vary depending on events happened on Test Agent. Test Agent reports about the number of instances of that objects with :ref:`rcf_pch_cfg_object::list <doxid-structrcf__pch__cfg__object_1a908690657283ab4537a7bf36c4877123>` handler.

In order to add a new object instance you should use one of the following functions:

* :ref:`cfg_add_instance() <doxid-group__confapi__base__access_1ga856f5dcea2bf506805e79fc13aa02cde>`;

* :ref:`cfg_add_instance_str() <doxid-group__confapi__base__access_1ga7771fd0dd155ef377ca94d8be282b47c>`;

* :ref:`cfg_add_instance_fmt() <doxid-group__confapi__base__access_1ga45cfda8cc1cc54d72c2482c6f48a507e>`.

The following diagram shows the sequence of events caused by calling any of these functions.

.. image:: /static/image/ten_conf_add_instance.png
	:alt: Sequence of events caused by cfg_add_instance() call

Similar things happen when you call a function to delete an object instance:

* :ref:`cfg_del_instance() <doxid-group__confapi__base__access_1ga5abd708a2c82ecf19e40f91c08916d1e>`;

* :ref:`cfg_del_instance_fmt() <doxid-group__confapi__base__access_1ga30090ed0cdf07ca43115b5496a4fe606>`.

You can also use *local* version of instance add functions:

* :ref:`cfg_add_instance_local() <doxid-group__confapi__base__access_1ga6962cddecb5fdbd01e628550a256c85c>`;

* :ref:`cfg_add_instance_local_str() <doxid-group__confapi__base__access_1ga4365e5060e03cba6145615bf6df7c965>`;

* :ref:`cfg_add_instance_local_fmt() <doxid-group__confapi__base__access_1gab4943c0e980604b56b98692d6c9ef71e>`.

The only difference is that these functions will not cause :ref:`rcf_pch_cfg_object::commit <doxid-structrcf__pch__cfg__object_1a10e8489e786107818fa832d52e3659cf>` function to be called after :ref:`rcf_pch_cfg_object::add <doxid-structrcf__pch__cfg__object_1ad7ce244750f7ef2c2b850dd98a9af7f2>`. Instead :ref:`rcf_pch_cfg_object::commit <doxid-structrcf__pch__cfg__object_1a10e8489e786107818fa832d52e3659cf>` is called when a test calls :ref:`cfg_commit() <doxid-group__confapi__base__access_1gaad97002e07f02c462da2aa3298b93fa1>` or :ref:`cfg_commit_fmt() <doxid-group__confapi__base__access_1gabbe8ba443fbb0a5f47f1c1613b7a2d03>` function for newly created object instance.

To add a new instance of ``col_object`` object one could use the following piece of code in their tests:

.. ref-code-block:: c

	rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
	                          "/agent:%s/col_object:%s", agent_name, instance_name);
	if (rc != 0)
	    TEST_FAIL("Failed to add a new instance to 'col_object' collection");

Please note that you can also add an instance of any "read-create" object via :ref:`Configurator <doxid-group__te__engine__conf>` configuration file.

.. ref-code-block:: xml

	<add>
	  <instance oid="/agent:Agt_A/col_object:B"/>
	  <instance oid="/agent:Agt_A/col_object:C"/>
	</add>

These lines will force :ref:`Configurator <doxid-group__te__engine__conf>` to create on start-up two instances of /agent/col_object object on Test Agent Agt_A with instance names A amd C. (For more information about configuration file see :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` section).


.. _doxid-group__confapi_1confapi_usage_set:

Set/Get configuration value operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

More frequently used operations are to Get node instance value or to Set new value to a node instance.

To Set a node instance value use:

* :ref:`cfg_set_instance() <doxid-group__confapi__base__access_1ga8c979af06cc536645552a1f51aad8066>`;

* :ref:`cfg_set_instance_fmt() <doxid-group__confapi__base__access_1ga0ef30d2e2cd16ee8490abb8d45fec2ca>`.

Or corresponding local varsions:

* :ref:`cfg_set_instance_local() <doxid-group__confapi__base__access_1ga45d21edf256b590df80b23e6d6fa88c6>`;

* :ref:`cfg_set_instance_local_fmt() <doxid-group__confapi__base__access_1ga71a414e93926cb59927d20d3d55ec191>`.


.. image:: /static/image/ten_conf_set_instance.png
	:alt: Sequence of events caused by cfg_set_instance() call

One useful feature of object node declaration is specifying dependencies. An object node can be supplied with the list of object nodes on whose values it depends. Then :ref:`Configurator <doxid-group__te__engine__conf>` will track changes of nodes on which another node depends. In case any of these nodes changes its value, :ref:`Configurator <doxid-group__te__engine__conf>` will update the local copy of values of dependent nodes.

For example we can specify:

.. ref-code-block:: xml

	<object oid="/agent/col_object/var1" access="read_write" type="string"/>
	<object oid="/agent/col_object/var2" access="read_write" type="string">
	    <depends oid="/agent/col_object/var1"/>
	</object>

This means that the value of /agent/col_object/var2 depends on /agent/col_object/var1 - any changes to /agent/col_object/var1 may cause change of /agent/col_object/var2.

.. image:: /static/image/ten_conf_set_dep_instance.png
	:alt: Sequence of events caused by cfg_set_instance() call with dependency processing

To understand the necessity of dependencies we need to know how :ref:`Configurator <doxid-group__te__engine__conf>` handles Get operation.

You can use the following functions to Get the value of object instance node:

* :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga46f53025b3ceb5adbfa99ad3424a5869>`;

* :ref:`cfg_get_instance_fmt() <doxid-group__confapi__base__access_1ga8713300c1f12431b4d6dd6d6d8de17d9>`.

.. image:: /static/image/ten_conf_get_instance.png
	:alt: Sequence of events caused by cfg_get_instance() call

Please note that :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga46f53025b3ceb5adbfa99ad3424a5869>` call does not cause any exchange between :ref:`Configurator <doxid-group__te__engine__conf>` and Test Agents, but rather value to return is got from local object instance database.

If you want to get the value from Test Agent you can do one of the following:

* call :ref:`cfg_get_instance_sync() <doxid-group__confapi__base__access_1ga408535c456988093cc5e4bd38bb39961>` or :ref:`cfg_get_instance_sync_fmt() <doxid-group__confapi__base__access_1gae72e82b1419c4ac9c27248f6999fe0a1>` that will first synchronize object instance value with Test Agent and the return an updated value;

* call :ref:`cfg_synchronize() <doxid-group__confapi__base__sync_1ga5634a6c78a078ce3dd839afddc532418>` or :ref:`cfg_synchronize_fmt() <doxid-group__confapi__base__sync_1ga353e4f44de81fa19463c36a3e7399f43>` to synchronize a subtree of configuration nodes and then call ordinary :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga46f53025b3ceb5adbfa99ad3424a5869>` function.


.. image:: /static/image/ten_conf_get_instance_sync.png
	:alt: Sequence of events caused by cfg_get_instance_sync() call

Please note that you should use synced calls only if you are sure that object instance values can change in the backgroud, otherwise it is better to use non-synced calls in order to minimize data exchange between :ref:`Configurator <doxid-group__te__engine__conf>` and Test Agents.

Please note that you can also do set operation in :ref:`Configurator <doxid-group__te__engine__conf>` configuration file.

.. ref-code-block:: xml

	<set>
	  <instance oid="/agent:Agt_A/col_object:B/var1:" value="Some value"/>
	</set>

These lines will force :ref:`Configurator <doxid-group__te__engine__conf>` to run Set operation on start-up for instance /agent:Agt_A/col_object:B/var1:. (For more information about configuration file see :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` section).
