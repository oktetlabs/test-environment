..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

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

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Builder <doxid-group__te__engine__builder>` to prepare libraries and executables for all TE Subsystems (except :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`), Test Packages, Test Agents and bootable NUT image(s).  :ref:`Builder <doxid-group__te__engine__builder>` is passed a configuration file that describes a set of executables to be built with a set of options for building process.

   :ref:`Builder <doxid-group__te__engine__builder>` configuration file name specified via conf-builder option of :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`.

   (For information about :ref:`Builder <doxid-group__te__engine__builder>` configuration file read :ref:`Builder configuration file <doxid-group__te__engine__builder_1te_engine_builder_conf_file>`).

   Please note that traces of building process are output into the console (they are not accumulated in log file);

#. As soon as :ref:`Builder <doxid-group__te__engine__builder>` successfully built and installed all required components, :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts launching :ref:`Test Engine <doxid-group__te__engine>` componentns. First component to start is :ref:`Logger <doxid-group__te__engine__logger>`. :ref:`Logger <doxid-group__te__engine__logger>` is passed a configuration file whose name can be specified via conf-logger :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` command line option (for information about the format of :ref:`Logger <doxid-group__te__engine__logger>` configuration file refer to :ref:`Configuration File <doxid-group__te__engine__logger_1te_engine_logger_conf_file>`).

   :ref:`Logger <doxid-group__te__engine__logger>` starts listening for incoming log requests that can come from tests and other TEN components;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`. :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` is passed a configuration file that describes Test Agents to be started (for information about the format of :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file refer to :ref:`RCF Configuration File <doxid-group__te__engine__rcf_1te_engine_rcf_conf_file>`).

   As a part of initialization :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` establishes communication with Test Agents using Test Protocol;

#. As soon as :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` has initialized, :ref:`Logger <doxid-group__te__engine__logger>` starts a thread that is responsible for polling Test Agents in order to gather log messages accumulated on Test Agent side. Polling interval is configured via :ref:`Logger <doxid-group__te__engine__logger>` configuration file;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Configurator <doxid-group__te__engine__conf>`. :ref:`Configurator <doxid-group__te__engine__conf>` is passed a configuration file that describes configuration objects to register as well as object instances to add (for information about the format of :ref:`Configurator <doxid-group__te__engine__conf>` configuration file refer to :ref:`Configurator Configuration File <doxid-group__te__engine__conf_1te_engine_conf_file>`). On start-up :ref:`Configurator <doxid-group__te__engine__conf>` retrives configuration information from Test Agents and initializes local trees of objects and instances;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` starts :ref:`Tester <doxid-group__te__engine__tester>`. :ref:`Tester <doxid-group__te__engine__tester>` processes configuration file and if necessary asks :ref:`Builder <doxid-group__te__engine__builder>` to build test suites (test executables). Then :ref:`Tester <doxid-group__te__engine__tester>` processes test package description files and runs tests in corresponding order and with specified set of parameter values. (For information about :ref:`Tester <doxid-group__te__engine__tester>` configuration file format refer to :ref:`Configuration File <doxid-group__te__engine__tester_1te_engine_tester_conf>` section).

   Before running tests, :ref:`Tester <doxid-group__te__engine__tester>` asks :ref:`Configurator <doxid-group__te__engine__conf>` to make a backup of configuration tree. When all tests are finished :ref:`Tester <doxid-group__te__engine__tester>` restores the initial configuration from initial backup. To prevent tests from interfering, a backup is created and optionally restored before each test as well.

#. When :ref:`Tester <doxid-group__te__engine__tester>` returns (all tests finished), :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Configurator <doxid-group__te__engine__conf>`;

#. Flushing of the log from all Test Agents is performed;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`. During its shutdown, :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` performs a shutdown of all Test Agents;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` stops :ref:`Logger <doxid-group__te__engine__logger>`. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` calls Report Generator tool to convert the log from a raw format to the text and/or HTML format;

#. :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` script finishes its work.


.. _doxid-group__te__engine__dispatcher_1logs_publishing:

Publishing logs to Bublik web application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Testing logs may be published to Bublik web application using Dispatcher
option `--publish` or a standalone script `scripts/publish_logs`. A path
to test suite and site specific logs publishing script should be passed via
`--publish` option of Dispatcher or `--script` option of
`scripts/publish_logs`. TE will then create a tar archive containing
testing metadata (`meta_data.json`) and raw log bundle
(`raw_log_bundle.tpxz`), and pass it as the only argument to the
script that you provide.

It is assumed that your script will then copy the tar archive to log
storage server, extract it there in a proper place and request
Bublik web application to import logs from the corresponding URL.


.. _doxid-group__te__engine__dispatcher_1te_engine_dispatcher_options:

Dispatcher Command Line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Dispatcher script accepts a lot of command-line options. Some of them are its own options, and the rest are passed through to other TE subsystems. Here is the complete list of the dispatcher.sh script options as well as their descriptions obtained by calling it with help option:
The Dispatcher script accepts a lot of command-line options. Some of them are
its own, and the rest are passed through to other TE subsystems: an option
starting with ``--tester-`` goes to :ref:`Tester <doxid-group__te__engine__tester>`,
``--logger-`` to :ref:`Logger <doxid-group__te__engine__logger>`, ``--trc-`` to
:ref:`Test Results Comparator <doxid-group__trc>`, and so on.

