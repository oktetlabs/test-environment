.. index:: pair: group; Dispatcher
.. _doxid-group__te__engine__dispatcher:

Dispatcher
==========

.. toctree::
	:hidden:



.. _doxid-group__te__engine__dispatcher_1te_engine_dispatcher_introduction:

Introduction
~~~~~~~~~~~~

Dispatcher is a subsystem providing a proper initialization and shutdown of the TEN subsystems. It prepares the environment (creates directories for temporary files, exports environment variables, etc.), initiates building, if necessary, and initializes TEN applications according to options provided on the command line.

From user point of view :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` is a BASH script that launches processes and TEN components according to specified command line options.

During its operation :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` sets a few environment variables among which the most important are:

* TE_BASE

  Location of Test Environment sources. If the Dispatcher script is called from the source directory, this variable is exported automatically. Otherwise if building is necessary (i.e., TE is not pre-installed), TE_BASE should be exported manually.

* TE_BUILD

  This variable is exported automatically unless already exported. It is set to a start directory (a directory from which the Dispatcher script is called) or, if a file configure.ac is present in the start directory, to the (created if needed) build subdirectory of the start directory: [start directory]/build.

* TE_INSTALL

  This variable is passed as the value of the prefix option to the main configure script. Moreover, its value is used when path variables for the search of headers and libraries are constructed. It may be set manually. If it is empty, it is set to the directory where the Dispatcher script is located (if the installed Dispatcher script is used) or to ${TE_BUILD}/inst (if the Dispatcher script from the source directory is used).

* TE_INSTALL_SUITE User may export this variable to specify the location of Test Suite executables (for :ref:`Builder <doxid-group__te__engine__builder>` and :ref:`Tester <doxid-group__te__engine__tester>`). If this variable is empty, it is set automatically to ${TE_INSTALL}/suites.

* TE_TMP

  This variable is set by :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` to [start directory]/te_tmp by default. However, if it's desirable to use some other directory for temporary files, it may be exported manually.

* LD_LIBRARY_PATH This variable is exported by :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` automatically and used for shared library search. It is set to ${TE_INSTALL}/[host platform]/lib.

