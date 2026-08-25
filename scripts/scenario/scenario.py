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

from cstep import compare
from emit_c import emit_test
from mdparse import parse_package
from model import Package, ScenarioError

if TYPE_CHECKING:
    from collections.abc import Iterator


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

    args = parser.parse_args(argv)
    try:
        rc: int = args.func(args)
    except ScenarioError as exc:
        print(exc, file=sys.stderr)
        return 1
    return rc


if __name__ == '__main__':
    sys.exit(main())
