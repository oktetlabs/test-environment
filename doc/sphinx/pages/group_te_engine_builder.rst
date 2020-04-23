.. index:: pair: group; Builder
.. _doxid-group__te__engine__builder:

Builder
=======

.. toctree::
	:hidden:



.. _doxid-group__te__engine__builder_1te_engine_builder_introduction:

Introduction
~~~~~~~~~~~~

Builder subsystem is not a separate closed software but rather set of principles, conventions, standard files, templates and scripts that allow to built TE libraries, applications, NUT bootable images and Test Suites in a single, convenient, standard and portable way. TE Builder is based on GNU autotools (autoconf, automake).

From the Builder point of view Test Environment consists of a lot of packages: libraries, applications, tools and Test Agents. Each package has input files for autotools:

* configure.ac for autoconf;

* Makefile.am for automake;

* optionally files for autoheader.

Each package may be built for one or several platforms. The platform for which TEN applications, Test API libraries and tests are built is called host platform. Test Agents and libraries linked with them may be built for platforms other than host platform.

Test Environment depends on many third party libraries that need to be installed on :ref:`Test Engine <doxid-group__te__engine>` host in order to build all Test Environment components successfully. The list of required libraries can be found in ${TE_BASE}/README file.





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file:

Builder configuration file
~~~~~~~~~~~~~~~~~~~~~~~~~~

Builder configuration file contains a set of directives that describe what to build and how to build. Available directives can be found in ${TE_BASE}/engine/builder/builder.m4 file.

By means of Builder configuration file for each platform it is possible to specify:

* options to be passed to configure scripts for all packages built for this platform;

* compiler to use (when it is not specified, it is discovered by configure script basing on the platform name);

* pre-processor flags;

* compiler flags;

* linker flags;

* list of libraries to be built for the platform.

Additionally for each package it is possible to specify:

* options to be passed to package configure script;

* pre-processor flags;

* compiler flags;

* linker flags.

For applications (TEN applications, tools, Test Agents), the list of libraries to be linked may be specified as well.

Executable is built not for each Test Agent, but rather for each Test Agent type (linux, bsd, etc.).

By default Builder searches libraries and Test Agent sources in the Test Environment source tree. However it's possible to specify that library or TA sources are located in the private place.

If one package should be built for several platforms and/or with different parameters, the configure script is called several times for it. In the second case different names should be assigned to these packages in the configuration file.



.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_samples:

Samples of Builder configuration files
--------------------------------------

You can find samples of :ref:`Builder <doxid-group__te__engine__builder>` configuration files under ${TE_BASE}/conf directory.

Here we will discuss the usage of directives defined in ${TE_BASE}/engine/builder/builder.m4 file.



.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_platform:

TE_PLATFORM
+++++++++++

.. ref-code-block:: none

	TE_PLATFORM([platform name],
	            [configure parameters],
	            [additional preprocessor flags],
	            [additional compiler flags],
	            [additional linker flags],
	            [list of all libraries to be built])

This directive can be used to specify platform-specific parameters that will be applied for building all packages of this platform.

For example if you need to have some target that requires cross compilation (for Test Agents) you should write something like the following in your builder configuration file:

.. ref-code-block:: none

	TE_PLATFORM([cygwin], [--host=i686-pc-cygwin],
	            [-I${CYGWIN_HOME}/include -D__CYGWIN__],
	            [-Os],
	            [-L${CYGWIN_HOME}/lib],
	            [tools conf_oid rpcxdr rpc_types \
	             comm_net_agent loggerta rcfpch])

In this sample we define a new target platform called cygwin for which we require building the following libraries (from Test Environment ${TE_BASE}/lib directory): tools, conf_oid, rpcxdr, rpc_types, comm_net_agent, loggerta, rcfpch.

While building any library from the list the following options are passed:

* to configure script:

  * **--host=i686-pc-cygwin**

* to C preprocessor:

  * **-I${CYGWIN_HOME}/include**

  * **-D\__CYGWIN\__**

* to C compiler:

  * **-Os**

* to object linker:

  * **-L${CYGWIN_HOME}/lib**

Please note that when you specify parameters for host platform (a platform where :ref:`Test Engine <doxid-group__te__engine>` runs) you should keep platform name as an empty string. For example:

.. ref-code-block:: none

	TE_PLATFORM([], [], [-D_GNU_SOURCE], [-D_GNU_SOURCE], [],
	            [tools logic_expr conf_oid rpcxdr rpc_types asn ndn \
	             comm_net_agent loggerta rcfpch tce tad tad_dummy \
	             ipc bsapi loggerten rcfapi confapi comm_net_engine rcfunix \
	             tapi rcfrpc tapi_rpc tapi_env trc netconf lua])





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_lib_params:

TE_LIB_PARMS
++++++++++++

