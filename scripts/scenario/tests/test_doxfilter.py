# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the AST-based doxygen input filter."""

import textwrap
from pathlib import Path

import pytest

import aststeps
from aststeps import Cond
from doxfilter import filter_text, render_dox
from steptree import Node

HEADER_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup demo-check Demo check
 * @ingroup demo
 * @{
 *
 * @objective Check something
 *
 * @param mode  The mode
 *
 * @author A U Thor <author@example.com>
 *
 * @par Scenario:
 */

/** A member doc that must not survive filtering */
int
main(void)
{
    return 0;
}

/** @} */
"""


def test_render_dox() -> None:
    tree = [
        Node(kind='STEP', text='Prepare'),
        Node(
            kind='COND',
            cond=Cond(kind='if', cond='mode > 0', desc='if (mode > 0)'),
            children=[
                Node(kind='STEP', text='Extra check'),
                Node(
                    kind='COND',
                    cond=Cond(kind='goto', cond='0', desc='if (0), reached by goto'),
                    children=[Node(kind='SUBSTEP', text='Recover')],
                ),
            ],
        ),
    ]
    assert render_dox(tree) == [
        '   -# Prepare',
        '   -# If `mode > 0`:',
        '      - Extra check',
        '      - Only on the error path:',
        '         - Recover',
    ]


FILTER_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup demo-check Demo check
 * @ingroup demo
 * @{
 *
 * @objective Check something
 *
 * @param mode  The mode
 *
 * @author A U Thor <author@example.com>
 *
 * @par Scenario:
 */

int
main(void)
{
    int mode;

    TEST_STEP("Prepare");
    if (mode > 0)
    {
        TEST_STEP("Extra check");
    }
    TEST_STEP("Run the loop:");
    for (mode = 0; mode < 3; mode++)
    {
        TEST_SUBSTEP("Iterate once");
    }
    return 0;
}

/** @} */
"""


@pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')
def test_filter_text(tmp_path: Path) -> None:
    src = tmp_path / 'demo.c'
    src.write_text(textwrap.dedent(FILTER_SRC), encoding='utf-8')
    out = filter_text(src)
    lines = out.splitlines()

    assert '/** @defgroup demo-check Demo check' in lines
    # The author block moves to the bottom of the comment, after
    # the scenario.
    author = lines.index(' * @author A U Thor <author@example.com>')
    assert lines.index('         - Iterate once') < author < lines.index(' */')
    assert lines.count(' * @par Scenario:') == 1
    assert '   -# Prepare' in lines
    assert '   -# If `mode > 0`:' in lines
    assert '      - Extra check' in lines
    assert '   -# Run the loop:' in lines
    assert '      - For each iteration (`mode = 0; mode < 3; mode++`):' in lines
    assert '         - Iterate once' in lines
    assert lines[-2] == ' */'
    assert lines[-1] == '/** @} */'
    # The scenario sits inside the header comment.
    assert lines.index('   -# Prepare') < lines.index(' */')


@pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')
def test_filter_text_adds_scenario_marker(tmp_path: Path) -> None:
    src = tmp_path / 'demo.c'
    bare = FILTER_SRC.replace(' * @par Scenario:\n', '')
    src.write_text(textwrap.dedent(bare), encoding='utf-8')
    out = filter_text(src)
    assert ' * @par Scenario:' in out.splitlines()


@pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')
def test_filter_text_no_steps(tmp_path: Path) -> None:
    src = tmp_path / 'demo.c'
    src.write_text(textwrap.dedent(HEADER_SRC), encoding='utf-8')
    out = filter_text(src)
    lines = out.splitlines()
    assert '/** @defgroup demo-check Demo check' in lines
    assert not any(line.lstrip().startswith(('-#', '- ')) for line in lines)
    assert lines[-1] == '/** @} */'
