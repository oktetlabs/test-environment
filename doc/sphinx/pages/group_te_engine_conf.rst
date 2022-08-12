.. index:: pair: group; Configurator
.. _doxid-group__te__engine__conf:

Configurator
============

.. toctree::
	:hidden:

	group_confapi.rst



.. _doxid-group__te__engine__conf_1te_engine_conf_introduction:

Introduction
~~~~~~~~~~~~

Configurator (Configuration Subsystem, CS) is an application of :ref:`Test Engine <doxid-group__te__engine>` that exports configuration tree. A node of configuration tree can be associated with some software or hardware component controlled or tracked by a Test Agent. Such nodes have well-known path names and require support on te_agents side. Also :ref:`Configurator <doxid-group__te__engine__conf>` allows creating an arbitrary set of auxiliary configuration nodes that are not associated with anything and rather play role of shared storage or database.

Configurator features:

* stores a configuration database;

* synchronizes the database with te_agents (See :ref:`Synchronization configuration tree with Test Agent <doxid-group__confapi__base__sync>`);

* provides an API for traversing configuration tree;

* provides an API to tests for the configuration reading and changing (See :ref:`Configuration tree traversal <doxid-group__confapi__base__traverse>` and :ref:`Contriguration tree access operations <doxid-group__confapi__base__access>`);

* provides an API to tests and :ref:`Tester <doxid-group__te__engine__tester>` for backuping, verifying and restoring the configuration (See :ref:`Configuration backup manipulation <doxid-group__confapi__base__backup>`);

* provides an API to tests for te_agents rebooting with or without restoring of the configuration (See :ref:`Test Agent reboot <doxid-group__confapi__base__reboot>`).



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

Tree of instances contains information about real configuration items observed by CS on te_agents, and/or instances created during processing of the configuration file or test requests.

Each instance also has an object identifier. It also consists of a set of labels separated by slashes, but each label contains both a sub-identifier of the corresponding object and an instance name, which identifies uniquely the particular configuration item. Instance name is separated from the sub-identifier by a colon.

For example, the instance /agent:nut/interface:eth0/net_addr:1.2.3.4 of the object /agent/interface/net_addr corresponds to IP address 1.2.3.4 on the network interface eth0 of the station on which Test Agent named nut is running.

It's allowed to use empty instance names. For example, /agent/:nut/interface:eth0/link_addr: identifier is possible because the interface may have only one link address. An object sub-identifier must not contain symbols : (however this symbol is allowed in the instance name), '\*' and ' ' (space).

Empty instance name is used when the object has only one instance.

Instances which belong to /agent: subtree correspond to real configuration items observed on the te_agents (network interfaces, IP addresses, routes, ARP entries, daemons, etc.). Their change may lead to re-configuration of remote hosts.

The list of basic configuration objects, which is likely to be supported by any Test Agent, can be found in ${TE_BASE}/doc/cm/cm_base.xml file.

Other subtrees may be considered as information storage: changing instances in these subtrees does not affect the hosts controlled by te_agents, but may be used to share data between tests.

API to browse configuration trees can be found at :ref:`Configuration tree traversal <doxid-group__confapi__base__traverse>` page.





.. _doxid-group__te__engine__conf_1te_engine_conf_oper:

Configuration Operations
~~~~~~~~~~~~~~~~~~~~~~~~

Two operations are allowed for the objects: Register and Unregister. Register operation describes attributes of a new object (identifier, type, access, dependencies) to :ref:`Configurator <doxid-group__te__engine__conf>`. Unregister command forces :ref:`Configurator <doxid-group__te__engine__conf>` to forget about an object. Usually a command Register is used in the configuration file.

Three operations are allowed for instances: Set (change the value), Add (add a new instance) and Delete (delete an existing instance).

Moreover, :ref:`Configurator <doxid-group__te__engine__conf>` provides an API for read access to the object and instance databases (including different kinds of lookup).

All operations requested in the configuration file and by the tests are stored in the history to allow quick configuration restoring.

API to read and modify configuration tree can be found at :ref:`Contriguration tree access operations <doxid-group__confapi__base__access>` page.





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

You can find samples of :ref:`Configurator <doxid-group__te__engine__conf>` configuration files under ${TE_BASE}/conf directory.

Normally the structure of :ref:`Configurator <doxid-group__te__engine__conf>` configuration files is organized as following:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<history>
	  <!-- Register object nodes section -->
	  <register>
	      ...
	  </register>

	  <!-- Add object instance section -->
	  <add>
	      ...
	  </add>
	</history>

