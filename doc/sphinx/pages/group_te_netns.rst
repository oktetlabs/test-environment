..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Network namespaces
.. _doxid-group__te__netns:

Network namespaces
==================

.. toctree::
	:hidden:



.. _doxid-group__te__netns_1te_netns_introduction:

Introduction
~~~~~~~~~~~~

This document describes how to configure and use network namespaces in Test Environment.

As known linux provides ability to have a few network namespaces on a host. Network namespace feature is dedicated to isolate and virtualize system network resources of a collection of processes. In particularly the following resources:

* network interfaces and IP addresses;

* ARP table;

* routing table;

* firewall rules;

* kernel network options.

The most important features which are supported:

* create/destroy or grab a network namespace;

* move network interfaces to/from a namespace;

* configuration of complex interfaces chains (e.g. eth-bond-vlan-macvlan) where any part of a such chain can be moved to different namespace;

* roll back all network configurations after tests execution.





.. _doxid-group__te__netns_1te_netns_tree:

Configurator tree
~~~~~~~~~~~~~~~~~

The following configurator subtree can be used to create and destroy network namespaces, move network interfaces between namespaces.

=============================  =======  ======================================================================================================
Object ID                      Type     Description
=============================  =======  ======================================================================================================
/agent/namespace               RO none  Generic linux namespaces management subtree.
/agent/namespace/net           RC none  Network namespaces list. This node can be used to create new network namespaces or grab existing ones.
/agent/namespace/net/inteface  RC none  Interfaces which are moved from the current agent namespace to the specified.
=============================  =======  ======================================================================================================

XML code to register objects:

.. ref-code-block:: cpp

	<register>
	  <object oid="/agent/namespace" access="read_only" type="none"/>
	  <object oid="/agent/namespace/net" access="read_create" type="none">
	    <depends oid="/agent/rsrc"/>
	  </object>
	  <object oid="/agent/namespace/net/interface" access="read_create" type="none"/>
	</register>

There is test API to work with the tree, find details in :ref:`Agent namespaces configuration <doxid-group__tapi__namespaces>`.





.. _doxid-group__te__netns_1te_netns_agent:

Test agent in a network namespace
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is required to run Test Agent in a network namespace to manage its resources (interfaces, routes, kernel options, etc) properly using Configurator - track values, change them and roll back changes.

The simplest way to run anything in a specified network namespace is using standard tools. For linux it is **ip netns** :

.. ref-code-block:: cpp

	ip netns exec <namespace name> <command>

