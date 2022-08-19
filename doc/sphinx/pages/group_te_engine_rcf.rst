.. index:: pair: group; Remote Control Facility (RCF)
.. _doxid-group__te__engine__rcf:

Remote Control Facility (RCF)
=============================

.. toctree::
	:hidden:

	group_rcfapi.rst
	group_te_lib_rpc.rst



.. _doxid-group__te__engine__rcf_1te_engine_rcf_introduction:

Introduction
~~~~~~~~~~~~

Remote Control Facility (RCF) is a distributed entity allowing interactions between :ref:`Test Engine <doxid-group__te__engine>` and :ref:`Test Agents <doxid-group__te__agents>`.

RCF is responsible for:

* starting :ref:`Test Agents <doxid-group__te__agents>` during initialization or after reloading the NUT or after Test Agent death;

* time synchronization;

* low-level interactions between :ref:`Test Agents <doxid-group__te__agents>` and :ref:`Test Engine <doxid-group__te__engine>` (serial port, telnet, ssh, etc.);

* construction and passing commands from :ref:`Test Engine <doxid-group__te__engine>` to :ref:`Test Agents <doxid-group__te__agents>` and processing answers;

* providing convenient API to other TE subsystems and tests;

* interpreting commands on the :ref:`Test Agents <doxid-group__te__agents>` calling appropriate handles;

* logging commands and answers.

The following diagram illustrates data flows in RCF:

.. image:: /static/image/ten_rcf_data_flow.png
	:alt: Data flows in RCF


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ta_interaction:

Interaction with Test Agents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On start-up RCF gets from RCF configuration file the list of :ref:`Test Agents <doxid-group__te__agents>` to be started. For each Test Agent the file contains its name, type and additional configuration parameters.

After TA was run, RCF tries to connect to it several times (3 by default) before exiting with failure if connection cannot be established. The number of attempts to connect can be changed with RCF_TA_MAX_CONN_ATTEMPTS environment variable.

Test Agent type value is used by the RCF to find out which shared library should be dynamically linked to the RCF process to interact with the Test Agent. This library “knows” how to start and stop the Test Agent and how to send/receive commands to/from it. It provides to RCF the strictly specified set of functions.

For example, the library rcfunix is responsible for interacting with UNIX-like :ref:`Test Agents <doxid-group__te__agents>` (Linux, BSD, Cygwin). It uses scp to copy Test Agent image to the remote host and ssh to start or stop it. TCP connection is used to send/receive commands.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_test_proto:

Test Protocol
~~~~~~~~~~~~~

RCF interacts with the :ref:`Test Agents <doxid-group__te__agents>` using text-based Test Protocol. Some text commands may be accompanied by binary attachments. When the RCF is compiled with high log level, all Test Protocol commands and answers appear in the log.

It is possible to create logical sub-channels (“sessions”) in the connection to the Test Agent. Inside the particular session, the commands are sent to the Test Agent one-by-one (i.e., next command is sent only after receiving the answer to the previous one). However, if several sessions are created, the Test Agent may be asked to do several actions simultaneously (for example, generate traffic, catch traffic and call a remote routine).

Efficient using of this feature is possible on multi-thread :ref:`Test Agents <doxid-group__te__agents>` only.

RCF awaits a command acknowledgement before sending the next one.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_pch:

Portable Command Handler
~~~~~~~~~~~~~~~~~~~~~~~~

:ref:`Test Agents <doxid-group__te__agents>` are usually based on the RCF library “Portable Command Handler” (lib/rcfpch) which is responsible for parsing Test Protocol commands and calling TA-specific handlers for the command processing. It also provides default handlers and functions for TA-specific handlers implementation.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad:

Application Domains
~~~~~~~~~~~~~~~~~~~

All functionality provided by RCF can be divided into several areas – Application Domains. This chapter describes shortly the functionality provided by each AD.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_ta_ctrl:

Test Agent Control
------------------

RCF is able to start the Test Agent, shutdown it, restart Test Agent itself as well as reboot the host on which the Test Agent is running or NUT controlled by the Test Agent. RCF allows to retrieve the list of all running :ref:`Test Agents <doxid-group__te__agents>` as well as their types.