.. ref-code-block:: none

	TE_LIB_PARMS([library name],
	             [platform name],
	             [source directory],
	             [additional configure script parameters],
	             [additional preprocessor flags],
	             [additional compiler flags],
	             [additional linker flags])

If we need to specify some additional options to configure script, preprocessor, compiler or object linker, we can use TE_LIB_PARMS directive in your :ref:`Builder <doxid-group__te__engine__builder>` configuration file.

For example if we need to add some additional options to configure script of tad library, we should use the following:

.. ref-code-block:: none

	TE_LIB_PARMS([tad], [cygwin], [],
	             [--with-eth --with-arp --with-ipstack],
	             [], [], [])

Please note that we do not specify value for source directory parameter, which means the source directory of that library located under ${TE_BASE}/lib/[library name] subdirectory - ${TE_BASE}/lib/tad in our sample.





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_ta_type:

TE_TA_TYPE
++++++++++

.. ref-code-block:: none

	TE_TA_TYPE([Test Agent type],
	           [platform],
	           [sources location],
	           [additional configure script parameters],
	           [additional preprocessor flags],
	           [additional compiler flags],
	           [additional linker flags],
	           [list of external libraries])

This directive is necessary to build a Test Agent of the particular type. Platform type should be one of platforms specified with TE_PLATFORM directive (For host platform this should be an empty string).

