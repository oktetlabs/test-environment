..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; API Usage: Configurator API
.. _doxid-group__confapi:

API Usage: Configurator API
===========================

.. toctree::
	:hidden:

	/generated/group_confapi_base.rst
	group_tapi_conf.rst



.. _doxid-group__confapi_1confapi_introduction:

Usage of Configurator API from test scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test scenarios should use functions exported via:

* base :ref:`API: Configurator <doxid-group__confapi__base>` (``lib/confapi/conf_api.h``);

* semantic based interface :ref:`TAPI: Test API for configuration nodes <doxid-group__tapi__conf>`.

Here we will show how to play with samples discussed at te_agents_conf page.





.. _doxid-group__confapi_1confapi_usage_conf:

Tuning Configurator configuration file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to let :ref:`Configurator <doxid-group__te__engine__conf>` know about instances of new nodes we should register these new object nodes in configurator tree. Then :ref:`Configurator <doxid-group__te__engine__conf>` will be able to get instances of these objects from te_agents. Otherwise :ref:`Configurator <doxid-group__te__engine__conf>` ignores node instances whose object nodes were not registered.

.. image:: /static/image/ten_conf_startup.png
	:alt: Configurator start-up event flow

When :ref:`Configurator <doxid-group__te__engine__conf>` starts it processes configurator configuration file that keeps object descriptions that need to be registered in local tree of objects (see arrow [2]).

Configuration file can also keep rules to add object instances, but these instances can not be applied for /agent subtree. /agent subtree is in control by te_agents.

:ref:`Configurator <doxid-group__te__engine__conf>` should ask te_agents about these instances that is why it call :ref:`rcf_ta_cfg_get() <doxid-group__rcfapi__base_1ga92bb850be576f887a71251e4d86ccd45>` function with wildcard object instance identifier (arrow [4] in the figure).

When :ref:`Configurator <doxid-group__te__engine__conf>` receives a reply with the list of object instance names it checks whether an instance name has corresponding object node in its local object tree. If yes, then it adds an instance into its instance configuration tree, otherwise it ignores an instance name and tests will not be able to access those instances until they register corresponding object nodes in :ref:`Configurator <doxid-group__te__engine__conf>` (see arrow [6]).

Regarding an example described at te_agents_conf page, we should add the following lines into :ref:`Configurator <doxid-group__te__engine__conf>` configuration file to let :ref:`Configurator <doxid-group__te__engine__conf>` know about our new supported object instances:

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

* :ref:`cfg_add_instance_fmt() <doxid-group__confapi__base__access_1ga53648a566529159c2f6831541a00d9c3>`.

The following diagram shows the sequence of events caused by calling any of these functions.

.. image:: /static/image/ten_conf_add_instance.png
	:alt: Sequence of events caused by cfg_add_instance() call

Similar things happen when you call a function to delete an object instance:

* :ref:`cfg_del_instance() <doxid-group__confapi__base__access_1ga5f6fdcc65f8ebbc6b0581b383141a368>`;

* :ref:`cfg_del_instance_fmt() <doxid-group__confapi__base__access_1gad03c25531ecc9ccaeeba5c96c87238c9>`.

You can also use *local* version of instance add functions:

* :ref:`cfg_add_instance_local() <doxid-group__confapi__base__access_1ga8ddd7977bcfa71c75cb5bf6e23ceee07>`;

* :ref:`cfg_add_instance_local_str() <doxid-group__confapi__base__access_1ga4365e5060e03cba6145615bf6df7c965>`;

* :ref:`cfg_add_instance_local_fmt() <doxid-group__confapi__base__access_1ga37be0a1caccba48210e75d655d48d5fd>`.

