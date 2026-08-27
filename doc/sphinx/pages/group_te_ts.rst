..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te
  te-order: 20

.. index:: pair: group; Test Suite
.. _doxid-group__te__ts:

Test Suite
==========

.. include:: _toctree/te_ts.inc

.. _doxid-group__te__ts_1te_ts_terminology:

Terminology
~~~~~~~~~~~

============  ================================================================================================================================================================================
Term          Definition
============  ================================================================================================================================================================================
Test Package  Group of tightly related tests or test packages, which may share internal libraries and usually run together (one-by-one or simultaneously).
              Test Package may consist of one test. It may have a prologue (performing some initialization) and epilogue (releasing resources and restoring TE configuration).
Test Script   A test which is a minimal structural unit of a test harness.
Test Suite    Test Package which may be considered as standalone entity from organisational point of view and build issues.
============  ================================================================================================================================================================================


.. _doxid-group__te__ts_1te_ts_tree_structure:

Directory tree structure
~~~~~~~~~~~~~~~~~~~~~~~~

Test suite can be distributed in two forms:

#. pre-installed binary form;

#. source based form.

For pre-installed binary test suite does not require building procedure, which is why there is no need to have build related files.

Pre-installed binary test suite has the following directory structure:

.. code-block:: none


	${TS_ROOT}
	  +-- package.xml
	  +-- prologue
	  +-- epilogue
	  +-- p1_test1
	  +   ...
	  +-- p1_testN
	  +-- subpackage
	       +-- package.xml
	       +-- prologue
	       +-- epilogue
	       +-- p2_test1
	       +   ...
	       +-- p2_testN

A test suite consists of a set of packages each containing a number of test executables and package description file. For the details on the format of package.xml files refer to :ref:`Tester Package Description File <doxid-group__te__engine__tester_1te_engine_tester_conf_pkg>` section.

A source based test suite additionally has build files. Like every other
component of TE it is built through :ref:`Builder <doxid-group__te__engine__builder>`,
which uses meson, so each directory that contains tests needs a ``meson.build``
next to its ``package.xml``:

.. code-block:: none


	${TS_ROOT}
	  +-- package.xml
	  +-- meson.build
	  +-- prologue.c
	  +-- epilogue.c
	  +-- p1_test1.c
	  +   ...
	  +-- p1_testN.c
	  +-- subpackage
	       +-- package.xml
	       +-- meson.build
	       +-- prologue.c
	       +-- epilogue.c
	       +-- p2_test1.c
	       +   ...
	       +-- p2_testN.c

There is nothing to run by hand before the build: :ref:`Builder <doxid-group__te__engine__builder>`
invokes meson itself when :ref:`Tester <doxid-group__te__engine__tester>` asks
for the suite.

.. note:: Some old suites in the tree still carry ``configure.ac`` and
	``Makefile.am`` from the days when Builder used autotools. Do not copy
	them for new work.


.. _doxid-group__te__ts_1te_ts_min:

Minimal Test Suite
~~~~~~~~~~~~~~~~~~

The smallest complete example lives in TE's own self-test suite, under
``${TE_BASE}/suites/selftest``. It is built and run on every change to TE, so
unlike a written-down example it cannot quietly stop working.

Its layout is the one described above:

.. code-block:: none


	${TE_BASE}/suites/selftest
	  +-- run.sh                 - entry point, wraps dispatcher.sh
	  +-- conf
	  |     +-- builder.conf
	  |     +-- cs.conf
	  |     +-- logger.conf
	  |     +-- rcf.conf
	  |     +-- tester.conf
	  +-- ts
	        +-- meson.build      - suite level build file
	        +-- package.xml      - root package
	        +-- prologue.c
	        +-- minimal          - the package we look at below
	              +-- meson.build
	              +-- package.xml
	              +-- helloworld.c
	              +-- ...

:ref:`Tester <doxid-group__te__engine__tester>` needs to be told where the suite
sources are. That is what ``conf/tester.conf`` does:

.. code-block:: xml


	<?xml version="1.0"?>
	<tester_cfg version="1.0">
	    <maintainer mailto="te-maint@oktetlabs.ru"/>
	    <description>TE Self Tests</description>
	    <syntax strip_indent="true"/>

	    <suite name="ts" src="${TE_TS_DIR}"/>

	    <run>
	        <package name="ts"/>
	    </run>
	</tester_cfg>

The ``src`` attribute points at the suite sources and the ``run`` section says
which package to execute. Everything below that point is described by
``package.xml`` files.

To run it:

