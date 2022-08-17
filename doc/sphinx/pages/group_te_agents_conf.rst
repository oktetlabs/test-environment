..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Test Agents: Creating new configuration nodes in Test Agent
.. _doxid-group__te__agents__conf:

Test Agents: Creating new configuration nodes in Test Agent
===========================================================

.. toctree::
	:hidden:



.. _doxid-group__te__agents__conf_1Introduction:

Introduction
~~~~~~~~~~~~

Configurator API exports functionality that makes it possible to:

* Register/Unregister objects of configuration tree;

* traverse a configuration tree (get parent, neighbor and child nodes);

* Add new object instances;

* Delete existing object instances;

* change the value of an object instance;

* Get the value of an object instace.

Details of API usage can be found at :ref:`API Usage: Configurator API <doxid-group__confapi>` page.

Here we will explain how to add support for a new objects and instances on Test Agent side.





.. _doxid-group__te__agents__conf_1te_agt_extending_cfg_design:

Design of configuration nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

First thing to do while creating new configuration nodes is to design structure of new nodes, their relationship, access rights and types of value kept by them.

Descriptions of configuration nodes are kept under doc/cm directory (cm is derived from Configuration Model). You should update one of existing files or create a new one with information about your new nodes.

The format of Configuration Model files complies to format of :ref:`Configurator <doxid-group__te__engine__conf>` backups, which makes it possible to use lines from these files in :ref:`Configurator <doxid-group__te__engine__conf>` configuration file when you need to add this or that object into your run time configuration.

Read some more information about configuration objects and instances at :ref:`Configuration tree structure - Objects and Instances <doxid-group__te__engine__conf_1te_engine_conf_tree>`.

In our example we will define the following objects:

.. ref-code-block:: xml

	<!-- An example of read-only object with integer type of value.
	     Name: an arbitrary string -->
	<object oid="/agent/ro_object" access="read_only" type="integer"/>

	<!-- An example of read-write object with address type of value.
	     Name: an arbitrary string -->
	<object oid="/agent/rw_object" access="read_write" type="address"/>


	<!-- An example of collection object.
	     Name: an arbitrary string -->
	<object oid="/agent/col_object" access="read_create" type="none"/>

	<!-- An example of a child node of a collection object.
	     Name: an arbitrary string -->
	<object oid="/agent/col_object/var" access="read_only" type="string"/>





.. _doxid-group__te__agents__conf_1te_agt_extending_cfg_init:

Test Agent initialization
~~~~~~~~~~~~~~~~~~~~~~~~~

On Test Agent start-up we should pass control to PCH (Portable Command Handler) main loop function :ref:`rcf_pch_run() <doxid-group__rcf__pch_1ga674454c23e4d97f6597fedb057aaf835>`. As a part of initialization process for configuration support :ref:`rcf_pch_run() <doxid-group__rcf__pch_1ga674454c23e4d97f6597fedb057aaf835>` function calls:

* :ref:`rcf_ch_conf_init() <doxid-group__rcf__ch__cfg_1gade2d2cfc87dcd5ba89eb0f7a994a2ae6>` - to do all Test Agent specific initialization.

In :ref:`rcf_ch_conf_init() <doxid-group__rcf__ch__cfg_1gade2d2cfc87dcd5ba89eb0f7a994a2ae6>` function you should register all object nodes that Test Agent will support during its operation. Nodes are registered to RCF PCH with :ref:`rcf_pch_add_node() <doxid-group__rcf__pch_1gaf3f0c34fb03ec93f821bc5b4007192bb>` function where you specify parent object path and pointer to object description structure of :ref:`rcf_pch_cfg_object <doxid-structrcf__pch__cfg__object>` type.

.. image:: /static/image/ta_conf_call_diagram.png
	:alt: Test Agent configuration support event sequence diagram

To specify node description you should use :ref:`Configuration node definition macros <doxid-group__rcf__ch__cfg__node__def>`.

For example definition of nodes of our sample might look like:

.. ref-code-block:: c

	RCF_PCH_CFG_NODE_RO(ro_node, "ro_object", NULL, NULL, ro_object_get);
	RCF_PCH_CFG_NODE_RW(rw_node, "rw_object", NULL,
	                    &ro_node, /* neighbor node */
	                    rw_object_get, rw_object_set);

	RCF_PCH_CFG_NODE_RO(col_obj_var, "var", NULL, NULL, var_get);
	RCF_PCH_CFG_NODE_COLLECTION(col_node, "col_object",
	                            &col_obj_var, /* son node */
	                            &rw_node, /* bother node (neighbor node) */
	                            NULL, NULL, col_list, NULL);

These definitions should be placed in the source of Test Agent. Then in the implementation of :ref:`rcf_ch_conf_init() <doxid-group__rcf__ch__cfg_1gade2d2cfc87dcd5ba89eb0f7a994a2ae6>` you should put the following line of code that will register your configuration subtree:

.. ref-code-block:: c

	int
	rcf_ch_conf_init(void)
	{
	    ...
	    if (rcf_pch_add_node("/agent", &col_node) != 0)
	    {
	        /* Do error handling */
	    }
	    ...
	}





.. _doxid-group__te__agents__conf_1te_agt_extending_cfg_handlers:

Implementation of node handlers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The main thing to be done is to define node handlers that in most cases interact with system interfaces to get necessary value or to update system settings.

All data exchange between PCH and CH is done in terms of strings. For example if we have a node with the value of integer type we should anyway return value in a character string form and on set operation the value passed to a callback is in a character string form.

Implementation of handlers for ``ro_object`` and ``rw_object`` objects might look like the following:

.. ref-code-block:: c

	static te_errno
	ro_object_get(unsigned int gid, const char *oid, char *value, const char *instN,...)
	{
	    int val;

	    /*
	     * Some comments about function parameters:
	     * gid - Identifier associated with an operation
	     *       (make sense for SET and COMMIT operations);
	     * oid - Object instance identifier whose value to get;
	     * value - buffer of RCF_MAX_VAL size to fill with value;
	     * instN... - the list of instance names of nodes
	     *            in "oid" starting from the son of "/agent" node.
	     *            For example for "oid": "/agent:Agt_A/ro_object:"
	     *            "instN" will be an empty string, and the next
	     *            element of the list will be NULL.
	     *            for "oid": "/agent:Agt_A/col_object:a/var:"
	     *            "instN" will be "a", the next element will be an
	     *            empty string and the third element will be NULL.
	     */

	    ...
	    /* Get value of integer type */
	    ...
	    snprintf(value, RCF_MAX_VAL, "%d", val);
	    return 0;
	}

	static te_errno
	rw_object_get(unsigned int gid, const char *oid, char *value, const char *instN,...)
	{
	    ...
	    /* Get the value of address type */
	    ...
	    /* Save address value in the return buffer in string format */
	    if (inet_ntop(af, addr, value, RCF_MAX_VAL) == NULL)
	        return TE_OS_RC(TE_TA_UNIX, errno);

	    return 0;
	}

	static te_errno
	rw_object_set(unsigned int gid, const char *oid, const char *value, const char *instN,...)
	{
	    if (inet_pton(AF_INET, value, addr_buf) == 1)
	        af= AF_INET;
	    else if (inet_pton(AF_INET6, value, addr_buf) == 1)
	        af = AF_INET6;
	    else
	        ...

	    /* Do something with address value */

	    return 0;
	}

Please note that for scalar (not collection) objects instance name will be an empty string.

Implementation of handlers for ``col_object`` might look like the following:

