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

Test Environment is an open source project. Its sources live in
|te_repository| and can be cloned by anyone:

.. parsed-literal::


	git clone |te_clone_url| te

If you have a test suite which is not structurally embedded in the TE subtree (not in suites/) directory you also need to download its sources.

So in general case after all source code is retrieved from the repository one should have:

.. code-block:: none


	work_dir/
	   te/
	   my-ts/





.. _doxid-group__te__user_1te_user_build:

Building TE components
~~~~~~~~~~~~~~~~~~~~~~

When you get TE in source code form you need to build TE before it can be used for testing.

To build either the Test Engine or a Test Agent you first need a development
toolchain. On Debian derivatives:

* build-essential - pulls in ``gcc``, ``g++``, ``libc-dev`` and ``make``;

* pkg-config - used to locate every library listed below;

* meson - at least 0.49.0, the version required by TE's own ``meson.build``;

* ninja-build - the backend meson drives.

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install build-essential pkg-config meson ninja-build



.. _doxid-group__te__user_1te_deps:

Test Environment Engine dependencies
------------------------------------

The Test Engine needs the following packages. What wants each one is named,
so that a build failure is easier to place.

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Package
     - Needed for
   * - bash
     - at least 4.3; runs ``dispatcher.sh`` and the helper scripts
   * - bison, flex
     - flex at least 2.5.31; the :ref:`Tester <doxid-group__te__engine__tester>` test path parser, the test environment parser and logical expressions
   * - gawk, file, wget, ssh
     - the build and run scripts; ssh is also how :ref:`RCF <doxid-group__te__engine__rcf>` reaches a Test Agent
   * - libxml2-dev
     - at least 2.6.10; XML configuration files and raw log processing
   * - xsltproc
     - filtering subtrees out of a :ref:`Configurator <doxid-group__te__engine__conf>` backup; the command is run as it is, so the binary is what is needed, not the library headers
   * - libpopt-dev
     - command line parsing in every engine application
   * - libjansson-dev
     - JSON: :ref:`Tester <doxid-group__te__engine__tester>`, :ref:`Logger <doxid-group__te__engine__logger>`, :ref:`Report Generator Tool <doxid-group__rgt>` and :ref:`Test Results Comparator <doxid-group__trc>`
   * - libyaml-dev
     - the YAML configuration files: :ref:`Configurator <doxid-group__te__engine__conf>`, :ref:`Logger <doxid-group__te__engine__logger>` and :ref:`RCF <doxid-group__te__engine__rcf>`
   * - libcurl4-openssl-dev
     - publishing logs from the :ref:`Logger <doxid-group__te__engine__logger>`
   * - libglib2.0-dev
     - :ref:`Report Generator Tool <doxid-group__rgt>`
   * - libssl-dev
     - :ref:`Test Results Comparator <doxid-group__trc>`
   * - libpcre2-dev
     - log post-processing
   * - libbsd-dev
     - string helpers in the common tools library; optional, the build falls back to its own implementations

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install bison flex gawk file wget ssh libxml2-dev xsltproc libpopt-dev libjansson-dev libyaml-dev libcurl4-openssl-dev libglib2.0-dev libssl-dev libpcre2-dev libbsd-dev

Optional libraries and packages:

* libreadline-dev and libncurses-dev - enable the interactive :ref:`Tester <doxid-group__te__engine__tester>` mode (the ``--tester-interactive`` option). Without them the build succeeds and the option is simply unavailable;

* perl-Time-HiRes - package on Redhat/Fedora is very useful (it allows to avoid mixture in log because of unprecise timestamps in messages logged by Dispatcher (via logging script)).





.. _doxid-group__te__user_1ta_build_deps:

Test Agent build dependencies
-----------------------------

A default Test Agent build needs:

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Package
     - Needed for
   * - gawk, file, wget
     - the build and run scripts
   * - libpcap-dev
     - packet capture and injection in the :ref:`Traffic Application Domain <doxid-group__tapi__tad__main>`
   * - libtirpc-dev
     - Sun RPC; glibc no longer provides it, so this package is needed on any current distribution
   * - libpcre2-dev
     - agent job control
   * - libnl-3-dev
     - netlink, used for network configuration on Linux agents
   * - openssh-server
     - not a build dependency: an ssh server has to run on the agent host, because :ref:`RCF <doxid-group__te__engine__rcf>` copies the agent there and starts it over ssh

