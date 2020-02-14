.. index:: pair: group; Packet Serial Parser
.. _doxid-group__serial:

Packet Serial Parser
====================

.. toctree::
	:hidden:

Serial console parsing framework provide ability to detect certain logs appearance on the console and report it back to the test. Example of such logs may be “Kernel panic”, “Something crashed”, “U-Boot bla-bla”. The test (if exists at the moment of the event) will be notified ASAP and can take appropriate actions. Examples of actions may include diagnostics gathering, some smart logging, DUT reboot and not touching the DUT. Also it is possible just save serial console output into the TE logs.



.. _doxid-group__serial_1serial_launch:

How to launch
~~~~~~~~~~~~~

To use the framework of serial console parsing make sure that the two following steps done.



.. _doxid-group__serial_1serial_launch_build:

Build options
-------------

To compile the agents with support of serial console parsing framework should be added option **–with-serialparse** to agent type options:

.. ref-code-block:: cpp

	TE_TA_TYPE([linux], [], [unix], [--with-serialparse], [], [], [], [])





.. _doxid-group__serial_1serial_launch_conf:

Configurator objects
--------------------

The TE has auxiliary configuration file **cs.conf.serial** with configurator objects for the serial parser management. The file should be included to the config:

.. ref-code-block:: cpp

	<xi:include href="cs.conf.serial" parse="xml" xmlns:xi="http://www.w3.org/2003/XInclude"/>

To use event handlers the instance **/local:** should be added and value of the instance **/local:/tester:/enable:** should be set to **1**. The last to launch the thread on the Tester to handle events.

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<history>
	    <add>
	      <instance oid="/local:"/>
	    </add>
	    <set>
	      <instance oid="/local:/tester:/enable:" value="1"/>
	    </set>
	</history>







.. _doxid-group__serial_1serial_use:

Simple use cases
~~~~~~~~~~~~~~~~

There are two ways to configure and manage the serial console parsers: using the **configurator** and using the **Test API**. See the examples below for the both methods.



.. _doxid-group__serial_1serial_use_logging:

Logging of the serial console
-----------------------------

By default the logging is **enabled** for all active parsers. So to launch the logging should be added and enabled a **parser** instance. Level of the logs by default is **WARN**.



.. _doxid-group__serial_1serial_use_logging_conf:

Configurator
++++++++++++

To logging serial console messages to the main log, insert the following code to the configuration file. The code launches a new thread on the agent **Agt_A**. The thread establishes connection with a serial console named **console_name** by port **3105** and gathers messages from the console. **my_parser** is name of the parser.

.. ref-code-block:: cpp

	<add>
	  <instance oid="/agent:Agt_A/parser:my_parser" value="console_name"/>
	</add>
	<set>
	  <instance oid="/agent:Agt_A/parser:my_parser/port:" value="3105"/>
	  <instance oid="/agent:Agt_A/parser:my_parser/enable:" value="1"/>
	</set>

User can enable, disable or set of message level value of the logging:

.. ref-code-block:: cpp

	<set>
	  <instance oid="/agent:Agt_A/parser:my_parser/logging:/level:" value="RING"/>
	  <instance oid="/agent:Agt_A/parser:my_parser/logging:" value="1"/>
	</set>

There is the variable **logging** sets to **1**, it is to enable logging (**0** - disable). The the log **level** sets to **RING**.





.. _doxid-group__serial_1serial_use_logging_tapi:

Test API
++++++++

The second way to launch logging of the serial console is a using of the **Test API**. See the following code example to launch the same parser as before, but from a **test** :

.. ref-code-block:: cpp

	#include "tapi_serial_parse.h"

	tapi_parser_id     *pars;

	pars = tapi_serial_id_init("Agt_A", "console_name", "my_parser");

	tapi_serial_parser_add(pars);
	tapi_serial_logging_enable(pars, "WARN");

	tapi_serial_parser_del(pars);
	tapi_serial_id_cleanup(pars);

By default port for connection to the conserver is **3109**. User can change any connection configurations in the **parser id** structure. For more detailed description see the :ref:`Test API <doxid-group__serial_1serial_config_tapi>` section.





.. _doxid-group__serial_1serial_use_logging_rcf:

RCF
+++

