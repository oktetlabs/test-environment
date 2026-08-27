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