Agents configured with extra features need more:

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Package
     - Needed for
   * - bison, flex
     - flex at least 2.5.31; the DNS and DHCP server configuration support, which is off unless the agent is built with those daemons
   * - libssl-dev
     - OpenVPN daemon support
   * - libbsd-dev
     - string helpers in the common tools library; optional, the build falls back to its own implementations

Same for Debian derivatives in one line:

.. code-block:: none


	apt-get install gawk file wget libpcap-dev libtirpc-dev libpcre2-dev libnl-3-dev openssh-server

Optional libraries and packages:

* libelf-dev - BPF/XDP support on the agent;

* libsnmp-dev - for SNMP support, used by the power control agent;

* libyang-dev and libnetconf2-dev - NETCONF/RESTCONF RPCs;

* libpam0g-dev - PAM support;

* tcl-dev and expect - Tcl and expect support in :ref:`Traffic Application Domain <doxid-group__tapi__tad__main>`.





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

In order to build sources you will need a project-specific
:ref:`Builder <doxid-group__te__engine__builder>` configuration file, normally
called ``builder.conf``. For the details on the file format please read
:ref:`Builder configuration file <doxid-group__te__engine__builder_1te_engine_builder_conf_file>`.

If you're dealing with an existing test suite the file was usually already
written by the suite author or maintainer, and lives in the suite's ``conf/``
directory.

It says which TE libraries and tools to build for the engine, and what kind of
Test Agent to build. Cut down to its essentials, it looks like this:

.. code-block:: none


	TE_PLATFORM([], [], [-D_GNU_SOURCE], [], [],
	            [logger_core tools conf_oid asn ndn logic_expr ipc bsapi \
	             loggerten rcfapi confapi comm_net_engine rcfunix trc tapi \
	             rpcxdr rcfrpc rpc_types tapi_rpc tapi_env tapi_job])

	TE_TOOLS([rgt trc])

	TE_TA_TYPE([linux], [], [unix],
	           [--with-rcf-rpc --with-libnetconf], [], [], [],
	           [comm_net_agent asn ndn])

Every library a test links against has to appear in the ``TE_PLATFORM`` list
here as well as in the suite's ``meson.build``. See
``${TE_BASE}/suites/selftest/conf/builder.conf`` for a complete, working one.




.. _doxid-group__te__user_1te_build_do:

Building
--------

Before building you **must** export the TE_BASE environment variable pointing at
the root directory of the Test Environment sources. As you usually work with one
copy of TE it is worth putting

.. code-block:: none


	export TE_BASE=/path/to/TE_root_dir

into your ~/.bashrc. Suites that ship a ``scripts/guess.sh`` work it out
themselves if TE is checked out next to them.

There is nothing to configure and no ``make`` to run: ``dispatcher.sh`` drives
:ref:`Builder <doxid-group__te__engine__builder>`, which drives meson. Do not
invoke meson by hand.

To build TE on its own, without any test suite:

.. code-block:: none


	cd ${TE_BASE}
	./dispatcher.sh --no-run

To build a suite together with TE, run the suite's own ``run.sh`` and tell
Tester not to run anything:

.. code-block:: none


	cd /path/to/my-ts
	./run.sh --cfg=<configuration> --tester-no-run

``run.sh`` is a wrapper around ``dispatcher.sh`` that adds suite-specific
options and defaults; ``--cfg=<name>`` picks one of the suite's configurations.
See :ref:`TE Execution <doxid-group__te__user_1te_user_run>` or
``dispatcher.sh --help`` for the details.

Useful while working on the build:

* ``--build-only`` --- build everything, including the test suites, but run no tests;

* ``--build-from-scratch`` --- discard previous build artefacts, which is what
  you want after changing the toolchain or the Builder configuration;

* ``--build-parallel[=num]`` --- build with several jobs;

* ``--builder-debug`` --- be verbose about what the build is doing.

If you get errors during the build, first check that all the required packages
are installed; see
:ref:`Test Environment Engine dependencies <doxid-group__te__user_1te_deps>`.

