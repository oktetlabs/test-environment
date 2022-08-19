..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Tester
.. _doxid-group__te__engine__tester:

Tester
======

.. toctree::
	:hidden:



.. _doxid-group__te__engine__tester_1te_engine_tester_introduction:

Introduction
~~~~~~~~~~~~

:ref:`Tester <doxid-group__te__engine__tester>` is an application of :ref:`Test Engine <doxid-group__te__engine>` that controls running tests according to configuration files.

.. image:: /static/image/ten_tester_context.png
	:alt: Tester context in TE

In the context of Test Environment tests are organized in groups called Test Packages. From the Tester point of view, the Test Package is a set of executables (binary or script files can be run in shell with set of parameters passed via command line) accompanied by a file package.xml describing how to run these scripts (execution order, test parameter values, etc.). See :ref:`Tester Concepts <doxid-group__te__engine__tester_1te_engine_tester_concepts>` for more details.

It is possible to run the same test with different parameters. Parameters for a test are specified in package.xml file. Tests may be iterated with different combinations of parameter values. Such iterations are performed by :ref:`Tester <doxid-group__te__engine__tester>`.

:ref:`Tester <doxid-group__te__engine__tester>` has complex parameters iteration and multiplication logic, see :ref:`Iteration and usecases <doxid-group__te__engine__tester_1te_engine_tester_iteration>`.

Some tests may be marked as (associated with) checking the particular requirement(s) of the product. :ref:`Tester <doxid-group__te__engine__tester>` allows to run tests checking a particular set of requirements. See :ref:`Test Requirements <doxid-group__te__engine__tester_1te_engine_tester_req>`.

:ref:`Tester <doxid-group__te__engine__tester>` may be asked to build the tests from sources. In this case tests should be built using meson or GNU tools (make/autoconf/automake). All TE libraries used by the tests should be specified in the :ref:`Builder <doxid-group__te__engine__builder>` configuration file and be built/installed before :ref:`Tester <doxid-group__te__engine__tester>` starting.

:ref:`Tester <doxid-group__te__engine__tester>` is responsible for the error recovery. It utilizes a service provided by :ref:`Configurator <doxid-group__te__engine__conf>` to make a backup configuration for each package and test and to restore backups if necessary.

:ref:`Tester <doxid-group__te__engine__tester>` utilizes logging facilities provided by :ref:`Logger <doxid-group__te__engine__logger>` to log test/package starting and finishing events. This information is then used by Report Generator to split the log to sections corresponding to particular tests/packages.





.. _doxid-group__te__engine__tester_1te_engine_tester_concepts:

Tester Concepts
~~~~~~~~~~~~~~~

**Test** is a complete sequence of actions required to achieve a specific purpose (check that tested system provides a required functionality, complies a standard, etc.) and producing a verdict pass/fail (possibly accompanied by additional data).

**Test** **Package** is a group of tightly related tests or test packages, which may share internal libraries and usually run together (one-by-one or simultaneously). Test Package may consist of one test. It may have a prologue (performing some initialization) and epilogue (releasing resources and restoring TE configuration).

**Test** **Script** is a test which is a minimal structural unit of a test harness.

**Test** **Suite** is a Test Package which may be considered as standalone entity from organisational point of view and build issues.

The hierarchy of these objects is:

* Test Suite consists of several Test Packages;

* Test Package includes several Tests;

* each Test has a corresponding Test Script.





.. _doxid-group__te__engine__tester_1te_engine_tester_req:

Test Requirements
~~~~~~~~~~~~~~~~~

Requirements are string labels which describe:

* product features;

* configuration features;

* scenario description.

Test can be associated with a set of requirements. I.e.

* **LINUX** the test should be executed only on Linux;

* **LINUX-3.2** specific kernel version is required;

* **PRODUCTION_BUILD** test should be executed only on production build (non-debug);

* **SELECT** test checks select() function behaviour;

* **IGMP** test checks IGMP-support of the product;

* **LONG** test scenario is really long;

* **BROKEN** test scenario is broken - avoid!.

