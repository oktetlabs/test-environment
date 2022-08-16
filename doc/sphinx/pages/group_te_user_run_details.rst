..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; TE Config Files Basics
.. _doxid-group__te__user__run__details:

TE Config Files Basics
======================

.. toctree::
	:hidden:



.. _doxid-group__te__user__run__details_1te_user_run_rcf:

Running RCF
~~~~~~~~~~~

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
	  +-- conf_ipv6
	        +-- builder.conf.ipv6_demo
	        +-- rcf.conf.bsd_win
	        +-- rcf.conf.bsd_linux
	        +-- rcf.conf.linux_win

To start TE with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, but still without :ref:`Configurator <doxid-group__te__engine__conf>` and :ref:`Tester <doxid-group__te__engine__tester>`, run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf_ipv6 --conf-builder=builder.conf.ipv6_demo --conf-rcf=rcf.conf.bsd_win --no-cs --no-tester

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
	  +-- conf_ipv6
	        +-- builder.conf.ipv6_demo
	        +-- rcf.conf.bsd_win
	        +-- rcf.conf.bsd_linux
	        +-- rcf.conf.linux_win
	        +-- logger.conf
	        +-- cs.conf.common
	        +-- cs.conf.bsd_win
	        +-- cs.conf.bsd_linux
	        +-- cs.conf.linux_win

Where cs.conf.bsd_win file can look as following:

.. code-block:: none


	<?xml version="1.0"?>
	<history>
	  <xi:include href="cs.conf.common" parse="xml"
	              xmlns:xi="http://www.w3.org/2003/XInclude"/>
	  <!-- BSD vs Win specific objects and instances descriptions -->

To start TE with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, :ref:`Configurator <doxid-group__te__engine__conf>`, but without :ref:`Tester <doxid-group__te__engine__tester>`, run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf_ipv6 --conf-builder=builder.conf.ipv6_demo --conf-rcf=rcf.conf.bsd_win --conf-cs=cs.conf.bsd_win --no-tester





.. _doxid-group__te__user__run__details_1te_user_run_tester:

Running Tester
--------------

Running :ref:`Tester <doxid-group__te__engine__tester>` requires some test suite to be availabe.

For more information on :ref:`Tester <doxid-group__te__engine__tester>` configuration file read :ref:`Tester Root Configuration File <doxid-group__te__engine__tester_1te_engine_tester_conf_root>` section.

For information on how to create a test suite read :ref:`Test Suite <doxid-group__te__ts>` page.

Suppose you have the following test project directory structure:

.. code-block:: none


	${PRJ_ROOT}
	  +-- conf_ipv6
	  |     +-- builder.conf.ipv6_demo
	  |     +-- rcf.conf.bsd_win
	  |     +-- rcf.conf.bsd_linux
	  |     +-- rcf.conf.linux_win
	  |     +-- logger.conf
	  |     +-- cs.conf.common
	  |     +-- cs.conf.bsd_win
	  |     +-- cs.conf.bsd_linux
	  |     +-- cs.conf.linux_win
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

The content of ${PRJ_ROOT}/conf_ipv6/tester.conf is:

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

Before we run TE with :ref:`Tester <doxid-group__te__engine__tester>` we need to make sure test suite tree has configure script in the top level directory and Makefile.in files in all subdirectories. These files can be generated with autoreconf utility run in test suite top level directory.

If we need to (re-)build test suite sources at :ref:`Tester <doxid-group__te__engine__tester>` start-up we should run :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` as:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf_ipv6 --conf-builder=builder.conf.ipv6_demo --conf-rcf=rcf.conf.bsd_win --conf-cs=cs.conf.bsd_win --tester-conf=tester.conf

This command will build TE, build test suites specified in tester.conf file and run all tests according to :ref:`Tester <doxid-group__te__engine__tester>` configuration file and test package description files.

If you need to run the particular test from a test suite (say test6) you can run:

.. code-block:: none


	${TE_BASE}/dispatcher.sh --conf-dir=conf_ipv6 --conf-builder=builder.conf.ipv6_demo --conf-rcf=rcf.conf.bsd_win --conf-cs=cs.conf.bsd_win --no-builder --tester-no-build --tester-run=test-suite/pkg2/test6

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

