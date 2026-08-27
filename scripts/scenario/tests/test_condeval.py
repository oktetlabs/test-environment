# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the tri-state C condition evaluator."""

from condeval import evaluate, trip_count, value

ENV = {'iters': 100, 'TEST_IF_UP': 1, 'TEST_IF_UP_DOWN': 2, 'if_status': 1, 'x': 0}


def test_decided() -> None:
    assert evaluate('if_status == TEST_IF_UP', ENV) is True
    assert evaluate('if_status == TEST_IF_UP || if_status == TEST_IF_UP_DOWN', ENV) is True
    assert evaluate('iters > 1000', ENV) is False
    assert evaluate('!(x)', ENV) is True
    assert evaluate('iters % 2 == 0', ENV) is True
    assert evaluate('x ? 1 : 0', ENV) is False
    assert evaluate('(iters - 1) * 2 >= 198', ENV) is True
    assert evaluate('iters != 0 && if_status <= 2', ENV) is True


def test_undecided() -> None:
    assert evaluate('unknown > 3', ENV) is None
    assert evaluate('strcmp(a, b) == 0', ENV) is None
    assert evaluate('p->len != 0', ENV) is None
    assert evaluate('buf[0] == 1', ENV) is None
    assert evaluate('rc != -TE_RC(TE_TAPI, TE_ENOENT)', {}) is None
    assert evaluate('iters / x > 1', ENV) is None  # division by zero
    assert evaluate("c == 'a'", ENV) is None
    assert evaluate('', ENV) is None


def test_tristate_shortcuts() -> None:
    assert evaluate('x && unknown', ENV) is False
    assert evaluate('unknown || 1', ENV) is True
    assert evaluate('unknown && 1', ENV) is None
    assert evaluate('unknown || x', ENV) is None
    assert evaluate('!unknown', ENV) is None


def test_value() -> None:
    assert value('iters - 1', ENV) == 99
    assert value('0x10', {}) == 16
    assert value('7 / 2', {}) == 3  # C integer division
    assert value('-7 / 2', {}) == -3  # truncates toward zero
    assert value('unknown', ENV) is None
    assert value('0x1F', {}) == 31
    assert value('0xff', {}) == 255
    assert value('0x10UL', {}) == 16


def test_prefix_operators() -> None:
    assert evaluate('flags == 0x1F', {'flags': 1}) is False
    assert evaluate('--x == 0', {'x': 1}) is None
    assert evaluate('++n', {'n': 1}) is None


def test_trip_count() -> None:
    env = {'iters': 100}
    assert trip_count('i = 0', 'i < iters', 'i++', env) == ('i', 0, 100)
    assert trip_count('unsigned int i = 2', 'i <= iters', '++i', env) == ('i', 2, 99)
    assert trip_count('i = 0', 'i < 4', 'i += 1', env) == ('i', 0, 4)
    assert trip_count('i = 5', 'i < 3', 'i++', env) == ('i', 5, 0)
    assert trip_count('i = 0', 'i < unknown', 'i++', env) is None
    assert trip_count(None, 'running', None, env) is None
    assert trip_count('i = 0', 'i < iters', 'i--', env) is None
    assert trip_count('i = 0', 'j < iters', 'i++', env) is None
