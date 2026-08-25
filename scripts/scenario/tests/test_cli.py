# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""End-to-end tests for the scenario.py CLI."""

import textwrap
from pathlib import Path

import pytest

from scenario import main

PKG = """\
# usecases: Basic

## one: First test

Objective one.

Steps:

1. Do the thing.
2. Check the thing.
"""


def make_suite(tmp_path: Path) -> Path:
    root = tmp_path / 'ts'
    (root / 'usecases').mkdir(parents=True)
    (root / 'usecases' / 'package.md').write_text(textwrap.dedent(PKG), encoding='utf-8')
    return tmp_path


def test_generate_and_check(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    suite = make_suite(tmp_path)
    rc = main(
        [
            'generate',
            'usecases/one',
            '-t',
            str(suite),
            '--author',
            'A <a@b.c>',
        ]
    )
    assert rc == 0
    c = suite / 'ts' / 'usecases' / 'one.c'
    assert c.exists()
    text = c.read_text(encoding='utf-8')
    assert '#define TE_TEST_NAME "usecases/one"' in text
    assert 'TEST_STEP("Do the thing.");' in text

    rc = main(['check', '-t', str(suite)])
    assert rc == 0

    drifted = text.replace('Do the thing.', 'Do another thing.')
    c.write_text(drifted, encoding='utf-8')
    rc = main(['check', '-t', str(suite)])
    assert rc == 1
    assert 'text differs' in capsys.readouterr().out


def test_generate_refuses_overwrite(tmp_path: Path) -> None:
    suite = make_suite(tmp_path)
    args = [
        'generate',
        'usecases/one',
        '-t',
        str(suite),
        '--author',
        'A <a@b.c>',
    ]
    assert main(args) == 0
    assert main(args) == 1
    assert main([*args, '--force']) == 0


def test_list(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    suite = make_suite(tmp_path)
    assert main(['list', '-t', str(suite)]) == 0
    out = capsys.readouterr().out
    assert 'usecases/one pending' in out
    main(
        [
            'generate',
            'usecases/one',
            '-t',
            str(suite),
            '--author',
            'A <a@b.c>',
        ]
    )
    main(['list', '-t', str(suite)])
    out = capsys.readouterr().out
    assert 'usecases/one implemented' in out


def test_check_strict_uncovered(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    suite = make_suite(tmp_path)
    (suite / 'ts' / 'usecases' / 'stray.c').write_text(
        'int main(void) { return 0; }\n', encoding='utf-8'
    )
    assert main(['check', '-t', str(suite)]) == 0
    assert main(['check', '--strict', '-t', str(suite)]) == 1
    assert 'stray.c' in capsys.readouterr().out


def test_unknown_test(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    suite = make_suite(tmp_path)
    rc = main(
        [
            'generate',
            'usecases/nope',
            '-t',
            str(suite),
            '--author',
            'A <a@b.c>',
        ]
    )
    assert rc == 1
    assert 'one' in capsys.readouterr().err
