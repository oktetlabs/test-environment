#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Test scenario tool: markdown scenarios to C stubs and back.

Scenarios live in package.md files next to package.xml in a test
suite; this tool generates C test stubs from them, checks
implemented tests for drift against the markdown, and lists the
backlog. See the TE documentation for the dialect reference.
"""

from __future__ import annotations

import argparse
import datetime
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

import aststeps
import condeval
import emit_md
from cstep import compare
from emit_c import emit_test
from mdparse import parse_package
from model import Package, ScenarioError

if TYPE_CHECKING:
    from collections.abc import Callable, Iterator


def _root(opt: str) -> Path:
    """Resolve the test root: <opt>/ts when present, else <opt>."""
    base = Path(opt)
    ts = base / 'ts'
    return ts if ts.is_dir() else base


def _git_author() -> str:
    def cfg(key: str) -> str:
        return subprocess.run(  # noqa: S603 - fixed argv, no input
            ['git', 'config', key],  # noqa: S607 - git comes from PATH
            capture_output=True,
            text=True,
            check=False,
        ).stdout.strip()

    name, email = cfg('user.name'), cfg('user.email')
    return f'{name} <{email}>' if name else 'FIXME <fixme@example.com>'


def _default_copyright() -> str:
    year = datetime.date.today().year  # noqa: DTZ011 - local year is fine
    return f'Copyright (C) {year} OKTET Ltd.'


def _packages(root: Path, names: list[str]) -> Iterator[Path]:
    if names:
        for n in names:
            yield root / n / 'package.md'
    else:
        yield from sorted(root.rglob('package.md'))


def _pkg_ref(root: Path, md: Path) -> str:
    """The package reference of a package.md, relative to root."""
    return md.parent.relative_to(root).as_posix()


def _cmd_generate(args: argparse.Namespace) -> int:
    root = _root(args.test_suite)
    ref = args.test
    if '/' not in ref:
        print(f'test reference "{ref}" must be <package>/<test>', file=sys.stderr)
        return 1
    pkg_ref, test_name = ref.rsplit('/', 1)
    pkg = parse_package(root / pkg_ref / 'package.md')
    test = next((t for t in pkg.tests if t.name == test_name), None)
    if test is None:
        known = ', '.join(sorted(t.name for t in pkg.tests))
        print(
            f'no test "{test_name}" in {pkg.path}; known tests: {known}',
            file=sys.stderr,
        )
        return 1
    out_path = root / pkg_ref / f'{test_name}.c'
    if out_path.exists() and not args.force:
        print(f'{out_path} exists, use --force to overwrite', file=sys.stderr)
        return 1
    author = args.author or _git_author()
    copyright_line = args.copyright or _default_copyright()
    out_path.write_text(
        emit_test(pkg, test, author=author, copyright_line=copyright_line),
        encoding='utf-8',
    )
    print(out_path)
    return 0


def _check_package(root: Path, md: Path, *, strict: bool) -> list[str]:
    findings: list[str] = []
    try:
        pkg = parse_package(md)
    except ScenarioError as exc:
        return [str(exc)]
    ref = _pkg_ref(root, md)
    for t in pkg.tests:
        c_path = md.parent / f'{t.name}.c'
        if c_path.exists():
            findings.extend(compare(t, c_path.read_text(encoding='utf-8'), f'{ref}/{t.name}'))
    if strict:
        known = {t.name for t in pkg.tests}
        findings.extend(
            f'{ref}/{c.name}: not described in {md.name}'
            for c in sorted(md.parent.glob('*.c'))
            if c.stem not in known
        )
    return findings


def _cmd_check(args: argparse.Namespace) -> int:
    root = _root(args.test_suite)
    findings: list[str] = []
    for md in _packages(root, args.packages):
        findings.extend(_check_package(root, md, strict=args.strict))
    for f in findings:
        print(f)
    if findings:
        print(f'{len(findings)} finding(s)')
        return 1
    print('ok')
    return 0


def _iter_tests(root: Path) -> Iterator[tuple[str, Package, Path]]:
    for md in _packages(root, []):
        pkg = parse_package(md)
        yield _pkg_ref(root, md), pkg, md.parent


def _cmd_list(args: argparse.Namespace) -> int:
    root = _root(args.test_suite)
    for ref, pkg, pkg_dir in _iter_tests(root):
        for t in pkg.tests:
            implemented = (pkg_dir / f'{t.name}.c').exists()
            if args.pending and implemented:
                continue
            if args.implemented and not implemented:
                continue
            state = 'implemented' if implemented else 'pending'
            print(f'{ref}/{t.name} {state}')
    return 0


def _parse_params(pairs: list[str]) -> dict[str, str]:
    """Parameter values from repeated --param NAME=VALUE options.

    Raises:
        ValueError: An option is not of the NAME=VALUE form.
    """
    params: dict[str, str] = {}
    for pair in pairs:
        name, eq, val = pair.partition('=')
        if not (name and eq):
            msg = f'--param wants NAME=VALUE, got "{pair}"'
            raise ValueError(msg)
        params[name] = val
    return params


def _numeric(raw: str) -> condeval.Num | None:
    """The number a value string spells (any int base, float), or None."""
    try:
        return int(raw, 0)
    except ValueError:
        try:
            return float(raw)
        except ValueError:
            return None


def _env(
    info: aststeps.SourceInfo, params: dict[str, str], *, quiet: bool = False
) -> dict[str, condeval.Num]:
    """Evaluation environment: constants plus bound parameters.

    Args:
        info: The analyzed source, providing enum constants, integer
            macros, and the parameter bindings.
        params: Raw parameter values by name, as strings.
        quiet: Suppress the stderr note about values that bind
            neither through a mapping nor as a number.

    Returns:
        Identifier values for condeval: macros, then enum constants,
        then the parameters that could be bound (parameters win on
        name collisions).
    """
    env: dict[str, condeval.Num] = {}
    env.update(info.macros)
    env.update(info.enums)
    for name, raw in params.items():
        binding = info.bindings.get(name)
        val: condeval.Num | None = None
        if binding is not None and binding.mapping is not None:
            val = binding.mapping.get(raw)
            if val is None and binding.kind == 'bool':
                val = binding.mapping.get(raw.upper())
        if val is None:
            val = _numeric(raw)
        if val is None:
            if not quiet:
                print(f'note: {name}={raw} has no numeric value, kept unbound', file=sys.stderr)
            continue
        env[name] = val
    return env


def _reset_for_var(cond: aststeps.Cond, env: dict[str, condeval.Num]) -> None:
    """Drop any stale binding of a for-loop's own init variable.

    The init clause assigns this variable, so an earlier binding of
    it (e.g. from a bound test parameter of the same name) is dead
    from here on, for both the loop condition and its body.
    """
    if cond.init is None:
        return
    var = condeval.init_var(cond.init)
    if var is not None:
        env.pop(var, None)


def _judge(  # noqa: PLR0911
    cond: aststeps.Cond, env: dict[str, condeval.Num]
) -> tuple[bool | None, str | None]:
    """Verdict for one construct: (taken, annotation to keep).

    A one-trip counting loop binds its variable in env for the
    inner conditions of the same step (env is a per-step copy,
    conds are outermost-first).

    Args:
        cond: The construct to judge.
        env: Identifier values for the evaluation, updated in place
            for one-trip loop variables.

    Returns:
        The verdict and the annotation to keep: False means the
        construct is decidably not entered (the step drops), None
        means undecided (the annotation stays), True drops the
        annotation only.
    """
    if cond.kind in ('switch', 'goto') or not cond.cond:
        return None, cond.desc
    if cond.kind == 'for':
        _reset_for_var(cond, env)
        trip = condeval.trip_count(cond.init, cond.cond, cond.incr, env)
        if trip is not None:
            var, start, count = trip
            if count == 0:
                return False, None
            if count == 1:
                env[var] = start
                return True, None
            return None, f'repeats {count} times'
    verdict = condeval.evaluate(cond.cond, env)
    if cond.kind == 'else':
        verdict = None if verdict is None else not verdict
    elif cond.kind == 'do':
        # The body runs at least once; a false condition only means
        # it does not repeat.
        return (True, None) if verdict is False else (None, cond.desc)
    elif cond.kind in ('for', 'while') and verdict is True:
        # Entered, but the repeat count is unknown.
        return None, cond.desc
    return (verdict, None) if verdict is not None else (None, cond.desc)


def _render(
    steps: list[aststeps.SourceStep], env: dict[str, condeval.Num]
) -> list[tuple[str, bool]]:
    """(line, taken) for each step under the given environment.

    Args:
        steps: The steps in source order.
        env: Identifier values; copied per step, so a judge may bind
            step-local values without leaking across steps.

    Returns:
        The rendered line of every step, with taken False for steps
        decidably not reached (the falsifying construct's
        description is kept as their annotation).
    """
    rendered = []
    for step in steps:
        pairs, taken = _judge_step(step, dict(env))
        notes = [override if override is not None else cond.desc for cond, override in pairs]
        scope = f'({step.func}) ' if step.func and step.func != 'main' else ''
        where = f'[{"] [".join(notes)}] ' if notes else ''
        rendered.append((f'{step.kind}\t{scope}{where}{step.text}', taken))
    return rendered


def _judge_step(
    step: aststeps.SourceStep, env: dict[str, condeval.Num]
) -> tuple[list[tuple[aststeps.Cond, str | None]], bool]:
    """Judge one step's constructs: kept (cond, note override) pairs.

    Args:
        step: The step to judge.
        env: Identifier values; mutated by one-trip loop bindings,
            so pass a per-step copy.

    Returns:
        The constructs that stay on the step - the override is a
        replacement note like "repeats N times", None to show the
        construct itself - and whether the step is taken at all.
        For an untaken step the falsifying construct ends the list.
    """
    kept: list[tuple[aststeps.Cond, str | None]] = []
    for cond in step.conds:
        verdict, note = _judge(cond, env)
        if verdict is False:
            kept.append((cond, None))
            return kept, False
        if note is not None:
            kept.append((cond, None if note == cond.desc else note))
    return kept, True


def _md_judge(
    env: dict[str, condeval.Num],
) -> Callable[[aststeps.SourceStep], tuple[list[str], bool]]:
    """A step judge for the markdown emitter under bound parameters.

    Decided-true constructs disappear, untaken steps are dropped,
    and the surviving constructs turn into note sentences.
    """

    def judge(step: aststeps.SourceStep) -> tuple[list[str], bool]:
        pairs, taken = _judge_step(step, dict(env))
        if not taken:
            return [], False
        notes = []
        for cond, override in pairs:
            if override is None:
                notes.append(emit_md.cond_note(cond))
            else:
                notes.append(f'{override[0].upper()}{override[1:]}.')
        return notes, True

    return judge


def _steps_from_source(args: argparse.Namespace) -> int:
    path = Path(args.source)
    db = Path(args.compile_db) if args.compile_db else aststeps.find_compile_db(path)
    if db is None:
        print(
            f'no compile_commands.json found for {path}; build the suite or pass --compile-db',
            file=sys.stderr,
        )
        return 1
    info = aststeps.analyze(path, compile_db=db)
    params = _parse_params(args.param)
    if args.flat:
        if params:
            _print_scenario(info, params, show_skipped=args.show_skipped)
        else:
            _print_annotated(info.steps)
        return 0
    judge = _md_judge(_env(info, params)) if params else None
    items = emit_md.step_items(info.steps, judge=judge)
    text = path.read_text(encoding='utf-8', errors='replace')
    print(emit_md.emit_test_md(path.stem, text, items), end='')
    return 0


def _print_annotated(steps: list[aststeps.SourceStep]) -> None:
    """Print every step flat, with its full condition annotations."""
    for step in steps:
        where = f'[{c}] ' if (c := '] ['.join(c.desc for c in step.conds)) else ''
        scope = f'({step.func}) ' if step.func != 'main' else ''
        print(f'{step.kind}\t{scope}{where}{step.text}')


def _print_scenario(
    info: aststeps.SourceInfo,
    params: dict[str, str],
    *,
    show_skipped: bool,
    quiet: bool = False,
) -> None:
    """Print the scenario evaluated for one set of parameter values.

    Steps not taken are summarized in a trailing count line, or
    printed SKIP-prefixed with show_skipped.
    """
    rendered = _render(info.steps, _env(info, params, quiet=quiet))
    for line, taken in rendered:
        if taken:
            print(line)
        elif show_skipped:
            print(f'SKIP\t{line}')
    skipped = sum(1 for _, taken in rendered if not taken)
    if skipped and not show_skipped:
        print(f'{skipped} step(s) not taken with these parameters')


def _cmd_steps(args: argparse.Namespace) -> int:
    try:
        return _steps_from_source(args)
    except (OSError, ValueError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        return 1


def _add_root_arg(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        '-t',
        '--test-suite',
        default='.',
        help='test suite root (default: current directory)',
    )


def main(argv: list[str] | None = None) -> int:
    """Run the scenario tool."""
    parser = argparse.ArgumentParser(
        prog='scenario.py',
        description='markdown test scenarios: stub generation and drift check',
    )
    sub = parser.add_subparsers(dest='verb', required=True)

    p = sub.add_parser('generate', help='generate a C stub from package.md')
    p.add_argument('test', help='test reference: <package>/<test>')
    _add_root_arg(p)
    p.add_argument('--force', action='store_true', help='overwrite an existing file')
    p.add_argument('--author', help='author for the doxygen block')
    p.add_argument('--copyright', help='copyright line for the header')
    p.set_defaults(func=_cmd_generate)

    p = sub.add_parser('check', help='check implemented tests for drift')
    p.add_argument('packages', nargs='*', help='package paths (default: all)')
    _add_root_arg(p)
    p.add_argument(
        '--strict',
        action='store_true',
        help='also report .c files not described in markdown',
    )
    p.set_defaults(func=_cmd_check)

    p = sub.add_parser('list', help='list scenarios and their status')
    _add_root_arg(p)
    g = p.add_mutually_exclusive_group()
    g.add_argument('--pending', action='store_true', help='only unimplemented')
    g.add_argument('--implemented', action='store_true', help='only implemented')
    p.set_defaults(func=_cmd_list)

    p = sub.add_parser('steps', help='read the scenario back from a test source')
    p.add_argument(
        'source',
        help='a test .c source; emits its scenario as a markdown'
        ' test section, steps annotated with their control flow'
        ' (needs the libclang package)',
    )
    p.add_argument(
        '--compile-db',
        help='compile_commands.json to take compiler flags from'
        ' (default: found in a build tree near the source)',
    )
    p.add_argument(
        '--param',
        action='append',
        default=[],
        metavar='NAME=VALUE',
        help='bind a test parameter and evaluate step conditions'
        ' (repeatable); conditions the values decide drop their'
        ' annotation or their step',
    )
    p.add_argument(
        '--show-skipped',
        action='store_true',
        help='with --flat, print steps not taken with the given parameters, marked SKIP',
    )
    p.add_argument(
        '--flat',
        action='store_true',
        help='one line per step instead of the markdown test section',
    )
    p.set_defaults(func=_cmd_steps)

    args = parser.parse_args(argv)
    try:
        rc: int = args.func(args)
    except ScenarioError as exc:
        print(exc, file=sys.stderr)
        return 1
    return rc


if __name__ == '__main__':
    sys.exit(main())