Requirements can be normal and sticky. Sticky requirements are inherited by all descendants (see the hierarchy description in :ref:`Tester Concepts <doxid-group__te__engine__tester_1te_engine_tester_concepts>`).

Tester can be asked to run test satisfying certain requirements via tester-req option which can be set to logical expression where logical variables are test requirements and logical operators are ! (not; should be escaped in bash), \| (or) and & (and). For example,

.. code-block:: shell


	$ dispatcher.sh --tester-req=\!BROKEN : skip broken tests
	$ dispatcher.sh --tester-req="MSG_MORE|TCP_CORK" : run all the tests involving MSG_MORE flag or TCP_CORK socket option

If you specify multiple tester-req options, it works as if their values are concatenated into a single logical expression by means of & (and) logical operator and passed with a single tester-req option.

.. code-block:: shell


	$ dispatcher.sh --tester-req=IGMP --tester-req=\!LONG : run fast IGMP tests





.. _doxid-group__te__engine__tester_1te_engine_tester_conf:

Configuration File
~~~~~~~~~~~~~~~~~~



.. _doxid-group__te__engine__tester_1te_engine_tester_conf_root:

Tester Root Configuration File
------------------------------

Tester configuration file is a root of testing scenario. This file contains some auxiliary information like description and list of maintainers as well as information about test suites.

XSD schema of :ref:`Tester <doxid-group__te__engine__tester>` configuration file can be found at ${TE_BASE}/doc/xsd/tester_config.xsd file.

:ref:`Tester <doxid-group__te__engine__tester>` configuration file can look like:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<tester_cfg version="1.0">
	    <maintainer name="TE Maintainers" mailto="te-maint@oktetlabs.ru"/>

	    <description>IPv6 Protocol Testing</description>

	    <suite name="ipv6_host" src="${TE_TS_IPV6_HOST}"/>
	    <suite name="ipv6_router" bin="${TE_TS_IPV6_ROUTER_BIN}"/>

	    <run>
	        <package name="ipv6_host"/>
	    </run>
	    <run>
	        <package name="ipv6_router"/>
	    </run>
	</tester_cfg>

Here is some description of used TAGs of configuration file:

* maintainer - specify information about the maintainer of this configuration;

* description - optional description of this configuration. Please note that this is rather configuration file description and it has nothing in common to the description of test suite(s) - you can refer to the same test suite from different :ref:`Tester <doxid-group__te__engine__tester>` configuration files;

* suite - specify information about the location of top level test package (test suite). This directive tells :ref:`Tester <doxid-group__te__engine__tester>` where to find sources or binaries of a test suite. The only suites mentioned in suite directives are built (if necessary) by the :ref:`Tester <doxid-group__te__engine__tester>`.

  suite tag can keep the following attributes:

  * name - specifies test suite name that is used as an identifier associated with this test suite;

  * src - tells :ref:`Tester <doxid-group__te__engine__tester>` where to find sources of mentioned test suite;

  * bin - tells :ref:`Tester <doxid-group__te__engine__tester>` where to find binarires of mentioned test suite (please note that src and bin attributes are mutualy exclusive, only one of them can present at the same time in suite directive).

    It is possible not to have both src and bin attributes in suite tag, which means you ask :ref:`Tester <doxid-group__te__engine__tester>` to build a test suite located under ${TE_BASE}/suites/[suite name] directory.

* run - a block of tests, packages to run.

In the above example we tell :ref:`Tester <doxid-group__te__engine__tester>` :

* to build a test suite located under ${TE_TS_IPV6_HOST} directory. :ref:`Tester <doxid-group__te__engine__tester>` will ask :ref:`Builder <doxid-group__te__engine__builder>` to build and install this test suite;

* to associate a directory ${TE_TS_IPV6_ROUTER_BIN} with installed version of test suite reffered as ipv6_router;

* to run a test suite named ipv6_host (it will be run from the place where it is installed by :ref:`Builder <doxid-group__te__engine__builder>`);