Build artefacts go to the directory named by the TE_BUILD environment variable.
If it is not set, they are placed under the directory from which you started
``dispatcher.sh``.



.. _doxid-group__te__user__run__details:

Configuring a run
~~~~~~~~~~~~~~~~~

Every TE subsystem is driven by its own configuration file.  This
section walks through the five of them in the order dispatcher.sh
starts the subsystems.

.. _doxid-group__te__user__run__details_1te_user_run_rcf:

Running RCF
-----------

:ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file specifies the list of Test Agents to run with a a set of parameters associated with them. For the detailed information on how to write :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file please refer to :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>` section.

More likely you will already have some :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file or you will need to do your own version of configuration file based on existing one.

First thing that you need to take into account while writing :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file is the how and where you are going to organize testing process.

Suppose you need to test some communication API between two end-points (for example it could be Socket API) and you have the following network topology:

.. image:: /static/image/te_user_net_conf_rcf_sample1.png
	:alt: Sample network topology

You would like to test communication between end point pairs:

* BSD and Windows;

* BSD and Linux;

* Windows and Linux.

The API to be tested is the same on all platforms, which means we can use the same test suite for each pair. The only thing specific for our test set-up is where to run Test Agent that supports interface to be tested (assume we exported interface to be tested via :ref:`TAPI: Remote Procedure Calls (RPC) <doxid-group__te__lib__rpc__tapi>`).

For testing BSD vs Windows configuration we should use the following set-up:

.. image:: /static/image/te_user_net_conf_rcf_sample2.png
	:alt: TE components location for testing BSD vs Windows configuration

In this scenario :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file would look like:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<rcf>
	    <ta name="Agt_A" type="bsd" rcflib="rcfunix">
	        <conf name="host">gollum</conf>
	        <conf name="port">5000</conf>
	        <conf name="sudo"/>
	    </ta>
	    <ta name="Agt_B" type="win" rcflib="rcfunix">
	        <conf name="host">aule</conf>
	        <conf name="port">5000</conf>
	        <conf name="sudo"/>
	    </ta>
	</rcf>

Please note that we use the same :ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>`, but different Test Agent types.

For testing BSD vs Linux configuration we should use the following set-up:

.. image:: /static/image/te_user_net_conf_rcf_sample3.png
	:alt: TE components location for testing BSD vs Linux configuration

In this scenario :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file would look like:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<rcf>
	    <ta name="Agt_A" type="bsd" rcflib="rcfunix">
	        <conf name="host">gollum</conf>
	        <conf name="port">5000</conf>
	        <conf name="sudo"/>
	    </ta>
	    <ta name="Agt_B" type="linux" rcflib="rcfunix">
	        <conf name="port">5000</conf>
	        <conf name="sudo"/>
	    </ta>
	</rcf>

Note that we can avoid specifying host name for Test Agent Agt_B, because it runs on the same host as :ref:`Test Engine <doxid-group__te__engine>`.

Similar set-up would be for testing Windows vs Linux set-up.

Now we have :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration files ready and we can run TE with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`.

Our project tree has the following structure:

.. code-block:: none


	${PRJ_ROOT}
	  +-- conf
	        +-- builder.conf.mysuite
	        +-- rcf.conf.mytestbed
	        +-- rcf.conf.mytestbed2
	        +-- rcf.conf.mytestbed3

To start TE with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, but still without :ref:`Configurator <doxid-group__te__engine__conf>` and :ref:`Tester <doxid-group__te__engine__tester>`, run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --conf-builder=builder.conf.mysuite --conf-rcf=rcf.conf.mytestbed --no-cs --no-tester

If you have some problems with copying Test Agent images to set-up hosts or if you have problems with connection to these Agents you should first check that you are able to enter these hosts without password prompt (read :ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>` for more information).

Anyway when dispatcher.sh script finishes you can check results in text log file build/log.txt.



.. _doxid-group__te__user__run__details_1te_user_run_logger:

Running Logger
--------------

:ref:`Logger <doxid-group__te__engine__logger>` configuration file depends on :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file in case we need to specify log polling intervals on per Test Agent basis, but in most cases logger configuration file specifies common polling interval to use for accessing all Test Agents.