RCF discovers a TA death by breaking of the connection or a command time-out. In this case, the TA application is restarted. Moreover, RCF may be asked to force a check of the Test Agent validity to discover if all :ref:`Test Agents <doxid-group__te__agents>` are still alive or not.

Finally RCF provides to :ref:`Logger <doxid-group__te__engine__logger>` get log command allowing retrieval of the log as a binary attachment.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_cfg:

TA Configuring
--------------

A service of this application domain is used by :ref:`Configurator <doxid-group__te__engine__conf>` as the previous one and allows to add/delete/change object instance on the TA as well as obtain the value of existing instance.

It is also possible to perform wildcard get operations.

Usually :ref:`Configurator <doxid-group__te__engine__conf>` issues commands like get \*:\* or get /a:a1/b:b1/... to retrieve identifiers of all instances (or only instances of a particular subtree) present on the Test Agent. Then it may separately get value of each instance by its identifier.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_vars:

Variables
---------

RCF allows to get or change the value of a global variable of the Test Agent program (integer or string) or an environment variable of the Test Agent (string). This feature is used rarely and usually for environment variables only.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_files:

Files
-----

Following services are included in the application domain:

* copy file from the :ref:`Test Engine <doxid-group__te__engine>` to the :ref:`Test Agents <doxid-group__te__agents>`;

* copy file from the :ref:`Test Agents <doxid-group__te__agents>` to the :ref:`Test Engine <doxid-group__te__engine>`;

* delete file on the :ref:`Test Agents <doxid-group__te__agents>`.

Piece of memory in the TA address space may be copied as well using /memory/<address>:<length> path.

Files are transferred between TA and :ref:`Test Engine <doxid-group__te__engine>` as attachments of protocol commands.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_traffic:

Traffic
-------

Following services are included in the application domain:

* create/destroy CSAPs (Communication Service Access Points) on the Test Agent;

* obtain/change CSAP parameters;

* transmit packets from the Test Agent according to a specified template;

* receive packets matched to a specified pattern on the Test Agent.

CSAP should be created before any traffic sending or receiving. It is similar to socket in Berkeley Socket API, but may provide an access to CLI or other communication facilities as well.

CSAP may consist of several layers to allow send/receive packets corresponding to a protocol stack. For example, it's possible to create CSAP "dhcp.udp.ipv4.aal5.atm".

Parameters of the CSAP as well as packet templates and patterns are described using ASN.1. Each level (protocol) is described according to its rules.

CSAP parameters and packets on the Test Engine are stored as files on the TEN and passed to/from Test Agent as binary attachments (compiling of RCF with high log level however allows to observe content of these files in the log).

Test API libraries are provided to work with TAD. They allow to create CSAP and send/receive packets using functions adapted for particular media and/or protocol.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_ad_proc:

Routines and Processes
----------------------

RCF allows to call a C routine on the Test Agent in different ways:

* call it in the context of the Test Agent;

* start the thread in the same address space with the routine as an entry point;

* start the process with the routine as an entry point.

The routine is specified by the name and should correspond to a global routine defined in the Test Agent or a library linked with it (symbol table is built for the Test Agent). For process starting, it is also possible to specify the name of the program instead of the name of the routine.

The mechanism described above allows very restricted routine parameters (only integers and strings) and only integer return code. It also does not assume output parameters.

Another mechanism based on SUN RPC is provided by RCF for API testing (white box testing). It allows:

* create a separate process: RPC server;

* fork one RPC server from another;

* create a thread in the RPC server;

* destroy the RPC server;

* call the routing with arbitrary input and output parameters.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_conf_file:

RCF Configuration File
~~~~~~~~~~~~~~~~~~~~~~

:ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` **has** its own configuration file that specifies the list of :ref:`Test Agents <doxid-group__te__agents>` to run. Examples of :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file can be found under ``${TE_BASE}/conf`` directory.

Simple configuration file looks like the following:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<rcf>
	    <ta name="Agt_B" type="linux_snmp" rcflib="rcfunix">
	        <conf name="host">olwe</conf>
	        <conf name="port">5012</conf>
	        <conf name="sudo"/>
	    </ta>
	</rcf>

This configuration file tells :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` to run one Test Agent, where the meaning of attributes is following:

* ``name`` - specifies the name to be associated with running Test Agent. While using functions of :ref:`API: RCF <doxid-group__rcfapi__base>` you will need to pass the value of this attribute (Agt_B in our sample) as the value of the first parameter.

  .. ref-code-block:: c

    ta_name = "Agt_B";
    rc = rcf_ta_del_file(ta_name, session_id, file_path);

* ``type`` - specifies type of Test Agent. This value shall match with any type of Test Agent specified via TE_TA_TYPE directive of :ref:`Builder <doxid-group__te__engine__builder>` configuration file (See :ref:`TE_TA_TYPE <doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_ta_type>` for additional information). For example:

  .. ref-code-block:: none

    # From builder.conf
    # Agent with WiFi support for PRISM 54 Wireless card
    TE_TA_TYPE([linux_wifi_prism54], [], [unix],
               [--with-rcf-rpc --enable-wifi --enable-8021x
                --with-tad=dummy_tad],
               [-DWIFI_CARD_PRISM54],
               [], [], [comm_net_agent ndn asn])



  .. ref-code-block:: xml

    <?xml version="1.0"?>
    <rcf>
        <ta name="Agt_A" type="linux_wifi_prism54" rcflib="rcfunix"/>
    </rcf>

* ``rcflib`` - specifies the name of shared library to be used by :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` in order to communicate with this particular Test Agent. rcfunix corresponds to a library located under ${TE_BASE}/lib/rcfunix directory.

The Test Agent configuration parameters that are passed to communication library (to a library specified via rcflib attribute) should be written in separated subitems "attr:name" (see :ref:`RCF Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib>`), like this:

.. ref-code-block:: xml

	<?xml version="1.0"?>
	<rcf>
	    <ta name="Agt_A" type="linux_wifi_prism54" rcflib="rcfunix">
	        <conf name="host">marcus</conf>
	        <conf name="port">5012</conf>
	        <conf name="sudo"/>
	    </ta>
	</rcf>

Other attributes that you can specify for a Test Agent are:

* ``synch_time`` - Enable/disable time synchronization between Test Engine and Test Agent (possile values are yes or no, the default is no);

* ``rebootable`` - Tell RCF whether Test Agent can be rebooted or not (possile values are yes or no, the default is no). If this attribute is not enabled, :ref:`rcf_ta_reboot() <doxid-group__rcfapi__base_1ga65756a262339c28a61f62c04766734ff>` function from :ref:`API: RCF <doxid-group__rcfapi__base>` returns ``TE_EPERM`` error code;

* ``disabled`` - Whether RCF shall ignore this Test Agent (act as if there was no such Test Agent in configuration file). This attribute can be used to tune RCF configuration via environment variables. For example:

  .. ref-code-block:: xml

    <ta name="Agt_A" type="${TE_TST_TA_TYPE:-linux}" disabled="${TE_TST:-yes}" rcflib="rcfunix"
        rebootable="${TE_TST_REBOOTABLE}">
        <conf name="host">${TE_TST}</conf>
        <conf name="port">${TE_TST_PORT:-${TE_RCF_PORT:-50000}}</conf>
        <conf name="sudo" cond="${TE_TST_SUDO:-true}"/>
        <conf name="shell">${TE_TST_VG}</conf>
    </ta>

  This is an example of tunable RCF configuration. Regarding our ``disabled`` attribute we can skip running Test Agent ``Agt_A`` if ``TE_TST`` environment variable is not set;

* ``fake`` - Tells RCF that this Test Agent is already running (for example because it is run under gdb);

* ``cold_reboot`` - Specifies Power Control Test Agent name and its parameters that can be used to perform cold reboot ot this TA. Usually has the form ``[power TA name]:[outlet name]``, like:

  .. ref-code-block:: xml

    <?xml version="1.0"?>
    <rcf>
        <ta name="Agt_A" type="bsd" rcflib="rcfunix" cold_reboot="Agt_Power:5">
            <conf name="host">192.168.1.2</conf>
            <conf name="port">5050</conf>
            <conf name="sudo"/>
        </ta>
        <ta name="Agt_Power" type="power_ctl" rcflib="rcfunix">
            <conf name="host">192.168.1.120</conf>
            <conf name="port">${TE_POWER_PORT:-${TE_RCF_PORT:-50000}}</conf>
        </ta>
    </rcf>


.. _doxid-group__te__engine__rcf_1te_engine_rcf_conf_file_threads:

Agent side threads/processes
----------------------------

RCF configuration file allows to specify a number of threads or processes to create on Test Agent at start-up. Also it is possible to specify the list of functions to be called on Test Agent at start-up. These features are implemented via standard RCF mechanisms that can be done from test scenarios with calls to:

* :ref:`rcf_ta_start_task() <doxid-group__rcfapi__base_1ga6bb2cb53cce5237a8c50a0ffbd09b823>`;

* :ref:`rcf_ta_start_thread() <doxid-group__rcfapi__base_1ga354bb92b6a19239e5ead46a9985ed8cc>`;

* :ref:`rcf_ta_call() <doxid-group__rcfapi__base_1ga2f02c1a61e6756b4c97772e5d07a31da>`.



.. ref-code-block:: xml

	<?xml version="1.0"?>
	<rcf>
	    <ta name="Agt_C" type="linux" rcflib="rcfunix">
	        <conf name="host">olwe</conf>
	        <conf name="port">5000</conf>
	        <conf name="sudo"/>
	        <thread name="log_serial">
	            <arg value="${TE_TST2_LOG_SERIAL}"/>
	            <arg value="WARN"/>
	            <arg value="10"/>
	            <arg value="3109:${TE_LOG_SERIAL_USER:-tester}:${TE_TST2_LOG_SERIAL}"/>
	        </thread>
	        <task name="temp_init" when="${TE_TST_TEMP_TASK}"/>
	        <function name="usb_init">
	            <arg value="ehci"/>
	        </function>
	    </ta>
	</rcf>

This configuration file tells RCF to run one Test Agent named ``Agt_C`` and on start-up do:

#. create a thread in Test Agent application passing control to ``log_serial`` function with four arguments;

#. if ``TE_TST_TEMP_TASK`` is not empty, create a separate task on Test Agent node using ``temp_init`` function as task entry point;

#. call function ``usb_init`` passing one argument with value ehci.

This feature can be useful if we need to do some system initialization on Test Agent side and we do not want to modify test suite sources.

Another good example is serial logs collection:

.. ref-code-block:: xml

	<thread name="log_serial" when="${TE_AGT_LOG_SERIAL}">
	    <arg value="${TE_AGT_LOG_SERIAL}"/>
	    <arg value="WARN"/>
	    <arg value="10"/>
	    <arg value="3109:${TE_LOG_SERIAL_USER:-tester}:${TE_AGT_LOG_SERIAL}"/>
	</thread>

Below element tells RCF that an agent should have a thread running :ref:`log_serial() <doxid-group__te__tools__te__kernel__log_1gac4c1a83a4811e07cba3e529f6b644324>` function if TE_AGT_LOG_SERIAL is set. Arguments are passed to the thead function.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_conf_file_xsd:

RCF Configuration file XSD
--------------------------

XSD schema of :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file can be found at ``${TE_BASE}/doc/xsd/rcf_config.xsd`` file.


.. _doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib:

RCF Communication Library
~~~~~~~~~~~~~~~~~~~~~~~~~

For each Test Agent listed in :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file we specify rcflib attribute whose value define the name of communication library that :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` shall use while sending or receiving commands to/from that Test Agent. Currently TE supports only one type of :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` communication library - :ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>`.