**RCF API** also can be used to launch logging thread. In this case the thread will be launched without any participation of Configurator. See the following example.

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<rcf>
	<ta name="Agt_A" type="linux" fake="no" synch_time="no" rebootable="no"
	    rcflib="rcfunix">
	    <conf name="host">bilbo</conf>
	    <conf name="port">18001</conf>
	    <conf name="sudo"/>
	    <thread name="serial_console_log" when="rcfuser">
	        <arg value="rcfuser"/>
	        <arg value="WARN"/>
	        <arg value="100"/>
	        <arg value="3105:user:console_name"/>
	    </thread>
	</ta>
	</rcf>







.. _doxid-group__serial_1serial_use_event:

Event handling
--------------

To handle any events, one or more **event** and **handler** instances should be added on the **Tester** subtree. For detailed description of handlers see the :ref:`Tester <doxid-group__serial_1serial_arch_tester>` section. The value of the **handler** instance is a path to the executable (binary or shell command). To detect events to **agent** should be added **parser** instance. The parser should contain **event** instance. Use a name of one of the **events** declared in the **Tester** subtree as value of the **agent event** instance, in the examples it is **my_event** value. Also should be added **pattern** instances. Patterns used to detect events. An event happens if a serial log message matches at least one of the patterns, associated with the **event**. Write a pattern to the value of the **pattern** instance.



.. _doxid-group__serial_1serial_use_event_conf:

Configurator
++++++++++++

There is example how to add a parser with events via configuration file:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<history>
	    <add>
	      <instance oid="/local:"/>
	      <instance oid="/local:/tester:/event:my_event"/>
	      <instance oid="/local:/tester:/event:my_event/handler:external_handler"
	                value="/home/andrey/work/trunk/handlers/new_handler"/>
	      <instance oid="/local:/tester:/event:my_event/handler:internal_handler"/>

	      <instance oid="/agent:Agt_A/parser:my_parser" value="console_name"/>
	      <instance oid="/agent:Agt_A/parser:my_parser/event:ag_ev" value="my_event"/>
	      <instance oid="/agent:Agt_A/parser:my_parser/event:ag_ev/pattern:1" value="fooo"/>
	      <instance oid="/agent:Agt_A/parser:my_parser/event:ag_ev/pattern:2" value="bar"/>
	    </add>
	    <set>
	      <instance oid="/local:/tester:/enable:" value="1"/>
	      <instance oid="/local:/tester:/event:my_event/handler:internal_handler/internal:" value="1"/>
	      <instance oid="/local:/tester:/event:my_event/handler:internal_handler/signal:" value="SIGINT"/>
	      <instance oid="/agent:Agt_A/parser:my_parser/port:" value="3105"/>
	      <instance oid="/agent:Agt_A/parser:my_parser/enable:" value="1"/>
	    </set>
	</history>

The instance name of **pattern** object is index **number**, should be used only **integer numbers greater than zero**.





.. _doxid-group__serial_1serial_use_event_tapi:

Test API
++++++++

The second way to handle events is using the **Test API**. See the following code example to launch the same parser as before, but from a test:

.. ref-code-block:: cpp

	#include "tapi_serial_parse.h"

	...

	tapi_parser_id      *pars;

	...

	pars = tapi_serial_id_init("Agt_A", "console_name", "my_parser");
	pars->port = 3105;

	tapi_serial_tester_event_add("my_event");
	tapi_serial_handler_ext_add("my_event", "external_handler", 0,
	                          "/home/andrey/work/trunk/handlers/new_handler");
	tapi_serial_handler_int_add("my_event", "internal_handler", 0, SIGINT);

	tapi_serial_parser_add(pars);
	tapi_serial_parser_event_add(pars, "ag_ev", "my_event");
	tapi_serial_parser_pattern_add(pars, "ag_ev", 1, "fooo");
	tapi_serial_parser_pattern_add(pars, "ag_ev", 2, "bar");

	tapi_serial_parser_del(pars);
	tapi_serial_tester_event_del("my_event");
	tapi_serial_id_cleanup(pars);









.. _doxid-group__serial_1serial_arch:

Architecture of the framework
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



.. _doxid-group__serial_1serial_arch_agent:

Agent
-----

