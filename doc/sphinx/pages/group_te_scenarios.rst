.. index:: pair: group; The format of test suite scenarios
.. _doxid-group__te__scenarios:

The format of test suite scenarios
==================================

.. toctree::
	:hidden:



.. _doxid-group__te__scenarios_1te_scenarios_introduction:

Introduction
~~~~~~~~~~~~

This document describes the format of test suite scenarios and the script, which helps to work with it. The document format of test suite scenarios is written on **yaml** language. The script scenarios.py is designed to automate some actions with test suite scenarios. The script can manage the test suite scenarios: create review requests with test scenario on ReviewBoard and generate templates of tests.





.. _doxid-group__te__scenarios_1te_scenarios_specification:

Elements descriptions for test scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* The root element is a dictionary with:

  * mandatory field:

    * **groups** - contains the list of **group** elements.

* The element **group** is a dictionary with:

  * mandatory fields:

    * **group** - the group name (the full reference to the test contains this field);

    * **summary** - the short group description (the test header contains this field);

    * **objective** - the full group description.

  * optional fields:

    * **sub** - the list of **sub** elements;

    * **groups** - the list of **group** elements;

    * **tests** - the list of **test** elements.

* The element **sub** is similar to an element **group**. This element contains same fields as element **group**. But **sub** is used for logical division and it does not create subpackages, i.e. it does not affect real packages structure.

* The element **test** is a dictionary with:

  * mandatory fields:

    * **test** - the test name (the test filename contains this field);

    * **summary** - the short test description (the file header contains this field in the **Doxygen** command **@page**).

  * optional fields of unimplemented tests (these fields will be automatically moved from the document to a template of test when the test template is generated, i.e. these fields are absent in the document for the implemented tests):

    * **objective** - the full test description;

    * **params** - the list of **param** elements;

    * **steps** - the list of **step** elements (the test **@par** **Scenario** contains this list).

* The element **param** is a dictionary with:

  * mandatory fields:

    * **param** - the parameter name;

    * **description** - the parameter description.

  * optional fields:

    * **type** - the parameter type, e.g. "boolean";

    * **values** - the list of **value** elements.

* The element **value** has two forms:

  #. the string with the parameter value;

  #. the dictionary with the single pair (the key contains the parameter value, the value of this pair contains the parameter description).

* The element **step** has two forms:

  #. the string with step description;

  #. the dictionary with the single pair (the key contains the step description, the value of this pair contains the list of **step** elements).





.. _doxid-group__te__scenarios_1te_scenarios_examples:

Some examples of test suite scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The minimal test suite scenario:

.. ref-code-block:: cpp

	groups:
	  - group: new_ts
	    summary: Summary for new test suite
	    objective: Objective for new test suite

The complex test suite scenario:

.. ref-code-block:: cpp

	groups:
	- group: example_ts
	  summary: Example test suite description
	  objective: Long description for example test suite
	  groups:
	  - group: first_part
	    summary: First part of test suite
	    objective: Several implemented tests and one unimplemented test.
	    sub:
	    - group: subgroup
	      summary: Test second feature
	      objective: Several implemented tests which test the second feature.
	      tests:
	      - test: second_1
	        summary: Test first part of second feature

	      - test: second_2
	        summary: Test second part of second feature

	    tests:
	    - test: first
	      summary: Test first feature

	    - test: third
	      summary: Test third feature
	      objective: This test should test third feature
	      params:
	      - param: alpha
	        description: alpha parameter
	        values:
	        - 1
	        - 25
	        - default: do not set (default alpha value is @c 16)
	      - param: beta
	        description: beta parameter
	      steps:
	      - Step can be any string.
	      - We can create step with substeps:
	          - substep is just the same step
	          - and also can have subparagraphs:
	            - first step on third level
	          - last step on second level





.. _doxid-group__te__scenarios_1te_scenarios_workflow:

Workflow with test suite scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Create a new test suite:

   * Create the minimal scenario for new test suite.

   * Create the new review request using the script:

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --postreview ...

   * Push the file with full scenarios after review.

   Examples:

   * If we use the real file with test suite scenarios, e.g. OKTL-0000999-example_ts.yml.

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --postreview -s OKTL-0000999-example_ts.yml -t example-ts -a "TE Maintainers <te-maint@oktetlabs.ru>" -c "Copyright (C) {year} Example Ltd." -r example-ts

     Where:

     * ``-s OKTL-0000999-example_ts.yml`` the file which contains test suite scenarios;

     * ``-t example-ts`` the relative or absolute test suite location;

     * ``-a "TE Maintainers <te-maint@oktetlabs.ru>"`` the author for test templates in the created review request;

     * ``-c "Copyright (C) {year} Example Ltd."`` the copyright string format. Can contain argument **{year}** which will be replaced by the current year value. Optional. Default is **TE_COPYRIGHT**;

     * ``-r example-ts`` the repository ID for the created review request.

   * If we use the doclist with some document ID.

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --post -d 999 -t example-ts -a "TE Maintainers <te-maint@oktetlabs.ru>" -r example-ts

     Where:

     * ``--post`` the short form of ``--postreview``;

     * ``-d 999`` the document ID when we use doclist.

#. Import implemented tests list to an existing or new test scenarios document:

   * .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --import-ts --scenario document.yaml ...

     If the document.yaml exists, then the command adds entries in it, otherwise the command creates the new document.

   Examples:

   * .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --import-ts -s example_ts.yml -t example-ts --sorting

     Where:

     * ``--sorting`` the requirement to sort lists of groups and tests.

#. Add a new test scenario:

   * Post the current test suite scenarios on ReviewBoard using the script (only for yourself, without any groups and only with you in reviewers) and manually publish it:

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --postreview ...

   * Add the new test scenario in the document of test suite scenarios.

   * Update the published review request with the new test suite scenario.

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --postreview --existing review_id ...

   * Add nececcary groups and users and publish the updated review request.

   * Push the file with full scenarios after review.

   Example:

   * .. code-block:: none

     	$TE_BASE/scripts/scenarios.py -p -s OKTL-0000999-example_ts.yml -t example-ts -a "TE Maintainers <te-maint@oktetlabs.ru>" -r example-ts
     	$TE_BASE/scripts/scenarios.py -p -s OKTL-0000999-example_ts.yml -t example-ts -a "TE Maintainers <te-maint@oktetlabs.ru>" -e 1111

     Where:

     * ``-p`` the short form of ``--postreview``;

     * ``-e 1111`` review request ID from the first command.

#. Implement a new test:

   * Create the .c template and update the file with test suite scenarios:

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --implement ...

     After this command the scenario from test suite scenarios will be moved to the newly created .c template.

   * Push the file with test suite scenarios;

   * Implement the new test and publish it on ReviewBoard.

   * Push the test after review.

   Example:

   * The following command creates/updates the test file "third.c".

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py -i -s OKTL-0000999-example_ts.yml -t example-ts -a "TE Maintainers <te-maint@oktetlabs.ru>" --scenario-in-test 10 -f -r example_ts.first_part.third

     Where:

     * ``-i`` the short form of ``--implement``;

     * ``--scenario-in-test 10`` locate the scenario in a code and expand steps in the scenario up to 10 nesting levels;

     * ``-f`` force rewrite if a test file already exists;

     * ``-r example_ts.first_part.third`` the test name to implementation.

#. Get the script documentation:

   * The list of possible operation modes of the script can be displayed using this line:

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --help

   * Each mode has the similar way to display the documentation:

     .. code-block:: none

     	$TE_BASE/scripts/scenarios.py --postreview --help





.. _doxid-group__te__scenarios_1te_scenarios_dependencies:

Dependencies of scenarios.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* python3

* python3-requests

* python3-yaml (>= 3.11)

To install this packages on Debian derivatives you can run this line:

.. code-block:: none

	apt-get install python3 python3-requests python3-yaml

