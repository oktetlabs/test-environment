# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the scenario data model."""

from pathlib import Path

from model import (
    ScenarioError,
    Step,
    flatten_steps,
    normalize_ws,
    resolve_inline,
)


def test_error_str() -> None:
    e = ScenarioError(Path('a/package.md'), 7, 'bad heading')
    assert str(e) == 'a/package.md:7: bad heading'


def test_resolve_param() -> None:
    out = resolve_inline('Set `mtu` on `iut_port`.', ['mtu'], [])
    assert out == 'Set @p mtu on iut_port.'


def test_resolve_value_and_caps() -> None:
    out = resolve_inline(
        'In `TEST_ETHDEV_STARTED` use `stopped`.',
        [],
        ['stopped'],
    )
    assert out == 'In @c TEST_ETHDEV_STARTED use @c stopped.'


def test_resolve_other_stays_literal() -> None:
    assert resolve_inline('Call `foo_bar()`.', [], []) == 'Call foo_bar().'


def test_normalize_ws() -> None:
    assert normalize_ws('  a\n   b  c ') == 'a b c'


def test_flatten_steps() -> None:
    tree = [
        Step(text='one', line=1, sub=[Step(text='two', line=2)]),
        Step(text='three', line=3),
    ]
    flat = [(d, s.text) for d, s in flatten_steps(tree)]
    assert flat == [(1, 'one'), (2, 'two'), (1, 'three')]
