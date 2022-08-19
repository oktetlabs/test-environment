..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; TE: User Guide
.. _doxid-group__te__user:

TE: User Guide
==============

.. toctree::
	:hidden:

	group_te_netns.rst
	group_te_user_run_details.rst



.. _doxid-group__te__user_1te_user_introduction:

Introduction
~~~~~~~~~~~~

This page gives step by step guideline on where to start with TE, how to get the sources, build them and run a simple test script.

TE can be supplied in two release types:

* pre-installed form (external headers and binaries are available);

* source code form (all sources of TE components are available).

If you have pre-installed binaries you can obviously skip the sources download and build part.





.. _doxid-group__te__user_1te_user_src:

Getting TE sources
~~~~~~~~~~~~~~~~~~

Test Environment sources can be obtained from
`https://git.oktetlabs.ru/git/oktetlabs/test-environment <https://git.oktetlabs.ru/git/oktetlabs/test-environment.git>`__

You will need OKTET Labs account and appropriate permissions, contact `Konstantin.Ushakov@oktetlabs.ru <mailto:Konstantin.Ushakov@oktetlabs.ru>`__ if you need any assistance.

If you have a test suite which is not structurally embeded in the TE subtree (not in suites/) directory you also need to download its sources.

So in general case after all source code is retrieved from the repository one should have:

.. code-block:: none


	work_dir/
	   te/
	   my-ts/





.. _doxid-group__te__user_1te_user_build:

Building TE components
~~~~~~~~~~~~~~~~~~~~~~

When you get TE in source code form you need to build TE before it can be used for testing.

If you're living on a deb-based distribution you could benefit from installing ol-te and other packages. They will automatically install all the deps for the engine and your project. Contact developers if you need access to the the debian repository or to get the .deb file.

To build any of components TE Engine or TA it is necessary to install development toolchain. You can use these packages on Debian derivatives:

* build-essential - this package provokes the installation of the following packages:

  * g++,

  * gcc,

  * libc-dev,

  * make;

* pkg-config.

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install build-essential pkg-config



.. _doxid-group__te__user_1te_deps:

Test Environment Engine dependencies
------------------------------------

Test Environment Engine depends on a set of 3-rd party libraries and packages:

* bash - at least 4.3;

* bison;

* curl;

* file;

* flex - at least 2.5.31;

* gawk;

* libcurl4-openssl-dev;

* libexpat1;

* libexpat1-dev;

* libglib2.0-dev;

* libncurses5;

* libncurses5-dev;

* libpam0g;

* libpam0g-dev;

* libpcap0.8-dev;

* libpopt-dev;

* libreadline5 - at least 5.0;

* libreadline-dev - at least 5.0;

* libssl-dev;

* libusb-1.0-0;

* libusb-1.0-0-dev;

* libxml2;

* libxml2-dev - at least 2.6.10;

* libxslt1.1;

* libxslt1-dev - at least 1.1.6;

* meson - at least 0.45.1;

* ssh;

* tcl-dev;

* wget.

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install bison curl file flex gawk libcurl4-openssl-dev libexpat1 libexpat1-dev libglib2.0-dev libncurses5 libncurses5-dev libpam0g libpam0g-dev libpcap0.8-dev libpopt-dev libreadline5 libreadline-dev libssl-dev libusb-1.0-0 libusb-1.0-0-dev libxml2 libxml2-dev libxslt1.1 libxslt1-dev meson ssh tcl-dev wget

Or easy way, you can install meta package **oktetlabs-te-dev** from OKTET Labs repository.

Optional libraries and packages:

* perl-Time-HiRes - package on Redhat/Fedora is very usefull (it allows to avoid mixture in log because of unprecise timestamps in messages logged by Dispatcher (via logging script)).





.. _doxid-group__te__user_1ta_build_deps:

Test Agent build dependencies
-----------------------------

Default Test Agent build depends on a set of 3-rd party libraries and packages:

* bison;

* file;

* flex - at least 2.5.31;

* gawk;

* libpcap0.8-dev;

* libpopt-dev;

* ssh server, in Debian derivatives it is openssh-server;

* wget.

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install bison file flex gawk libpcap0.8-dev libpopt-dev openssh-server wget