The only difference is that these functions will not cause :ref:`rcf_pch_cfg_object::commit <doxid-structrcf__pch__cfg__object_1a10e8489e786107818fa832d52e3659cf>` function to be called after :ref:`rcf_pch_cfg_object::add <doxid-structrcf__pch__cfg__object_1ad7ce244750f7ef2c2b850dd98a9af7f2>`. Instead :ref:`rcf_pch_cfg_object::commit <doxid-structrcf__pch__cfg__object_1a10e8489e786107818fa832d52e3659cf>` is called when a test calls :ref:`cfg_commit() <doxid-group__confapi__base__access_1gaf4e4bb6bee1e3e9a1336a88a007172cf>` or :ref:`cfg_commit_fmt() <doxid-group__confapi__base__access_1gae8f98dfd0646b2ef86aeefec7291aa57>` function for newly created object instance.

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

* :ref:`cfg_set_instance() <doxid-group__confapi__base__access_1gabe22c59fbbef6fc79f6f22ed91fff4c3>`;

* :ref:`cfg_set_instance_fmt() <doxid-group__confapi__base__access_1ga611a5f1a85332c38cd2e38598e78198d>`.

Or corresponding local varsions:

* :ref:`cfg_set_instance_local() <doxid-group__confapi__base__access_1ga45d21edf256b590df80b23e6d6fa88c6>`;

* :ref:`cfg_set_instance_local_fmt() <doxid-group__confapi__base__access_1ga2b7d39a2044fd1175cd620919e507bae>`.



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

* :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga958125eed1df71e6a6bf4d96056e01d2>`;

* :ref:`cfg_get_instance_fmt() <doxid-group__confapi__base__access_1ga35697384772bdf748c2e348ae22dedf7>`.

.. image:: /static/image/ten_conf_get_instance.png
	:alt: Sequence of events caused by cfg_get_instance() call

Please note that :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga958125eed1df71e6a6bf4d96056e01d2>` call does not cause any exchange between :ref:`Configurator <doxid-group__te__engine__conf>` and te_agents, but rather value to return is got from local object instance database.

If you want to get the value from Test Agent you can do one of the following:

* call :ref:`cfg_get_instance_sync() <doxid-group__confapi__base__access_1ga408535c456988093cc5e4bd38bb39961>` or :ref:`cfg_get_instance_sync_fmt() <doxid-group__confapi__base__access_1ga23c24b00c4113d08b4f0636d8f14162a>` that will first synchronize object instance value with Test Agent and the return an updated value;

* call :ref:`cfg_synchronize() <doxid-group__confapi__base__sync_1gace10730546274a648c2e7534a29adb32>` or :ref:`cfg_synchronize_fmt() <doxid-group__confapi__base__sync_1gad769295f5a2e8ee80640f88124a84a26>` to synchronize a subtree of configuration nodes and then call ordinary :ref:`cfg_get_instance() <doxid-group__confapi__base__access_1ga958125eed1df71e6a6bf4d96056e01d2>` function.



.. image:: /static/image/ten_conf_get_instance_sync.png
	:alt: Sequence of events caused by cfg_get_instance_sync() call

Please note that you should use synced calls only if you are sure that object instance values can change in the backgroud, otherwise it is better to use non-synced calls in order to minimize data exchange between :ref:`Configurator <doxid-group__te__engine__conf>` and te_agents.

Please note that you can also do set operation in :ref:`Configurator <doxid-group__te__engine__conf>` configuration file.

.. ref-code-block:: xml

	<set>
	  <instance oid="/agent:Agt_A/col_object:B/var1:" value="Some value"/>
	</set>