To listen serial console, perform the logging of the received data, to parse data for events a new thread starts on the **agent**. The serial console parser thread is responsible for logs gathering. The **Configurator** performs configure and launch it. It listens for the data from the conserver running on the host and receives arbitrary amount of data. The thread parses a data and searches one of events. When event is detected, the thread changes the event state in the appropriate instance. Optional, the thread can to print data from the serial console in the main log.





.. _doxid-group__serial_1serial_arch_tester:

Tester
------

The **Tester** has a separate thread too. It should periodically poll status of the parsers events from the Configurator, when an event occurs the thread calls an appropriate handler or a series of handlers.



.. _doxid-group__serial_1serial_arch_handlers:

Handlers
++++++++

When an event has been detect, the Tester calls a sequence of handlers. A handler can be internal or external, this indicates the **/local/tester/event/handler/internal** instance. The handler subtree has **priority** field, that is used to control of the handlers sequence. The higher priority - the sooner the handler performed.

If a **handler** is **internal** and an event occurred, then the Tester should send a **signal** specified in the **handler** subtree. The **event** will be processed by **test** itself.

An **external** handler is an executable program. It can to perform any actions to handle the event, but it must return one of the following values:

* 0 : continue handlers execution

* 1 : stop handlers execution

* 2 : stop both handlers and test execution

* 3 : stop handlers execution, kill the test and stop tests sequence execution.

The end user, specifying full path, can use any handler programs or scripts, that are located anywhere. But if a short name is used, the handler is searched in the default handlers directory.

By default the handlers directory is **RUN_DIR/handlers**. The directory of handlers can be changed by user.









.. _doxid-group__serial_1serial_config:

Configuration
~~~~~~~~~~~~~



.. _doxid-group__serial_1serial_config_conf:

Configurator
------------

Two configuration subtrees provided to manage the serial parser framework. The first subtree is **/local/tester**, it is used to declare **Tester events**. There is also performed binding of some **handlers** to the events. ====================================  =======  ====================================================================
OID                                   Type     Description
====================================  =======  ====================================================================

/local/tester                         RW none  Tester subtree.
/local/tester/enable                  RW int   Enable the Tester thread to listen for events. By default disabled.
/local/tester/period                  RW int   Period for polling of events status. By default is 100 milliseconds.
/local/tester/location                RW str   Directory with handlers for short named handlers. By default
/local/tester/event                   RC none  Subtree of a
/local/tester/event/handler           RC str   Event handler. Use a path to the executable as value.
/local/tester/event/handler/priority  RW int   Handler priority.By default is
/local/tester/event/handler/signal    RW int   Signal number.
/local/tester/event/handler/internal  RW int   Indicates that it is internal handler. By default, it is external.
====================================  =======  ====================================================================

A new subtree of the agent is **/agent/parser**. It is used to configure and launch a **parser** thread for a serial consoles on the agent.

===========================  ======  ===============================================================================
OID                          Type    Description
===========================  ======  ===============================================================================

/agent/parser                RC str  The
/agent/parser/port           RW int  Conserver port corresponds to the parameters of conserver launch. By default is
/agent/parser/user           RW str  Conserver user name. By default is
/agent/parser/enable         RW int  Start/stop the
/agent/parser/interval       RW int  Intreval of polling messages from the console.
/agent/parser/reset          RW int  Reset status for all events.
/agent/parser/event          RC str
                                     declared in the Tester subtree as value.
/agent/parser/event/pattern  RC str  Pattern string.
/agent/parser/event/counter  RW int  Counter of the happened event.
/agent/parser/event/status   RW int  Status of the event.
/agent/parser/logging        RW int  Enable logging of the console messages in the main log. By defaul enabled.
/agent/parser/logging/level  RW str  Level of the message for logging. By default is
===========================  ======  ===============================================================================

