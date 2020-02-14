.. index:: pair: group; Command Output Logging
.. _doxid-group__te__cmd__monitor:

Command Output Logging
======================

.. toctree::
	:hidden:

TE Command Output Logging feature allows to execute periodically some shell command on **Test Agent** and print its output in the testing log. Is is currently available only for Unix TA.

There are three ways to configure and use this feature - in tests via its **Test API**, in package.xml with using XML tag command_monitor, and in dispatcher.sh(run.sh) command line with **tester-cmd-monitor** option. All these ways ultimately use Configurator TAPI to add/change/remove command_monitor nodes in configuration subtree of Test Agent of interest.



.. _doxid-group__te__cmd__monitor_1cmd_monitor_conf:

Configuration tree
~~~~~~~~~~~~~~~~~~

To use this feature, you need to register command_monitor node and its children in configuration file of Configurator by adding in it

.. ref-code-block:: cpp

	<register>
	    <object oid="/agent/command_monitor"
	            access="read_create" type="none">
	    </object>
	    <object oid="/agent/command_monitor/time_to_wait"
	            access="read_write" type="integer">
	    </object>
	    <object oid="/agent/command_monitor/command"
	            access="read_write" type="string">
	    </object>
	    <object oid="/agent/command_monitor/enable"
	            access="read_write" type="integer">
	    </object>
	</register>





.. _doxid-group__te__cmd__monitor_1cmd_monitor_tapi:

Test API
~~~~~~~~

You can start logging command output by executing

.. ref-code-block:: cpp

	tapi_cfg_cmd_monitor_begin(ta_name, monitor_name, command, time_to_wait);

and stop it with

.. ref-code-block:: cpp

	tapi_cfg_cmd_monitor_end(ta_name, monitor_name);

Here **ta_name** is unique Test Agent name, **monitor_name** is monitor node name used to identify it within the Test Agent's configuration subtree, **command** is command to be executed and logged periodically, **time_to_wait** specifies time to wait between subsequent command executions (in milliseconds).

For example,

.. ref-code-block:: cpp

	tapi_cfg_cmd_monitor_begin(pco_iut->ta, "stackdump_monitor",
	                           "/tmp/te_onload_stdump",
	                           1000);
	/* Now it loggs output of te_onload_stdump every second. */
	<...>
	tapi_cfg_cmd_monitor_end(pco_iut->ta, "stackdump_monitor");
	/* Logging stopped. */





.. _doxid-group__te__cmd__monitor_1cmd_monitor_pkg:

Defining command output logging in package.xml
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Inside package.xml you can specify command output logging by adding command_monitor nodes as the first children of either run or session nodes. After that for all the tests inside this run or session nodes the specified command output logging will be run.

For example, if you want to run onload_stackdump and log its output every second for all the tests from some package when they are run with Onload library, you can add the command_monitor node in session child node of the package node:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<package version="1.0">
	    <description>Input/Output multiplexing</description>
	    <author mailto="Andrew.Rybchenko@oktetlabs.ru"/>

	    <session>
	        <command_monitor>
	            <ta>Agt_A</ta>
	            <command>${SFC_ONLOAD_GNU}/tools/ip/onload_stackdump</command>
	            <time_to_wait>1000</time_to_wait>
	            <run_monitor>${L5_RUN}</run_monitor>
	        </command_monitor>
	        <!--- Tests specification --->
	    </session>
	</package>

Here run_monitor specifies a condition that should be met to enable this command output logging. If the text inside it is "yes" or non-zero number, it is considered true (L5_RUN environment variable evaluates to "yes" when tests are run with Onload library). This node is optional; if it is omitted, specified command output logging is always enabled. :ref:`Node <doxid-structNode>` ta, specifying the name of Test Agent on which to run and log output of a given command, is optional as well; if it is omitted, then name of Test Agent is taken from TE_IUT_TA_NAME environment variable.

You can specify several command_monitor nodes to enable logging of several commands simultaneously.





.. _doxid-group__te__cmd__monitor_1cmd_monitor_cli:

Command line option
~~~~~~~~~~~~~~~~~~~

You can use command line option **tester-cmd-monitor** when running dispatcher.sh(perhaps via run.sh of your test suite; it is eventually passed as **cmd-monitor** option to Tester).

.. ref-code-block:: cpp

	--tester-cmd-monitor="[ta_name,]time_to_wait:command"

**ta_name** is optional here; if not specified, then it is taken from TE_IUT_TA_NAME environment variable.

For example, to log onload_stackdump output every second while running sockapi-ts/iomux/many_sockets test:

.. ref-code-block:: cpp

	./run.sh --cfg=l5elrond --tester-run=sockapi-ts/iomux/many_sockets -n
	--tester-cmd-monitor="1000:$SFC_ONLOAD_GNU/tools/ip/onload_stackdump"