These lines will force :ref:`Configurator <doxid-group__te__engine__conf>` to run Set operation on start-up for instance /agent:Agt_A/col_object:B/var1:. (For more information about configuration file see :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` section).

|	:ref:`API: Configurator<doxid-group__confapi__base>`
|		:ref:`Configuration backup manipulation<doxid-group__confapi__base__backup>`
|		:ref:`Configuration tree traversal<doxid-group__confapi__base__traverse>`
|		:ref:`Contriguration tree access operations<doxid-group__confapi__base__access>`
|		:ref:`Synchronization configuration tree with Test Agent<doxid-group__confapi__base__sync>`
|		:ref:`Test Agent reboot<doxid-group__confapi__base__reboot>`
|	:ref:`TAPI: Test API for configuration nodes<doxid-group__tapi__conf>`
|		:ref:`ARL table configuration<doxid-group__tapi__conf__arl>`
|		:ref:`Agent namespaces configuration<doxid-group__tapi__namespaces>`
|		:ref:`Agents, namespaces and interfaces relations<doxid-group__tapi__host__ns>`
|		:ref:`Bonding and bridging configuration<doxid-group__tapi__conf__aggr>`
|		:ref:`CPU topology configuration of Test Agents<doxid-group__tapi__conf__cpu>`
|		:ref:`Command monitor TAPI<doxid-group__tapi__cmd__monitor__def>`
|		:ref:`DHCP Server configuration<doxid-group__tapi__conf__dhcp__serv>`
|		:ref:`DUT serial console access<doxid-group__tapi__conf__serial>`
|		:ref:`Environment variables configuration<doxid-group__tapi__conf__sh__env>`
|		:ref:`Ethernet PHY configuration<doxid-group__tapi__conf__phy>`
|		:ref:`Ethernet interface features configuration<doxid-group__tapi__conf__eth>`
|		:ref:`IP rules configuration<doxid-group__tapi__conf__ip__rule>`
|		:ref:`IPv6 specific configuration<doxid-group__tapi__conf__ip6>`
|		:ref:`Kernel modules configuration<doxid-group__tapi__conf__modules>`
|		:ref:`L2TP configuration<doxid-group__tapi__conf__l2tp>`
|		:ref:`Local subtree access<doxid-group__tapi__conf__local>`
|		:ref:`Manipulation of network address pools<doxid-group__tapi__conf__net__pool>`
|		:ref:`NTP daemon configuration<doxid-group__tapi__conf__ntpd>`
|		:ref:`Neighbour table configuration<doxid-group__tapi__conf__neigh>`
|		:ref:`Network Base configuration<doxid-group__tapi__conf__base__net>`
|		:ref:`Network Emulator configuration<doxid-group__tapi__conf__netem>`
|		:ref:`Network Interface configuration<doxid-group__tapi__conf__iface>`
|		:ref:`Network Switch configuration<doxid-group__tapi__conf__switch>`
|		:ref:`Network sniffers configuration<doxid-group__tapi__conf__sniffer>`
|		:ref:`Network statistics access<doxid-group__tapi__conf__stats>`
|		:ref:`Network topology configuration of Test Agents<doxid-group__tapi__conf__net>`
|		:ref:`PPPoE Server configuration<doxid-group__tapi__conf__pppoe>`
|		:ref:`Processes configuration<doxid-group__tapi__conf__process>`
|		:ref:`Queuing Discipline configuration<doxid-group__tapi__conf__qdisc>`
|			:ref:`tc qdisc TBF configuration<doxid-group__tapi__conf__tbf>`
|		:ref:`Routing Table configuration<doxid-group__tapi__conf__route>`
|		:ref:`Serial console parsers configuration<doxid-group__tapi__conf__serial__parse>`
|		:ref:`Solarflare PTP daemon configuration<doxid-group__tapi__conf__sfptpd>`
|		:ref:`System parameters configuration<doxid-group__tapi__cfg__sys>`
|		:ref:`TA system options configuration (OBSOLETE)<doxid-group__tapi__conf__proc>`
|		:ref:`Test API to handle a cache<doxid-group__tapi__cache>`
|		:ref:`VTund configuration<doxid-group__tapi__conf__vtund>`
|		:ref:`Virtual machines configuration<doxid-group__tapi__conf__vm>`
|		:ref:`XEN configuration<doxid-group__tapi__conf__xen>`
|		:ref:`iptables configuration<doxid-group__tapi__conf__iptable>`