For more information on :ref:`Logger <doxid-group__te__engine__logger>` configuration file read :ref:`Configuration File <doxid-group__te__engine__logger_1te_engine_logger_conf_file>`.

More often :ref:`Logger <doxid-group__te__engine__logger>` configuration file is the same for different test set-ups, so preferably if its name is logger.conf, because :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` uses this file name as the default :ref:`Logger <doxid-group__te__engine__logger>` configuration file.





.. _doxid-group__te__user__run__details_1te_user_run_conf:

Running Configurator
--------------------

To run :ref:`Configurator <doxid-group__te__engine__conf>` you need to prepare a configuration file whose name is passed to dispatcher.sh script. For the details on :ref:`Configurator <doxid-group__te__engine__conf>` configuration file read :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>` section.

Assuming our :ref:`Configurator <doxid-group__te__engine__conf>` configuration file is split into two parts we would have the following directory tree structure:

.. code-block:: none


	${PRJ_ROOT}
	  +-- conf
	        +-- builder.conf.mysuite
	        +-- rcf.conf.mytestbed
	        +-- rcf.conf.mytestbed2
	        +-- rcf.conf.mytestbed3
	        +-- logger.conf
	        +-- cs.conf.common
	        +-- cs.conf.mytestbed
	        +-- cs.conf.mytestbed2
	        +-- cs.conf.mytestbed3

Where cs.conf.mytestbed file can look as following:

.. code-block:: none


	<?xml version="1.0"?>
	<history>
	  <xi:include href="cs.conf.common" parse="xml"
	              xmlns:xi="http://www.w3.org/2003/XInclude"/>
	  <!-- BSD vs Win specific objects and instances descriptions -->

To start TE with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, :ref:`Configurator <doxid-group__te__engine__conf>`, but without :ref:`Tester <doxid-group__te__engine__tester>`, run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --conf-builder=builder.conf.mysuite --conf-rcf=rcf.conf.mytestbed --conf-cs=cs.conf.mytestbed --no-tester





.. _doxid-group__te__user__run__details_1te_user_run_tester:

Running Tester
--------------

Running :ref:`Tester <doxid-group__te__engine__tester>` requires some test suite to be availabe.

For more information on :ref:`Tester <doxid-group__te__engine__tester>` configuration file read :ref:`Tester Root Configuration File <doxid-group__te__engine__tester_1te_engine_tester_conf_root>` section.

For information on how to create a test suite read :ref:`Test Suite <doxid-group__te__ts>` page.

Suppose you have the following test project directory structure:

.. code-block:: none


	${PRJ_ROOT}
	  +-- conf
	  |     +-- builder.conf.mysuite
	  |     +-- rcf.conf.mytestbed
	  |     +-- rcf.conf.mytestbed2
	  |     +-- rcf.conf.mytestbed3
	  |     +-- logger.conf
	  |     +-- cs.conf.common
	  |     +-- cs.conf.mytestbed
	  |     +-- cs.conf.mytestbed2
	  |     +-- cs.conf.mytestbed3
	  |     +-- tester.conf
	  +-- suite-src
	        +-- configure.ac
	        +-- Makefile.am
	        +-- package.xml
	        +-- prologue.c
	        +-- test1.c
	        +-- test2.c
	        +-- pkg1
	        |     +-- package.xml
	        |     +-- test3.c
	        |     +-- test4.c
	        +-- pkg2
	              +-- package.xml
	              +-- test5.c
	              +-- test6.c

The content of ${PRJ_ROOT}/conf/tester.conf is:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<tester_cfg version="1.0">
	    <maintainer mailto="te-maint@oktetlabs.ru"/>
	    <description>Minimal test suite</description>

	    <suite name="test-suite" src="${PRJ_ROOT}/suite-src"/>
	    <run>
	        <package name="test-suite"/>
	    </run>
	</tester_cfg>

Before we run TE with :ref:`Tester <doxid-group__te__engine__tester>` we need to make sure the test suite tree has a ``meson.build`` next to every ``package.xml``. Nothing has to be generated by hand: :ref:`Builder <doxid-group__te__engine__builder>` runs meson itself.