There can be multiple occurences of register object and add instance sections and they can be mixed.

For example:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<history>
	  <register>
	    <object oid="/agent/env" access="read_create" type="string"/>
	    <object oid="/agent/uname" access="read_only" type="string"/>
	  </register>

	  <register>
	    <object oid="/agent/interface" access="read_create" type="none"/>
	    <object oid="/agent/interface/index"
	            access="read_only" type="integer" volatile="true"/>

	    <!--
	      For IPv6 interfaces 'net_addr' depends on 'link_addr' because
	      IPv6 Link-Local Address value derived from Link Layer address
	      (MAC address of an interface).
	      It means that once we have an update of 'link_addr',
	      the collection of 'net_addr' should be synced between
	      Test Agent and Configurator.
	     -->
	    <object oid="/agent/interface/net_addr" access="read_create" type="integer">
	      <depends oid="/agent/interface/link_addr"/>
	    </object>
	  </register>

	  <register>
	    <object oid="/snmp" access="read_create" type="none"/>
	    <object oid="/snmp/mibs" access="read_create" type="none"/>
	    <object oid="/snmp/mibs/load" access="read_create" type="none"/>
	    <object oid="/snmp/version" access="read_create" type="integer"/>
	    <object oid="/snmp/community" access="read_create" type="string"/>
	    <object oid="/snmp/timeout" access="read_create" type="integer"/>
	  </register>

	  <add>
	    <instance oid="/snmp:"/>
	    <instance oid="/snmp:/mibs:"/>
	    <instance oid="/snmp:/mibs:/load:SNMPV2-MIB"/>
	    <instance oid="/snmp:/mibs:/load:IEEE802dot11-MIB"/>
	  </add>

	  <add>
	    <instance oid="/snmp:/version:" value="2"/>
	    <instance oid="/snmp:/community:" value="public"/>
	    <instance oid="/snmp:/timeout:" value="5"/>
	  </add>
	</history>

This sample registers few object nodes and then adds some object instances. Instance adding feature is very useful when you need to tune some configuration parameters via configurator configuration file. Then from test scenarios it is possible to gather this information, which means there is no need to rebuild sources when you need to change some configuration, but instead you can just modify a simple text file.

Note that you can also add new instances and set instance values of /agent subtree.

For example if you want to switch off IPv4 forwarding in your test suite you can write the following lines in your configuration file:

.. ref-code-block:: xml

	<register>
	  <object oid="/agent/ip4_fw" access="read_write" type="integer"/>
	</register>

	<set>
	  <!-- Switch off IPv4 forwarding on Agent 'Agt_A' -->
	  <instance oid="/agent:Agt_A/ip4_fw:" value="0"/>
	</set>

XSD schema of :ref:`Configurator <doxid-group__te__engine__conf>` configuration file can be found at ${TE_BASE}/doc/xsd/cs_config.xsd.





.. _doxid-group__te__engine__conf_1te_engine_conf_file_features:

Special features
----------------

One useful feature of :ref:`Configurator <doxid-group__te__engine__conf>` configuration file is that it is possible to include the content of one file into another with XML **include** tag. For example your upper level file can keep only includes to different parts of configuration:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<history>
	  <xi:include xmlns:xi="http://www.w3.org/2003/XInclude" href="cs.conf.common" parse="xml"/>
	  <xi:include xmlns:xi="http://www.w3.org/2003/XInclude" href="cs.conf.env" parse="xml"/>
	  <xi:include xmlns:xi="http://www.w3.org/2003/XInclude" href="cs.conf.hw" parse="xml"/>
	</history>

:ref:`Configurator <doxid-group__te__engine__conf>` allows to use environment variables in the content of configuration file. Which makes it possible to tune configuration via environment variables without modification of configuration file. For example:

.. ref-code-block:: xml

	<!--
	  Set '/local:Agt_B/libdir:' to the value of environment variable TEST_LIBDIR.
	  If this variable is not set, then the value is set to '/usr/lib'.
	  This is just ordinary "${parameter:-default}" BASH substitution.
	  -->
	<set>
	  <instance oid="/local:Agt_B/libdir:" value="${TEST_LIBDIR:-/usr/lib}"/>
	</set>
	<!--
	  Set the value of '/local:Agt_A/env:LOG_LEVEL' instace to the value of
	  TEST_LOG_LEVEL environment variable.
	  --
	<add>
	  <instance oid="/local:Agt_A/env:LOG_LEVEL" value="${TEST_LOG_LEVEL}"/>
	</add>

