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
    int i;
    for (i = 0; i < iters; i++)
    {
        TEST_SUBSTEP("Poke the device");
        if (i > 0)
        {
            TEST_SUBSTEP("Settle");
        }
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

    rc = main(['steps', str(src), '--compile-db', db, '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert '[if (verbose)] Dump the state' in out
    assert '[if (iters > 10)] Warm up' in out

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=100', '--flat'])
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
            '--flat',
        ]
    )
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Warm up' not in out
    assert 'Dump the state' not in out
    assert '2 step(s) not taken with these parameters' in out

    rc = main(
        [
            'steps',
            str(src),
            '--compile-db',
            db,
            '--param',
            'iters=5',
            '--show-skipped',
            '--flat',
        ]
    )
    assert rc == 0
    out = capsys.readouterr().out
    assert 'SKIP\tSTEP\t[if (iters > 10)] Warm up' in out


def write_steps_src(tmp_path: Path, text: str) -> Path:
    src = tmp_path / 'demo.c'
    src.write_text(text, encoding='utf-8')
    db = tmp_path / 'compile_commands.json'
    db.write_text(
        json.dumps([{'directory': str(tmp_path), 'file': 'demo.c', 'command': 'cc -c demo.c'}]),
        encoding='utf-8',
    )
    return src


ENUM_MAP_SRC = """\
#define TEST_STEP(...) (void)0
#define TEST_GET_ENUM_PARAM(x_, map_) (void)0

enum {
    M_ONE = 7,
};

#define M_MAP { "1", M_ONE }

int
main(void)
{
    int mode;

    TEST_GET_ENUM_PARAM(mode, M_MAP);
    if (mode == 7)
    {
        TEST_STEP("Chosen");
    }
    return 0;
}
"""


@needs_clang
def test_steps_param_mapping_wins_over_numeric(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """A mapping key that looks numeric must resolve through the map.

    "mode" maps the string "1" to the enum constant M_ONE (7); with
    --param mode=1 the binding must resolve through the mapping, not
    be misread as the literal number 1.
    """
    src = write_steps_src(tmp_path, ENUM_MAP_SRC)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'mode=1', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'STEP\tChosen' in out
    assert '[if (mode == 7)]' not in out


@needs_clang
def test_steps_trip_count(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    src = make_steps_suite(tmp_path)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=100', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'SUBSTEP\t[repeats 100 times] Poke the device' in out
    assert '[repeats 100 times] [if (i > 0)] Settle' in out

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=1', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'SUBSTEP\tPoke the device' in out  # runs once: no loop annotation
    assert 'Settle' not in out  # i is bound to 0, if (i > 0) is false

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=0', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Poke the device' not in out


FOR_NONCANON_SRC = """\
#define TEST_STEP(...) (void)0
#define TEST_GET_INT_PARAM(x_) (void)0

int
main(void)
{
    int iters;

    TEST_GET_INT_PARAM(iters);
    for (iters = 0; iters < 5; iters += 2)
    {
        TEST_STEP("Loop body");
    }
    return 0;
}
"""


FOR_CANON_SRC = """\
#define TEST_STEP(...) (void)0
#define TEST_GET_INT_PARAM(x_) (void)0

int
main(void)
{
    int iters;

    TEST_GET_INT_PARAM(iters);
    for (iters = 0; iters < 3; iters++)
    {
        if (iters == 0)
        {
            TEST_STEP("Inner");
        }
    }
    return 0;
}
"""


@needs_clang
def test_steps_for_init_resets_noncanonical(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """A stale outer binding of the loop variable must not decide it.

    The loop reuses the "iters" parameter as its own counter with a
    non-canonical increment (+= 2), so trip_count cannot model it and
    the condition falls back to evaluate(); with the outer --param
    iters=100 still bound, the condition must not be judged against
    that stale value.
    """
    src = write_steps_src(tmp_path, FOR_NONCANON_SRC)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=100', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Loop body' in out
    assert 'not taken' not in out


@needs_clang
def test_steps_for_init_resets_canonical(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """A canonical multi-trip loop must also clear the stale binding.

    The loop variable "iters" is also the name of a bound --param;
    the outer binding must not leak into the inner condition, which
    should stay undecided (annotated), not be wrongly decided False.
    """
    src = write_steps_src(tmp_path, FOR_CANON_SRC)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=100', '--flat'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Inner' in out
    assert '[if (iters == 0)]' in out


MD_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup demo-md Markdown demo
 * @ingroup demo
 * @{
 *
 * @objective Check the markdown output.
 *
 * @param iters  Loop count
 *
 * @par Scenario:
 */
#define TEST_STEP(...) (void)0
#define TEST_SUBSTEP(...) (void)0
#define TEST_GET_INT_PARAM(x_) (void)0

int
main(void)
{
    int iters;
    int i;

    TEST_GET_INT_PARAM(iters);
    TEST_STEP("Prepare");
    if (iters > 10)
    {
        TEST_STEP("Warm up");
    }
    for (i = 0; i < iters; i++)
    {
        TEST_SUBSTEP("Poke");
    }
    return 0;
}
"""


@needs_clang
def test_steps_md_default(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    src = write_steps_src(tmp_path, MD_SRC)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db])
    assert rc == 0
    out = capsys.readouterr().out
    assert out.startswith('## demo: Markdown demo')
    assert 'Check the markdown output.' in out
    assert '- `iters`: Loop count' in out
    assert '1. Prepare' in out
    assert '2. Warm up' in out
    assert '   Only when `iters > 10`.' in out
    assert '   - Poke' in out
    assert '     For each iteration (`i = 0; i < iters; i++`).' in out


@needs_clang
def test_steps_md_param(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    src = write_steps_src(tmp_path, MD_SRC)
    db = str(tmp_path / 'compile_commands.json')

    rc = main(['steps', str(src), '--compile-db', db, '--param', 'iters=3'])
    assert rc == 0
    out = capsys.readouterr().out
    assert 'Warm up' not in out  # decided false: step dropped
    assert '1. Prepare' in out
    assert '   - Poke' in out
    assert '     Repeats 3 times.' in out
    assert 'Only when' not in out
