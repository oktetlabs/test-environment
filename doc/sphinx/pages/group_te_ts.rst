.. index:: pair: group; Test Suite
.. _doxid-group__te__ts:

Test Suite
==========

.. toctree::
	:hidden:

	/generated/group_te_lib_tapi_conf_net.rst
	/generated/group_te_ts_tapi_test.rst
	group_te_scenarios.rst



.. _doxid-group__te__ts_1te_ts_terminology:

Terminology
~~~~~~~~~~~

============  =============================================================================================================================================================================================================================================================================================================

============  =============================================================================================================================================================================================================================================================================================================

Test Package  Group of tightly related tests or test packages, which may share internal libraries and usually run together (one-by-one or simultaneously). Test Package may consist of one test. It may have a prologue (performing some initialization) and epilogue (releasing resources and restoring TE configuration).
Test Script   A test which is a minimal structural unit of a test harness.
Test Suite    Test Package which may be considered as standalone entity from organisational point of view and build issues. @tdt_tend
============  =============================================================================================================================================================================================================================================================================================================





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

.. code-block:: none

A test suite consists of a set of packages each containing a number of test executables and package description file. For the details on the format of package.xml files refer to :ref:`Tester Package Description File <doxid-group__te__engine__tester_1te_engine_tester_conf_pkg>` section.

Source based test suite additionally has build-specific files. As any other component of TE, a source based test suite is expected to be built with the help of :ref:`Builder <doxid-group__te__engine__builder>`. :ref:`Builder <doxid-group__te__engine__builder>` uses autotools in the backgroud, which means a top level package directory shall include:

* configure script.

And all directories shall include:

* Makefile.in file;

More often source based test suite will have base files of autotools:

* Makefile.am;

* configure.ac.

And before running TE we there will be need to generate configure script and Makefile.in files with the following command:

.. code-block:: none


	autoreconf --install --force

The directory structure of source-based test suite is following:

.. code-block:: none


	${TS_ROOT}
	  +-- package.xml
	  +-- configure.ac
	  +-- Makefile.am
	  +-- prologue.c
	  +-- epilogue.c
	  +-- p1_test1.c
	  +   ...
	  +-- p1_testN.c
	  +-- subpackage
	       +-- package.xml
	       +-- Makefile.am
	       +-- prologue.c
	       +-- epilogue.c
	       +-- p2_test1.c
	       +   ...
	       +-- p2_testN.c

.. code-block:: none





.. _doxid-group__te__ts_1te_ts_min:

Minimal Test Suite
~~~~~~~~~~~~~~~~~~

Minimal test suite can be found under ${TE_BASE}/suites/minimal/suite directory.

The structure of minimal project is:

.. code-block:: none


	${TE_BASE}/suites/minimal
	  +-- conf
	  |     +-- builder.conf
	  |     +-- cs.conf
	  |     +-- logger.conf
	  |     +-- rcf.conf
	  |     +-- tester.conf
	  +-- suite
	        +-- configure.ac
	        +-- Makefile.am
	        +-- package.xml
	        +-- sample1.c

:ref:`Tester <doxid-group__te__engine__tester>` configuration file (tester.conf) refers to suite sources and adds this suite for :ref:`Tester <doxid-group__te__engine__tester>` run:

.. ref-code-block:: cpp

This test suite has simplest package description file (package.xml):

.. ref-code-block:: cpp

Package description file specifies one test executable sample1 to run.

To run this test suite do the following steps:

#. under ${TE_BASE}/suites/minimal/suite run:

   .. code-block:: none


   	autoreconf --install --force

   This will generate configure script and Makefile.in file under ${TE_BASE}/suites/minimal/suite directory.

#. export TS_BASE environment variable, because :ref:`Tester <doxid-group__te__engine__tester>` configuration file (tester.conf) uses it as the value of src attribute of suite TAG.

   .. code-block:: none


   	export TS_BASE=${TE_BASE}/suites/minimal

#. set TE_BUILD variable to specify allocation of build artifacts.

   .. code-block:: none


   	export TE_BUILD=${TS_BASE}/build

   If this variable is not set, all generated files will be created in working directory.

#. run TE:

   .. code-block:: none


   	${TE_BASE}/dispatcher.sh --conf-dir=${TS_BASE}/conf

   In our test project all configuration file names match with the default names, which is why we do not specify them with separate conf-<prog> options, but only tell where to find configuration files with conf-dir option.

As the result of these operations a number of directories are created that have TE and suite build and installation files.

.. code-block:: none


	${TE_BASE}/suites/minimal
	  +-- conf
	  +-- suite
	  +-- build - build directory
	        +-- engine - :ref:`Test Engine <doxid-group__te__engine>` build directory
	        +-- agents - :ref:`Test Agents <doxid-group__te__agents>` build directory
	        +-- lib - build directory for TE libraries
	        +-- include - build directory for includes
	        +-- platforms - platforms build
	        +-- suites - test suites build directory
	              +-- minimal -- build directory of our minimal test suite
	                    +-- sample1 - test executable
	        +-- inst - installation directory
	              +-- agents - :ref:`Test Agents <doxid-group__te__agents>` installation directory
	              +-- default - te_test_engine installation directory
	              +-- suites - test suites installation directory
	                    +-- minimal -- installation directory of our minimal test suite
	                          +-- package.xml - installed package description file
	                          +-- sample1 - test executable