Actually as object instance value you can use syntax of BASH Parameter Substitution.

|	:ref:`API Usage: Configurator API<doxid-group__confapi>`
|		:ref:`API: Configurator<doxid-group__confapi__base>`
|			:ref:`Configuration backup manipulation<doxid-group__confapi__base__backup>`
|			:ref:`Configuration tree traversal<doxid-group__confapi__base__traverse>`
|			:ref:`Contriguration tree access operations<doxid-group__confapi__base__access>`
|			:ref:`Synchronization configuration tree with Test Agent<doxid-group__confapi__base__sync>`
|			:ref:`Test Agent reboot<doxid-group__confapi__base__reboot>`
|		:ref:`TAPI: Test API for configuration nodes<doxid-group__tapi__conf>`
|			:ref:`ARL table configuration<doxid-group__tapi__conf__arl>`
|			:ref:`Agent namespaces configuration<doxid-group__tapi__namespaces>`
|			:ref:`Agents, namespaces and interfaces relations<doxid-group__tapi__host__ns>`
|			:ref:`Bonding and bridging configuration<doxid-group__tapi__conf__aggr>`
|			:ref:`CPU topology configuration of Test Agents<doxid-group__tapi__conf__cpu>`
|			:ref:`Command monitor TAPI<doxid-group__tapi__cmd__monitor__def>`
|			:ref:`DHCP Server configuration<doxid-group__tapi__conf__dhcp__serv>`
|			:ref:`DUT serial console access<doxid-group__tapi__conf__serial>`
|			:ref:`Environment variables configuration<doxid-group__tapi__conf__sh__env>`
|			:ref:`Ethernet PHY configuration<doxid-group__tapi__conf__phy>`
|			:ref:`Ethernet interface features configuration<doxid-group__tapi__conf__eth>`
|			:ref:`IP rules configuration<doxid-group__tapi__conf__ip__rule>`
|			:ref:`IPv6 specific configuration<doxid-group__tapi__conf__ip6>`
|			:ref:`Kernel modules configuration<doxid-group__tapi__conf__modules>`
|			:ref:`L2TP configuration<doxid-group__tapi__conf__l2tp>`
|			:ref:`Local subtree access<doxid-group__tapi__conf__local>`
|			:ref:`Manipulation of network address pools<doxid-group__tapi__conf__net__pool>`
|			:ref:`NTP daemon configuration<doxid-group__tapi__conf__ntpd>`
|			:ref:`Neighbour table configuration<doxid-group__tapi__conf__neigh>`
|			:ref:`Network Base configuration<doxid-group__tapi__conf__base__net>`
|			:ref:`Network Emulator configuration<doxid-group__tapi__conf__netem>`
|			:ref:`Network Interface configuration<doxid-group__tapi__conf__iface>`
|			:ref:`Network Switch configuration<doxid-group__tapi__conf__switch>`
|			:ref:`Network sniffers configuration<doxid-group__tapi__conf__sniffer>`
|			:ref:`Network statistics access<doxid-group__tapi__conf__stats>`
|			:ref:`Network topology configuration of Test Agents<doxid-group__tapi__conf__net>`
|			:ref:`PPPoE Server configuration<doxid-group__tapi__conf__pppoe>`
|			:ref:`Processes configuration<doxid-group__tapi__conf__process>`
|			:ref:`Queuing Discipline configuration<doxid-group__tapi__conf__qdisc>`
|				:ref:`tc qdisc TBF configuration<doxid-group__tapi__conf__tbf>`
|			:ref:`Routing Table configuration<doxid-group__tapi__conf__route>`
|			:ref:`Serial console parsers configuration<doxid-group__tapi__conf__serial__parse>`
|			:ref:`Solarflare PTP daemon configuration<doxid-group__tapi__conf__sfptpd>`
|			:ref:`System parameters configuration<doxid-group__tapi__cfg__sys>`
|			:ref:`TA system options configuration (OBSOLETE)<doxid-group__tapi__conf__proc>`
|			:ref:`TR-069 Auto Configuration Server Engine (ACSE) configuration<doxid-group__tapi__conf__acse>`
|			:ref:`Test API to handle a cache<doxid-group__tapi__cache>`
|			:ref:`VTund configuration<doxid-group__tapi__conf__vtund>`
|			:ref:`Virtual machines configuration<doxid-group__tapi__conf__vm>`
|			:ref:`XEN configuration<doxid-group__tapi__conf__xen>`
|			:ref:`iptables configuration<doxid-group__tapi__conf__iptable>`


