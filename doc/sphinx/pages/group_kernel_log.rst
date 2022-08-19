..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Kernel Logging
.. _doxid-group__kernel__log:

Kernel Logging
==============

.. toctree::
	:hidden:

	group_console_ll.rst
	group_serial.rst

Kernel log can be obtained from two sources - either from serial console (directly or via Conserver) or from netconsole kernel module sending kernel logs via UDP to the specified address.

TE can process kernel log either in Test Agent or in Logger.



.. _doxid-group__kernel__log_1kernel_log_direct:

Using serial console directly
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Serial console can be logged directly only in Test Agent residing on the same host as this serial console.

To configure direct serial console logging, in :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` add a child **thread** tag to the **ta** tag corresponding to related Test Agent:

.. ref-code-block:: cpp

	<thread name="log_serial" when="<!--if not void, thread will be created-->">
	    <arg value="elrond"/><!-- Host name to be displayed in log -->
	    <arg value="WARN"/> <!-- Log messages level -->
	    <arg value="10"/> <!-- Timeout, in milliseconds; when elapsed, what
	                           gathered will be printed -->
	    <arg value="/dev/ttyS0"/> <!-- Path to serial console -->
	    <arg value="exclusive"/> <!-- What to do if other process has already
	                                  opened this device file: if argument is
	                                  omitted or value is 'exclusive', exit
	                                  with error;
	                                  if value is 'force', kill other
	                                  process(es) using the serial console
	                                  before opening it;
	                                  if value is 'shared', just open it. -->
	</thread>





.. _doxid-group__kernel__log_1kernel_log_conserver:

Using Conserver
~~~~~~~~~~~~~~~

If you have configured Conserver, you can tell Test Agent to use it adding the following **thread** child in its tag in :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` :

.. ref-code-block:: cpp

	<thread name="log_serial" when="<!-- not used but should not be void-->">
	    <arg value="elrond"/><!-- Host name to be displayed in log -->
	    <arg value="WARN"/> <!-- Log messages level -->
	    <arg value="10"/> <!-- Timeout, in milliseconds; when elapsed, what
	                           gathered will be printed -->
	    <!-- Conserver configuration string; the first parameter can be
	         omitted if conserver resides on the same host; IPv6 address
	         should be specified in parenthesis -->
	    <arg value="address or hostname:port:user:console"/>
	</thread>

If you want to use Logger directly, the above tag should be specified in Logger :ref:`Configuration File <doxid-group__te__engine__logger_1te_engine_logger_conf_file>` instead.





.. _doxid-group__kernel__log_1kernel_log_netconsole:

Using Netconsole
~~~~~~~~~~~~~~~~

If kernel netconsole module is configured already then all you should do is to add in tag corresponding to Test Agent residing on the host to which netconsole sends logs in :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` the following:

.. ref-code-block:: cpp

	<thread name="log_serial" when="<!--not used but should not be void-->">
	    <arg value="elrond"/><!-- Host name to be displayed in log -->
	    <arg value="WARN"/> <!-- Log messages level -->
	    <arg value="10"/> <!-- Timeout, in milliseconds; when elapsed, what
	                           gathered will be printed -->
	    <!-- Supposing that netconsole sends logs to UDP port 1234 -->
	    <arg value="netconsole:1234"/>
	</thread>

If Logger resides on the host to which logs are sent, you can specify this tag in Logger :ref:`Configuration File <doxid-group__te__engine__logger_1te_engine_logger_conf_file>` instead to use Logger directly.

If netconsole kernel module is not configured and you wish to configure it from Test Agent automatically, in :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` add the following (replacing **Agt_A** with whatever you need):

.. ref-code-block:: cpp

	<register>
	    <object oid="/agent/netconsole" access="read_create" type="string"/>
	</register>
	<add>
	    <instance oid="/agent:Agt_A/netconsole:netconsole_name"
	              <!-- local_port:remote_host:remote_port -->
	              value="6666:kili:6666"/>
	</add>

You can add more than one **netconsole** node. Note: if the default configfs directory (/sys/kernel/config) is not available, the netconsole dynamic reconfiguration will not be used, but kernel module will be reloaded with new parameters.

|	:ref:`Console Log Level Configuration<doxid-group__console__ll>`
|	:ref:`Packet Serial Parser<doxid-group__serial>`