Please note that :ref:`Tester <doxid-group__te__engine__tester>` runs a test suite from inst/suites/<suite_name> directory.



.. _doxid-group__te__ts_1te_ts_min_builder:

Autotools files
---------------

The content of configure.ac of our minimal test suite is following:

.. ref-code-block:: cpp

The content of Makefile.am is following:

.. ref-code-block:: cpp





.. _doxid-group__te__ts_1te_ts_min_test_file:

Test scenario file
------------------

A test scenario file used in minimal test suite is a C source file:

.. ref-code-block:: cpp







.. _doxid-group__te__ts_1te_ts_scenario_layout:

Test scenario layout
~~~~~~~~~~~~~~~~~~~~

Each test scenario shall have:

#. Definition of ``TE_TEST_NAME`` macro in order to specify Entity name of log messages generated from test scenario. Usually this is set to test name.

   .. ref-code-block:: cpp

   	#define TE_TEST_NAME "sample1"

#. Inclusion of ``te_config.h`` file. This file defines macros generated by TE configure script after verification of header files, structure fields, structure sizes (i.e. it has definitions of ``HAVE_xxx_H``, ``SIZEOF_xxx`` macros).

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

The main function to process test parameters in test scenario context is :ref:`test_get_param() <doxid-group__te__ts__tapi__test__param_1ga95801d63b3a278c981269d17e389bebc>`. It gets parameter name as an argument value and returns string value associated with that parameter.

Apart from base function :ref:`test_get_param() <doxid-group__te__ts__tapi__test__param_1ga95801d63b3a278c981269d17e389bebc>` there are a number of macros that process type-specific parameters:

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
	    te_bool  oob;
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