Optional libraries and packages:

* libsnmp-dev - for SNMP support.





.. _doxid-group__te__user_1ta_run_deps:

Test Agent run dependencies
---------------------------

Default Test Agent run depends on a set of 3-rd party libraries and packages:

* file;

* gawk;

* ssh server, in Debian derivatives it is openssh-server.

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install file gawk openssh-server

Optional libraries and packages:

* libsnmp - for SNMP support.





.. _doxid-group__te__user_1te_build_config:

Build configuration
-------------------

In order to build sources you will need to prepare project-specific :ref:`Builder <doxid-group__te__engine__builder>` configuration file. For the details on the file format please read :ref:`Builder configuration file <doxid-group__te__engine__builder_1te_engine_builder_conf_file>` section.

If you're dealing with existing test suite the file usualy was already written by the suite author/maintainer.

For example builder.conf file for a sample test suite located under ${TE_BASE}/suites/ipv6-demo is following:

.. ref-code-block:: cpp

	#
	# Builder configuration file for IPv6 Demo Test Suite.
	#

	TE_PLATFORM([], [], [-D_GNU_SOURCE], [], [],
	            [logger_core tools logic_expr rpc_types conf_oid rpcxdr \
	             comm_net_agent loggerta rpctransport agentlib rpcserver rcfpch trc \
	             ipc bsapi loggerten rcfapi confapi comm_net_engine rcfunix \
	             tapi rcfrpc tapi_env tapi_rpc dummy_tad netconf asn ndn])

	TE_TOOLS([rgt trc])

	TE_TOOL_PARMS([trc], [--with-popups --with-log-urls])

	TE_LIB_PARMS([dummy_tad], [], [tad], [])

	TE_TA_TYPE([linux], [], [unix],
	           [--with-tad=dummy_tad --with-rcf-rpc \
	            --with-cfg-unix-daemons='dns ftp Xvfb smtp telnet rsh vncserver dhcp vtund' \
	            --with-libnetconf],
	           [], [], [], [comm_net_agent asn ndn])





.. _doxid-group__te__user_1te_build_do:

Building
--------

When you prepared a :ref:`Builder <doxid-group__te__engine__builder>` configuration file, you are ready to start building process.

You should have two folders: Test Environment sources folder; test suite sources folder.

Before building you **must** export TE_BASE environment variable that points to the root directory of Test Environment sources.

As you usually work with one copy of Test Environment it is usefull to add

.. ref-code-block:: cpp

	export TE_BASE=/path/to/TE_root_dir

into your ~/.bashrc.

Suppose we have the following structure under our project directory (a directory where we run Test Environment building procedure):

.. code-block:: none


	${PRJ_ROOT}
	  +-- conf_ipv6
	        +-- builder.conf.ipv6_demo

To start building process we should run the following command:

.. code-block:: none


	cd $PRJ_ROOT
	${TE_BASE}/dispatcher.sh --conf-dir=conf_ipv6 --conf-builder=builder.conf.ipv6_demo --build-only --no-tester

--build-only option means that we don't try to run any tests and --no-tester means that dispatcher.sh doesn't start the te_tester.

If you build with a test suite (which means that you have run.sh) you can call:

.. code-block:: none


	cd $PRJ_ROOT
	${TE_BASE}/run.sh --cfg=myconfiguration_name --tester-norun

run.sh script is a wrapper for dispatcher.sh which introduces some suite-specific options and defaults. The above run.sh invocations will build both TE and the test suite. See @te_user_run or dispatcher.sh help for details on the --cfg option.

If you get some errors during building procedure, you should first check if your have all necessary packages installed on your development platform. Please refer to :ref:`Test Environment Engine dependencies <doxid-group__te__user_1te_deps>` section to check the list of required packages and libraries.

If you do not specify TE_BUILD environment variable, :ref:`Builder <doxid-group__te__engine__builder>` calls configure scripts and calls make under ${PRJ_ROOT} directory (i.e. a directory where you run dispatcher.sh).







.. _doxid-group__te__user_1te_user_run:

TE Exectuion
~~~~~~~~~~~~

To run TE one needs to have configuration files for all the TE subsystems.

