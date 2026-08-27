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

    assert by_text['Check the failure is reported'].conds == ['if (must_fail)']
    assert by_text['Check the result'].conds == ['!(must_fail)']

    loop = by_text['Process item']
    assert loop.kind == 'SUBSTEP'
    assert loop.conds == ['for (i = 0; i < n; i++)']

    nested = by_text['First item detail']
    assert nested.kind == 'PUSH'
    assert nested.conds == ['for (i = 0; i < n; i++)', 'if (i == 0)']

    assert by_text['From a helper'].func == 'helper'
    assert by_text['From a helper'].conds == []

    # Condition text comes from the source verbatim: unary minus and
    # macro call spelling survive.
    gate = by_text['Check the negative macro gate']
    assert gate.conds == ['if (n == -FOO(1, 2))']

    pad = by_text['Recover after failure']
    assert pad.conds == ['if (0), reached by goto']


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
    ]
