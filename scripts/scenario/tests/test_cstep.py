# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for C-side step extraction and drift comparison."""

from pathlib import Path

from cstep import compare, extract_steps
from model import Step, Test

C_TEXT = """
int
main(int argc, char *argv[])
{
    TEST_START;

    TEST_STEP("Initialize EAL and configure iut_port.");
    TEST_STEP("Set @p mtu on @p iut_port in "
              "@p ethdev_state.");
    TEST_SUBSTEP("Send %u bytes", len);
    TEST_STEP_PUSH("Poll the Rx queue.");
    TEST_STEP_NEXT("Check the counter.");
    TEST_STEP_POP("");

    TEST_SUCCESS;
cleanup:
    TEST_END;
}
"""


def test_extract() -> None:
    assert extract_steps(C_TEXT) == [
        (1, 'Initialize EAL and configure iut_port.'),
        (1, 'Set @p mtu on @p iut_port in @p ethdev_state.'),
        (2, 'Send %u bytes'),
        (3, 'Poll the Rx queue.'),
        (3, 'Check the counter.'),
    ]


def test_extract_escaped_quote() -> None:
    text = 'TEST_STEP("Say \\"hi\\" now.");'
    assert extract_steps(text) == [(1, 'Say "hi" now.')]


def make_md_test(steps: list[Step]) -> Test:
    return Test(
        name='t',
        summary='T',
        path=Path('p/package.md'),
        line=1,
        objective='O',
        steps=steps,
    )


def test_compare_in_sync() -> None:
    md = make_md_test(
        [
            Step(text='Do `a` thing.', line=1),
            Step(text='More.', line=2),
        ]
    )
    c = 'TEST_STEP("Do a thing.");\nTEST_STEP("More.");\n'
    assert compare(md, c, 'p/t') == []


def test_compare_text_drift() -> None:
    md = make_md_test([Step(text='Do a thing.', line=1)])
    c = 'TEST_STEP("Do another thing.");\n'
    findings = compare(md, c, 'p/t')
    assert len(findings) == 1
    assert 'text differs' in findings[0]


def test_compare_count_drift() -> None:
    md = make_md_test([Step(text='One.', line=1)])
    c = 'TEST_STEP("One.");\nTEST_STEP("Two.");\n'
    findings = compare(md, c, 'p/t')
    assert len(findings) == 1
    assert '1 steps in markdown, 2 in C' in findings[0]


def test_extract_escaped_backslash() -> None:
    text = 'TEST_STEP("Run ethtool \\\\--show-ring iut_if");'
    assert extract_steps(text) == [(1, 'Run ethtool \\--show-ring iut_if')]


def test_extract_unknown_escape() -> None:
    text = 'TEST_STEP("a\\-b");'
    assert extract_steps(text) == [(1, 'a-b')]


def test_extract_reset_restarts_depth() -> None:
    text = (
        'TEST_STEP("One");\n'
        'TEST_SUBSTEP("Two");\n'
        'TEST_STEP_PUSH("Three");\n'
        'TEST_STEP_RESET();\n'
        'TEST_STEP_PUSH("Four");\n'
    )
    assert extract_steps(text) == [
        (1, 'One'),
        (2, 'Two'),
        (3, 'Three'),
        (1, 'Four'),
    ]


def test_extract_info_variants_excluded() -> None:
    text = (
        'TEST_STEP("Visible");\n'
        'TEST_STEP_PUSH_INFO("Hidden detail");\n'
        'TEST_STEP_POP_INFO("");\n'
    )
    assert extract_steps(text) == [(1, 'Visible')]