There are well over a hundred of them, so the sections below name the ones you
are likely to reach for day to day. The complete list, generated from
``dispatcher.sh --help``, follows at the end of the page --- and you can always
get the same text by running:

.. code-block:: none


	./dispatcher.sh --help


Choosing configuration files
----------------------------

``--conf-dir=<directory>`` points at the directory holding the configuration
files, and ``--conf-dirs=<dir>:<dir>`` at a colon-separated list of them
(highest priority first). Within those directories each subsystem picks up its
own file, and each can be overridden individually:

===================  ==================  ==============================================================
Option               Default file        Subsystem
===================  ==================  ==============================================================
``--conf-builder=``  ``builder.conf``    :ref:`Builder <doxid-group__te__engine__builder>`
``--conf-cs=``       ``cs.conf``         :ref:`Configurator <doxid-group__te__engine__conf>`
``--conf-logger=``   ``logger.conf``     :ref:`Logger <doxid-group__te__engine__logger>`
``--conf-rcf=``      ``rcf.conf``        :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`
``--conf-rgt=``      ``rgt.conf``        :ref:`Report Generator Tool <doxid-group__rgt>`
``--conf-tester=``   ``tester.conf``     :ref:`Tester <doxid-group__te__engine__tester>`
===================  ==================  ==============================================================

Command lines get long quickly, so ``--opts=<filename>`` reads further options
from a file. Test suites normally wrap all of this in their own ``run.sh``; see
:ref:`TE Execution <doxid-group__te__user_1te_user_run>`.


Building
--------

``--no-builder`` skips the build entirely and ``--build-only`` does the
opposite --- build everything, run nothing. ``-n`` is the shorthand for building
nothing at all, neither TE nor the test suites.

``--build-from-scratch`` throws away previous build artefacts, which is what you
want after changing the toolchain or the
:ref:`Builder <doxid-group__te__engine__builder>` configuration.
``--build-parallel[=num]`` builds with several jobs, ``--builder-debug`` makes
the build verbose when you need to see what it is actually doing, and
``--profile-build=<logfile>`` records where the time went.


Selecting what to run
---------------------

``--tester-run=<testpath>`` is the option you will use most --- it runs the
tests under the given path. Its relatives are ``--tester-run-from=``,
``--tester-run-to=``, ``--tester-exclude=`` and ``--tester-run-while=``.
``--tester-fake=<testpath>`` walks the scenario without running anything, which
is the quickest way to check that a path selects what you expected.

A test path is more than a directory name; it can pin parameter values and
iterations::

	--tester-run=mysuite/mypkg/mytest:p1={a1,a2}
	--tester-run=mysuite/mypkg/mytest%3*10

The first runs every iteration where parameter ``p1`` is ``a1`` or ``a2``; the
second runs the third iteration ten times.

``--tester-req=<expression>`` filters by requirements instead, and
``--tester-no-reqs`` ignores requirements altogether. Skipped iterations are
quiet by default; ``--tester-verbskip`` logs them.


Logs
----

The raw log is written to ``tmp_raw_log`` in the log directory
(``--log-dir=<dirname>``). What you get out of it depends on which of these you
ask for: ``--log-txt=<filename>`` (text, on by default), ``--log-html=<dirname>``
(browsable HTML, the most useful during development), ``--log-plain-html=``,
``--log-json=`` and ``--log-junit=`` for CI. ``--live-log`` runs
:ref:`Report Generator Tool <doxid-group__rgt>` in live mode so you can watch
the run as it happens.

``--publish=<script>`` hands the log bundle to a site-specific script; see
:ref:`Publishing logs to Bublik web application <doxid-group__te__engine__dispatcher_1logs_publishing>`.


Debugging a run
---------------

``--tester-gdb=<testpath>`` and ``--tester-vg=<testpath>`` run the selected test
scripts under gdb or valgrind. ``--gdb-tester``, ``--vg-tester``, ``--vg-cs``,
``--vg-logger``, ``--vg-rcf`` and ``--vg-engine`` do the same for the engine
applications themselves.

``--test-wof`` stops before the jump to cleanup when a test fails, so you can
look at what was actually configured; ``--test-woc`` does it regardless of the
result. ``--cs-print-trees`` dumps the
:ref:`Configurator <doxid-group__te__engine__conf>` object and instance trees.


Expected results
----------------

``--trc-db=<filename>`` selects the
:ref:`Test Results Comparator <doxid-group__trc>` database to compare the run
against, ``--trc-tag=<TAG>`` picks the expectations for a particular
configuration, and ``--trc-html=<filename>`` writes the report. ``--trc-update``
updates the database from the run --- and ``--trc-init`` rewrites it from
scratch, so treat it with care.


Complete option list
--------------------

.. include:: dispatcher_options.inc
