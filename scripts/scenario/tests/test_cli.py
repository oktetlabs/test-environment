# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""End-to-end tests for the scenario.py CLI."""

import json
import textwrap
from pathlib import Path

import pytest

import aststeps
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


STEPS_SRC = """\
#define TEST_STEP(...) (void)0
#define TEST_SUBSTEP(...) (void)0
#define TEST_GET_INT_PARAM(x_) (void)0
#define TEST_GET_BOOL_PARAM(x_) (void)0

int
main(void)
{
    int iters;
    int verbose;

    TEST_GET_INT_PARAM(iters);
    TEST_GET_BOOL_PARAM(verbose);
    TEST_STEP("Prepare");
    if (verbose)
    {
        TEST_STEP("Dump the state");
    }
    if (iters > 10)
    {
        TEST_STEP("Warm up");
    }
    return 0;
}
"""


def make_steps_suite(tmp_path: Path) -> Path:
    src = tmp_path / 'demo.c'
    src.write_text(STEPS_SRC, encoding='utf-8')
    db = tmp_path / 'compile_commands.json'
    db.write_text(
        json.dumps([{'directory': str(tmp_path), 'file': 'demo.c', 'command': 'cc -c demo.c'}]),
        encoding='utf-8',
    )
    return src


needs_clang = pytest.mark.skipif(not aststeps.HAVE_CLANG, reason='libclang not installed')


@needs_clang
def test_steps_param(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    src = make_steps_suite(tmp_path)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db])
    assert rc == 0
    out = capsys.readouterr().out
    assert '[if (verbose)] Dump the state' in out
    assert '[if (iters > 10)] Warm up' in out

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=100'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'STEP\tWarm up' in out  # decided true: annotation gone
    assert '[if (verbose)] Dump the state' in out  # still undecided
    assert 'not taken' not in out

    rc = main(
        [
            'steps',
            str(src),
            '--compile-db',
            db,
            '--param',
            'iters=5',
            '--param',
            'verbose=FALSE',
        ]
    )
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Warm up' not in out
    assert 'Dump the state' not in out
    assert '2 step(s) not taken with these parameters' in out

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=5', '--show-skipped'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'SKIP\tSTEP\t[if (iters > 10)] Warm up' in out