Test Agent type value is an arbitrary string that will be reffered to from RCF configuration file (See RCF :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>`).

Sources of Test Agent are got from [source location] directory, which is either a subdirectory of ${TE_BASE}/agents directory, or full absolute path to sources.

Please note that all items mentioned in the list of external libraries specified as the last argument of TE_TA_TYPE directive must be listed in the list of libraries to be build in corresponding TE_PLATFORM directive (with the same platform type).

For example:

.. ref-code-block:: none

	TE_PLATFORM([freebsd], [--host=i386-pc-freebsd6],
	            [-I${FREEBSD6_HOME}/sys-include], [], [-L${FREEBSD6_HOME}/lib],
	            [tools conf_oid rpcxdr rpc_types asn ndn \
	             comm_net_agent loggerta rcfpch tad])
	#
	# Define 'freebsd' platform and list libraries to
	# be built for this platform.
	#

	TE_TA_TYPE([freebsd6], [freebsd], [unix],
	           [--with-rcf-rpc], [], [], [],
	           [comm_net_agent ndn asn])
	#
	# Test Agent of type 'freebsd6' requires 'comm_net_agent', 'ndn' and 'asn'
	# libraries. These libraries are listed in TE_PLATFORM() directive for
	# 'freebsd' platform among libraries to be built.
	#





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_app:

TE_APP
++++++

.. ref-code-block:: none

	TE_APP([list of Test Engine components])

This directive specifies which applications (:ref:`Test Engine <doxid-group__te__engine>` components) to build by :ref:`Builder <doxid-group__te__engine__builder>`.

Normally you will not need to specify this directive because by default all necessary :ref:`Test Engine <doxid-group__te__engine>` components are built, but if you for some reason need to build only the particular set of :ref:`Test Engine <doxid-group__te__engine>` components you can add this directive in your :ref:`Builder <doxid-group__te__engine__builder>` configuration file.

By default TE_APP is set to:

.. ref-code-block:: none

	TE_APP([builder rcf logger tester configurator])

For example if you need to build only Test Environment tools you can exclude building all :ref:`Test Engine <doxid-group__te__engine>` components with:

.. ref-code-block:: none

	TE_APP([])





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_app_parms:

TE_APP_PARMS
++++++++++++

Each :ref:`Test Engine <doxid-group__te__engine>` application can be built with dedicated set of parameters passed to configure script, C preprocessor, C compiler and object linker. This can be controlled with TE_APP_PARMS directive.

.. ref-code-block:: none

	TE_APP_PARMS([Test Engine application name],
	             [additional configure script parameters],
	             [additional preprocessor flags],
	             [additional compiler flags],
	             [additional linker flags],
	             [list of libraries])

As the first argument of TE_APP_PARMS directive you specify the name of :ref:`Test Engine <doxid-group__te__engine>` application to which this directive applies. The name must be a directory name under ${TE_BASE}/engine directory.

For example:

.. ref-code-block:: none

	TE_APP_PARMS([rcf], [--enable-ltdl-install])
	TE_APP_PARMS([tester], [--without-trc])

Please note that we specify the values only for the first two parameters and omit the rest parameter values because they are empty strings.

Please also note that parameter values specified in TE_PLATFORM directive for host platform (with empty string platform name) applied to Test Engine applications.





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_tools:

TE_TOOLS
++++++++

While building Test Environment you can build some auxiliary tools. The list of tools to be built are specified with TE_TOOLS directive.

.. ref-code-block:: none

	TE_TOOLS([list of Test Environment tools])

Test Environment tools specified in TE_TOOLS directive should be equal to the names of directories reside under ${TE_BASE}/tools directory.

Normally you would have:

.. ref-code-block:: none

	TE_TOOLS([rgt trc tce])





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_tool_parms:

TE_TOOL_PARMS
+++++++++++++

Similarly to TE_APP, TE_APP_PARMS pair there is TE_TOOL_PARMS supplementing TE_TOOLS directive.

TE_TOOLS directive can be used to specify building process parameters for the particular Test Environment tool.

.. ref-code-block:: none

	TE_APP_PARMS([Test Environment tool name],
	             [additional configure script parameters],
	             [additional preprocessor flags],
	             [additional compiler flags],
	             [additional linker flags],
	             [list of libraries names])

For example:

.. ref-code-block:: none

	TE_TOOL_PARMS([rgt], [--with-glib=/mnt/shared/glib])





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_shell:

TE_SHELL
++++++++

You can specify arbitrary shell scripts to run during build procedure with TE_SHELL directive.

.. ref-code-block:: none

	TE_SHELL([shell script to run])

For example TE_SHELL directive can be useful when you want to set values of environment variables if they are not set. Later you can refer to these variables in your builder configuration file directives.

.. ref-code-block:: none

	TE_SHELL([test -z "${SOLARIS2_HOME}" && \
	          SOLARIS2_HOME="/srv/local/cross/i386-pc-solaris2.11"])
	#
	# Check if SOLARIS2_HOME environment variable is set.
	# If not set this variable to some default value.
	#

	TE_PLATFORM([solaris], [--host=i386-pc-solaris2.11],
	            [-I${SOLARIS2_HOME}/sys-include -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED=1 -D__EXTENSIONS__],
	            [-pthreads],
	            [-L${SOLARIS2_HOME}/lib],
	            [tools conf_oid rpcxdr rpc_types asn ndn \
	             comm_net_agent loggerta rcfpch tad])
	#
	# Refer to environment variable from other directives.
	#





.. _doxid-group__te__engine__builder_1te_engine_builder_conf_file_include:

Include directive
+++++++++++++++++

You can keep parts of builder configuration in separate files and merge them togeter with the help of include directive. This let reuse common parts of builder configuration in separate files (for the cases when you have more than one set-up to build).

.. ref-code-block:: none

	#
	# Assume this is the top level builder configuration file
	# and we need to include a number of files in it.
	#

	include(builder.part.common)
	include(builder.part.linux.platform)
	include(builder.part.freebsd.platform)









.. _doxid-group__te__engine__builder_1te_engine_builder_build_process:

Building process
~~~~~~~~~~~~~~~~

Test Environment build should be initiated **ONLY** by :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`. It is not expected that configure script is executed manually.

Building is performed as follows:

#. The root configure script analyses :ref:`Builder <doxid-group__te__engine__builder>` configuration file. It creates directories for all packages to be built and calls configure script for all host packages. If configure script or Makefile.in file are missing in the package source directory, autotools are used to generate them;

#. make is called. It builds and installs libraries, TEN applications and Test Agents for the host platform;

#. make calls the Builder script for cross-compiling, which calls configure and make for each package to be cross-compiled.

Re-building of the whole Test Environment is done only if :ref:`Builder <doxid-group__te__engine__builder>` configuration file changes. Otherwise the configure is called for the package only if there is no Makefile in the package build directory.

It is possible to skip a TE building step by specifying --no-builder option to :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`.

If you want to cleanup files generated by :ref:`Builder <doxid-group__te__engine__builder>` (Makefile.in, configure, etc.) you need to run bootstrap.sh script in Test Environment base directory (TE_BASE).

.. ref-code-block:: shell

	cd ${TE_BASE}
	./bootstrap.sh





.. _doxid-group__te__engine__builder_1te_engine_builder_tests:

Building of Tests Packages
~~~~~~~~~~~~~~~~~~~~~~~~~~

Building of the test packages is initiated by :ref:`Tester <doxid-group__te__engine__tester>` during processing of package.xml where test package source directory is specified. configure or configure.ac should present in the source directory.

Tests are always built for the host platforms and installed to TE_INSTALL_SUITE directory where the executable scripts are then accessed by the :ref:`Tester <doxid-group__te__engine__tester>`.

All built tests are included into the distribution.

Building of tests may be skipped by specifying tester-no-build option to :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`.





.. _doxid-group__te__engine__builder_1te_engine_builder_make:

Calling make directly
~~~~~~~~~~~~~~~~~~~~~

make can be called directly to:

* make the distribution:

  call make in the root of build directory with the target **dist** or **distcheck**;

* to build/install a pre-configured package without running the te_engine_dispatcher:

  call make in the package build directory with **install** target.

The second approach is useful for fixing compilation errors.

