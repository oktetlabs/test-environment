..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; Test Results Comparator
.. _doxid-group__trc:

Test Results Comparator
=======================

.. toctree::
	:hidden:



.. _doxid-group__trc_1trc_introduction:

Introduction
~~~~~~~~~~~~

Test Environment allows to create structured test harnesses with parametrized test packages and scripts.

Behaviour of tests and testing results may differ for different conditions: versions of tested product, operating system or NUTs. Usually it is not an option to adapt a test to different conditions, since it will significantly complicate the test and may lead to endless updates of the tests to newly arising conditions.

However, to track regressions in the tested product and find out differences in product behaviour in different conditions, it is necessary to know which test verdicts are expected for specified version/conditions and which are not.

Testing Results Comparator (TRC) tool provides an interface to a database of the expected test results for the product. It allows to:

* update the database according to obtained test results;

* obtain the report with the list of unexpected results;

* associate addtional information (like bug IDs) with the test record in the DB.





.. _doxid-group__trc_1trc_tool:

Tool execution
~~~~~~~~~~~~~~

TRC is invoked automatically at the end of dispatcher.sh execution or manually with the trc.sh script located in the test suite.

In both cases it will produce report like below:

.. code-block:: none


	Run (total)                            2616
	  Passed, as expected                  2456
	  Failed, as expected                    97
	  Passed unexpectedly                    45
	  Failed unexpectedly                    18
	  Aborted (no useful result)              0
	  New (expected result is not known)      0
	Not Run (total)                       13725
	  Skipped, as expected                    0
	  Skipped unexpectedly                    0



.. _doxid-group__trc_1trc_tool_result:

Result explanation
------------------

====================  ==============================================================================================================================================================================
                      Description
====================  ==============================================================================================================================================================================

Run(total)            Total number of iterations executed.
Passed, as expected   Number of tests which were expected to pass and passed
Failed, as expected   Number of tests which were expected to pass and passed
Passed unexpectedly   Number of tests which were not expected to pass but passed. For instance because the bug got fixed and TRC was not udpated. Or because certain SW version has a known problem.
Failed unexpectedly   Number of tests which were not expected to fail but failed. The usual failures
Aborted               Will appear if test was kiled with Ctrl-C or by a signal; tests which died of SEGFAULT signal are also considered as aborted.
Not Run (total)       Total number of iterations which were not executed. This one makes sence only if TRC DB contains
Skipped as expected   The tests were skipped as we expected. In the below example TRC is not filled properly so Not Run is not a sum of expected and unexpected counters.
Skipped unexpectedly  The test was expected to execute in given conditions but it did not.
====================  ==============================================================================================================================================================================

The skipped section





.. _doxid-group__trc_1trc_tool_runtime:

Runtime results comparison
--------------------------

If **te_tester** is built without (--without-trc) flag (default behaviour) then it will perform run-time results comparison.

To disable **te_tester** integration with TRC add

.. code-block:: none


	TE_APP_PARMS([tester], [--without-trc])

into builder.conf, see :ref:`TE_APP_PARMS <doxid-group__te__engine__builder_1te_engine_builder_conf_file_te_app_parms>` for details.

When running **te_tester** which was built with enabled runtime TRC support the usual output:

.. code-block:: none


	Starting package arp
	Starting test prologue                                               PASSED
	Starting test arp_remove                                             PASSED
	Starting test arp_change                                             PASSED
	Starting test stale_entry                                            PASSED
	Starting test stale_entry                                            FAILED
	Done package arp PASSED

is replaced with:

.. code-block:: none


	Starting package arp
	Starting test prologue                                               pass
	Starting test arp_remove                                             PASSED?
	Starting test arp_change                                             pass
	Starting test stale_entry                                            pass
	Starting test stale_entry                                            fail
	Done package arp pass

where

* **pass** means that test expectedly passed;

* **PASSED**? and **FAILED**? mean that test passed or failed but there is no expectation for it in the TRC database;

* **fail** means that test expectedly failed.

And in the text log after test completion Tester will add Obtained and Expected resutls.

.. code-block:: none


	RING Tester Run 23:13:39 743 ms
	Obtained result is:
	FAILED
	Expected results are:
	PASSED







.. _doxid-group__trc_1trc_tags:

Tags
~~~~

Every test can be executed in various conditions:

* different HW;

* different SW (version or instance);

* different configuration of the SW/HW under test;

* and many others.

The expected behaviou may differ depending on all these factors. So for proper judgement of the result TRC should be given the description of the conditions in which the test was executed.

For instance if the test validates a cross-platform application or library the expected behaviour may differ depending on the operating system. So on one OS the test will pass and on another it will fail in a certain manner. And both results are expected and acceptable.

To describe the conditions in which the test (or tests) was executed TRC is given a number of tags. The tags can either be passed to the dispatcher.sh or can be passed directly to the TRC tool.

In the first case dispatcher.sh will include full list of tags into the log (RING message with Entity=Dispatcher and User=TRC tags). TRC is capable of parsing the log and finding the tags list; so when asked to produce a report basing on the log file it will (if not asked the oposite with --ignore-log-tags option) take them into account.

Explicit tags are passed to the TRC tool via --tag=mytag option.

Todo Other TRC options description.





.. _doxid-group__trc_1trc_db:

TRC Database
~~~~~~~~~~~~



.. _doxid-group__trc_1trc_db_syntax:

Syntax
------

TRC database is an XML file with pretty simple syntax. XSD for it can be found at doc/xsd/trc_db.xsd.

The file is usually located in the conf/ directory in the suite. You can explicitly pass the path to the database with the --db option.

Below are some examples of the syntax features.





.. _doxid-group__trc_1trc_db_simple:

Simple entry
------------

.. ref-code-block:: xml

	<test name="mytest" type="script">
	  <objective>Check this freaking functionality.</objective>
	  <iter result="PASSED">
	    <notes/>
	    <arg name="env">VAR.iut_only</arg>
	  </iter>
	</test>





.. _doxid-group__trc_1trc_db_tags:

Tags
----

If certain test is expected to fail if tag **mytag** is present you can write:

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="arg">foobar</arg>
	  <notes/>
	  <results tags="mytag">
	    <result value="FAILED"/>
	  </results>
	</iter>

In general this is a bad way to describe the failure. Because it's unnatural for the test to fail in a random manner. Usually you expect the test to fail in exact way and you want to know if the test fails differently. See the next example for the way to describe such condition.

TRC supports logical expressions in the tags section, so the following examples are valid:

.. ref-code-block:: xml

	<results tags="linux&amp;x86">
	  <result value="FAILED"/>
	</results>
	<results tags="!freebsd|!windows">
	  <result value="FAILED"/>
	</results>

Tags are passed to the TRC tool as --tag=mytag meaning that the run should be compared against expected results for the tag mytag. For example --tag=linux means that we're running on linux. And --tag=!linux means that we're not.





.. _doxid-group__trc_1trc_db_verdicts:

Verdicts
--------

Imagine that the test **foobar** is expected to fail because of socket creation failure. Then one can write:

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="arg">foobar</arg>
	  <notes/>
	  <results tags="mytag">
	    <result value="FAILED">
	      <verdict>Failed to create a socket, errno=EPERM</verdict>
	    </result>
	  </results>
	</iter>

**Verdict** is a special message which test produces during it's execution. It can be just before failure or at the middle. It's produced with :ref:`TEST_VERDICT() <doxid-tapi__test__log_8h_1aa36c8292e774ba1625bcb80593cfb006>` call.





.. _doxid-group__trc_1trc_db_tags_cmp:

Tags Comparison
---------------

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="arg">foobar</arg>
	  <notes/>
	  <!-- Below it's written that (num>5)&(val<=3) -->
	  <results tags="(num&gt;5)&amp;(val&lt;=3)">
	    <result value="FAILED">
	      <verdict>Failed to do something at certain step</verdict>
	    </result>
	  </results>
	</iter>

The above code specifies certain conditions in which the test is expected to fail. To be specific '(num >= 5) and (val < 3)'. Special characters must be escaped.

Values are passed to the trc tool (or trc.sh wrapper script} as --tag=num:3.

Supported operators are ( operator : representation):

* > : &gt;

* < : &lt;

* <= : &lt;=

* >= : &gt;=

* = : =





.. _doxid-group__trc_1trc_db_keys:

Keys
----

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="arg">foobar</arg>
	  <notes/>
	  <results tags="mytag&amp;some_other_tag" key="OL 239">
	    <result value="FAILED">
	      <verdict>Failed to do something at certain step</verdict>
	    </result>
	  </results>
	</iter>

If during test behaviour investigation or development a bug to track the failure was created - it should be added as a 'key' to the TRC database result. Each project has it's own agreement on keys syntax and semantics; from the TRC point of view it's an arbitrary string.





.. _doxid-group__trc_1trc_db_notes:

Notes
-----

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="arg">foobar</arg>
	  <notes/>
	  <results tags="mytag&amp;some_other_tag" key="WONTFIX" notes="This is a known system behaviour">
	    <result value="FAILED">
	      <verdict>Failed to do something at certain step</verdict>
	    </result>
	  </results>
	</iter>

**Notes** attribute provides a way to describe the reasons or aspects of the result. This is extremely useful when you're dealing with so-called features of the product which often have WONTFIX key. It's also worth noting if the result is unstable.





.. _doxid-group__trc_1trc_db_wildcards:

Wildcards
---------

TRC database supports wildcard entries. If parameter value is ommited:

.. ref-code-block:: xml

	<iter result="PASSED">
	  <arg name="sock_type"/>
	  <results tags="linux" notes="Test fails because of a known system bug" key="WONTFIX">
	    <result value="FAILED"/>
	  </results>
	</iter>

then the entry describes expected results for all iterations of the test with particular parameter. In the above example for all types of socket the test will fail because of a known system bug.





.. _doxid-group__trc_1trc_db_navigation:

Database navigation tips
------------------------

As the database may be huge and contain several thousands of tests there exists a simple search mechanism.

Every iteration in the TRC database is identified by it's arguments and their values.

TRC tool is capable of searching the iteration by either parameter name/value pair or MD5 hash calculated from the set of iteration parameters and their values

.. code-block:: shell


	./trc.sh --search --test=<test> --hash=<MD5hash>
	./trc.sh --search --test=<test> --params <param1> <value1> [param2 value2]...

Sure there is no need to calculate the MD5 hash manually. It's printed in the log right in the beginning of the test







.. _doxid-group__trc_1trc_update:

Database update
~~~~~~~~~~~~~~~

Todo Add information about trc_update utility.

