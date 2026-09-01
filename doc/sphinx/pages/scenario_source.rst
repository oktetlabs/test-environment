..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2026 OKTET Ltd.
  te-parent: te_ts

.. _te_scenario_source:

Test scenarios from the sources
===============================

Once a test is implemented, its scenario lives in the
``TEST_STEP()`` macros of the ``.c`` file. The
``scripts/scenario/scenario.py`` tool reads it back from there:
it parses the source with libclang, so unlike a text scan it
knows which condition, loop, or step group encloses every step.
The listing serves reviewers, documentation generation, and any
tool that wants the scenario without running the test. Scenarios
written before the code exists are covered by
:ref:`te_scenario_md`.

Listing the steps
~~~~~~~~~~~~~~~~~

``scenario.py steps <test.c>`` emits the test's section in the
markdown dialect: summary, objective, and parameters recovered
from the doxygen header comment, then every
``TEST_STEP()``/``TEST_SUBSTEP()`` (and the
``TEST_STEP_PUSH``/``NEXT``/``POP`` stack) as the dialect step
list. The ``if``/``else``/loop/``switch`` constructs enclosing a
step become notes under it. The section is built to round-trip:
pasted into a ``package.md`` it passes the drift check as is,
which is how an existing suite bootstraps onto the markdown
workflow. This needs the optional ``libclang`` pip package and a
compiled ``compile_commands.json`` for the suite - found
automatically by walking up from the source for a ``build/``
directory, or given explicitly with ``--compile-db``.

A trimmed fragment and its section:

.. code-block:: c

    TEST_STEP("Load the driver");
    if (if_status == IF_UP)
    {
        TEST_SUBSTEP("Bring the interface up");
    }
    for (i = 0; i < iters; i++)
    {
        TEST_SUBSTEP("Unload and reload the driver");
    }
    TEST_STEP("Check the driver is loaded");

.. code-block:: markdown

    ## driver_unload: Driver unload stress test

    Check that the driver survives an unload loop.

    Parameters:

    - `iters`: How many times to reload
    - `if_status`: Interface state during the loop

    Steps:

    1. Load the driver.

       - Bring the interface up.

         Only when `if_status == IF_UP`.

       - Unload and reload the driver.

         For each iteration (`i = 0; i < iters; i++`).

    2. Check the driver is loaded.

``--flat`` prints the raw listing instead, one line per step:
``STEP`` or ``SUBSTEP``, a tab, the enclosing conditions in the
order they nest, and the step text - handy for grepping:

.. code-block:: none

    $ scenario.py steps ts/driver/driver_unload.c --flat
    STEP	Load the driver
    SUBSTEP	[if (if_status == IF_UP)] Bring the interface up
    SUBSTEP	[for (i = 0; i < iters; i++)] Unload and reload the driver
    STEP	Check the driver is loaded

Evaluating parameters
~~~~~~~~~~~~~~~~~~~~~

``--param NAME=VALUE`` (repeatable) binds a test parameter - one
read via a ``TEST_GET_*_PARAM()`` call - and evaluates every
condition it appears in.

How a value binds:

- a plain number binds as is (any C base);
- a bool parameter binds through ``TRUE``/``FALSE``, any case;
- an enum parameter, declared as ``TEST_GET_ENUM_PARAM(name, MAP)``,
  resolves through its mapping macro: ``--param if_status=up``
  decides ``if (if_status == IF_UP)`` the same as the numeric
  constant would, provided ``MAP`` expands to
  ``{ "up", IF_UP }, ...`` pairs the tool can read;
- anything else leaves the parameter unbound, and its conditions
  stay annotated as before.

What a decided condition does to the listing:

- decided true: the annotation disappears, the step stays;
- decided false: the whole step is dropped; with ``--flat`` the
  dropped steps fold into a trailing
  ``N step(s) not taken with these parameters`` line, or are
  printed with a ``SKIP`` tab prefix under ``--show-skipped``;
- a ``for`` loop whose trip count the bound parameters pin down is
  not unrolled: it prints once, annotated ``repeats N times``
  (dropped entirely for a zero count, unannotated for a one-shot
  loop).

What stays undecided on purpose:

- conditions on runtime state, or on identifiers the parameters do
  not bind;
- ``switch`` and the ``if (0)`` error-path landing pad;
- a loop whose repeat count the parameters do not pin down - the
  tool evaluates conditions, it does not interpret loop bodies.

Doxygen filter
~~~~~~~~~~~~~~

Test suites that publish doxygen documentation can feed it through
the same extractor instead of the classic ``c2dox`` awk filter:

.. code-block:: none

    FILTER_PATTERNS = *.c="${TE_BASE}/scripts/c2dox_ast"

The filter keeps the test's doxygen header comment and injects
the scenario after ``@par Scenario:``, moving ``@author`` to the
bottom of the page (which ``c2dox`` meant to do, but lost the
line instead). The scenario is a nested list: steps
under the condition or loop that guards them ("If ``cond``:",
"For each iteration (...):"), substeps under their step,
``TEST_STEP_PUSH`` groups under their heading. Step strings are
decoded properly - concatenation, escapes - and ``@p``/``@c``
references stay for doxygen to resolve.