* PATH

  Path to TEN executables is provided automatically by :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`. It updates PATH variable by ${TE_INSTALL}/[host platform]/bin. Moreover, if scripts provided by :ref:`Logger <doxid-group__te__engine__logger>`, :ref:`Builder <doxid-group__te__engine__builder>` and storage library to :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` are not installed yet, :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` adds to PATH variable path to these scripts in the source directory.

* TE_LOG_DIR Directory to store log files. Usually set to TE_RUN_DIR which in it's turn is set to the current directory (PWD).





.. _doxid-group__te__engine__dispatcher_1te_run_time:

Start/stop sequence
~~~~~~~~~~~~~~~~~~~

The following sequence of events happen each time when you launch Test Environment with dispatcher.sh or run.sh script:

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` script starts with some command line options (for more information on :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` options read :ref:`Dispatcher Command Line Options <doxid-group__te__engine__dispatcher_1te_engine_dispatcher_options>`);

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` runs te_log_init script to initialize script based logging facility. All further actions can be logged via script based interface (te_log_message script). Please note that :ref:`Logger <doxid-group__te__engine__logger>` application hasn't started yet;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Builder <doxid-group__te__engine__builder>` to prepare libraries and executables for all TE Subsystems (except :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`), Test Packages, te_agents and bootable NUT image(s).  te_engine_builder is passed a configuration file that describes a set of executables to be built with a set of options for building process.

   :ref:`Builder <doxid-group__te__engine__builder>` configuration file name specified via conf-builder option of :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`.

   (For information about :ref:`Builder <doxid-group__te__engine__builder>` configuration file read :ref:`Builder configuration file <doxid-group__te__engine__builder_1te_engine_builder_conf_file>`).

   Please note that traces of building process are output into the console (they are not accumulated in log file);

#. As soon as :ref:`Builder <doxid-group__te__engine__builder>` successfully built and installed all required components, :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts launching :ref:`Test Engine <doxid-group__te__engine>` componentns. First component to start is :ref:`Logger <doxid-group__te__engine__logger>`. :ref:`Logger <doxid-group__te__engine__logger>` is passed a configuration file whose name can be specified via conf-logger :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` command line option (for information about the format of :ref:`Logger <doxid-group__te__engine__logger>` configuration file refer to :ref:`Configuration File <doxid-group__te__engine__logger_1te_engine_logger_conf_file>`).

   :ref:`Logger <doxid-group__te__engine__logger>` starts listening for incoming log requests that can come from tests and other TEN components;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`. :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` is passed a configuration file that describes te_agents to be started (for information about the format of :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file refer to :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>`).

   As a part of initialization :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` establishes communication with te_agents using Test Protocol;

#. As soon as :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` has initialized, :ref:`Logger <doxid-group__te__engine__logger>` starts a thread that is responsible for polling te_agents Test Agents in order to gather log messages accumulated on Test Agent side. Polling interval is configured via :ref:`Logger <doxid-group__te__engine__logger>` configuration file;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Configurator <doxid-group__te__engine__conf>`. :ref:`Configurator <doxid-group__te__engine__conf>` is passed a configuration file that describes configuration objects to register as well as object instances to add (for information about the format of :ref:`Configurator <doxid-group__te__engine__conf>` configuration file refer to :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>`). On start-up :ref:`Configurator <doxid-group__te__engine__conf>` retrives configuration information from te_agents and initializes local trees of objects and instances;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Tester <doxid-group__te__engine__tester>`. :ref:`Tester <doxid-group__te__engine__tester>` processes configuration file and if necessary asks :ref:`Builder <doxid-group__te__engine__builder>` to build test suites (test executables). Then :ref:`Tester <doxid-group__te__engine__tester>` processes test package description files and runs tests in corresponding order and with specified set of parameter values. (For information about :ref:`Tester <doxid-group__te__engine__tester>` configuration file format refer to :ref:`Configuration File <doxid-group__te__engine__tester_1te_engine_tester_conf>` section).

   Before running tests, :ref:`Tester <doxid-group__te__engine__tester>` asks :ref:`Configurator <doxid-group__te__engine__conf>` to make a backup of configuration tree. When all tests are finished :ref:`Tester <doxid-group__te__engine__tester>` restores the initial configuration from initial backup. To prevent tests from interfering, a backup is created and optionally restored before each test as well.

#. When :ref:`Tester <doxid-group__te__engine__tester>` returns (all tests finished), :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Configurator <doxid-group__te__engine__conf>`;

#. Flushing of the log from all Test Agents is performed;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`. During its shutdown, :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` performs a shutdown of all te_agents;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Logger <doxid-group__te__engine__logger>`. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` calls Report Generator tool to convert the log from a raw format to the text and/or HTML format;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` script finishes its work.





.. _doxid-group__te__engine__dispatcher_1te_engine_dispatcher_options:

Dispatcher Command Line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Dispatcher script accepts a lot of command-line options. Some of them are its own options, and the rest are passed through to other TE subsystems. Here is the complete list of the dispatcher.sh script options as well as their descriptions obtained by calling it with help option:

.. code-block:: none


	Usage: dispatcher.sh [<generic options>="">] [[<test options>=""> tests ]...
	Generic options:
	  -q                            Suppress part of output messages
	  force                       Never prompt

.. code-block:: none

	daemon[=<PID>]              Run/use TE engine daemons
	shutdown[=<PID>]            Shut down TE engine daemons on exit

.. code-block:: none

	conf-dir=<directory>        specify configuration file directory,
	                              overrides previous value and conf-dirs
	                              (${TE_BASE}/conf or . by default)
	conf-dirs=<directories>     specify list of configuration file directories
	                              separated by colon (top priority first,
	                              may be specified many times, appends to
	                              conf-dir)

.. code-block:: none

	In configuration files options below <filename> is full name of the
	configuration file or name of the file in the configuration directory.

.. code-block:: none

	conf-builder=<filename>     te_engine_build config file ( by default)
	conf-cs=<filename>          :ref:`Configurator <doxid-group__te__engine__conf>` config file ( by default)
	conf-logger=<filename>      :ref:`Logger <doxid-group__te__engine__logger>` config file ( by default)
	conf-rcf=<filename>         :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` config file ( by default)
	conf-rgt=<filename>         RGT config file (rgt.conf by default)
	conf-tester=<filename>      :ref:`Tester <doxid-group__te__engine__tester>` config file ( by default)
	conf-nut=<filename>         NUT config file ( by default)

.. code-block:: none

	script=<filename>           Name of the file with shell script to be
	                              included as source

.. code-block:: none

	live-log                    Run RGT in live mode

.. code-block:: none

	log-dir=<dirname>           Directory where to save tmp_raw_log file
	                              (used if TE_LOG_RAW is not set directly)
	log-html=<dirname>          Name of the directory with structured HTML logs
	                              to be generated (do not generate by default)
	log-plain-html=<filename>   Name of the file with plain HTML logs
	                              to be generated (do not generate by default)
	log-txt=<filename>          Name of the file with logs in text format
	                              to be generated (log.txt by default)
	log-txt-detailed-packets    Include detailed packet dumps in text log.
	log-junit=<filename>        Name of the file with logs in JUnit format
	                              to be generated.

.. code-block:: none

	no-builder                  Do not build TE and TA
	no-nuts-build               Do not build NUTs
	no-tester                   Do not run :ref:`Tester <doxid-group__te__engine__tester>`
	no-cs                       Do not run :ref:`Configurator <doxid-group__te__engine__conf>`
	no-rcf                      Do not run :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`
	no-run                      Do not run :ref:`Logger <doxid-group__te__engine__logger>`, :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, :ref:`Configurator <doxid-group__te__engine__conf>` and :ref:`Tester <doxid-group__te__engine__tester>`
	no-autotool                 Do not try to perform autoconf/automake after
	                              package configure failure

.. code-block:: none

	opts=<filename>             Get additional command-line options from file

.. code-block:: none

	cs-print-trees              Print configurator trees.
	cs-log-diff                 Log backup diff unconditionally.

.. code-block:: none

	build-autotools             Build using autotools (autoconf, automake, make)
	build-meson                 Build using meson/ninja

.. code-block:: none

	build-only                  Build TE, do not run :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` and :ref:`Configurator <doxid-group__te__engine__conf>`,
	                              build but do not run Test Suites

.. code-block:: none

	build=path                  Build package specified in the path.
	build-log=path              Build package with log level 0xFFFF.
	build-nolog=path            Build package with undefined log level.
	build-cs                    Build :ref:`Configurator <doxid-group__te__engine__conf>`.
	build-logger                Build :ref:`Logger <doxid-group__te__engine__logger>`.
	build-rcf                   Build :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`.
	build-tester                Build :ref:`Tester <doxid-group__te__engine__tester>`.
	build-lib-xxx               Build host library xxx.
	build-log-xxx               Build package with log level 0xFFFF.
	build-nolog-xxx             Build package with undefined log level.
	build-parallel[=num]        Enable parallel build using num threads.

.. code-block:: none

	build-ta-none               Don't build Test Agents.
	build-ta-missing            Build only new Test Agents.
	build-ta-all                Force build all Test Agents.
	build-ta-for=<hostname>     Rebuild agent (and all the libraries) used for <hostname>.
	build-ta-rm-lock            Remove lock from previous agent build even
	                              if it is still in progress.
	profile-build=<logfile>     Gather timings for the build process into <logfile>

.. code-block:: none

	no-rcf-cc-simple            Do not execute simple :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` consistency checks.

.. code-block:: none

	tester-suite=<name>:<path>  Specify path to the Test Suite.
	tester-no-run               Don't run any tests.
	tester-no-build             Don't build any Test Suites.
	tester-no-trc               Don't use Testing Results Comparator.
	tester-no-cs                Don't interact with :ref:`Configurator <doxid-group__te__engine__conf>`.
	tester-no-cfg-track         Don't track configuration changes.
	tester-no-logues            Disable prologues and epilogues globally.
	tester-only-req-logues      Run only prologues/epilogues under which
	                              at least one test will be run according to
	                              requirements passed in command line. This
	                              may not work well if your prologues can add
	                              requirements on their own in /local:/reqs:.
	tester-req=<reqs-expr>      Requirements to be tested (logical expression).
	tester-no-reqs              Ignore requirements, run all possible
	                              iterations.
	tester-quietskip            Quietly skip tests which do not meet specified
	                              requirements (default).
	tester-verbskip             Force :ref:`Tester <doxid-group__te__engine__tester>` to log skipped iterations.

.. code-block:: none

	tester-cmd-monitor          Specify command monitor to be run for all
	                              tests in form [ta,]time_to_wait:command

.. code-block:: none

	The following :ref:`Tester <doxid-group__te__engine__tester>` options get test path as a value:
	    <testpath>      :=  / | <path-item> | <testpath>/<path-item>
	    <path-item>     := <test-name>[:<args>][%%<iter-select>][*<repeat>]
	    <args>          := <arg>[,<args>]
	    <arg>           := <param-name>=<values> | <param-name>~=<values>
	    <values>        := <value> | { <values-list> }
	    <values-list>   := <value>[,<values-list>]
	    <iter-select>   := <iter-number>[+<step>] | <hash>
	For example,
	    tester-run=mysuite/mypkg/mytest:p1={a1,a2}
	requests to run all iterations of the test 'mytest' when its parameter
	'p1' is equal to 'a1' or 'a2';
	    tester-run=mysuite/mypkg/mytest%%3*10
	requests to run 10 times third iteration of the same test.

.. code-block:: none

	tester-fake=<testpath>      Don't run any test scripts, just emulate test
	                              scenario.
	tester-run=<testpath>       Run test under the path.
	tester-run-from=<testpath>  Run tests starting from the test path.
	tester-run-to=<testpath>    Run tests up to the test path.
	tester-exclude=<testpath>   Exclude specified tests from campaign.
	tester-vg=<testpath>        Run test scripts under specified path using
	                              valgrind.
	tester-gdb=<testpath>       Run test scripts under specified path using
	                              gdb.

.. code-block:: none

	tester-random-seed=<number> Random seed to initialize pseudo-random number
	                              generator
	tester-verbose              Increase verbosity of the :ref:`Tester <doxid-group__te__engine__tester>` (the first
	                              level is set by default).
	tester-quiet                Decrease verbosity of the :ref:`Tester <doxid-group__te__engine__tester>`.
	tester-out-tin              Output Test Identification Numbers to terminal
	tester-out-expected         If result is expected (in accordance with TRC),
	                              output the result together with OK
	tester-interactive          Interactive ask user for tests to run.

.. code-block:: none

	test-sigusr2-verdict        Handle the SIGUSR2 signal in test and stop it by TEST_VERDICT.
	                              By default the SIGUSR2 handled like SIGINT, it stops testing.
	test-wof                    Wait before jump to cleanup on test failure. Useful to
	                              take a look at what's configured etc. Requires some
	                              nodes in the /local:/test: tree.
	test-woc                    Wait before jump to cleanup regardless of test result.

.. code-block:: none

	  trc-log=<filename>          Generate bzip2-ed TRC log
	  trc-db=<filename>           TRC database to be used
	  trc-tag=<TAG>               Tag to get specific expected results
	  trc-key2html=<filename>     File with key substitutions when output to HTML
	                                report
	  trc-ignore-log-tags         Ignore tags from log
	  trc-html=<filename>         Name of the file for HTML report
	  trc-brief-html=<filename>   Name of the file for brief HTML report
	  trc-html-header=<filename>  Name of the file with header for all HTML
	                                reports.
	  trc-txt=<filename>          Name of the file for text report
	                                (by default, it is generated to stdout)
	  trc-quiet                   Do not output total statistics to stdout
	  trc-comparison=<method>     Specify the method to match parameter values in TRC
	                                exact (the default)
	
	
	* casefold
	                                  normalised (XML-style space normalization)
	                                  tokens (the values are split into tokens which are
	                                  either sequences of XML name characters or single characters;
	                                  the matching is done on these lists; in additional, numeric
	                                  tokens are compared as numbers (so e.g. 10 and 0xa render equal)
	    trc-update                  Update TRC database
	    trc-init                    Initialize TRC database (be careful,
	                                  TRC database file will be rewritten)

.. code-block:: none

	vg-engine                   Run :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, :ref:`Configurator <doxid-group__te__engine__conf>`, :ref:`Logger <doxid-group__te__engine__logger>` and :ref:`Tester <doxid-group__te__engine__tester>` under
	                              valgrind (without by default)
	vg-cs                       Run :ref:`Configurator <doxid-group__te__engine__conf>` under valgrind
	vg-logger                   Run :ref:`Logger <doxid-group__te__engine__logger>` under valgrind (without by default)
	vg-rcf                      Run :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` under valgrind (without by default)
	                              (without by default)
	vg-tester                   Run :ref:`Tester <doxid-group__te__engine__tester>` under valgrind (without by default)
	gdb-tester                  Run :ref:`Tester <doxid-group__te__engine__tester>` under gdb.
	tce                         Do TCE processing for all TCE-enabled components
	tce=




	                Do TCE processing for specific components (comma-separated) or 'all'

.. code-block:: none

	sniff-not-feed-conf          Do not feed the sniffer configuration file
	                               to :ref:`Configurator <doxid-group__te__engine__conf>`.
	sniff=<TA/iface>             Run sniffer on *iface* of the *TA*.
	sniff-filter=<filter>        Add for the sniffer filter(tcpdump-like
	                               syntax). See 'man 7 pcap-filter'.
	sniff-name=<name>            Add for the sniffer a human-readable name.
	sniff-snaplen=<val>          Add for the sniffer restriction on maximum
	                               number of bytes to capture for one packet.
	                               By default: .
	sniff-space=<val>            Add for the sniffer restriction on maximum
	                               overall size of temporary files, Mb.
	                               By default: 64 Mb.
	sniff-fsize=<val>            Add for the sniffer restriction on maximum
	                               size of the one temporary file, Mb.
	                               By default: 16 Mb.
	sniff-rotation=<x>           Add for the sniffer restriction on number of
	                               temporary files. This option excluded by
	                               the sniff-ta-log-ofill-drop option.
	                               By default: 4.
	sniff-ofill-drop             Change overfill handle method of temporary
	                               files for the sniffer to tail drop.
	                               By default overfill handle method is rotation.
	sniff-log-dir=<path>         Path to the :ref:`Test Engine <doxid-group__te__engine>` side capture files.
	                               By default used: /home/aizrailev/te/caps.
	sniff-log-name=<pattern>     :ref:`Test Engine <doxid-group__te__engine>` side log file naming pattern, the
	                               following format specifies are supported:
	                               %a : agent name
	                               %u : user name
	                               %i : iface name
	                               %s : sniffer name
	                               %n : sniffer session sequence number
	                               By default '%a_%i_%s_%n' is used. The pcap
	                               extension will be added automatically.
	sniff-log-osize=<val>        Maximum :ref:`Test Engine <doxid-group__te__engine>` side logs cumulative size for all
	                               sniffers, Mb.
	                               By default: unlimited.
	sniff-log-space=<val>        Maximum :ref:`Test Engine <doxid-group__te__engine>` side logs cumulative size for one
	                               sniffer, Mb. By default: 256 Mb.
	sniff-log-fsize=<val>        Maximum :ref:`Test Engine <doxid-group__te__engine>` side capture file size for each
	                               sniffer in Mb.
	                               By default: 64 Mb.
	sniff-log-ofill-drop         Change overfill handle method to tail drop.
	                               By default overfill handle method is rotation.
	sniff-log-period=<val>       Period of taken logs from agents, milliseconds.
	                               By default: 200 msec.
	sniff-log-conv-disable       Option to disable capture logs conversion
	                               and merge with the main log.

.. code-block:: none

	Environment variables defining where raw log is stored:
	
	
	.. code-block:: c
	
		TE_LOG_RAW          Where to save raw log file, by default tmp_raw_log
		                    in directory specified by --log-dir (if provided)
		                    or in the current directory.
		TE_LOG_BUNDLE       Where to save raw log bundle (tarball compressed
		                    with pixz). If it is not set, raw log bundle is not
		                    created.
		
		The script exits with a status of zero if everything does smoothly and
		all tests, if any tests are run, give expected results. A status of two
		is returned, if some tests are run and give unexpected results.
		A status of one indicates start up or any internal failure.