|	:ref:`Network topology configuration of Test Agents<doxid-group__te__lib__tapi__conf__net>`
|	:ref:`Test API<doxid-group__te__ts__tapi>`
|		:ref:`Agent job control<doxid-group__tapi__job>`
|			:ref:`Helper functions for handling options<doxid-group__tapi__job__opt>`
|				:ref:`convenience macros for option<doxid-group__tapi__job__opt__bind__constructors>`
|				:ref:`functions for argument formatting<doxid-group__tapi__job__opt__formatting>`
|		:ref:`BPF/XDP configuration of Test Agents<doxid-group__tapi__bpf>`
|		:ref:`Control NVMeOF<doxid-group__tapi__nvme>`
|			:ref:`Kernel target<doxid-group__tapi__nvme__kern__target>`
|			:ref:`ONVMe target<doxid-group__tapi__nvme__onvme__target>`
|		:ref:`Control network channel using a gateway<doxid-group__ts__tapi__route__gw>`
|		:ref:`DPDK RTE flow helper functions TAPI<doxid-group__tapi__rte__flow>`
|		:ref:`DPDK helper functions TAPI<doxid-group__tapi__dpdk>`
|		:ref:`DPDK statistics helper functions TAPI<doxid-group__tapi__dpdk__stats>`
|		:ref:`Engine and TA files management<doxid-group__ts__tapi__file>`
|		:ref:`FIO tool<doxid-group__tapi__fio>`
|			:ref:`Control a FIO tool<doxid-group__tapi__fio__fio>`
|			:ref:`FIO tool internals<doxid-group__tapi__fio__internal>`
|		:ref:`GTest support<doxid-group__tapi__gtest>`
|		:ref:`High level TAPI to configure network<doxid-group__ts__tapi__network>`
|		:ref:`High level TAPI to work with sockets<doxid-group__ts__tapi__sockets>`
|		:ref:`I/O multiplexers<doxid-group__tapi__iomux>`
|		:ref:`Macros to get test parameters on a gateway configuration<doxid-group__ts__tapi__gw>`
|		:ref:`Network Traffic TAPI<doxid-group__net__traffic>`
|			:ref:`CSAP TAPI<doxid-group__csap>`
|			:ref:`Route Gateway TAPI<doxid-group__route__gw>`
|			:ref:`TAPI for testing TCP states<doxid-group__tapi__tcp__states>`
|				:ref:`TAPI definitions for testing TCP states<doxid-group__tapi__tcp__states__def>`
|			:ref:`TCP socket emulation<doxid-group__tcp__sock__emulation>`
|		:ref:`RADIUS Server Configuration and RADIUS CSAP<doxid-group__tapi__radius>`
|		:ref:`Stack of jumps<doxid-group__ts__tapi__jmp>`
|		:ref:`Target requirements modification<doxid-group__ts__tapi__reqs>`
|		:ref:`Test<doxid-group__te__ts__tapi__test>`
|			:ref:`Network environment<doxid-group__tapi__env>`
|			:ref:`Test execution flow<doxid-group__te__ts__tapi__test__log>`
|			:ref:`Test misc<doxid-group__te__ts__tapi__test__misc>`
|			:ref:`Test parameters<doxid-group__te__ts__tapi__test__param>`
|			:ref:`Test run status<doxid-group__te__ts__tapi__test__run__status>`
|		:ref:`Test API to control a network throughput test tool<doxid-group__tapi__performance>`
|		:ref:`Test API to operate the UPnP<doxid-group__tapi__upnp>`
|			:ref:`Test API for DLNA UPnP commons<doxid-group__tapi__upnp__common>`
|			:ref:`Test API to operate the DLNA UPnP Content Directory Resources<doxid-group__tapi__upnp__cd__resources>`
|			:ref:`Test API to operate the DLNA UPnP Content Directory Service<doxid-group__tapi__upnp__cd__service>`
|			:ref:`Test API to operate the DLNA UPnP Device information<doxid-group__tapi__upnp__device__info>`
|			:ref:`Test API to operate the DLNA UPnP Service information<doxid-group__tapi__upnp__service__info>`
|			:ref:`Test API to operate the UPnP Control Point<doxid-group__tapi__upnp__cp>`
|		:ref:`Test API to operate the media data<doxid-group__tapi__media>`
|			:ref:`Test API to control a media player<doxid-group__tapi__media__player>`
|			:ref:`Test API to control the mplayer media player<doxid-group__tapi__media__player__mplayer>`
|			:ref:`Test API to operate the DLNA media files<doxid-group__tapi__media__dlna>`
|			:ref:`Test API to operate the media files<doxid-group__tapi__media__file>`
|		:ref:`Test API to operate the storage<doxid-group__tapi__storage>`
|			:ref:`Test API to control the local storage device<doxid-group__tapi__local__storage__device>`
|			:ref:`Test API to control the storage FTP client<doxid-group__tapi__storage__client__ftp>`
|			:ref:`Test API to control the storage client<doxid-group__tapi__storage__client>`
|			:ref:`Test API to control the storage server<doxid-group__tapi__storage__server>`
|			:ref:`Test API to perform the generic operations over the storage<doxid-group__tapi__storage__wrapper>`
|		:ref:`Test API to use memory-related functions conveniently<doxid-group__tapi__mem>`
|		:ref:`Test API to use packetdrill test tool<doxid-group__tapi__packetdrill>`
|		:ref:`Tools to work with "struct sockaddr"<doxid-group__ts__tapi__sockaddr>`
|		:ref:`Traffic Application Domain<doxid-group__tapi__tad__main>`
|			:ref:`ARP<doxid-group__tapi__tad__arp>`
|			:ref:`ATM<doxid-group__tapi__tad__atm>`
|			:ref:`CLI<doxid-group__tapi__tad__cli>`
|			:ref:`DHCP<doxid-group__tapi__tad__dhcp>`
|			:ref:`Ethernet<doxid-group__tapi__tad__eth>`
|			:ref:`Forwarder module<doxid-group__tapi__tad__forw>`
|			:ref:`Generic TAD API<doxid-group__tapi__tad>`
|			:ref:`IGMP<doxid-group__tapi__tad__igmp>`
|			:ref:`IP stack<doxid-group__tapi__tad__ipstack>`
|				:ref:`Common functions for IPv4 and IPv6<doxid-group__tapi__tad__ip__common>`
|				:ref:`ICMP<doxid-group__tapi__tad__icmp>`
|				:ref:`ICMP4<doxid-group__tapi__tad__icmp4>`
|				:ref:`ICMP6<doxid-group__tapi__tad__icmp6>`
|				:ref:`IPv4<doxid-group__tapi__tad__ip4>`
|				:ref:`IPv6<doxid-group__tapi__tad__ip6>`
|				:ref:`Socket<doxid-group__tapi__tad__socket>`
|				:ref:`TCP<doxid-group__tapi__tad__tcp>`
|				:ref:`UDP<doxid-group__tapi__tad__udp>`
|			:ref:`Network Data Notation (NDN)<doxid-group__tapi__ndn>`
|			:ref:`PCAP<doxid-group__tapi__tad__pcap>`
|			:ref:`PPP<doxid-group__tapi__tad__ppp>`
|			:ref:`PPPOE<doxid-group__tapi__tad__pppoe>`
|			:ref:`Packet flow processing<doxid-group__tapi__tad__flow>`
|			:ref:`RTE mbuf<doxid-group__tapi__tad__rte__mbuf>`
|			:ref:`SNMP<doxid-group__tapi__tad__snmp>`
|			:ref:`STP<doxid-group__tapi__tad__stp>`
|			:ref:`iSCSI<doxid-group__tapi__tad__iscsi>`
|		:ref:`tool functions TAPI<doxid-group__tapi__wrk>`
|		:ref:`tool functions TAPI<doxid-group__tapi__netperf>`
|	:ref:`The format of test suite scenarios<doxid-group__te__scenarios>`


