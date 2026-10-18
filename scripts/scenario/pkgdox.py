# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
"""Doxygen input filter: the description of a package.md.

Reads a package scenario document and prints its "## Description"
section as documentation of the package's doxygen group, so the
group page carries the prose next to the tests.  The group itself
stays defined where it is (a package.dox, say): the comment only
adds to it, and a build without this filter loses the description,
not the group.

The section is markdown and stays markdown, doxygen renders it.
Headings below it become bold lines with a "<group>-" prefixed
anchor to @ref, not doxygen sections: those would add entries to
the navigation sidebar of the group page, which lists the tests.
"> impl:" blockquotes are advice for the implementor and are left
out.  The comment is written with "///" lines, which a "*/" in the
prose cannot end.

A test the document describes but the package does not implement
yet - there is no <test>.c - gets a page of its own in the group,
built from the markdown and opening with a warning that there is no
test to run.  Once the source exists, its page replaces this one.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from mdparse import parse_package
from model import Package, ScenarioError

if TYPE_CHECKING:
    from collections.abc import Sequence

    from model import Step, Test, Value

_HEADING = re.compile(r'^#{3,6}\s+(.*?)\s*$')
_FENCE = re.compile(r'^\s*(```|~~~)')
_IMPL_QUOTE = re.compile(r'^\s*>\s*impl:', re.IGNORECASE)
_QUOTE = re.compile(r'^\s*>')


def _anchor(group: str, title: str) -> str:
    slug = re.sub(r'[^a-z0-9]+', '-', title.lower()).strip('-')
    return f'{group}-{slug}'


def _doc_lines(pkg: Package) -> list[str]:
    """The description as doxygen input, before comment markup."""
    out: list[str] = []
    fenced = False
    in_impl = False
    for line in pkg.description:
        if _FENCE.match(line):
            fenced = not fenced
            out.append(line)
            continue
        if fenced:
            out.append(line)
            continue
        if in_impl and _QUOTE.match(line):
            continue
        in_impl = bool(_IMPL_QUOTE.match(line))
        if in_impl:
            continue
        h = _HEADING.match(line)
        if h:
            out.append(f'@anchor {_anchor(pkg.group, h.group(1))}')
            out.append(f'**{h.group(1)}**')
        else:
            out.append(line)
    # A dropped quote leaves its blank separators behind.
    squeezed: list[str] = []
    for line in out:
        if line.strip() or (squeezed and squeezed[-1].strip()):
            squeezed.append(line)
    while squeezed and not squeezed[-1].strip():
        squeezed.pop()
    return squeezed


def _steps(steps: list[Step], depth: int = 0) -> list[str]:
    """Doxygen list markup for steps: numbered at the top, bullets below."""
    pad = '   ' * (depth + 1)
    marker = '-#' if depth == 0 else '-'
    out: list[str] = []
    for step in steps:
        out.append(f'{pad}{marker} {step.text}')
        text_pad = pad + ' ' * (len(marker) + 1)
        out.extend(f'{text_pad}{note.text}' for note in step.notes if not note.impl)
        out.extend(_steps(step.sub, depth + 1))
    return out


def _value(value: Value) -> str:
    """A listed parameter value as a bullet under its @param."""
    if value.comment:
        return f'  - `{value.name}` ({value.comment})'
    return f'  - `{value.name}`'


def _planned_lines(pkg: Package, test: Test) -> list[str]:
    """A page for a test that is described but not implemented yet."""
    out = [
        f'@defgroup {pkg.name}-{test.name} {test.summary}',
        f'@ingroup {pkg.group}',
        '@{',
        '',
        (
            f'@warning Not implemented yet: this is the planned scenario from '
            f'{pkg.name}/package.md, there is no test to run.'
        ),
        '',
        f'@objective {test.objective}',
    ]
    if test.params:
        out.append('')
    for p in test.params:
        desc = p.description.rstrip('.:') + ':' if p.values else p.description
        out.append(f'@param {p.name} {desc}')
        out.extend(_value(v) for v in p.values)
    for note in test.notes:
        out += ['', f'@note {note}']
    if test.type:
        out += ['', f'@type {test.type}']
    out += ['', '@par Scenario:', *_steps(test.steps), '@}']
    return out


def _comment(lines: list[str]) -> str:
    return '\n'.join(f'/// {line}' if line else '///' for line in lines) + '\n'


def render(pkg: Package, pending: Sequence[Test] = ()) -> str:
    """The doxygen comments for the package group.

    Args:
        pkg: The parsed package document.
        pending: The tests described but not implemented yet.

    Returns:
        The comment adding the description to the package group, and
        one more per pending test; an empty string when there is
        neither a description nor a pending test.
    """
    blocks: list[list[str]] = []
    if pkg.description:
        blocks.append([f'@addtogroup {pkg.group}', '', *_doc_lines(pkg)])
    blocks.extend(_planned_lines(pkg, t) for t in pending)
    # An empty line between blocks keeps doxygen from joining them.
    return '\n'.join(_comment(b) for b in blocks)


def filter_text(source: Path) -> str:
    """The filtered form of a package.md, ready for doxygen.

    Args:
        source: The package document doxygen wants filtered.

    Returns:
        The group comments; empty for a document with neither a
        description nor a pending test, or one that does not parse -
        the markdown itself must never reach doxygen, which would read
        it as C.
    """
    try:
        pkg = parse_package(source)
    except ScenarioError as exc:
        print(f'pkgdox: warning: {exc}; the description is not documented', file=sys.stderr)
        return ''
    pending = [t for t in pkg.tests if not (source.parent / f'{t.name}.c').exists()]
    return render(pkg, pending)


def main(argv: list[str] | None = None) -> int:
    """Run the filter on one package.md, printing to stdout."""
    parser = argparse.ArgumentParser(description='doxygen filter for package.md documents')
    parser.add_argument('source', help='the package.md doxygen wants filtered')
    args = parser.parse_args(argv)
    sys.stdout.write(filter_text(Path(args.source)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