So to run test agent in net namespace it could be enough just to add *"ip netns exec <ns_name>"* in **confstr** of :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` to use the line as prefix for TA running command. But unfortunately it is impossible to setup a network namespace using Configurator and run a test agent there using config files only, because RCF is run before Configurator. Luckily RCF has test API which can be used to run new test agent, so net namespaces + TA configuration can be done in the following way:

#. Setup a net namespace using initial Configurator configs or in test suite prologue.

#. Do the following in the prologue:

   #. configure DUT network so that namespaced TA could communicate with TEN;

   #. run test agent(-s) in the namespace using RCF TAPI;

   #. process extra Configurator configs to prepare namespaced TA if required (see :ref:`Configuration example <doxid-group__te__netns_1te_netns_run_ta_eg>`).

There is some test API implemented to simplify the listed steps, see subsections :ref:`Test API <doxid-group__te__netns_1te_netns_tapi>` for details.



.. _doxid-group__te__netns_1te_netns_tapi:

Test API
--------

There are two ways to organize TEN<->TA communication channel:

#. MAC-level switching (host-shared bridge): the main network interface holds both the host's IP and the child NS IP addresses.

#. IP-level switching: using masquerade and IP forwarding.

The following two functions can be used (alternatively) to create a network namespace and configure the options above accordingly:

#. :ref:`tapi_netns_create_ns_with_macvlan() <doxid-group__tapi__namespaces_1gae4d4c5c946acf8d4f36986c3a7352a85>`

#. :ref:`tapi_netns_create_ns_with_net_channel() <doxid-group__tapi__namespaces_1gad617211e067a647d0730f2f1b73bff38>`

Then function :ref:`tapi_netns_add_ta() <doxid-group__tapi__namespaces_1gae37e6ce59ca62fe31c9deba812488f36>` can be used to create test agent in the prepared namespace.

Note, function :ref:`tapi_netns_destroy_ns_with_macvlan() <doxid-group__tapi__namespaces_1ga1b94e2b0bab72a524fb756ddede3f26c>` must be called (in epilogue) to neutralise :ref:`tapi_netns_create_ns_with_macvlan() <doxid-group__tapi__namespaces_1gae4d4c5c946acf8d4f36986c3a7352a85>` configurations.





.. _doxid-group__te__netns_1te_netns_run_ta_eg:

Configuration example
---------------------

The following code can be executed for example in test suite prologue.

.. ref-code-block:: cpp

	/*
	 * The following code does:
	 *  1. Create auxiliary network namespace @p ns_name using test agent @p ta.
	 *  2. Configure the communication channel TEN<->netns.
	 *  3. Create new test agent @p ta_ns in the new network namespace @p ns_name.
	 *  4. Move @p iut_if to the namespace @p ns_name.
	 *  5. Process auxiliary Configurator config file @p cs_cfg.
	 *
	 * Note! All hardcoded constants usually can be obtained from test suite
	 * Configurator tree and environments.
	 */
	static void
	setup_netns(void)
	{
	    const char *host        = "Agt_A_hostname";
	    const char *ta          = "Agt_A";
	    const char *ta_ns       = "Agt_A_netns";
	    const char *ta_type     = "linux64";
	    const char *ns_name     = "aux_netns";
	    const char *ctl_if      = "eth0";
	    const char *iut_if      = "eth2";
	    const char *macvlan     = "ctl.mvlan";
	    const char *cs_cfg      = "aux_cs_config";
	    int         rcfport     = 30667;
	    char        addr[RCF_MAX_NAME] = {};

	    CHECK_RC(tapi_netns_create_ns_with_macvlan(ta, ns_name, ctl_if, macvlan,
	                                               addr, sizeof(addr)));
	    CHECK_RC(tapi_netns_add_ta(host, ns_name, ta_ns, ta_type, rcfport, addr));
	    CHECK_RC(tapi_host_ns_if_change_ns(ta, iut_if, ns_name, ta_ns));

	    /* Optionally auxiliary Configurator file can be processed - for example
	     * to apply some configurations on the new added test agent `ta_ns`. */
	    CHECK_RC(cfg_restore_backup_nohistory(cs_cfg));
	}







.. _doxid-group__te__netns_1te_netns_relations:

Interfaces relations through namespaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It can be very tricky (if possible at all) to track network interfaces relations through namespaces using system tools due to the isolation. On the other hand test agents have completely independent subtrees in TE.

So the following configurator subtree was added to describe relations between agents, namespaces and network interfaces on a host.

==================================  =========  ==========================================================================================================
Object ID                           Type       Description
==================================  =========  ==========================================================================================================
/local/host                         RO none    This configurator subtree describes relations between agents, namespaces and network interfaces on a host.
/local/host/agent                   RC none    Test Agent subtree.
/local/host/agent/netns             RW string  Network namespace where the agent is run.
/local/host/agent/interface         RC none    Network interface subtree.
/local/host/agent/interface/parent  RC string  Link to a parent interface (e.g. /local:/host:h1/agent:Agt_A/interface:eth1).
==================================  =========  ==========================================================================================================

XML code to register objects:

.. ref-code-block:: cpp

	<register>
	  <object oid="/local/host" access="read_create" type="none"/>
	  <object oid="/local/host/agent" access="read_create" type="none"/>
	  <object oid="/local/host/agent/netns" access="read_write" type="string"/>
	  <object oid="/local/host/agent/interface" access="read_create" type="none"/>
	  <object oid="/local/host/agent/interface/parent" access="read_create" type="string"/>
	</register>

Instances of the subtree **must** be changed due to the following actions:

* add/remove Test Agent;

* add/remove net namespace;

* add/remove interface;

* slave/release interface to/from an aggregation;

* moving interface to a net namespace.

It is user responsibility to keep the subtree in the correct state and up-to-date.

Few simple rules to avoid extra headache:

#. Grab all resources (interfaces) in initial Configurator configs (before testing).

#. Fill in /local/host subtree accordingly to the step above in initial configs.

#. Don't use Configurator API directly to setup things listed above. All the actions can be done using test API like tapi_cfg_base_if\_\* or tapi_netns\_\*. It helps to care about agent resources and other things like synchronization of subtree /local/host.

There is implemented test API which should simplify working with the tree, find details in :ref:`Agents, namespaces and interfaces relations <doxid-group__tapi__host__ns>`.

It is not necessary to have this subtree configured to control network namespaces using TE. But it is required for work of :ref:`Advanced test API <doxid-group__te__netns_1te_netns_advanced_tapi>`.





.. _doxid-group__te__netns_1te_netns_advanced_tapi:

Advanced test API
~~~~~~~~~~~~~~~~~



.. _doxid-group__te__netns_1te_netns_adv_tapi_proc:

Test API to manage system parameters
------------------------------------

There is Configurator subtree /agent/sys which is used to control system parameters (like files /proc/sys/net in linux). Some options may absent in auxiliary network namespace, but exist in default namespace. In this case a test writer can be interested in getting or setting an option in default net namespace if it does not exist in the target agent namespace. The following four TAPI functions help to do it:

* :ref:`tapi_cfg_sys_ns_get_int() <doxid-group__tapi__cfg__sys_1ga6b70d500da910939912bbf6110831efc>`

* :ref:`tapi_cfg_sys_ns_set_int() <doxid-group__tapi__cfg__sys_1ga13716406d802c242be37928d431b3040>`

* :ref:`tapi_cfg_sys_ns_get_str() <doxid-group__tapi__cfg__sys_1gaa4bc411124a2de258383d9965423b51e>`

* :ref:`tapi_cfg_sys_ns_set_str() <doxid-group__tapi__cfg__sys_1ga04d581a8e59418aeae7e8fa70af2294e>`





.. _doxid-group__te__netns_1te_netns_adv_tapi_eth_feature:

Test API to set interface feature
---------------------------------

The following function can be used to set a feature value of an ethernet interface and all its parents if they are.

* :ref:`tapi_eth_feature_set_all_parents() <doxid-group__tapi__conf__eth_1ga9014cda041f828426edd0b6ab2b14c16>`

The target interface ancestors can belong to different test agents and net namespaces.





.. _doxid-group__te__netns_1te_netns_adv_tapi_mtu:

Test API to set interface MTU
-----------------------------

There are TAPI functions to set MTU on the interface and on its relatives if required:

* :ref:`tapi_set_if_mtu_smart() <doxid-group__te__lib__rpc__misc_1ga7d11d94b74165f226bcc4038cde07a07>`

* :ref:`tapi_set_if_mtu_smart2() <doxid-group__te__lib__rpc__misc_1ga8e59bb5b1eb3186d9e6ac7817cd0b239>`, :ref:`tapi_set_if_mtu_smart2_rollback() <doxid-group__te__lib__rpc__misc_1ga8ab4e58bbef868426f26ed2238990ba8>`

Note, Configurator may fail to roll back MTU changes in some complex interface configuration schemas. In this case it is worth to use function :ref:`tapi_set_if_mtu_smart2() <doxid-group__te__lib__rpc__misc_1ga8e59bb5b1eb3186d9e6ac7817cd0b239>` to apply MTU changes and then function :ref:`tapi_set_if_mtu_smart2_rollback() <doxid-group__te__lib__rpc__misc_1ga8ab4e58bbef868426f26ed2238990ba8>` to undo them.