Configuration exmaple:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	  <history>
	    <add>
	      <instance oid="/local:"/>
	      <instance oid="/local:/tester:/event:new_event"/>
	      <instance oid="/local:/tester:/event:new_event/handler:1" value="~/work/trunk/handlers/myserialhandler"/>
	      <instance oid="/local:/tester:/event:new_event/handler:2" value="ext_handler_short_name"/>
	      <instance oid="/local:/tester:/event:new_event/handler:3"/>

	      <instance oid="/agent:Agt_A/parser:new_parser" value="console_name"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/event:ag_ev" value="new_event"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/event:ag_ev/pattern:1" value="fooo"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/event:ag_ev/pattern:2" value="bar"/>
	    </add>
	    <set>
	      <instance oid="/local:/tester:/enable:" value="1"/>
	      <instance oid="/local:/tester:/period:" value="200"/>
	      <instance oid="/local:/tester:/location:" value="/home/andrey/work/trunk/handlers"/>
	      <instance oid="/local:/tester:/event:new_event/handler:1/priority:" value="3"/>
	      <instance oid="/local:/tester:/event:new_event/handler:2/priority:" value="2"/>
	      <instance oid="/local:/tester:/event:new_event/handler:3/priority:" value="1"/>
	      <instance oid="/local:/tester:/event:new_event/handler:3/internal:" value="1"/>
	      <instance oid="/local:/tester:/event:new_event/handler:3/signal:" value="SIGINT"/>

	      <instance oid="/agent:Agt_A/parser:new_parser/port:" value="3105"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/user:" value="tester"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/interval:" value="100"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/logging:" value="1"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/logging:/level:" value="WARN"/>
	      <instance oid="/agent:Agt_A/parser:new_parser/enable:" value="1"/>
	    </set>
	  </history>





.. _doxid-group__serial_1serial_config_tapi:

Test API
--------

The Test API provides the possibilities to add, delete, configure and manage the parser threads and event handlers. The complete list and detailed description functions of the Test API you can find here ``lib/tapi/tapi_serial_parse.h``.

**Test API** uses a :ref:`tapi_parser_id <doxid-structtapi__parser__id>` structure to configure and control a parser:

.. ref-code-block:: cpp

	typedef struct tapi_parser_id {
	    const char *ta;      /**< Test agent name */
	    const char *name;    /**< The parser name */
	    const char *c_name;  /**< Serial console name */
	    const char *user;    /**< A user name for the conserver or NULL. In case if NULL, used default value 'tester'. */
	    int port;            /**< Port of the console */
	    int interval;        /**< Interval to poll data from the conserver. Use -1 for default value. */
	} tapi_parser_id;

It is possible to initialize the **parser id** structure using the **TAPI**. In this case the structure should be cleaned after use, because **TAPI** function allocates memory. Use the :ref:`tapi_serial_id_cleanup() <doxid-group__tapi__conf__serial__parse_1ga77e9ef3c3fae27066d9b1d0ed845a0f5>` to cleanup the **id** structure. By default the **user** field value is **NULL**, it means in logs will use the default user - **tester**. Default port is **3109**, interval - **100** milliseconds. See the following example.

.. ref-code-block:: cpp

	#include "tapi_serial_parse.h"

	tapi_parser_id     *pars;

	/* Initialize identifier*/
	pars = tapi_serial_id_init("Agt_B", "console_name", "my_parser");

	/* Cleanup identifier */
	tapi_serial_id_cleanup(pars);

Test API using example:

.. ref-code-block:: cpp

	#include "tapi_serial_parse.h"

	tapi_parser_id      pars;
	int                 pat_i;

	pars.ta         = "Agt_A";
	pars.name       = "new_parser";
	pars.c_name     = "console_name";
	pars.user       = NULL;
	pars.port       = 3109;
	pars.interval   = -1;              /* value -1 is used to set default value of the polling period */

	/* Add and initialize parser elements */
	tapi_serial_tester_event_add("new_event");
	tapi_serial_handler_ext_add("new_event", "ext_h", 5, "ext_handler_short_name");
	tapi_serial_handler_int_add("new_event", "int_h", 4, SIGINT);
	tapi_serial_parser_add(&pars);
	tapi_serial_parser_disable(&pars);
	tapi_serial_parser_enable(&pars);
	tapi_serial_logging_enable(&pars, NULL);
	tapi_serial_parser_event_add(&pars, "tapi_event", "new_event");
	pat_i = tapi_serial_parser_pattern_add(&pars, "tapi_event", "Segmentation fault");

	/* Remove the initialized elements */
	tapi_serial_parser_pattern_del(&pars, "tapi_event", pat_i);
	tapi_serial_parser_event_del(&pars, "tapi_event");
	tapi_serial_logging_disable(&pars);
	tapi_serial_parser_del(&pars);
	tapi_serial_handler_del("new_event", "ext_h");
	tapi_serial_handler_del("new_event", "int_h");
	tapi_serial_tester_event_del("new_event");