See :ref:`TE Config Files Basics <doxid-group__te__user__run__details>` or :ref:`Test Environment <doxid-group__te>` for more information about config files creation and syntax.

Below we describe what happens upon dispatcher.sh execution and what artifacts are left after it.

As it was mentioned above the TE can be started by run.sh script located in you test suite directory or by direct dispatcher.sh invocation.

You should either specify all the configuration files with --conf-\* options or use the default ones (see the dispatcher script):

.. code-block:: none


	CONF_BUILDER=builder.conf
	CONF_LOGGER=logger.conf
	CONF_TESTER=tester.conf
	CONF_CS=configurator.conf
	CONF_RCF=rcf.conf

To avoid extremely long command lines you can use --opts option and pass a file:

.. code-block:: none


	$ cat conf/run.opts.defaults
	--conf-builder=builder.conf.default
	--conf-tester=tester.conf.default
	$ ./dispatcher.sh --opts=run.opts.defaults --tester-run=foobar-ts/basic/trivial

In case you're running with run.sh there is even faster way to start the framework.

.. code-block:: none


	$ ./run.sh --cfg=<cfgname> --tester-run=foobar-ts/basic/trivial

In this case the conf/run.conf.<cfgname> file is considered as an option file. This is very useful as usually project has several test configurations (i.e. **apple**, **carrot** and **tomato**). Although each configuration may use its own hosts they usually share tester.conf, builder.conf (if all hosts run the same operating system), configurator.conf

When you start the TE the following is written to the console:

.. code-block:: none


	RUNDIR=/home/user/work/my_run_dir
	--->>> Starting Logger...done
	--->>> Starting RCF...done
	--->>> Starting Configurator...done
	--->>> Start Tester
	Starting package foobar-ts
	Starting test prologue                                               pass
	Starting package basic
	Starting test trivial                                                pass
	Done package basic pass
	Starting test epilogue                                               pass
	Done package foobar-ts pass
	--->>> Shutdown Configurator...done
	--->>> Flush Logs
	--->>> Shutdown RCF...done
	--->>> Shutdown Logger...done
	--->>> Logs conversion...done

.. code-block:: none

	Run (total)                               1
	  Passed, as expected                     1
	  Failed, as expected                     0
	  Passed unexpectedly                     0
	  Failed unexpectedly                     0
	  Aborted (no useful result)              0
	  New (expected result is not known)      0
	Not Run (total)                         239
	  Skipped, as expected                    0
	  Skipped unexpectedly                    0

So:

* Framework starts all its components; See :ref:`Start/stop sequence <doxid-group__te__engine__dispatcher_1te_run_time>` section for details.

* It executes **prologue** if exists (**prologue** is not a test it's a sequence of actions specific for a given package/suite; for instance it can assign IP addresses or start certain services);

* **tests** which were passed with the --tester-run option are executed; in the above case it's **{foobar-ts/basic/trivial}** (see te_tester for more details on the tests specification);

* **epilogue** is executed it can be used to rollback modifications done in **prologue** or perform arbitrary cleanup;

* framework terminates;

* :ref:`Test Results Comparator <doxid-group__trc>` prints some statistics, see :ref:`Result explanation <doxid-group__trc_1trc_tool_result>` for details (note, that prologues and epilogues are not counted in the tests statisctics as they **MUST** always sucess).

After execution is complete several new files appear in the log directory (which is PWD if not specified with --log-dir option. Name of some of the files can be changed via --log-\* options.

Files include:

* tmp_raw_log main log file in binary format; can be passed to the :ref:`Report Generator Tool <doxid-group__rgt>` and :ref:`Test Results Comparator <doxid-group__trc>` utils;

* ta.\* files with **stderr** from all of the agents, should contain only 'Exiting' word if all went fine;

* if --cs-print-trees option was given then objects and instances will be created; they will contain dump of objects and instances :ref:`Configurator <doxid-group__te__engine__conf>` trees.

You can read the logs in text or HTML format. See :ref:`Output Formats <doxid-group__rgt_1rgt_output_formats>` for more info on logs generation and looks.

|	:ref:`Network namespaces<doxid-group__te__netns>`
|	:ref:`TE Config Files Basics<doxid-group__te__user__run__details>`


