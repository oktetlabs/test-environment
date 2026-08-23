..
  te-parent: te_engine

.. index:: pair: group; Configurator
.. _doxid-group__te__engine__conf:

Configurator
============

.. _doxid-group__te__engine__conf_1te_engine_conf_introduction:

Introduction
~~~~~~~~~~~~

Configurator (Configuration Subsystem, CS) is an application of :ref:`Test Engine <doxid-group__te__engine>` that exports configuration tree. A node of configuration tree can be associated with some software or hardware component controlled or tracked by a Test Agent. Such nodes have well-known path names and require support on Test Agents side. Also :ref:`Configurator <doxid-group__te__engine__conf>` allows creating an arbitrary set of auxiliary configuration nodes that are not associated with anything and rather play role of shared storage or database.

Configurator features:

* stores a configuration database;

* synchronizes the database with Test Agents (See :ref:`Synchronization configuration tree with Test Agent <doxid-group__confapi__base__sync>`);

* provides an API for traversing configuration tree;

* provides an API to tests for the configuration reading and changing (See :ref:`Configuration tree traversal <doxid-group__confapi__base__traverse>` and :ref:`Configuration tree access operations <doxid-group__confapi__base__access>`);

* provides an API to tests and :ref:`Tester <doxid-group__te__engine__tester>` for backuping, verifying and restoring the configuration (See :ref:`Configuration backup manipulation <doxid-group__confapi__base__backup>`);

* provides an API to tests for Test Agents rebooting with or without restoring of the configuration (See :ref:`Test Agent reboot <doxid-group__confapi__base__reboot>`).


.. image:: /static/image/ten_conf_context.png
	:alt: Configurator context in TE


.. _doxid-group__te__engine__conf_1te_engine_conf_tree:

Configuration tree structure - Objects and Instances
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A configuration database consists of two trees: the tree of objects and the tree of instances.

Objects are used to specify the attributes of an abstract configuration item:

* type: integer, string, address (IPv4, IPv6, MAC);

* access rights for the configuration item: read-only, read-write, read-create;

* relations between configuration items (ownership of one item by another item);

* dependencies between items (if changing of one configuration item may affect the existence or value of another configuration item).

For example, MAC address of the network interface would have access rights “read-write”, type “address”, be owned by the interface, which in its turn is owned by the host. An IP address of the network interface (which is also “owned” by this interface) has access rights “read-create” because several IP addresses may be assigned to a single interface.

Each object is identified by a string, which consists of several labels (sub-identifiers) separated by slashes. Each node in the object tree has its own sub-identifier and the full object identifier of the node is a sequence of sub-identifiers of its ancestors.

For example, an object /agent/interface/net_addr is a son of the object /agent/interface which in its turn is a son of the object /agent.

Tree of instances contains information about real configuration items observed by CS on Test Agents, and/or instances created during processing of the configuration file or test requests.

Each instance also has an object identifier. It also consists of a set of labels separated by slashes, but each label contains both a sub-identifier of the corresponding object and an instance name, which identifies uniquely the particular configuration item. Instance name is separated from the sub-identifier by a colon.

For example, the instance /agent:nut/interface:eth0/net_addr:1.2.3.4 of the object /agent/interface/net_addr corresponds to IP address 1.2.3.4 on the network interface eth0 of the station on which Test Agent named nut is running.

It's allowed to use empty instance names. For example, /agent/:nut/interface:eth0/link_addr: identifier is possible because the interface may have only one link address. An object sub-identifier must not contain symbols : (however this symbol is allowed in the instance name), '\*' and ' ' (space).

Empty instance name is used when the object has only one instance.

Instances which belong to /agent: subtree correspond to real configuration items observed on the Test Agents (network interfaces, IP addresses, routes, ARP entries, daemons, etc.). Their change may lead to re-configuration of remote hosts.

The list of basic configuration objects, which is likely to be supported by any Test Agent, can be found in ${TE_BASE}/doc/cm/cm_base.yml file. The rest of ${TE_BASE}/doc/cm covers the remaining subtrees, one YAML file per area; every object there carries a ``d:`` field explaining what it means.

Other subtrees may be considered as information storage: changing instances in these subtrees does not affect the hosts controlled by Test Agents, but may be used to share data between tests.

API to browse configuration trees can be found at :ref:`Configuration tree traversal <doxid-group__confapi__base__traverse>` page.


.. _doxid-group__te__engine__conf_1te_engine_conf_oper:

Configuration Operations
~~~~~~~~~~~~~~~~~~~~~~~~

Two operations are allowed for the objects: Register and Unregister. Register operation describes attributes of a new object (identifier, type, access, dependencies) to :ref:`Configurator <doxid-group__te__engine__conf>`. Unregister command forces :ref:`Configurator <doxid-group__te__engine__conf>` to forget about an object. Usually a command Register is used in the configuration file.

Three operations are allowed for instances: Set (change the value), Add (add a new instance) and Delete (delete an existing instance).

Moreover, :ref:`Configurator <doxid-group__te__engine__conf>` provides an API for read access to the object and instance databases (including different kinds of lookup).

All operations requested in the configuration file and by the tests are stored in the history to allow quick configuration restoring.

API to read and modify configuration tree can be found at :ref:`Configuration tree access operations <doxid-group__confapi__base__access>` page.


.. _doxid-group__te__engine__conf_1te_engine_conf_backup:

Configuration Backup
~~~~~~~~~~~~~~~~~~~~

Configuration backup is a snapshot of the object and instance trees. It is stored in the file and may be associated with the point in the command history. It is possible to create several backups at one or different points of the history.

Backup verification is a simple comparison of the backup (snapshot) with the current state of the database.

Restoring the configuration may be performed using two approaches:

* Restoring by history (used only if a backup is associated with some point in the history):

  * The command list in the history is scanned in reverse order until the backup point is met.

  * The effect of each command is rolled back (for the add command the corresponding instance is deleted, for the delete command the corresponding instance is added etc.).

* Restoring by a backup file (used when the backup is not associated with the history point or when the first approach fails):

  * Current database is synchronized with the snapshot – excessive instances are removed, missed instances are added, incorrect values are changed.

After a successful restoring of the backup from the history or after a successful backup verification the command history may be cut off.

API to manipulate configuration backups can be found at :ref:`Configuration backup manipulation <doxid-group__confapi__base__backup>` page.


.. _doxid-group__te__engine__conf_1te_engine_conf_file:

Configurator Configuration File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:ref:`Configurator <doxid-group__te__engine__conf>` has its own configuration file where it is possible to specify the sequence of configuration operations to be run on :ref:`Configurator <doxid-group__te__engine__conf>` start-up. It is also possible to use a backup-like snapshot as a configuration file, which could be useful when you want to reproduce some problem that happens with particular configuration set-up.


.. _doxid-group__te__engine__conf_1te_engine_conf_file_content:

File Syntax
-----------

Configuration files are written in YAML. (Configurator still reads the older
XML form; it is described in :ref:`Legacy XML syntax <doxid-group__te__engine__conf_1te_engine_conf_file_xml>`
below, but new files should not use it.)

You can find samples under ${TE_BASE}/conf, and a larger real-world set in the
``cs/`` directory of the shared test suite configuration.

A configuration file is a YAML sequence of commands. Each command is one of:

============  ===========================================================
Command       Meaning
============  ===========================================================
register      describe new objects to Configurator
unregister    make Configurator forget objects
add           add object instances
set           change the value of existing instances
delete        remove instances
get           read instances
copy          copy instances
comment       free text, ignored by Configurator
include       process other configuration files
cond          process commands only if a condition holds
============  ===========================================================

The usual shape of a file is therefore:

.. code-block:: yaml


	---
	- comment: |
	    What this file is for.

	- register:
	    - oid: "/agent/env"
	      access: read_create
	      type: string

	- add:
	    - oid: "/agent:Agt_A/env:LOG_LEVEL"
	      value: "info"

``register`` and ``unregister`` take objects, everything else takes instances,
and the two may be mixed and repeated as many times as you like.


Describing objects
------------------

An object entry carries the attributes that define the node --- what it is
called, what it holds and who may change it:

.. code-block:: yaml


	- register:
	    - oid: "/agent/interface"
	      access: read_create
	      type: none
	      d: |
	        Network interface
	        Name: interface name

	    - oid: "/agent/interface/index"
	      access: read_only
	      type: integer
	      volatile: true

	    - oid: "/agent/interface/net_addr"
	      access: read_create
	      type: address
	      depends:
	        - oid: "/agent/interface/link_addr"
	      d: |
	        For IPv6 interfaces net_addr depends on link_addr, because an IPv6
	        link-local address is derived from the link layer address. Once
	        link_addr is updated, the net_addr collection has to be synced
	        between the Test Agent and Configurator.

The attributes an object entry accepts:

================  =============================================================
Attribute         Meaning
================  =============================================================
``oid``           object identifier, the only mandatory attribute
``access``        ``read_only``, ``read_write`` or ``read_create``
``type``          ``none``, ``integer``, ``uint64``, ``string`` or ``address``
``volatile``      ``true`` if the value may change without Configurator knowing
``depends``       list of ``oid`` entries this object depends on
``d``             description; this is what documents the node
``unit``          unit of the value, for numeric nodes
``substitution``  value substitution rules
``name``          marks an instance-keyed collection rather than a singleton
================  =============================================================

The ``d`` and ``name`` fields are not used by Configurator to make decisions ---
they document the node and feed code generators. The configuration model under
${TE_BASE}/doc/cm is written in exactly this form and is the reference for
what every standard node means.


Adding and setting instances
----------------------------

Instance entries take an ``oid`` and, for anything but a ``none`` type node, a
``value``:

.. code-block:: yaml


	- register:
	    - oid: "/agent/ip4_fw"
	      access: read_write
	      type: integer

	- set:
	    # Switch off IPv4 forwarding on Agent 'Agt_A'
	    - oid: "/agent:Agt_A/ip4_fw:"
	      value: 0

Adding instances from the configuration file is a convenient way to tune a test
suite: tests read the values back through the
:ref:`Configurator API <doxid-group__confapi>`, so changing behaviour does not
mean rebuilding anything, only editing a text file.

Note that you can add instances and set values in the /agent subtree the same
way, which is what actually reconfigures the hosts under test.


.. _doxid-group__te__engine__conf_1te_engine_conf_file_features:

Special features
----------------

**Including other files.** A top level file usually does little more than pull
in the parts it needs:

.. code-block:: yaml


	---
	- include:
	    - cs.conf.common.yml
	    - cs.conf.env.yml
	    - cs.conf.hw.yml

**Environment variables.** Values, and conditions, go through ordinary BASH
parameter substitution, so a configuration can be tuned from the environment
without being edited:

.. code-block:: yaml


	- set:
	    # Take the value from TEST_LIBDIR, or use /usr/lib if it is not set.
	    - oid: "/local:Agt_B/libdir:"
	      value: "${TEST_LIBDIR:-/usr/lib}"

	- add:
	    - oid: "/local:Agt_A/env:LOG_LEVEL"
	      value: "${TEST_LOG_LEVEL}"

**Conditions.** An ``if`` on a single entry skips just that entry:

.. code-block:: yaml


	- add:
	    - if: ${TE_ENV_IUT_NET_DRIVER} != ""
	      oid: "/local:${TE_IUT_TA_NAME}/net_driver:"
	      value: "${TE_ENV_IUT_NET_DRIVER}"

while a ``cond`` command guards a whole group, with an optional ``else``:

.. code-block:: yaml


	- cond:
	    if: ${TE_LOG_LISTENER} != ""
	    then:
	      - include:
	          - cm_netconsole.yml
	          - inc.log_listener.yml

This is how one set of configuration files serves many testbeds: the variables
come from the environment for the chosen configuration, and the conditions pick
the matching pieces.


.. _doxid-group__te__engine__conf_1te_engine_conf_file_xml:

Legacy XML syntax
-----------------

Configurator also reads configuration files in an older XML form. It is still
supported and plenty of it survives in existing suites, but it has no
conditions and is not being extended, so prefer YAML for anything new.

The structure mirrors the YAML one:

.. code-block:: xml


	<?xml version="1.0"?>
	<history>
	  <register>
	    <object oid="/agent/env" access="read_create" type="string"/>
	    <object oid="/agent/interface/index"
	            access="read_only" type="integer" volatile="true"/>
	    <object oid="/agent/interface/net_addr" access="read_create" type="address">
	      <depends oid="/agent/interface/link_addr"/>
	    </object>
	  </register>

	  <add>
	    <instance oid="/agent:Agt_A/env:LOG_LEVEL" value="${TEST_LOG_LEVEL}"/>
	  </add>

	  <set>
	    <instance oid="/agent:Agt_A/ip4_fw:" value="0"/>
	  </set>
	</history>

Files are pulled in with XInclude rather than the ``include`` command:

.. code-block:: xml


	<xi:include xmlns:xi="http://www.w3.org/2003/XInclude"
	            href="cs.conf.common" parse="xml"/>

The XSD schema for this form is at ${TE_BASE}/doc/xsd/cs_config.xsd.