The filter needs ``python3`` with the ``libclang`` package on
``PATH`` at documentation build time; without them it falls back
to ``c2dox`` with a warning on stderr, so the build still
produces pages, just flat ones.

Inline parameter documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test parameters are documented next to the code that reads them
with ``TEST_PARAM_DOC()``. The macro expands to nothing; each
argument after the parameter name is one line of the description,
so a value list needs no ``\n`` escapes, and adjacent string
literals within one argument continue the same line:

.. code-block:: c

    TEST_PARAM_DOC(mode,
        "Operation mode:",
        "- fast: skip checks",
        "- safe: full validation");
    TEST_GET_ENUM_PARAM(mode, MODE_MAPPING_LIST);

The doxygen filter turns these strings into the ``@param`` entries
of the test page. When the header comment still carries a
hand-written ``@param`` of the same name, the inline text wins;
header-only entries survive in front of the generated ones. The
end state of a migrated test has no ``@param`` in the header at
all.

For a parameter read through an enum getter - a direct
``TEST_GET_ENUM_PARAM()`` or a wrapper like
``TEST_GET_ETHDEV_STATE()`` - the filter appends the allowed value
names to the description automatically, taken from the getter's
mapping-list macro (harvested from the test source and the TAPI
headers under ``TE_BASE``). An explicit value list in the doc
suppresses the mechanical one: write your own bullets when the
values deserve explanations.

``scenario.py check`` enforces coverage: a parameter read with no
documentation, a doc without a matching read, a duplicated doc,
and description drift against the ``package.md`` parameter list
are findings. By default a header ``@param`` still counts as
documentation; ``--strict`` requires the inline form. The ``env``
parameter is the one exception to staleness checking, because its
read is hidden inside ``TEST_START``.

Parameter docs and scenario in the log
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The same ``TEST_PARAM_DOC()`` text and ``TEST_STEP()`` scenario
that feed the doxygen filter also reach the run log, without
running the tool by hand. At build time
``engine/builder/te_tests_info.sh`` calls
``scripts/scenario/tests_info.py`` for every test source and folds
its output into the suite's ``tests-info.xml`` alongside the
existing ``<objective>``. When the optional ``libclang`` package is
importable the scenario is the same control-flow-annotated tree the
doxygen filter renders - loop and branch headings such as ``For
each iteration (...)`` and ``If `deep`:`` become steps one depth
above their bodies. Without libclang the tool needs only the
standard library and falls back to the flat textual extraction
(``cstep``), listing the step texts in source order. Either
way the texts carry no list markers: the consumer derives the
list markup from the ``depth`` that travels with every step:

.. code-block:: xml

    <test name="parameters" page="minimal_parameters">
      <objective>Test that various types of parameters are properly handled.</objective>
      <param name="unit_param">Value with a decimal unit prefix:
    - a plain number (no prefix)
    - k, M, G, T suffixes (powers of 1000)</param>
      <scenario>
        <step depth="1">Getting required parameters</step>
      </scenario>
    </test>

This needs ``python3`` on ``PATH`` at build time. Without it - or
if the extractor fails - the build still succeeds: it falls back
to the classic ``te_tests_info.awk`` scrape, which only ever
produced ``<objective>``, so the ``<param>``/``<scenario>``
entries are silently lost for that run. The build log says so:

.. code-block:: none

    tests-info: python tests-info generator unavailable or failed;
        parameter descriptions and scenarios are not extracted
        (install python3, e.g. apt install python3)

With python3 but no libclang, a milder note marks the flat
scenarios:

.. code-block:: none

    tests_info: scenarios lack control-flow annotations
        (pip install libclang to add them)

The Tester reads the extra fields back out of ``tests-info.xml``
and logs them on the test's ``test_start`` MI message, bumped to
version 2 for the addition: ``param_docs`` (parameter name to
description, only the parameters that have one) and ``scenario``
(the ordered ``{"depth": ..., "text": ...}`` steps), both omitted
rather than emitted empty when a test has neither:

.. code-block:: none

    {"type":"test_start","version":2,"msg":{...,
    "param_docs":{"unit_param":"Value with a decimal unit prefix:\n..."},
    "scenario":[{"depth":1,"text":"Getting required parameters"}, ...],
    "objective":"..."}}

Bublik meets this data in two places. The log view renders it
directly: the JSON log ``rgt-xml2json`` produces (what
``rgt-proc-raw-log --json`` and the log bundle serve) carries a
``description`` alongside each documented parameter's ``value``
and a ``scenario`` array next to ``objective`` in the same test
node, so the viewer can show them with the rest of the node's
metadata. The plain-text and HTML renderers (``rgt-xml2text``,
``rgt-xml2html``, ``rgt-xml2html-multi``) leave both out on
purpose, so the flat log is not cluttered with text that is
already in the source. Separately, for bublik's own database
import, ``rgt-bublik-json`` copies ``param_docs`` and ``scenario``
onto the test object straight from the ``test_start`` message,
again present only when the test has them.