* to run a test suite named ipv6_router (it will be run from ${TE_TS_IPV6_ROUTER_BIN}directory that is previously associated with bin attribute of suite directive.

If you only need to build a set of test suites you can have :ref:`Tester <doxid-group__te__engine__tester>` configuration file as following:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<tester_cfg version="1.0">
	    <maintainer name="TE Maintainers" mailto="te-maint@oktetlabs.ru"/>
	    <description>Build IPv6 related test suites</description>

	    <suite name="ipv6_host" src="${TE_TS_IPV6_HOST}"/>
	    <suite name="ipv6_router"/>
	</tester_cfg>

This configuration file asks :ref:`Tester <doxid-group__te__engine__tester>` to:

* build a test suite whose sources located at ${TE_TS_IPV6_HOST} directory;

* build a test suite located at ${TE_BASE}/suites/ipv6_router directory.





.. _doxid-group__te__engine__tester_1te_engine_tester_conf_pkg:

Tester Package Description File
-------------------------------

Test package description files contain nodes of testing scenario tree. Testing scenario tree is built of test script nodes to be executed with particular set of parameters and particular execution order.

In order to group some test scripts together there are entities called sessions and packages.

Test script is an external applications to be called by Tester. Test session is a complex structure that can include different components to run. Test sessions may have prologue that is run at the start of a session, epilogue that is executed after the session and keep-alive validation to run before and after each test script execution.

XSD schema of Package description files can be found at ${TE_BASE}/doc/xsd/test_package.xsd.

Here is an example of the simplest package description file:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<package version="1.0">
	  <description>Device LED operation Test Suite</description>

	    <author mailto="te-maint@oktetlabs.ru"/>

	    <session>
	        <enum name="led_type">
	            <value>Power</value>
	            <value>USB</value>
	            <value>Ethernet</value>
	            <value>Error</value>
	        </enum>

	        <keepalive>
	            <script name="notify_test_step"/>
	        </keepalive>

	        <prologue>
	            <script name="led_prologue"/>
	        </prologue>

	        <epilogue>
	            <script name="led_epilogue"/>
	        </epilogue>

	        <run>
	            <script name="led_on_off"/>
	            <arg name="led" type="led_type"/>
	        </run>
	            <script name="led_blink"/>
	            <arg name="led" type="led_type"/>
	        <run>
	    </session>
	</package>

This package description file has:

* session - mandatory part of package description file.

* prologue - prologue entry. This entry defines execution subtree that is run by :ref:`Tester <doxid-group__te__engine__tester>` on package start-up. In our case we run only one script called led_prologue, which means that in test suite installation directory in corresponding place there should be a binary file or executable script with name led_prologue;

* epilogue - epilogue entry. This entry defines execution subtree that is run by :ref:`Tester <doxid-group__te__engine__tester>` on package termination (after processing of all run entries);

* keepalive - this is an entry that :ref:`Tester <doxid-group__te__engine__tester>` runs after processing each run entry;

* run - an item to run. run directive can contain one of the following:

  * script - to ask :ref:`Tester <doxid-group__te__engine__tester>` run a single executable (a number of times with different parameters);

  * package - to ask :ref:`Tester <doxid-group__te__engine__tester>` process another package, in which case :ref:`Tester <doxid-group__te__engine__tester>` will process a separate package description file recursively;

  * session - to have nested session syntax;

  In our case we have plain script directives that point to particular executables with names led_on_off and led_blink;

* at the beginning of the session we define enumeration with enum TAG that we can later refer while specifying test parameters. In our sample we define enumeration type that can be reffered by name led_type;

* we refer to enumeration type inside run TAG when specify parameters for scripts. In our case we define parameter led that has values of type led_type, which means the same executable will be run as many number of times as the number of possible values in type led_type.



.. _doxid-group__te__engine__tester_1te_engine_tester_track_conf:

Tracking and rolling back configuration changes
-----------------------------------------------

By default Tester saves current configuration tree state before starting
a test; after termination of the test, it then checks whether configuration
state changed. If it changed, it restores previous configuration state and
sets test result to **DIRTY**. Note that this does not apply to prologues;
configuration changes made by a prologue are rolled back at the end of
the corresponding session (including epilogue if it is present).

So, by default it is assumed that tests do not change configuration.

Default behaviour can be changed with help of **track_conf** attribute which
can be specified in package description file like this

.. ref-code-block:: xml

    <run>
      <script name="some_tcp_test" track_conf="silent">
        <req id="SOCK_STREAM"/>
      </script>
    </run>


**track_conf** can have the following values:
  * **no**. Tester does not track configuration changes at all, so it does not
    try to restore configuration state if it was changed.
  * **yes** or **barf**. This is the default behaviour described above.
  * **yes_nohistory** or **barf_nohistory**. This differs from the default
    behaviour only in that Tester does not try to roll configuration
    changes back by executing configuration commands stored in history in
    reverse order. Instead it jumps directly to restoring configuration
    by changing only those configuration objects whose state differs from
    configuration backup (normally this is done only if restoring
    configuration from history fails).
  * **silent**. Tester tracks and restores configuration state as usual, but
    it does not set to **DIRTY** results of the tests which changed
    configuration. This is convenient if you do not want to undo all
    changes manually in tests cleanup but instead rely on Tester to do it
    for you.
  * **nohistory** or **silent_nohistory**. Works like **silent** but also does
    not try to restore configuration from history like **yes_nohistory**.

There is also **sync** flag which can be added to these values (for **no**
it makes no sense though), like **nohistory|sync**. When the flag is set,
Tester synchronizes configuration state with Test Agents before checking
whether it is changed. This way it can catch fully unexpected changes
(like side effects of commands such as ioctl(SIOCETHTOOL/ETHTOOL_RESET)).
However synchronizing configuration with all Test Agents takes extra time.
That's why it is not done by default and extra flag is required to
enable it.

**track_conf** attribute can be specified not only for **<script>** but also
for **<session>** and **<run>**. In that case it can be used together with
**track_conf_handdown** attribute which specifies how **track_conf** attribute
is inherited by descendants.

.. ref-code-block:: xml

    <session track_conf="silent" track_conf_handdown="descendants">
      ...
    </session>


**track_conf_handdown** can have the following values:
  * **none**. **track_conf** is not inherited by any children.
  * **children**. **track_conf** is inherited by direct children only.
  * **descendants**. **track_conf** is inherited by all descendants.

By default **track_conf** is inherited by all descendants of **<run>** item
but only by direct children of **<session>** due to historical reasons.


.. _doxid-group__te__engine__tester_1te_engine_tester_package_syntax:

Package File Syntax
-------------------

XML schema for :ref:`Tester <doxid-group__te__engine__tester>` configuration file may be found in doc/xsd/tester_config.xsd file.







.. _doxid-group__te__engine__tester_1te_engine_tester_iteration:

Iteration and usecases
~~~~~~~~~~~~~~~~~~~~~~



.. _doxid-group__te__engine__tester_1te_engine_tester_iteration_parameters:

Parameters
----------

Every test has a set of parameters and every parameter has certain values. In the package.xml file it's represented as:

.. ref-code-block:: xml

	<session>
	  <run>
	    <script name="test1"/>
	    <arg name="arg1">
	       <value>val1</value>
	       <value>val2</value>
	       <value>val3</value>
	    </arg>
	   </run>
	</session>

So test **test1** has one argument **arg1** which has three possible values **val1**, **val2** and **val3**.

When executed it will result in:

.. code-block:: none


	$ ./run.sh --tester-run=myts/mypackage/test1
	....
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val1
	Starting test test1             PASSED     <- arg1=val2
	Starting test test1             PASSED     <- arg1=val3
	Done package mypackage PASSED

You can run test with specific argument value or values:

.. code-block:: none


	$ ./run.sh --tester-run=mysuite/mypkg/test1:arg1=val2
	or
	$ ./run.sh --tester-run=mysuite/mypkg/test1:arg1={val1, val3}

--tester-run-from and --tester-run-to can be used to run part of the sequence.

To run the second iteration one should call:

.. code-block:: none


	$ ./run.sh --tester-run=mysuite/mypkg/test1%2
	...
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val2
	Done package mypackage PASSED

You can also specify a step with (test1%1+2) sytax, this will run only odd iterations.

Iteration is specified with an asterisk:

.. code-block:: none


	$ ./run.sh --tester-run=mysuite/mypkg/test1%2*3
	...
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val2
	Starting test test1             PASSED     <- arg1=val2
	Starting test test1             PASSED     <- arg1=val2
	Done package mypackage PASSED

Negation with tilda:

.. code-block:: none


	$ ./run.sh --tester-run=myts/mypackage/test1:arg1~=val2
	....
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val1
	Starting test test1             PASSED     <- arg1=val3
	Done package mypackage PASSED





.. _doxid-group__te__engine__tester_1te_engine_tester_iteration_types:

Types
-----

Arguments can have certain types. If explicit values are specified:

.. ref-code-block:: xml

	<session>
	  <run>
	    <script name="test1"/>
	    <arg name="arg1" type="boolean">
	       <value>True</value>
	       <value>False</value>
	    </arg>
	   </run>
	</session>

then they are checked agains given type. If no values then all values of the type are iterated:

.. ref-code-block:: xml

	<enum name="sock_type">
	  <value>SOCK_STREAM</value>
	  <value>SOCK_DGRAM</value>
	</enum>
	<session>
	  <run>
	    <script name="test1"/>
	    <arg name="socket_type" type="sock_type"/>
	   </run>
	</session>





.. _doxid-group__te__engine__tester_1te_engine_teste_iteration_multiplication:

Multiplication
--------------

Set of argument values are multiplied:

.. ref-code-block:: xml

	<session>
	  <run>
	    <script name="test1"/>
	    <arg name="arg1">
	       <value>val1</value>
	       <value>val2</value>
	    </arg>
	    <arg name="arg2">
	       <value>val3</value>
	       <value>val4</value>
	    </arg>
	   </run>
	</session>

results into

.. code-block:: none


	$ ./run.sh --tester-run=mysuite/mypkg/test1
	...
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val1, arg2=val3
	Starting test test1             PASSED     <- arg1=val1, arg2=val4
	Starting test test1             PASSED     <- arg1=val2, arg2=val3
	Starting test test1             PASSED     <- arg1=val2, arg2=val4
	Done package mypackage PASSED

If full multiplication should not be performed then **list** attribute should be used:

.. ref-code-block:: xml

	<session>
	  <run>
	    <script name="test1"/>
	    <arg name="arg1" list="mylist">
	       <value>val1</value>
	       <value>val2</value>
	    </arg>
	    <arg name="arg2" list="mylist">
	       <value>val3</value>
	       <value>val4</value>
	    </arg>
	   </run>
	</session>

results into

.. code-block:: none


	$ ./run.sh --tester-run=mysuite/mypkg/test1
	...
	Staring package mypackage
	Starting test test1             PASSED     <- arg1=val1, arg2=val3
	Starting test test1             PASSED     <- arg1=val2, arg2=val4
	Done package mypackage PASSED

Important points:

* number of values for all arguments in the same list should be equal;

* multiplication and lists can be combined;

* multiplication significantly increases number of test iterations, so one should be sure that he wants to check all combinations.





.. _doxid-group__te__engine__tester_1te_engine_teste_iteration_variable:

Variables
---------

You can specify variables in package description file and later refer to values of these variables when specifying values for test arguments.

.. ref-code-block:: xml

	<session>
	  <var name="long_path" global="true">
	    <value>/this/is/very/long/path/to/a/file/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa</value>
	  </var>
	  <var name="up_case_name">
	    <value>AAAAAA.TXT</value>
	  </var>

	  <run>
	    <script name="readdir"/>
	    <arg name="path">
	       <value>/</value>
	       <value>/dir</value>
	       <value ref="long_path"/>
	    </arg>
	  </run>

	  <run>
	    <script name="file_open"/>
	    <arg name="path">
	       <value ref="up_case_name"/>
	    </arg>
	  </run>
	</session>

In this sample we create two variables **long_path** and **up_case_name** with particular values. Later we can refer to these variables using ref attribute - as one of values for **path** parameter of **readdir** test or as the value for **path** parameter of **file_open** test.

Todo More details should be added.