.. code-block:: none


	cd ${TE_BASE}/suites/selftest
	./run.sh --cfg=localhost

``run.sh`` is a thin wrapper around ``dispatcher.sh`` that fills in the
suite-specific options; ``--cfg=<name>`` selects one of the configurations under
``conf/run``. See :ref:`TE Execution <doxid-group__te__user_1te_user_run>` for
what happens next and where the logs end up.

The build artefacts appear under the build directory:

.. code-block:: none


	build
	  +-- engine     - :ref:`Test Engine <doxid-group__te__engine>` build directory
	  +-- agents     - :ref:`Test Agents <doxid-group__te__agents>` build directory
	  +-- lib        - build directory for TE libraries
	  +-- include    - build directory for includes
	  +-- platforms  - platforms build
	  +-- suites     - test suites build directory
	  +-- inst       - installation directory
	        +-- agents
	        +-- default
	        +-- suites
	              +-- ts
	                    +-- package.xml    - installed package description file
	                    +-- minimal
	                          +-- helloworld  - test executable

Please note that :ref:`Tester <doxid-group__te__engine__tester>` runs a test suite from the inst/suites/<suite_name> directory.


.. _doxid-group__te__ts_1te_ts_min_builder:

Build files
-----------

A package ``meson.build`` lists the tests in the directory and builds one
executable per test. This is the whole of ``ts/minimal/meson.build``, with the
list of tests cut down:

.. code-block:: none


	tests = [
	    'helloworld',
	    'verdict',
	]

	foreach test : tests
	    test_exe = test
	    test_c = test + '.c'
	    package_tests_c += [ test_c ]
	    executable(test_exe, test_c, install: true, install_dir: package_dir,
	               dependencies: test_deps)
	endforeach

	install_data([ 'package.xml' ], install_dir: package_dir)

``package_dir``, ``package_tests_c`` and ``test_deps`` come from the suite level
``ts/meson.build``, which is where the TE libraries the tests link against are
declared:

.. code-block:: none


	project('selftest', 'c',
	    version : '1.0.0',
	    meson_version: '>= 0.49.0',
	)

	te_path = get_option('te_path')
	te_libdir = get_option('te_libdir')
	add_project_arguments(get_option('te_cflags').split(), language: 'c')
	add_project_link_arguments(get_option('te_ldflags').split(), language: 'c')

	test_deps = [ dependency('threads') ]

	te_libs = [
	    'rcfapi',
	    'confapi',
	    'tapi',
	    'tapi_rpc',
	    'tapi_env',
	    'tools',
	    'logger_core',
	    'logger_ten',
	]

	foreach lib : te_libs
	    test_deps += dependency('te-' + lib)
	endforeach

	package_dir = 'ts'
	package_tests_c = [ ]

	packages = [ 'minimal' ]

	mydir = package_dir
	foreach package : packages
	    package_dir = join_paths(mydir, package)
	    package_tests_c = []
	    subdir(package)
	endforeach

The ``te_path``, ``te_libdir``, ``te_cflags`` and ``te_ldflags`` options are
passed in by :ref:`Builder <doxid-group__te__engine__builder>`; a suite does not
set them itself. Each TE library is picked up as ``dependency('te-<name>')``,
and the same library must also be listed in the suite's ``builder.conf`` so that
it gets built in the first place --- see
:ref:`Builder configuration file <doxid-group__te__engine__builder_1te_engine_builder_conf_file>`.

Adding a test to an existing package therefore means three things: write the
``.c`` file, add its name to ``tests`` in ``meson.build``, and add a ``run``
entry for it in ``package.xml``.


.. _doxid-group__te__ts_1te_ts_min_test_file:

Test scenario file
------------------

A test scenario is a plain C program. This is ``ts/minimal/helloworld.c`` in
full --- it is as small as a TE test gets:

.. code-block:: c


	/* SPDX-License-Identifier: Apache-2.0 */
	/** @file
	 * @brief Minimal test
	 *
	 * Minimal test scenario.
	 *
	 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
	 */

	/** @page minimal_helloworld Hello World test
	 *
	 * @objective Demo of minimal Hello World test
	 *
	 * For each test @p TEST_STEP() is required. This is needed to generate
	 * documentation of test steps.
	 *
	 * @par Test sequence:
	 *
	 */

	#ifndef DOXYGEN_TEST_SPEC

	/** Logging subsystem entity name */
	#define TE_TEST_NAME    "helloworld"

	#include "te_config.h"
	#include "tapi_test.h"

	int
	main(int argc, char **argv)
	{
	    TEST_START;

	    TEST_STEP("Print \"Hello, World!\"");
	    RING("Hello, World!");

	    TEST_SUCCESS;

	cleanup:

	    TEST_END;
	}

	#endif /* !DOXYGEN_TEST_SPEC */

