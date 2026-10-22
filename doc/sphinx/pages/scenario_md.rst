..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2026 OKTET Ltd.
  te-parent: te_ts

.. _te_scenario_md:

Test scenarios in markdown
==========================

A test scenario can be written as markdown before a line of C
exists. The ``scripts/scenario/scenario.py`` tool turns such a
description into a test stub with ``TEST_STEP()`` macros, and
later checks the implemented test against the description. The
markdown stays in the suite tree: it is the durable specification
of the test, readable by reviewers and by code generation tools
alike.

Files and naming
~~~~~~~~~~~~~~~~

Each package directory holds one ``package.md`` next to its
``package.xml``:

.. code-block:: none

    ts/usecases/
        package.xml
        package.md          the package scenario document
        set_mtu.md          optional per-test file
        set_mtu.c           the generated stub, then the test

The package name is the directory name; the test name is the
heading. Test ``set_mtu`` described in ``ts/usecases/package.md``
generates ``ts/usecases/set_mtu.c`` with
``TE_TEST_NAME "usecases/set_mtu"``.

A test is described either inline in ``package.md`` or in its own
file referenced from it; the two forms are interchangeable and
can be mixed in one document. Each package directory is
independent: the tool discovers ``package.md`` files by walking
the suite tree.

The dialect
~~~~~~~~~~~

A complete package document:

.. code-block:: markdown

    # usecases: Reliability in normal use

    Free prose is allowed anywhere at the top level.

    ## set_mtu: Set MTU of IUT

    Make sure that when we set the MTU, the port receives packets
    with size of no more than MTU, and does not receive others.

    Type: use case

    Parameters:

    - `mtu`: MTU on IUT
    - `ethdev_state`: the state of the device
      - `TEST_ETHDEV_CONFIGURED`: configured, not started
      - `TEST_ETHDEV_STARTED`

    Steps:

    1. Initialize EAL and configure the port.
    2. Set `mtu` on the port in `ethdev_state`.

       The driver may round the value down to the nearest
       supported MTU; read it back and use the effective value.

       > impl: use tapi_cfg_base_if_set_mtu(), not a raw cfg set.

    3. Transmit and check.
       - Send a packet of size `mtu`.
       - Check it is received.

    ## Referenced tests

    - [link_up_down](link_up_down.md)

The rules:

* ``# <package>: <summary>`` opens the document; ``<package>``
  must equal the directory name.
* ``## <test>: <summary>`` opens an inline test; the name matches
  ``[a-z0-9_]+``. Any other second-level heading is prose.
* A top-level list item ``- [<test>](<file>.md)`` delegates the
  test to a per-test file: the same syntax, with
  ``# <test>: <summary>`` as its first heading. A test name may
  appear only once per package.
* Inside a test, the first paragraph is the ``@objective``; a
  ``Type:`` paragraph sets the ``@type``; any other paragraph
  before ``Steps:`` becomes a ``@note``. A blockquote there is
  implementor advice for the test as a whole, placed in the stub
  as a comment before the first step; it is not documented and
  the drift check does not compare it.
* ``Parameters:`` is followed by a list of
  :literal:`\`name\`: description` items; nested items list the
  values, optionally with a comment after a colon.
* ``Steps:`` is followed by the step list. Ordered and unordered
  markers are interchangeable at every depth.
* Both section markers may equally be written as a heading one
  level below the test heading, with or without the colon:
  ``### Steps`` in a package file, ``## Steps`` in a per-test
  file. The label and heading forms mean the same thing.
* ``## Description`` opens the package description; a
  ``Group: <id>`` paragraph names the documentation group of the
  package. Both are described below.

Package description
~~~~~~~~~~~~~~~~~~~

Prose at the top level of a package document is for its readers
only. What the package documentation should say goes under
``## Description``:

.. code-block:: markdown

    # usecases: Reliability in normal use

    Planning notes for the reviewers stay out of the documentation.

    ## Description

    The tests check the port in the states a user application
    leaves it in.

    ### Common rules

    Every test reads the effective MTU back before using it.

    ## set_mtu: Set MTU of IUT

The rules:

* The section is optional, appears at most once, and comes before
  the first test - inline or referenced.
* It runs up to the next second-level heading. Deeper headings
  structure it; a test reference inside it is an error.
* The documentation group of the package is the package name,
  which is also the group the generated tests join with
  ``@ingroup``. A ``Group: <id>`` paragraph before the first
  second-level heading names another group, for instance the one
  of the suite root.

The ``scripts/md2dox`` doxygen input filter adds the description
to the documentation of that group, on the page that lists the
package's tests:

.. code-block:: none

    FILE_PATTERNS     = ... package.md
    FILTER_PATTERNS   = ... *.md="${TE_BASE}/scripts/md2dox"
    EXTENSION_MAPPING = ... md=C

The filter pattern needs the wildcard: doxygen silently skips a
filter whose pattern does not match the full path. Listing only
``package.md`` in ``FILE_PATTERNS`` keeps a ``README.md`` and the
per-test files out.

The group is still defined where it was, in a ``package.dox`` for
instance: the filter only adds to it with ``@addtogroup``. A build
without the filter loses the description, never the group or its
tests. Within the description:

* the markdown stays markdown, doxygen renders it;
* ``###`` to ``######`` headings become bold lines with a
  ``<group>-<title>`` anchor, so ``### Common rules`` of package
  ``dcb`` can be referred to as ``@ref dcb-common-rules``. They
  are not made doxygen sections, which would add entries to the
  navigation sidebar of the page;
* ``> impl:`` blockquotes are left out: they advise the
  implementor, as they do under a step.

A test the document describes but the package does not implement
yet - there is no ``<test>.c`` - gets a page as well, built from
its section. It sits in the group with the implemented tests and
opens with a warning that there is no test to run. Once the source
exists, the page comes from it instead, under the same name.

Steps, substeps and notes
~~~~~~~~~~~~~~~~~~~~~~~~~

The first paragraph of a step item is the step text. List depth
selects the macro: depth 1 is ``TEST_STEP()``, depth 2 is
``TEST_SUBSTEP()``, depth 3 and deeper use the
``TEST_STEP_PUSH()`` / ``TEST_STEP_NEXT()`` /
``TEST_STEP_POP()`` stack. Nesting deeper than three levels
produces a warning: such a step usually wants to be its own test.

Everything else inside a step item transfers into the stub as a
comment above the macro:

* a continuation paragraph is extra description: part of the
  specification that did not fit the one-liner;
* a blockquote (optionally tagged ``impl:``) is implementor
  advice: which TAPI to call, known pitfalls, where a reference
  implementation lives. It is prefixed ``IMPL:`` in the generated
  comment. Once the advice is consumed, the implementor is
  expected to delete the ``IMPL:`` text; the unprefixed part
  stays as documentation of the step.

Inline code spans
~~~~~~~~~~~~~~~~~

Backticked tokens in step text and parameter descriptions are
resolved when the stub is generated:

============================  ================================
Token                         Result
============================  ================================
a declared parameter name     ``@p name``
a listed value or ALL_CAPS    ``@c TOKEN``
anything else                 the bare token
============================  ================================

Text outside backticks is copied verbatim, so a raw ``@p`` or
``@c`` reference (for example to an environment interface such as
``iut_port`` that is not a test parameter) passes through
unchanged.

The tool
~~~~~~~~

Run from the test suite; the test root is ``-t`` (or ``-t/ts``
when that subdirectory exists, as in DPDK suites):

.. code-block:: shell

    # Write ts/usecases/set_mtu.c from the markdown
    ${TE_BASE}/scripts/scenario/scenario.py generate usecases/set_mtu

    # Compare every implemented test against its markdown;
    # non-zero exit on drift, so it can gate CI
    ${TE_BASE}/scripts/scenario/scenario.py check

    # What is described but not yet implemented
    ${TE_BASE}/scripts/scenario/scenario.py list --pending

Parameter descriptions from the ``Parameters:`` list are emitted
as ``TEST_PARAM_DOC()`` lines in front of the corresponding
getters, not as header ``@param`` entries; the doxygen filter
regenerates the ``@param`` block from the source.

The check compares the step texts, their order and their nesting
against the markdown; comments are never compared. A ``.c`` file
not described in any markdown is reported only with ``--strict``,
so a suite can adopt scenario documents incrementally.

The check also reports two mistakes that leave a page out of the
documentation tree:

* an implemented test whose header does not start with
  ``@defgroup <package>-<test>`` and ``@ingroup <group>``, as the
  generated stub does; a ``@page`` header, for instance, gives the
  test a page that nothing links to;
* a package description whose group no ``@defgroup`` defines, in a
  ``.dox`` file or a ``package.xml``: the description joins its
  group with ``@addtogroup``, which creates a new untitled group
  when the group does not exist.

Status
~~~~~~

The filesystem is the status: a test whose ``.c`` exists is
implemented, one without is pending. The markdown carries no
checkboxes or done-lists, and nothing needs updating when a test
is implemented - the drift check takes over from there.