.. ref-code-block:: c

	static te_errno
	var_get(unsigned int gid, const char *oid, char *value, const char *instN,...)
	{
	    /*
	     * oid - "/agent:<agt_name>/col_object:<inst_name>/var:",
	     *       where "agt_name" is an agent name where this code runs,
	     *             "inst_name" is collection object instance name,
	     *             in our sample it is one of: '1', '2', '3', '4'.
	     * value - buffer of RCF_MAX_VAL size to fill with some string value;
	     * instN... - the list of instance names of nodes.
	     *            For our sample "instN" would be one of: '1', '2', '3', '4'.
	     *            Next element of the list would be an empty string
	     *            (as the instance name of "var" node), and the third entry
	     *            would be NULL.
	     */
	    snprintf(value, RCF_MAX_VAL, "STRING VALUE for '%s'", instN);

	    return 0;
	}

	/*
	 * The function returns the list of instance names of "col_object" object.
	 * Instance names are arbitrary strings that better fits your design.
	 */
	static te_errno
	col_list(unsigned int gid, const char *oid, char **list, const char *instN,...)
	{
	    /*
	     * Here we return predefined list of four instance names:
	     * '1', '2', '3', '4'.
	     * More natuarally this list would grow dynamically depending on
	     * some system attributes. The list can vary from call to call.
	     */
	    *list = strdup("1 2 3 4");
	    if (*list == NULL)
	        TE_RC(TE_TA_UNIX, TE_ENOMEM);

	    return 0;
	}

Collection nodes can support synamic add/del operations which require implementation of corresponding handlers:

.. ref-code-block:: c

	RCF_PCH_CFG_NODE_COLLECTION(col_node, "col_object",
	                            &col_obj_var, /* son node */
	                            &rw_node, /* bother node (neighbor node) */
	                            col_add, col_del, col_list, NULL);

	static te_errno
	col_add(unsigned int gid, const char *oid, const char *value, const char *instN,...)
	{
	    /*
	     * Add a new instance whose name is "instN".
	     * In case we successfully added an entry, the next call to
	     * col_list() function shall include this new instance name
	     * into the list of instance names.
	     */

	    /* Install a new entry that is associated with name instN */

	    return 0;
	}

	static te_errno
	col_del(unsigned int gid, const char *oid, const char *instN,...)
	{
	    /*
	     * Remove an entry whose instance name is instN.
	     * The next call to col_list() function shall not include
	     * this instance name in returned list of instance names.
	     */

	    /* Deinstall an entry from the system */

	    return 0;
	}

Apart from basic get/set/list/add/del handlers there is one more handler :ref:`rcf_pch_cfg_object::commit <doxid-structrcf__pch__cfg__object_1a10e8489e786107818fa832d52e3659cf>` that is additionally called for each successful set/add/del operation. There are two modes of commit call:

* immediate call (commit is called after successful completion of set/add/del operation);

* postponed call (commit functions are called after a series of set/add/del operations and duplicate commit calls are avoided).



.. image:: /static/image/ta_conf_commit_immediate.png
	:alt: Commit calls in case of ordinary SET calls



.. image:: /static/image/ta_conf_commit_group.png
	:alt: Commit calls in case of group context SET calls

Please note that in the group call case the particular commit function is called only once - in our sample commit_X() is called once though it is a commit function for /X/a and /X/c objects whose instances are updated.

Usually the commit function is specified in parent node and children nodes refer to parent node with :ref:`RCF_PCH_CFG_NODE_RWC() <doxid-group__rcf__ch__cfg__node__def_1ga62bd1bcfb958d8bc6bb88486bdc7477f>` macro:

.. ref-code-block:: c

	static rcf_pch_cfg_object col_node;
	RCF_PCH_CFG_NODE_RWC(col_obj_var2, "var2", NULL, NULL,
	                     var2_get, var2_set,
	                     &col_node);
	RCF_PCH_CFG_NODE_RWC(col_obj_var1, "var1", NULL,
	                     &col_obj_var2, /* brother node */
	                     var1_get, var1_set,
	                     &col_node);
	RCF_PCH_CFG_NODE_COLLECTION(col_node, "col_object",
	                            &col_obj_var1, /* son node */
	                            &rw_node, /* bother node (neighbor node) */
	                            col_add, col_del, col_list, col_commit);

Please note that add/del/set handlers have no way to detect what the calling mode takes place (immediate commit or postponed commit). The only thing to keep in mind is that commit will be called anyway.

