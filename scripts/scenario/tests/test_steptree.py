# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the nested scenario tree builder."""

from aststeps import Cond, SourceStep
from steptree import Node, build, cond_label


def step(kind: str, text: str, conds: list[Cond] | None = None) -> SourceStep:
    return SourceStep(kind=kind, line=0, text=text, func='main', conds=conds or [])


def shape(nodes: list[Node]) -> list:
    """Nested (kind, text-or-cond-desc, children) tuples for asserts."""
    return [
        (
            n.kind,
            n.cond.desc if n.cond is not None else n.text,
            shape(n.children),
        )
        for n in nodes
    ]


IF = Cond(kind='if', cond='x > 0', desc='if (x > 0)')
ELSE = Cond(kind='else', cond='x > 0', desc='!(x > 0)')
FOR = Cond(kind='for', cond='i < n', desc='for (i = 0; i < n; i++)', init='i = 0', incr='i++')


def test_flat_steps_and_substeps() -> None:
    tree = build(
        [
            step('STEP', 'One'),
            step('SUBSTEP', 'One detail'),
            step('SUBSTEP', 'More detail'),
            step('STEP', 'Two'),
        ]
    )
    assert shape(tree) == [
        ('STEP', 'One', [('SUBSTEP', 'One detail', []), ('SUBSTEP', 'More detail', [])]),
        ('STEP', 'Two', []),
    ]


def test_conditional_groups() -> None:
    tree = build(
        [
            step('STEP', 'Prepare'),
            step('STEP', 'On success', [IF]),
            step('STEP', 'Still on success', [IF]),
            step('STEP', 'On failure', [ELSE]),
        ]
    )
    assert shape(tree) == [
        ('STEP', 'Prepare', []),
        (
            'COND',
            'if (x > 0)',
            [('STEP', 'On success', []), ('STEP', 'Still on success', [])],
        ),
        ('COND', '!(x > 0)', [('STEP', 'On failure', [])]),
    ]


def test_loop_of_substeps_lifts_under_step() -> None:
    tree = build(
        [
            step('STEP', 'Do in a loop:'),
            step('SUBSTEP', 'Poke', [FOR]),
            step('SUBSTEP', 'Sleep', [FOR, IF]),
        ]
    )
    assert shape(tree) == [
        (
            'STEP',
            'Do in a loop:',
            [
                (
                    'COND',
                    'for (i = 0; i < n; i++)',
                    [
                        ('SUBSTEP', 'Poke', []),
                        ('COND', 'if (x > 0)', [('SUBSTEP', 'Sleep', [])]),
                    ],
                ),
            ],
        ),
    ]


def test_loop_of_steps_stays_sibling() -> None:
    tree = build(
        [
            step('STEP', 'Prepare'),
            step('STEP', 'Repeat the check', [FOR]),
        ]
    )
    assert shape(tree) == [
        ('STEP', 'Prepare', []),
        ('COND', 'for (i = 0; i < n; i++)', [('STEP', 'Repeat the check', [])]),
    ]


def test_push_pop_nesting() -> None:
    tree = build(
        [
            step('PUSH', 'Configure the device'),
            step('STEP', 'Set the MTU'),
            step('POP', ''),
            step('STEP', 'Start'),
        ]
    )
    assert shape(tree) == [
        ('PUSH', 'Configure the device', [('STEP', 'Set the MTU', [])]),
        ('STEP', 'Start', []),
    ]


def test_next_and_reset() -> None:
    tree = build(
        [
            step('PUSH', 'Phase one'),
            step('STEP', 'A'),
            step('NEXT', 'Phase two'),
            step('STEP', 'B'),
            step('RESET', ''),
            step('STEP', 'Tail'),
        ]
    )
    assert shape(tree) == [
        ('PUSH', 'Phase one', [('STEP', 'A', [])]),
        ('NEXT', 'Phase two', [('STEP', 'B', [])]),
        ('STEP', 'Tail', []),
    ]


def test_info_variants_skipped() -> None:
    tree = build(
        [
            step('STEP', 'Real'),
            step('PUSH_INFO', 'noise'),
            step('POP_INFO', ''),
        ]
    )
    assert shape(tree) == [('STEP', 'Real', [])]


def test_cond_inside_push() -> None:
    tree = build(
        [
            step('PUSH', 'Group'),
            step('STEP', 'Guarded', [IF]),
            step('POP', ''),
        ]
    )
    assert shape(tree) == [
        ('PUSH', 'Group', [('COND', 'if (x > 0)', [('STEP', 'Guarded', [])])]),
    ]


def test_cond_labels() -> None:
    assert cond_label(IF) == ('If ', 'x > 0', ':')
    assert cond_label(ELSE) == ('If not ', 'x > 0', ':')
    assert cond_label(FOR) == ('For each iteration (', 'i = 0; i < n; i++', '):')
    assert cond_label(Cond(kind='while', cond='left > 0', desc='while (left > 0)')) == (
        'While ',
        'left > 0',
        ':',
    )
    assert cond_label(Cond(kind='do', cond='busy', desc='do while (busy)')) == (
        'Do, repeating while ',
        'busy',
        ':',
    )
    assert cond_label(Cond(kind='switch', cond='mode', desc='switch (mode)')) == (
        'Depending on ',
        'mode',
        ':',
    )
    assert cond_label(Cond(kind='goto', cond='0', desc='if (0), reached by goto')) == (
        'Only on the error path:',
        '',
        '',
    )
