# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the markdown emitter."""

import textwrap
from pathlib import Path

import pytest

import aststeps
from aststeps import Cond, SourceStep
from cstep import compare
from emit_md import cond_note, emit_test_md, step_items, uninline
from mdparse import parse_package


def test_uninline() -> None:
    assert uninline('Set @p mode to @c TRUE.') == 'Set `mode` to `TRUE`.'
    assert uninline('No references here.') == 'No references here.'


def test_cond_notes() -> None:
    assert cond_note(Cond(kind='if', cond='x > 0', desc='if (x > 0)')) == 'Only when `x > 0`.'
    assert (
        cond_note(Cond(kind='else', cond='x > 0', desc='!(x > 0)')) == 'Only when not `x > 0`.'
    )
    assert (
        cond_note(Cond(kind='for', cond='i < n', desc='for (i = 0; i < n; i++)'))
        == 'For each iteration (`i = 0; i < n; i++`).'
    )
    assert (
        cond_note(Cond(kind='goto', cond='0', desc='if (0), ...')) == 'Only on the error path.'
    )


def step(kind: str, text: str, conds: list[Cond] | None = None) -> SourceStep:
    return SourceStep(kind=kind, line=0, text=text, func='main', conds=conds or [])


def test_step_items_depths() -> None:
    items = step_items(
        [
            step('STEP', 'One'),
            step('SUBSTEP', 'Detail'),
            step('PUSH', 'Group'),
            step('STEP', 'Inside'),
            step('NEXT', 'Second group'),
            step('POP', ''),
            step('STEP', 'Tail', [Cond(kind='if', cond='x', desc='if (x)')]),
            step('PUSH_INFO', 'noise'),
        ]
    )
    assert [(d, t) for d, t, _ in items] == [
        (1, 'One'),
        (2, 'Detail'),
        (3, 'Group'),
        (1, 'Inside'),
        (1, 'Second group'),
        (1, 'Tail'),
    ]
    assert items[-1][2] == ['Only when `x`.']


ROUNDTRIP_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup pkg-demo Round trip demo
 * @ingroup pkg
 * @{
 *
 * @objective Check the round trip.
 *
 * @param mode  The mode
 *
 * @par Scenario:
 */

int
main(void)
{
    int mode;

    TEST_GET_INT_PARAM(mode);
    TEST_STEP("Prepare the @p mode device");
    if (mode > 0)
    {
        TEST_SUBSTEP("Check the fast path");
    }
    for (mode = 0; mode < 3; mode++)
    {
        TEST_SUBSTEP("Poke it");
    }
    TEST_STEP("Check the result is @c OK");
    return 0;
}

/** @} */
"""


@pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')
def test_round_trip(tmp_path: Path) -> None:
    """Emitted markdown must parse and drift-check clean against its source."""
    src = tmp_path / 'demo.c'
    src.write_text(textwrap.dedent(ROUNDTRIP_SRC), encoding='utf-8')
    stubs = [f'-D{name}(...)=(void)0' for name in aststeps.STEP_MACROS]
    stubs.append('-DTEST_GET_INT_PARAM(...)=(void)0')
    steps = aststeps.extract(src, extra_args=stubs)

    md = emit_test_md('demo', src.read_text(encoding='utf-8'), step_items(steps))
    assert md.startswith('## demo: Round trip demo')
    assert '- `mode`: The mode' in md
    assert '1. Prepare the `mode` device' in md
    assert 'Only when `mode > 0`.' in md
    assert 'For each iteration (`mode = 0; mode < 3; mode++`).' in md

    (tmp_path / 'pkg').mkdir()
    pkg_md = tmp_path / 'pkg' / 'package.md'
    pkg_md.write_text(f'# pkg: Demo package\n\n{md}', encoding='utf-8')
    pkg = parse_package(pkg_md)
    assert len(pkg.tests) == 1
    findings = compare(pkg.tests[0], src.read_text(encoding='utf-8'), 'pkg/demo')
    assert findings == []