and the ``package.xml`` entry that makes :ref:`Tester <doxid-group__te__engine__tester>`
run it:

.. code-block:: xml


	<?xml version="1.0"?>
	<package version="1.0">
	    <description>Package for demonstrating minimal tests</description>
	    <author mailto="te-maint@oktetlabs.ru"/>

	    <session>
	        <run>
	            <script name="helloworld"/>
	        </run>
	    </session>
	</package>

The next section goes through what each part of the C file is for.


.. _doxid-group__te__ts_1te_ts_scenario_layout:

Test scenario layout
~~~~~~~~~~~~~~~~~~~~

Each test scenario shall have:

#. Definition of ``TE_TEST_NAME`` macro in order to specify Entity name of log messages generated from test scenario. Usually this is set to test name.

   .. ref-code-block:: cpp

   	#define TE_TEST_NAME "helloworld"

#. Inclusion of ``te_config.h`` file. This file holds the macros the build generates after checking for header files, structure fields and structure sizes (i.e. it has definitions of ``HAVE_xxx_H``, ``SIZEOF_xxx`` macros).

   .. ref-code-block:: cpp

   	#include "te_config.h"

#. Inclusion of ``tapi_test.h`` file - basic API for test scenarios.

   .. ref-code-block:: cpp

   	#include "tapi_test.h"

#. main() function. A test scenario is an executable, so in case of C source file we should have program entry point, which is main() function for the default linker script. main() function has to have argc, argv parameters, because macros defined at ``tapi_test.h`` depend on them;

#. Mandatory test points:

   * TEST_START;

   * TEST_END;

   * TEST_SUCCESS.

   The mandatory test structure is:

   .. ref-code-block:: cpp

   	{
   	    TEST_START;
   	    ...
   	    TEST_SUCCESS;
   	cleanup:
   	    TEST_END;
   	}

The following things should be taken into account while writing a test scenario:

* If you need to add local variables to your test scenario, put them BEFORE TEST_START macro:

  .. ref-code-block:: cpp

  	{
  	    /* Local variables go before TEST_START macro */
  	    int              sock;
  	    struct sockaddr *addr4;
  	    struct sockaddr *addr6;
  	    int              opt_val;

  	    TEST_START;
  	    ...
  	    TEST_SUCCESS;
  	cleanup:
  	    TEST_END;
  	}

* If a set of tests require definition of the same set of local variables we can avoid duplication these variables from test to test by using TEST_START_VARS macro:

  .. ref-code-block:: cpp

  	/* test_suite.h */
  	#define TEST_START_VARS \
  	    int sock;                 \
  	    struct sockaddr *addr4;   \
  	    struct sockaddr *addr6

  In each test scenarion we will have:

  .. ref-code-block:: cpp

  	...
  	#include "test_suite.h"
  	...
  	int main(int argc, char **argv)
  	{
  	    /* Define test-specific local variables */
  	    int              opt_val;

  	    TEST_START;
  	    ...
  	    sock = rpc_socket(rpc_srv,
  	                      RPC_AF_INET, RPC_SOCK_STREAM, RPC_IPPROTO_TCP);
  	    ...
  	}

* Another useful macros are:

  * TEST_START_SPECIFIC;

  * TEST_END_SPECIFIC.

  They can be defined if you need some common parts of code to be executed during TEST_START and TEST_END procedures. For example tests suites that use :ref:`tapi_env <doxid-structtapi__env>` library may define these macros as:

  .. ref-code-block:: cpp

  	#define TEST_START_VARS TEST_START_ENV_VARS
  	#define TEST_START_SPECIFIC TEST_START_ENV
  	#define TEST_END_SPECIFIC TEST_END_ENV


.. _doxid-group__te__ts_1te_ts_scenario_params:

Test parameters
~~~~~~~~~~~~~~~

The main function to process test parameters in test scenario context is :ref:`test_get_param() <doxid-group__te__ts__tapi__test__param_1ga77a71497ad2b8ab7c1e29125938bb85b>`. It gets parameter name as an argument value and returns string value associated with that parameter.

Apart from base function :ref:`test_get_param() <doxid-group__te__ts__tapi__test__param_1ga77a71497ad2b8ab7c1e29125938bb85b>` there are a number of macros that process type-specific parameters:

* :ref:`TEST_GET_ENUM_PARAM() <doxid-group__te__ts__tapi__test__param_1ga165df4451b2456291410cb5203b7b787>`;

* :ref:`TEST_GET_STRING_PARAM() <doxid-group__te__ts__tapi__test__param_1gaa1d7b320bc887b35d9c486c0f5c01271>`;

* :ref:`TEST_GET_INT_PARAM() <doxid-group__te__ts__tapi__test__param_1ga42ce6e09659e68964858166357d9cda9>`;

* :ref:`TEST_GET_INT64_PARAM() <doxid-group__te__ts__tapi__test__param_1gac1b946de532bfa453f97eafcef33cde7>`;

* :ref:`TEST_GET_DOUBLE_PARAM() <doxid-group__te__ts__tapi__test__param_1gae80cda63a67e37dbc4ab610a9408b7ed>`;

* :ref:`TEST_GET_OCTET_STRING_PARAM() <doxid-group__te__ts__tapi__test__param_1gaec445c18a2c06f56472575cf072d866d>`;

* :ref:`TEST_GET_STRING_LIST_PARAM() <doxid-group__te__ts__tapi__test__param_1ga5554072fbebc45f2735cfa882a7ed338>`;

* :ref:`TEST_GET_INT_LIST_PARAM() <doxid-group__te__ts__tapi__test__param_1ga41da78b0997e884a8565765a2b3567c9>`;

* :ref:`TEST_GET_BOOL_PARAM() <doxid-group__te__ts__tapi__test__param_1ga3881eaa71326e2e6ee763ad9b76dbb9d>`;

* :ref:`TEST_GET_FILENAME_PARAM() <doxid-group__te__ts__tapi__test__param_1ga6a2f77fcb3e9abafa7ed3afbf37c54d8>`;

* :ref:`TEST_GET_BUFF_SIZE() <doxid-group__te__ts__tapi__test__param_1ga4ea8dadbaddcd936470f432740911332>`.

For example for the following test run (from package.xml):

.. ref-code-block:: cpp

	<run>
	  <script name="comm_sender"/>
	    <arg name="size">
	      <value>1</value>
	      <value>100</value>
	    </arg>
	    <arg name="oob">
	      <value>TRUE</value>
	      <value>FALSE</value>
	    </arg>
	    <arg name="msg">
	      <value>Test message</value>
	    </arg>
	</run>

we can have the following test scenario:

.. ref-code-block:: cpp

	int main(int argc, char **argv)
	{
	    int      size;
	    bool  oob;
	    char    *msg;

	    TEST_START;

	    TEST_GET_INT_PARAM(size);
	    TEST_GET_BOOL_PARAM(oob);
	    TEST_GET_STRING_PARAM(msg);
	    ...
	}

Please note that variable name passed to TEST_GET_xxx_PARAM() macro shall be the same as expected parameter name.

Test suite can also define parameters of enumeration type. For this kind of parameters you will need to define a macro based on :ref:`TEST_GET_ENUM_PARAM() <doxid-group__te__ts__tapi__test__param_1ga165df4451b2456291410cb5203b7b787>`.

For example if you want to specify something like the following in your package.xml files:

.. ref-code-block:: cpp

	<enum name="ledtype">
	  <value>POWER</value>
	  <value>USB</value>
	  <value>ETHERNET</value>
	  <value>WIFI</value>
	</enum>

	<run>
	  <script name="led_test"/>
	    <arg name="led" type="ledtype"/>
	</run>

You can define something like this in your test suite header file (test_suite.h):

.. ref-code-block:: cpp

	enum ts_led {
	    TS_LED_POWER,
	    TS_LED_USB,
	    TS_LED_ETH,
	    TS_LED_WIFI,
	};
	#define LEDTYPE_MAPPING_LIST \
	           { "POWER", (int)TS_LED_POWER },  \
	           { "USB", (int)TS_LED_USB },      \
	           { "ETHERNET", (int)TS_LED_ETH }, \
	           { "WIFI", (int)TS_LED_WIFI }

	#define TEST_GET_LED_PARAM(var_name_) \
	            TEST_GET_ENUM_PARAM(var_name_, LEDTYPE_MAPPING_LIST)

Then in your test scenario you can write the following:

.. ref-code-block:: cpp

	int main(int argc, char **argv)
	{
	    enum ts_led led;

	    TEST_START;

	    TEST_GET_LED_PARAM(led);
	    ...
	    switch (led)
	    {
	        case TS_LED_POWER:
	    ...
	    }
	    ...
	}
