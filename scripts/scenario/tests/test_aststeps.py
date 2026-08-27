# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the libclang source-mode step extractor."""

import textwrap
from pathlib import Path

import pytest

import aststeps
from aststeps import extract

pytestmark = pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')

MACRO_DEFS = [
    '-DTEST_STEP(...)=(void)0',
    '-DTEST_SUBSTEP(...)=(void)0',
    '-DTEST_STEP_PUSH(...)=(void)0',
    '-DTEST_STEP_POP(...)=(void)0',
    '-DFOO(...)=1',
    '-DTEST_GET_INT_PARAM(...)=(void)0',
    '-DTEST_GET_BOOL_PARAM(...)=(void)0',
    '-DTEST_GET_ENUM_PARAM(...)=(void)0',
    '-DTEST_GET_STRING_PARAM(...)=(void)0',
]

FIXTURE = """\
int must_fail;
int i, n;

static void
helper(void)
{
    TEST_STEP("From a helper");
}

int
main(void)
{
    TEST_STEP("Initialize the device");
    if (must_fail)
    {
        TEST_STEP("Check the failure is reported");
    }
    else
    {
        TEST_STEP("Check the result");
    }
    for (i = 0; i < n; i++)
    {
        TEST_SUBSTEP("Process item");
        if (i == 0)
            TEST_STEP_PUSH("First item detail");
    }
    if (n == -FOO(1, 2))
    {
        TEST_STEP("Check the negative macro gate");
    }
    if (0)
    {
recover:
        TEST_STEP("Recover after failure");
    }
    do
    {
        TEST_SUBSTEP("Poll once");
    } while (n < 3);
    helper();
    return 0;
}
"""


def write_fixture(tmp_path: Path) -> Path:
    src = tmp_path / 'fix.c'
    src.write_text(textwrap.dedent(FIXTURE), encoding='utf-8')
    return src


def test_conditions(tmp_path: Path) -> None:
    steps = extract(write_fixture(tmp_path), extra_args=MACRO_DEFS)
    by_text = {s.text: s for s in steps}

    assert by_text['Initialize the device'].conds == []
    assert by_text['Initialize the device'].func == 'main'

    assert [c.desc for c in by_text['Check the failure is reported'].conds] == [
        'if (must_fail)'
    ]
    assert [c.desc for c in by_text['Check the result'].conds] == ['!(must_fail)']

    loop = by_text['Process item']
    assert loop.kind == 'SUBSTEP'
    assert [c.desc for c in loop.conds] == ['for (i = 0; i < n; i++)']

    loop_cond = loop.conds[0]
    assert (loop_cond.kind, loop_cond.cond, loop_cond.init, loop_cond.incr) == (
        'for',
        'i < n',
        'i = 0',
        'i++',
    )

    nested = by_text['First item detail']
    assert nested.kind == 'PUSH'
    assert [c.desc for c in nested.conds] == ['for (i = 0; i < n; i++)', 'if (i == 0)']

    assert by_text['From a helper'].func == 'helper'
    assert by_text['From a helper'].conds == []

    # Condition text comes from the source verbatim: unary minus and
    # macro call spelling survive.
    gate = by_text['Check the negative macro gate']
    assert [c.desc for c in gate.conds] == ['if (n == -FOO(1, 2))']

    pad = by_text['Recover after failure']
    assert [c.desc for c in pad.conds] == ['if (0), reached by goto']
    assert by_text['Check the result'].conds[0].kind == 'else'
    assert by_text['Recover after failure'].conds[0].kind == 'goto'

    # clang orders do-while children [body, cond], unlike for/while.
    poll = by_text['Poll once']
    assert [c.desc for c in poll.conds] == ['do while (n < 3)']
    assert by_text['Poll once'].conds[0].cond == 'n < 3'


def test_source_order(tmp_path: Path) -> None:
    steps = extract(write_fixture(tmp_path), extra_args=MACRO_DEFS)
    assert [s.text for s in steps] == [
        'From a helper',
        'Initialize the device',
        'Check the failure is reported',
        'Check the result',
        'Process item',
        'First item detail',
        'Check the negative macro gate',
        'Recover after failure',
        'Poll once',
    ]


PARAM_FIXTURE = """\
enum {
    MODE_A,
    MODE_B = 5,
};

#define MODE_MAP \\
    { "a", MODE_A },  \\
    { "b", MODE_B }

#define LIMIT 42

int
main(void)
{
    int iters;
    int mode;
    int on;
    const char *name;

    TEST_GET_INT_PARAM(iters);
    TEST_GET_ENUM_PARAM(mode, MODE_MAP);
    TEST_GET_BOOL_PARAM(on);
    TEST_GET_STRING_PARAM(name);
    return 0;
}
"""


def test_bindings(tmp_path: Path) -> None:
    src = tmp_path / 'params.c'
    src.write_text(PARAM_FIXTURE, encoding='utf-8')
    info = aststeps.analyze(src, extra_args=MACRO_DEFS)

    assert set(info.bindings) == {'iters', 'mode', 'on', 'name'}
    assert info.bindings['iters'].kind == 'int'
    assert info.bindings['name'].kind == 'string'
    assert info.bindings['on'].kind == 'bool'
    assert info.bindings['on'].mapping == {'TRUE': 1, 'FALSE': 0}

    mode = info.bindings['mode']
    assert mode.kind == 'enum'
    assert mode.map_macros == ['MODE_MAP']
    assert mode.mapping is None  # resolution comes in a later task

    assert info.enums == {'MODE_A': 0, 'MODE_B': 5}
    assert info.macros['LIMIT'] == 42
    assert 'MODE_MAP' in info.macro_tokens


def test_extract_still_returns_steps(tmp_path: Path) -> None:
    steps = extract(write_fixture(tmp_path), extra_args=MACRO_DEFS)
    assert steps and steps[0].text == 'From a helper'  # noqa: PT018 - single condition read