.. _doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix:

RCF UNIX Communication Library
------------------------------

Test Environment: RCF library for UNIX Test Agents

:ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>` should be used to control and interact with :ref:`Test Agents <doxid-group__te__agents>` on Unix-like hosts. It uses ssh and scp utilities to run commands on other hosts and to copy files (Test Agent executables) to other hosts. Output from Test Agent (stdout and stderr) is directed to TE log with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` as logging entity name and Test Agent name as logging user name and to ta.<name> file in run directory.

Apart from rcflib attribute for each Test Agent :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file contains confstr attribute that specifies configuration string passed to communication library. The format of this configuration string is library specific.

Configuration string of the :ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>` has the following format:

.. code-block:: none

	[[user@]<IP_address_or_hostname>]:<port>
	[:key=<ssh_private_key_file>]
	[:ssh_port=<port>]
	[:ssh_proxy=<ssh-proxy>]
	[:copy_timeout=<timeout>]
	[:copy_tries=<number_of_tries>]
	[:kill_timeout=<timeout>]
	[:sudo|su][:<shell>][:<parameters>]

where elements in square brackets are optional and may be skipped.

* ``<IP_address_or_hostname>`` - is IPv4, IPv6 or DNS address of the host where to run Test Agent. If the value is an empty string (skipped), then the Test Agent runs on local host (on a host where :ref:`Test Engine <doxid-group__te__engine>` runs);

* ``user`` - if specified it is the user to log in as on the <IP_address_or_hostname>;

* ``<port>`` - is TCP port to bind TCP server on the Test Agent (:ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>` is based on TCP sockets and Test Agent plays role of TCP server in connection establishment with :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, which means :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` side shall know to which address and port to connect);

* ``key`` - specifies file from which the identity (private key) for RSA or DSA authentication is read;

* ``ssh_port`` - specifies TCP port to be used by SSH. May be unspecified or 0 to use standard SSH port 22;

* ``ssh_proxy`` - specifies SSH proxy to be used, it may include SSH options and must include proxy host name or IP address;

* ``copy_timeout`` - specifies the maximum time duration (in seconds) that is allowed for image copy operation. If image copy takes more than this timeout, Test Agent start-up procedure fails, provided that the copy operation was the last try (see copy_tries);

* ``copy_tries`` - specifies the number of tries to perform image copy operation. The time to wait between tries is doubled from RCFUNIX_COPY_RETRY_SLEEP_FIRST_SEC to RCFUNIX_COPY_RETRY_SLEEP_MAX_SEC with every next try. If all copy tries fail, Test Agent start-up procedure fails;

* ``kill_timeout`` - specifies the maximum time duration (in seconds) that is allowed for Test Agent termination procedure;

* ``sudo\|su`` - specify this option when we need to run agent under sudo\|su (with root privileges). This can be necessary if Test Agent access resources that require privileged permissions (for example network interface configuration);

* ``<shell>`` - is usually used to run the Test Agent under valgrind tool with a set of options (e.g. valgrind tool=memcheck). Note that this part of configuration string CANNOT contain collons;

* ``<parameters>`` - string value that is transparently passed to the Test Agent executables as command-line parameters (each token separated with spaces will go as a separate command line parameter).

Here are few examples of configuration strings suitable for :ref:`RCF UNIX Communication Library <doxid-group__te__engine__rcf_1te_engine_rcf_comm_lib_unix>` :

* .. code-block:: xml


    <ta name="Agt_A" type="linux" rcflib="rcfunix"
        confstr=":12000::"/>

  will run Test Agent on localhost host with user privileges, TCP server will be bound to port 12000;

* .. code-block:: xml


    <ta name="Agt_A" type="linux" rcflib="rcfunix"
        confstr=":12000::valgrind --tool=helgrind:"/>

  will run Test Agent on localhost host with user privileges, TCP server will be bound to port 12000, the Test Agent will be run under valgrind with a data-race detection tool;

* .. code-block:: xml


    <ta name="Agt_A" type="linux" rcflib="rcfunix"
        confstr="loki:12000:sudo:valgrind --tool=memcheck --num-callers=32:"/>

  will run Test Agent on loki host with root privileges. The Test Agent will be run under valgrind with memory checking tool and 32-depth call backtrace.

It is worth noting that a user who runs :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` script should be able to enter hosts specified in :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>` configuration file without password prompt (e.g. using public key). It requires special tunings of SSH daemon on remote host as well. File name that keeps private key for a particular Test Agent can be specified with ``key`` option of configuration string.

If ``sudo`` element is specified in the configuration string of a Test Agent, then it is assumed that user is sudoer without password.


.. _doxid-rcfunix_conflib_1rcfunix_conflib:

Building RCF UNIX library
~~~~~~~~~~~~~~~~~~~~~~~~~

In order to build RCF UNIX library you should just mention rcfunix in the list of platform libraries:

.. code-block:: none

    TE_PLATFORM([], [], [-D_GNU_SOURCE], [-D_GNU_SOURCE], [], [tools ... rcfunix ...])

This will build default version of rcfunix suitable for Linux, but there can be some UNIX specific issues that need to be resolved with an alternative build of rcfunix. For example there is a minor difference ofthis library for Solaris (On Solaris we should use 'coreadm' utility to configure the name of core files). To sort this out we can build a version of rcfunix for Solaris:

.. code-block:: none

    TE_PLATFORM([], [], [-D_GNU_SOURCE], [-D_GNU_SOURCE], [], [tools ... rcfunix rcfsolaris ...]) TE_LIB_PARMS([rcfsolaris], [], [rcfunix], [], [-DRCF_UNIX_SOLARIS])

Then in RCF configuration file we can specify 'rcfsolaris' as the value of 'rcflib' attribute.

|	:ref:`API Usage: Remote Control Facility API<doxid-group__rcfapi>`
|		:ref:`API: RCF<doxid-group__rcfapi__base>`
|	:ref:`API Usage: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc>`
|		:ref:`API: RCF RPC<doxid-group__te__lib__rcfrpc>`
|		:ref:`RPC pointer namespaces<doxid-group__rpc__type__ns>`
|		:ref:`TAPI: Remote Procedure Calls (RPC)<doxid-group__te__lib__rpc__tapi>`
|			:ref:`Macros for socket API remote calls<doxid-group__te__lib__rpcsock__macros>`
|			:ref:`TAPI for RTE EAL API remote calls<doxid-group__te__lib__rpc__rte__eal>`
|			:ref:`TAPI for RTE Ethernet Device API remote calls<doxid-group__te__lib__rpc__rte__ethdev>`
|			:ref:`TAPI for RTE FLOW API remote calls<doxid-group__te__lib__rpc__rte__flow>`
|			:ref:`TAPI for RTE MBUF API remote calls<doxid-group__te__lib__rpc__rte__mbuf>`
|			:ref:`TAPI for RTE MEMPOOL API remote calls<doxid-group__te__lib__rpc__rte__mempool>`
|			:ref:`TAPI for RTE mbuf layer API remote calls<doxid-group__te__lib__rpc__rte__mbuf__ndn>`
|			:ref:`TAPI for RTE ring API remote calls<doxid-group__te__lib__rpc__rte__ring>`
|			:ref:`TAPI for asynchronous I/O calls<doxid-group__te__lib__rpc__aio>`
|			:ref:`TAPI for directory operation calls<doxid-group__te__lib__rpc__dirent>`
|			:ref:`TAPI for interface name/index calls<doxid-group__te__lib__rpc__ifnameindex>`
|			:ref:`TAPI for miscellaneous remote calls<doxid-group__te__lib__rpc__misc>`
|			:ref:`TAPI for name/address resolution remote calls<doxid-group__te__lib__rpc__netdb>`
|			:ref:`TAPI for remote calls of Winsock2-specific routines<doxid-group__te__lib__rpc__winsock2>`
|			:ref:`TAPI for remote calls of dynamic linking loader<doxid-group__te__lib__rpc__dlfcn>`
|			:ref:`TAPI for remote calls of power switch<doxid-group__te__lib__rpc__power__sw>`
|			:ref:`TAPI for signal and signal sets remote calls<doxid-group__te__lib__rpc__signal>`
|			:ref:`TAPI for socket API remote calls<doxid-group__te__lib__rpc__socket>`
|			:ref:`TAPI for some file operations calls<doxid-group__te__lib__rpc__unistd>`
|			:ref:`TAPI for standard I/O calls<doxid-group__te__lib__rpc__stdio>`