If we need to (re-)build test suite sources at :ref:`Tester <doxid-group__te__engine__tester>` start-up we should run :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` as:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --conf-builder=builder.conf.mysuite --conf-rcf=rcf.conf.mytestbed --conf-cs=cs.conf.mytestbed --conf-tester=tester.conf

This command will build TE, build test suites specified in tester.conf file and run all tests according to :ref:`Tester <doxid-group__te__engine__tester>` configuration file and test package description files.

If you need to run the particular test from a test suite (say test6) you can run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --conf-builder=builder.conf.mysuite --conf-rcf=rcf.conf.mytestbed --conf-cs=cs.conf.mytestbed --no-builder --tester-no-build --tester-run=test-suite/pkg2/test6

Please note that we do not specify :ref:`Tester <doxid-group__te__engine__tester>` configuration file, because :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` uses tester.conf as the default name of :ref:`Tester <doxid-group__te__engine__tester>` configuration file.

Also note that we ask :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` to skip building TE (no-builder option) and skip building test suite (assuming we already built it, it is possible to specify tester-no-build option).

For more information on :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` options related to :ref:`Tester <doxid-group__te__engine__tester>` please read :ref:`Dispatcher Command Line Options <doxid-group__te__engine__dispatcher_1te_engine_dispatcher_options>`.





.. _doxid-group__te__user__run__details_1te_user_log_result:

Logging results
---------------

During TE run time a number of log messages generated from different components of TE. All messages are gathered by :ref:`Logger <doxid-group__te__engine__logger>` and put into a binary file that by default has tmp_raw_log name and put under a directory where dispatcher.sh run.

You can specify the location and name of binary raw log file exporting TE_LOG_RAW environment variable:

.. code-block:: none


	TE_LOG_RAW=/tmp/my_raw_log ${TE_BASE}/dispatcher.sh --conf-dir=conf

Alternatively you may put raw log file under a particular directory, then you should specify log-dir option:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --log-dir=log

In this case raw log file will be saved to ${PRJ_ROOT}/log/tmp_raw_log file.

By default :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` will generate log report in plain text format (with the help of RGT tool). By default plain text log is put under run directory with name log.txt.

If you want HTML-based multi-page structured log you should pass log-html option to :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` specifying directory name where to output log in HTML format:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf --log-html=html-out

As the result HTML based log report can be found under ${PRJ_ROOT}/html-out directory (open index.html file in a browser).

For more information on log related options of :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` read :ref:`Dispatcher Command Line Options <doxid-group__te__engine__dispatcher_1te_engine_dispatcher_options>` section.



.. _doxid-group__te__user_1te_user_run:

TE Execution
~~~~~~~~~~~~

To run TE one needs to have configuration files for all the TE subsystems.

The previous section, :ref:`Configuring a run <doxid-group__te__user__run__details>`, covers what each of them holds.

Below we describe what happens upon dispatcher.sh execution and what artifacts are left after it.

As it was mentioned above the TE can be started by run.sh script located in you test suite directory or by direct dispatcher.sh invocation.

You should either specify all the configuration files with --conf-\* options or use the default ones (see the dispatcher script):

.. code-block:: none


	CONF_BUILDER=builder.conf
	CONF_LOGGER=logger.conf
	CONF_TESTER=tester.conf
	CONF_CS=cs.conf
	CONF_RCF=rcf.conf
	CONF_NUT=nut.conf

To avoid extremely long command lines you can use --opts option and pass a file:

.. code-block:: none


	$ cat conf/run.opts.defaults
	--conf-builder=builder.conf.default
	--conf-tester=tester.conf.default
	$ ./dispatcher.sh --opts=run.opts.defaults --tester-run=foobar-ts/basic/trivial

In case you're running with run.sh there is even faster way to start the framework.

.. code-block:: none


	$ ./run.sh --cfg=<cfgname> --tester-run=foobar-ts/basic/trivial

In this case the conf/run.conf.<cfgname> file is considered as an option file. This is very useful as usually project has several test configurations (i.e. **apple**, **carrot** and **tomato**). Although each configuration may use its own hosts they usually share tester.conf, builder.conf (if all hosts run the same operating system), cs.conf

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


